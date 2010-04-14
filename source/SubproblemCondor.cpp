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

/* Suppress condor output if not in debug mode */
#ifndef DEBUG
#define SILENT_MODE
#endif


/* parameters that can be modified */
#define MAX_SUBMISSION_TRIES 6
#define SUBMISSION_FAIL_DELAY_BASE 2

#define WAIT_BETWEEN_BATCHES 10
#define WAIT_FOR_MORE_JOBS 3



/* static definitions (no need to modify) */
#define CONDOR_SUBMIT "condor_submit"
#define CONDOR_WAIT "condor_wait"
#define CONDOR_RM "condor_rm"
/* some string definition for filenames */
#define JOBFILE "subproblem.sub"
#define PREFIX_SOL "temp_sol."
#define PREFIX_EST "temp_est."
#define PREFIX_SUB "temp_sub."
#define PREFIX_LOG "temp_log."
#define PREFIX_STDOUT "temp_std."
#define PREFIX_STDERR "temp_err."

#define REQUIREMENTS_FILE "requirements.txt"

/* custom attributes for generated condor jobs */
#define CONDOR_ATTR_PROBLEM "daoopt_problem"
#define CONDOR_ATTR_THREADID "daoopt_threadid"

void SubproblemCondor::operator() () {

  CondorSubmission job(m_subproblem, m_threadId);

  boost::thread* waitProc = NULL;

  try {

  // pass subproblem to CondorSubmissionEngine
  {
    GETLOCK(m_spaceMaster->mtx_condorQueue,lk);
    m_spaceMaster->condorQueue.push(&job);
    NOTIFY(m_spaceMaster->cond_condorQueue);
  }

  // wait for signal from CondorSubmissionEngine
  {
    boost::mutex mtx_local;
    GETLOCK(mtx_local,lk);
    CONDWAIT(m_spaceMaster->cond_jobsSubmitted, lk);
  }

  // start condor_wait in new thread
  CondorWaitInstance wi(job.batch, job.process);
  waitProc = new boost::thread(wi);
  // wait for condor_wait
  waitProc->join();

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

  double optCost;
  BINREAD(in, optCost); // read opt. cost

  count_t nodesOR, nodesAND;
  BINREAD(in, nodesOR);
  BINREAD(in, nodesAND);

#ifndef NO_ASSIGNMENT
  int n;
  BINREAD(in, n); // read length of opt. tuple

  vector<val_t> tup(n,UNKNOWN);

  int v; // assignment saved as int, regardless of internal type
  for (int i=0; i<n; ++i) {
    BINREAD(in, v); // read opt. assignments
    tup[i] = (val_t) v;
  }
#endif

  // read node profiles
  int size;
  count_t c;
  BINREAD(in, size);
  m_subproblem->leafP.reserve(size);
  m_subproblem->nodeP.reserve(size);
  for (int i=0; i<size; ++i) { // leaf profile
    BINREAD(in, c);
    m_subproblem->leafP.push_back(c);
  }
  for (int i=0; i<size; ++i) { // full node profile
    BINREAD(in, c);
    m_subproblem->nodeP.push_back(c);
  }

  // done reading input file
  in.close();

  // Write subproblem solution value and tuple into search node
  m_subproblem->root->setValue(optCost);
#ifndef NO_ASSIGNMENT
  m_subproblem->root->setOptAssig(tup);
#endif
  // Write number of OR/AND nodes into subproblem
  m_subproblem->nodesOR  = nodesOR;
  m_subproblem->nodesAND = nodesAND;
  // Mark as solved
  m_subproblem->setSolved();

  double heur = m_subproblem->upperBound;

  stringstream ss;
  ss << "<-- Subproblem " << m_threadId << " (" << job.batch << "." << job.process << "): "
     << optCost << "|" << heur
     << " (" << nodesAND << '/' << m_subproblem->estimate << '/' << m_subproblem->hwb
     << ", w=" << m_subproblem->width << ")\n";
  myprint(ss.str());

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
    GETLOCK(m_spaceMaster->mtx_solved, lk);
    m_spaceMaster->solved.push(m_subproblem); // push node to solved queue
    m_spaceMaster->cond_solved.notify_one();
  }

}


