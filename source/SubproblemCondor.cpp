/*
 * SubproblemCondor.cpp
 *
 *  Created on: Nov 15, 2008
 *      Author: lars
 */

#include "SubproblemCondor.h"

#include "ProgramOptions.h"
#include "gzstream.h"

#ifdef PARALLEL_MODE

// Suppress condor output if not in debug mode
#ifndef DEBUG
#define SILENT_MODE
#endif


// parameters that can be modified
#define MAX_SUBMISSION_TRIES 6
#define SUBMISSION_FAIL_DELAY_BASE 2

#define WAIT_BETWEEN_BATCHES 8
#define WAIT_FOR_MORE_JOBS 2



// static definitions (no need to modify)
#define CONDOR_SUBMIT "condor_submit"
#define CONDOR_WAIT "condor_wait"
#define CONDOR_RM "condor_rm"
// some string definition for filenames
#define JOBFILE "subproblem.sub"
#define PREFIX_SOL "temp_sol."
#define PREFIX_EST "temp_est."
#define PREFIX_SUB "temp_sub."
#define PREFIX_LOG "temp_log."
#define PREFIX_STDOUT "temp_std."
#define PREFIX_STDERR "temp_err."

#define REQUIREMENTS_FILE "requirements.txt"

// custom attributes for generated condor jobs
#define CONDOR_ATTR_PROBLEM "daoopt_problem"
#define CONDOR_ATTR_THREADID "daoopt_threadid"

void SubproblemCondor::operator() () {

  CondorSubmission job(m_subproblem, m_threadId, m_estimate);

  boost::thread* waitProc = NULL;

  try {

  // pass subproblem to CondorSubmissionEngine
  {
    GETLOCK(m_space->mtx_condorQueue,lk);
    m_space->condorQueue.push(&job);
    NOTIFY(m_space->cond_condorQueue);
  }

  // wait for signal from CondorSubmissionEngine
  {
    boost::mutex mtx_local;
    GETLOCK(mtx_local,lk);
    CONDWAIT(m_space->cond_jobsSubmitted, lk);
  }

  // start condor_wait in new thread
  CondorWaitInstance wi(job.batch, job.process);
  waitProc = new boost::thread(wi);
  // wait for condor_wait
  waitProc->join();

/*
  {
    GETLOCK(mtx_io,lk2);
    cout << "<-- Receiving solution for subproblem " << m_threadId << '.' << endl;
  }
*/

  // Read solution from file
  ostringstream solutionFile;
  solutionFile << PREFIX_SOL << job.batch << '.' << job.process << ".gz";
  {
    ifstream inTemp(solutionFile.str().c_str());
    inTemp.close();
    if (inTemp.fail()) {
      GETLOCK(mtx_io, lk2);
      cerr << "Problem reading subprocess solution for thread " << m_threadId << '.' << endl;
      return;
    }
  }
  igzstream in(solutionFile.str().c_str(), ios::binary | ios::in);

  double d;
  BINREAD(in, d); // read opt. cost

#ifndef NO_ASSIGNMENT
  int n;
  BINREAD(in, n); // read length of opt. tuple

  vector<val_t> tup(n,UNKNOWN);

  val_t v;
  for (int i=0; i<n; ++i) {
    BINREAD(in, v); // read opt. assignments
    tup[i] = v;
  }
#endif

  in.close();

  // Write subproblem solution value and tuple into search node
  m_subproblem->setValue(d);
#ifndef NO_ASSIGNMENT
  m_subproblem->setOptAssig(tup);
#endif

  {
    GETLOCK(mtx_io,lk2);
    cout << "<-- Received solution for subproblem " << m_threadId << ": " << d << endl;
  }

  } catch (boost::thread_interrupted i) {
    // interrupt condor_wait thread
    waitProc->interrupt();
    // remove job from queue
    removeJob();
    waitProc->join();
  }

  // clean up pointers
  if (waitProc) delete waitProc;

  {
    GETLOCK(m_space->mtx_solved, lk);
    m_space->solved.push(m_subproblem);
    m_space->cond_solved.notify_one();
  }

}


void SubproblemCondor::removeJob() const {

  ostringstream rmCmd;
  rmCmd << CONDOR_RM << " -constraint '("
        << CONDOR_ATTR_PROBLEM << "==\"" << m_space->options->problemName << "\" && "
        << CONDOR_ATTR_THREADID <<  "==" << m_threadId << ")'" ;
#ifdef SILENT_MODE
  rmCmd << " > /dev/null";
#endif

  size_t noTries = 0;
  bool success = false;
  while (!success) {

    {
      GETLOCK(m_space->mtx_condor, lk);
      if ( !system( rmCmd.str().c_str() ) )
        success = true;
    }

    if ( !success ) {
      if (noTries++ == MAX_SUBMISSION_TRIES) {
        GETLOCK(mtx_io,lk2);
        cerr << "Failed invoking condor_rm for subproblem " << m_threadId << '.' << endl;
        return;
      } else {
        myprint("Problem invoking condor_rm, retrying...\n");
        CondorSubmissionEngine::mysleep(pow( SUBMISSION_FAIL_DELAY_BASE ,noTries+1)); // wait increases exponentially
      }
    } else {
      // Removal successful
      GETLOCK(mtx_io,lk2);
      cout << "-- ! Preempted condor job for subproblem " << m_threadId << '.' << endl;
      //break;
    }
  }

}