void SubproblemCondor::removeJob() const {

  ostringstream rmCmd;
  rmCmd << CONDOR_RM << " -constraint '("
        << CONDOR_ATTR_PROBLEM << "==\"" << m_spaceMaster->options->problemName << "\" && "
        << CONDOR_ATTR_THREADID <<  "==" << m_threadId << ")'" ;
#ifdef SILENT_MODE
  rmCmd << " > /dev/null";
#endif

  size_t noTries = 0;
  bool success = false;
  while (!success) {

    {
      GETLOCK(m_spaceMaster->mtx_condor, lk);
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
    << '+' << CONDOR_ATTR_PROBLEM << " = \"" << m_spaceMaster->options->problemName << "\"" << endl;

    // Logging options
    jobFile << endl
    << "output = " << PREFIX_STDOUT << m_curBatch << ".$(Process)" << endl
    << "error = " << PREFIX_STDERR << m_curBatch << ".$(Process)" << endl
    << "log = " << PREFIX_LOG << m_curBatch << ".$(Process)" << endl;

    waitForCond = true;

    queue<CondorSubmission*> localQ;

    // start collecting jobs for this batch
    do {
      {
        GETLOCK(m_spaceMaster->mtx_condorQueue, lk);

        if (waitForCond) {
          while (m_spaceMaster->condorQueue.empty())
            CONDWAIT(m_spaceMaster->cond_condorQueue, lk); // releases lock
          waitForCond = false;
        }

        // collect all jobs
        if (m_spaceMaster->condorQueue.size()) {
          while (m_spaceMaster->condorQueue.size()) {
            nextJob = m_spaceMaster->condorQueue.front();
            m_spaceMaster->condorQueue.pop();
            localQ.push(nextJob); // store pointer locally
          }
        } else {
          // only now encode the jobs
          GETLOCK( m_spaceMaster->mtx_space, lk2); // lock the search space
          while (localQ.size()) {
            jobFile << encodeJob( localQ.front() );
            localQ.pop();
          }
          // finally submit the collected jobs to Condor (before releasing the lock!)
          submitToCondor(jobFile); // also notifies waiting SubproblemHandlers
          waitForCond = true;
        }
      } // lock mtc_condorQueue is released here

      // wait a little before checking again
      if (!waitForCond)
        mysleep(WAIT_FOR_MORE_JOBS);

    } while (!waitForCond) ;

    // sleep for some seconds before checking for jobs again
    mysleep(WAIT_BETWEEN_BATCHES);

    ++m_curBatch; // increase batch counter

  } // end while(true)

  } catch (boost::thread_interrupted i) {

    GETLOCK(m_spaceMaster->mtx_activeThreads, lk);

    // find and terminate active subproblem processes
    map<Subproblem*, boost::thread*>::const_iterator itT = m_spaceMaster->activeThreads.begin();
    for (; itT!=m_spaceMaster->activeThreads.end(); ++itT) {
      itT->second->interrupt();
    }

    // join and delete threads instances
    itT = m_spaceMaster->activeThreads.begin();
    for (; itT!=m_spaceMaster->activeThreads.end(); ++itT) {
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
      GETLOCK(m_spaceMaster->mtx_condor, lk);
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
      cout << "----> Submitted " << m_nextProcess <<" jobs in batch " << m_curBatch << '.' << endl;
      //break;
    }

  }

  // signal waiting SubproblemHandlers
  NOTIFYALL( m_spaceMaster->cond_jobsSubmitted );

}


/* creates the required files on disk for job submission and generates
 * the string for the Condor submission file.
 * !!! ATTENTION: needs to be called inside a mtx_space lock !!! */
string CondorSubmissionEngine::encodeJob(CondorSubmission* P) {

  SearchNode* n = P->subproblem->root; // subproblem root node
  //size_t threadId = P.threadID;

  assert(n); // && !n->getSubprobContext().empty());

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

  // write context instantiations (as ints)
//  if (n->getSubprobContext().size())
//    con.write( (char*) (& n->getSubprobContext().at(0)), n->getSubprobContext().size() * sizeof(val_t));

  for (context_t::const_iterator it=n->getSubprobContext().begin();
       it!=n->getSubprobContext().end(); ++it) {
    int z = (int) *it;
    BINWRITE( con, z);
  }

  // write the PST
  // first, number of entries = depth of root node
  // write negative number to signal bottom-up traversal
  // (positive number means top-down, to be compatible with old versions)

  vector<double> pst;
  n->getPST(pst);
  int pstSize = ((int) pst.size()) / -2;
  BINWRITE( con, pstSize); // write negative PST size

  for(vector<double>::iterator it=pst.begin();it!=pst.end();++it) {
    BINWRITE( con, *it);
  }

  con.close(); // done with subproblem file

  {
    ostringstream ss;
    ss << " PST ";
    for(vector<double>::iterator it=pst.begin();it!=pst.end();++it) {
      ss << ' ' << *it; ++it; ss << '|' << *it;
    }
    ss << endl;
    myprint(ss.str());
  }

  // write complexity estimates to disk
  ostringstream estimateFile;
  estimateFile << PREFIX_EST << P->batch << '.' << P->process;
  ofstream estFile(estimateFile.str().c_str());
  if (!estFile) {
    GETLOCK(mtx_io,lk2);
    cerr << "Problem writing estimate file for thread " << P->threadID << '.' << endl;
    return "";
  }
  // ! processing scripts expect single line without carriage !
  estFile << P->subproblem->twb << '\t' << P->subproblem->hwb << '\t' << P->subproblem->width;
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
  << "transfer_input_files = " << m_spaceMaster->options->in_problemFile
  << ", " << m_spaceMaster->options->in_orderingFile
  << ", " << subprobFile.str();
  if (!m_spaceMaster->options->in_evidenceFile.empty())
    job << ", " << m_spaceMaster->options->in_evidenceFile;
  job << endl;

  // Build the argument list for the worker invocation
  ostringstream command;
  //command << m_spaceMaster->options->executableName << "-worker";
  command << " -f " << m_spaceMaster->options->in_problemFile;
  if (!m_spaceMaster->options->in_evidenceFile.empty())
    command << " -e " << m_spaceMaster->options->in_evidenceFile;
  command << " -o " << m_spaceMaster->options->in_orderingFile;
  command << " -s " << subprobFile.str();
  command << " -i " << m_spaceMaster->options->ibound;
  command << " -j " << m_spaceMaster->options->cbound_worker;
  command << " -c " << solutionFile.str();
  //command << " > /dev/null";

  // Add command line arguments to condor job
  job << "arguments = " << command.str() << endl;

  // Queue it
  job << "queue" << endl;
  {
    ostringstream ss;
    ss << "--> Subproblem " << P->threadID << " (" << P->batch
         << "." << P->process << "): "
         << P->subproblem->lowerBound << "/" << P->subproblem->upperBound
         << "\test: " << P->subproblem->estimate
//         << " " << P->subproblem->avgInc << "\test: " << P->subproblem->estInc
         << endl;
#ifdef DEBUG
    ss << "--------------------------------------" << endl << job.str()
	 << "--------------------------------------" << endl;
#endif
    myprint(ss.str());
  }
  return job.str();

}


#endif /* PARALLEL_MODE */