void CondorWaitInstance::operator ()() {

  try {
    // invoke 'condor_wait' and wait for subproblem to finish computing
    ostringstream waitCmd;
    waitCmd << CONDOR_WAIT << ' ' << PREFIX_LOG << m_batch << '.' << m_proc;
  #ifdef SILENT_MODE
    waitCmd << " > /dev/null";
  #endif

    // wait for the job to finish
    if ( system(waitCmd.str().c_str()) ) {
      // Check for interrupt first (-> jump to catch)
      boost::this_thread::interruption_point();
      // No interrupt -> error!
      GETLOCK(mtx_io,lk2);
      cerr << "Error invoking condor_wait for batch " << m_batch << ", process " << m_proc << endl;
      exit(1);
    }
    boost::this_thread::interruption_point();

  } catch (boost::thread_interrupted i) {
    // intentionally left empty
  }
}


void CondorSubmissionEngine::operator ()() {

  CondorSubmission* nextJob;
  bool waitForCond = true;

  try {
  // ATTENTION !!!
  // endless loop, thread needs to be interrupted by caller!
  while (true) {

    m_nextProcess = 0; // reset process counter

    // try to read custom requirements from file
    string reqStr = "true"; // default
    ifstream req(REQUIREMENTS_FILE);
    if (req) {
      reqStr = ""; string s;
      while (!req.eof()) {
        getline(req,s);
        if (s.size() && s.at(0) != '#') // filter comment lines
          reqStr += s;
      }
    }
    req.close();

    // Build the condor job description
    ostringstream jobFile;
    jobFile
    // Generic options
    << "universe = vanilla" << endl
    << "notification = never" << endl
    << "should_transfer_files = yes" << endl
    << "requirements = Arch == \"X86_64\" && (" << reqStr << ")" << endl
    << "when_to_transfer_output = always" << endl
    << "executable = daoopt.$$(Arch)" << endl
    << "copy_to_spool = false" << endl;

    // custom attributes
    jobFile
    << '+' << CONDOR_ATTR_PROBLEM << " = \"" << m_space->options->problemName << "\"" << endl;

//    // Condor log file
//    ostringstream logFile;
//    logFile << PREFIX_LOG << batch << ".$(Process) ";

    // Logging options
    jobFile << endl
    << "output = " << PREFIX_STDOUT << m_curBatch << ".$(Process)" << endl
    << "error = " << PREFIX_STDERR << m_curBatch << ".$(Process)" << endl
    << "log = " << PREFIX_LOG << m_curBatch << ".$(Process)" << endl;

    waitForCond = true;

    do {
      {
        GETLOCK(m_space->mtx_condorQueue, lk);

        if (waitForCond) {
          while (m_space->condorQueue.empty())
            CONDWAIT(m_space->cond_condorQueue, lk); // releases lock
          waitForCond = false;
        }

        // collect all jobs
        if (m_space->condorQueue.size()) {
          while (m_space->condorQueue.size()) {
            nextJob = m_space->condorQueue.front();
            m_space->condorQueue.pop();
            jobFile << encodeJob(nextJob);
          }
        } else {
          // submit the collected jobs to Condor (before releasing the lock!)
          submitToCondor(jobFile); // also notifies waiting SubproblemHandlers
          waitForCond = true;
        }
      } // lock is released here

      // wait a little before checking again
      if (!waitForCond)
        mysleep(WAIT_FOR_MORE_JOBS);

    } while (!waitForCond) ;

    // sleep for 5 seconds before checking for jobs again
    mysleep(WAIT_BETWEEN_BATCHES);

    ++m_curBatch; // increase batch counter

  } // end while(true)

  } catch (boost::thread_interrupted i) {

    GETLOCK(m_space->mtx_activeThreads, lk);

    // find and terminate active subproblem processes
    map<SearchNode*, boost::thread*>::const_iterator itT = m_space->activeThreads.begin();
    for (; itT!=m_space->activeThreads.end(); ++itT) {
      itT->second->interrupt();
    }

    // join and delete threads instances
    itT = m_space->activeThreads.begin();
    for (; itT!=m_space->activeThreads.end(); ++itT) {
      itT->second->join();
      delete itT->second;
    }

  } // catch interruption by main process

}


void CondorSubmissionEngine::submitToCondor(const ostringstream& jobstr) const {

  ofstream jobfile(JOBFILE);
  if (!jobfile) {
    myerror("Error writing condor job file.\n");
    return;
  }
  jobfile << jobstr.str();
  jobfile.close();

  ostringstream submitCmd;
  submitCmd << CONDOR_SUBMIT << ' ' << JOBFILE;
#ifdef SILENT_MODE
  submitCmd << " > /dev/null";
#endif

  size_t noTries = 0;
  bool success = false;

  while (!success) {

    {
      GETLOCK(m_space->mtx_condor, lk);
      if ( !system( submitCmd.str().c_str() ) ) // returns 0 if successful
        success = true;
    }

    if (!success) {
      if (noTries++ == MAX_SUBMISSION_TRIES) {
        GETLOCK(mtx_io,lk2);
        cerr << "Failed invoking condor_submit for batch " << m_curBatch << "." << endl;
        return;
      } else {
        myprint("Problem invoking condor_submit, retrying...\n");
        mysleep(pow( SUBMISSION_FAIL_DELAY_BASE ,noTries+1)); // wait increases exponentially
      }
    } else {
      // submission successful
      GETLOCK(mtx_io,lk2);
      cout << "----> Submitted " << m_nextProcess <<" jobs to batch " << m_curBatch << '.' << endl;
      //break;
    }

  }

  // signal waiting SubproblemHandlers
  NOTIFYALL( m_space->cond_jobsSubmitted );

}


// creates the required files on disk for job submission and generates
// the string for the Condor submission file
string CondorSubmissionEngine::encodeJob(CondorSubmission* P) {

  SearchNode* n = P->problem;
  //size_t threadId = P.threadID;

  assert(n && !n->getSubprobContext().empty());

  // set process identification
  P->batch = m_curBatch;
  P->process = m_nextProcess++;

  // generate subproblem file for this job
  ostringstream subprobFile;
  subprobFile << PREFIX_SUB << P->batch << '.' << P->process << ".gz";
  ogzstream con(subprobFile.str().c_str(), ios::out | ios::binary );
  if(!con) {
    GETLOCK(mtx_io,lk2);
    cerr << "Problem writing context file for thread " << P->threadID << '.' << endl;
    return "";
  }

  // subproblem root variable
  int rootVar = n->getVar();
  BINWRITE( con, rootVar );
  // subproblem context size
  int sz = n->getSubprobContext().size();
  BINWRITE( con, sz);

  // write context instantiations
  con.write( (char*) (& n->getSubprobContext().at(0)), n->getSubprobContext().size() * sizeof(val_t));
  //con.write(& n->getSubprobContext().at(0), n->getSubprobContext().size());

  //con << n->getSubprobContext(); // write subproblem context

  // write number of PST entries
  int pstSize = n->getPSTlist().size();
  BINWRITE(con,pstSize);
  // output PST data to enable advanced pruning in subproblem
  list<pair<double,double> >::const_iterator it = n->getPSTlist().begin();
  for (;it!=n->getPSTlist().end();++it) {
    BINWRITE(con, it->first);
    BINWRITE(con, it->second);
  }

  con.close();
  // subproblem file generated

  // write complexity estimate to disk
  ostringstream estimateFile;
  estimateFile << PREFIX_EST << P->batch << '.' << P->process;
  ofstream estFile(estimateFile.str().c_str());
  if (!estFile) {
    GETLOCK(mtx_io,lk2);
    cerr << "Problem writing estimate file for thread " << P->threadID << '.' << endl;
    return "";
  }
  estFile << P->estimate;
  estFile.close();
  // estimate written

  // Where the solution will be read from
  ostringstream solutionFile;
  solutionFile << PREFIX_SOL << P->batch << '.' << P->process << ".gz";

  // Build the condor job description
  ostringstream job;

  // Custom job attributes
  job
  << '+' << CONDOR_ATTR_THREADID << " = " << P->threadID << endl;

  // Job specifics
  job
  // Make sure input files get transferred
  << "transfer_input_files = " << m_space->options->in_problemFile
  << ", " << m_space->options->in_orderingFile
  << ", " << subprobFile.str();
  if (!m_space->options->in_evidenceFile.empty())
    job << ", " << m_space->options->in_evidenceFile;
  job << endl;

  // Build the argument list for the worker invocation
  ostringstream command;
  //command << m_space->options->executableName << "-worker";
  command << " -f " << m_space->options->in_problemFile;
  if (!m_space->options->in_evidenceFile.empty())
    command << " -e " << m_space->options->in_evidenceFile;
  command << " -o " << m_space->options->in_orderingFile;
  command << " -s " << subprobFile.str();
  command << " -i " << m_space->options->ibound;
  command << " -j " << m_space->options->cbound_worker;
  command << " -c " << solutionFile.str();
  //command << " > /dev/null";

  // Add command line arguments to condor job
  job << "arguments = " << command.str() << endl;

  // Queue it
  job << "queue" << endl;
  {
    GETLOCK(mtx_io,lk);
    cout << "--> Submitting subproblem " << P->threadID << " in batch " << P->batch
         << ", process " << P->process << '.' << endl;
#ifdef DEBUG
    cout << "--------------------------------------" << endl << job.str()
	 << "--------------------------------------" << endl;
#endif
  }
  return job.str();

}


#endif /* PARALLEL_MODE */

