/*
 * ParallelManager.cpp
 *
 *  Created on: Apr 11, 2010
 *      Author: lars
 */

#undef DEBUG

#include "ParallelManager.h"

#ifdef PARALLEL_STATIC

/* parameters that can be modified */
#define MAX_SUBMISSION_TRIES 6
#define SUBMISSION_FAIL_DELAY_BASE 2

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



bool ParallelManager::run() {

  SearchNode* node;
  double eval;

  // split subproblems
  while (m_open.size() && (int) m_open.size() < m_space->options->threads) {

    eval = m_open.top().first;
    node = m_open.top().second;
    m_open.pop();

    syncAssignment(node);

    if (doProcess(node)) {
      m_prop.propagate(node,true); continue;
    }
    if (doCaching(node)) {
      m_prop.propagate(node,true); continue;
    }
    if (doPruning(node)) {
      m_prop.propagate(node,true); continue;
    }
    if (deepenFrontier(node)) {
      m_prop.propagate(node,true); continue;
    }

  }

  // move from open stack to external queue
  m_external.reserve(m_open.size());
  while (m_open.size()) {
    m_open.top().second->setExtern();
    m_external.push_back(m_open.top().second);
    m_open.pop();
  }

  ostringstream ss;
  ss << "Generated " << m_external.size() << " external subproblems, " << m_local.size() << " local" << endl;
  myprint(ss.str());

  for (vector<SearchNode*>::iterator it = m_local.begin(); it!=m_local.end(); ++it) {
    solveLocal(*it);
  }

  // generates files for grid jobs and submits
  if (m_external.size()) {
    if (!submitToGrid())
      return false;
  }

  if (m_subprobCount) {
    // wait for grid jobs to finish
    if (!waitForGrid())
      return false;
    myprint("External jobs done.\n");

    // read results for external nodes
    if (!readExtResults())
      return false;
    myprint("Parsed external results.\n");

//    for (count_t i=0; i<m_subprobCount; ++i)
//      m_prop.propagate(m_external.at(i), true);
  }

  return true; // default: success

}


bool ParallelManager::submitToGrid() {

  const vector<SearchNode*>& nodes = m_external;

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
  ostringstream jobstr;
  jobstr
  // Generic options
  << "universe = vanilla" << endl
  << "notification = never" << endl
  << "should_transfer_files = yes" << endl
  << "requirements = ( Arch == \"X86_64\" || Arch == \"INTEL\" ) && (" << reqStr << ")" << endl
  << "when_to_transfer_output = always" << endl
  << "executable = daoopt.$$(Arch)" << endl
  << "copy_to_spool = false" << endl;

  // custom attributes
  jobstr
  << '+' << CONDOR_ATTR_PROBLEM << " = \"" << m_space->options->problemName << "\"" << endl;

  // Logging options
  jobstr << endl
  << "output = " << PREFIX_STDOUT << "$(Process).txt" << endl
  << "error = " << PREFIX_STDERR << "$(Process).txt" << endl
  << "log = " << PREFIX_LOG << "txt" << endl << endl; // one global log

  // encode actual subproblems
  for (vector<SearchNode*>::const_iterator it=nodes.begin(); it!=nodes.end(); ++it) {
    syncAssignment(*it);
    string job = encodeJob(*it,m_subprobCount);
    if (job == "") return false;
    jobstr << job;
    m_subprobCount += 1;
  }


  // write subproblem file
  ofstream file(JOBFILE);
  if (!file) {
    myerror("Error writing Condor job file.\n");
    return false;
  }
  file << jobstr.str();
  file.close();

  // Perform actual submission
  ostringstream submitCmd;
  submitCmd << CONDOR_SUBMIT << ' ' << JOBFILE;
  submitCmd << " > /dev/null";

  int noTries = 0;
  bool success = false;

  while (!success) {

    if ( !system( submitCmd.str().c_str() ) ) // returns 0 if successful
      success = true;

    if (!success) {
      if (noTries++ == MAX_SUBMISSION_TRIES) {
        ostringstream ss;
        ss << "Failed invoking condor_submit." << endl;
        myerror(ss.str());
        return false;
      } else {
        myprint("Problem invoking condor_submit, retrying...\n");
        sleep(pow( SUBMISSION_FAIL_DELAY_BASE ,noTries+1)); // wait increases exponentially
      }
    } else {
      // submission successful
      ostringstream ss;
      ss << "----> Submitted " << m_subprobCount << " jobs." << endl;
      myprint(ss.str());
    }

  } // while (!success)

  return true;

}


bool ParallelManager::waitForGrid() const {

  ostringstream waitCmd;
  waitCmd << CONDOR_WAIT << ' ' << PREFIX_LOG << "txt";
  waitCmd << " > /dev/null";

  // wait for the job to finish
  if ( system(waitCmd.str().c_str()) ) {
    cerr << "Error invoking condor_wait." << endl;
    return false; // error!
  }

  return true; // default: success

}


bool ParallelManager::readExtResults() {

  bool success = true;

  for (size_t id=0; id<m_subprobCount; ++id) {

    SearchNode* node = m_external.at(id);

    // Read solution from file
    ostringstream solutionFile;
    solutionFile << PREFIX_SOL << id << ".gz";
    {
      ifstream inTemp(solutionFile.str().c_str());
      inTemp.close();
      if (inTemp.fail()) {
        ostringstream ss;
        ss << "Problem reading subprocess solution for job " << id << '.' << endl;
        myerror(ss.str());
        success = false;
      }
    }
    igzstream in(solutionFile.str().c_str(), ios::binary | ios::in);

    double optCost;
    BINREAD(in, optCost); // read opt. cost
//    if (ISNAN(optCost)) optCost = ELEM_ZERO;

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
    vector<count_t> leafP, nodeP;
    leafP.reserve(size);
    nodeP.reserve(size);
    for (int i=0; i<size; ++i) { // leaf profile
      BINREAD(in, c);
      leafP.push_back(c);
    }
    for (int i=0; i<size; ++i) { // full node profile
      BINREAD(in, c);
      nodeP.push_back(c);
    }

    // done reading input file
    in.close();

    // Write subproblem solution value and tuple into search node
    node->setValue(optCost);
#ifndef NO_ASSIGNMENT
    node->setOptAssig(tup);
#endif

    ostringstream ss;
    ss  << "Read solution file " << id << " (" << *node
        << ") " << nodesOR << " / " << nodesAND
        << " v:" << node->getValue()
#ifndef NO_ASSIGNMENT
//    << " -assignment " << node->getOptAssig()
#endif
    << endl;
    myprint(ss.str());

    // Propagate
    m_prop.propagate(node,true);

  }

  return success;

}


string ParallelManager::encodeJob(SearchNode* node, size_t id) const {

  assert(node);

  int rootVar = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(rootVar);

  // create subproblem file
  ostringstream subprobFile;

  subprobFile << PREFIX_SUB << id << ".gz";
  ogzstream con(subprobFile.str().c_str(), ios::out | ios::binary );
  if(!con) {
    ostringstream ss;
    ss << "Problem writing subproblem file " << id << " - skipping"<< endl;
    myerror(ss.str());
    return "";
  }

  // subproblem root variable
  BINWRITE( con, rootVar );
  // subproblem context size
  const set<int>& ctxt = ptnode->getFullContext();
  int sz = ctxt.size();
  BINWRITE( con, sz );

  // write context instantiation
  for (set<int>::const_iterator it=ctxt.begin(); it!=ctxt.end(); ++it) {
    int z = (int) m_assignment.at(*it);
    BINWRITE( con, z);
  }

  // write the PST
  // first, number of entries = depth of root node
  // write negative number to signal bottom-up traversal
  // (positive number means top-down, to be compatible with old versions)
  vector<double> pst;
  node->getPST(pst);
  int pstSize = ((int) pst.size()) / -2;
  BINWRITE( con, pstSize); // write negative PST size

  for(vector<double>::iterator it=pst.begin();it!=pst.end();++it) {
    BINWRITE( con, *it);
  }

  con.close(); // done with subproblem file

  // Where the solution will be read from
  ostringstream solutionFile;
  solutionFile << PREFIX_SOL << id << ".gz";

  // Build the condor job description
  ostringstream job;

  // Custom job attributes
  job
  << '+' << CONDOR_ATTR_THREADID << " = " << id << endl;

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
  command << " -f " << m_space->options->in_problemFile;
  if (!m_space->options->in_evidenceFile.empty())
    command << " -e " << m_space->options->in_evidenceFile;
  command << " -o " << m_space->options->in_orderingFile;
  command << " -s " << subprobFile.str();
  command << " -i " << m_space->options->ibound;
  command << " -j " << m_space->options->cbound_worker;
  command << " -c " << solutionFile.str();
  command << " -t 0" ;

  // Add command line arguments to condor job
  job << "arguments = " << command.str() << endl;

  // Queue it
  job << "queue" << endl;

  return job.str();

}


void ParallelManager::syncAssignment(SearchNode* node) {

  // only accept OR nodes
  assert(node && node->getType()==NODE_OR);

  while (node->getParent()) {
    node = node->getParent(); // AND node
    m_assignment.at(node->getVar()) = node->getVal();
    node = node->getParent(); // OR node
  }

}


bool ParallelManager::isEasy(SearchNode* node) const {
  assert(node);

  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
//  int depth = ptnode->getDepth();
//  int height = ptnode->getSubHeight();
  int width = ptnode->getSubWidth();

  if (width <= m_space->options->cutoff_width)
    return true;

  return false; // default

}


/* expands a node -- assumes OR node! Will generate descendant OR nodes
 * (via intermediate AND nodes) and put them on the OPEN list */
bool ParallelManager::deepenFrontier(SearchNode* n) {

  assert(n && n->getType() == NODE_OR);
  vector<SearchNode*> chi, chi2;

#ifdef DEBUG
  ostringstream ss;
#endif

  if (generateChildrenOR(n,chi))
    return true; // no children

  for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it) {
    DIAG( ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; )
    if (generateChildrenAND(*it,chi2)) {
      m_prop.propagate(*it, true);
      continue;
    }
    DIAG( for (vector<SearchNode*>::iterator it=chi2.begin(); it!=chi2.end(); ++it) ss << "\t  " << *it << ": " << *(*it) << endl; )
    for (vector<SearchNode*>::iterator it=chi2.begin(); it!=chi2.end(); ++it) {
      if (isEasy(*it))
        m_local.push_back(*it);
      else
        m_open.push(make_pair(evaluate(*it),*it));
    }
    chi2.clear();
  }

  DIAG(myprint(ss.str()));

  return false; // default false

}


double ParallelManager::evaluate(SearchNode* node) const {
  assert(node && node->getType() == NODE_OR);

  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
//  int depth = ptnode->getDepth();
  int height = ptnode->getSubHeight();
  int width = ptnode->getSubWidth();

  double lb = lowerBound(node);
  double ub = node->getHeur();

  double d = (ub-lb) * pow(height,.5) * pow(width,.5);

  return d;
}


void ParallelManager::solveLocal(SearchNode* node) {

  assert(node && node->getType()==NODE_OR);

  DIAG(ostringstream ss; ss << "Solving subproblem locally: " << *node << endl; myprint(ss.str()));

  syncAssignment(node);

  // clear out stack first
  while (m_stack.size())
    m_stack.pop();
  // push subproblem root onto stack
  m_stack.push(node);

  while ( ( node=nextLeaf() ) )
    m_prop.propagate(node,true);

}


bool ParallelManager::doExpand(SearchNode* n) {

  assert(n);

  vector<SearchNode*> chi;

  if (n->getType() == NODE_AND) { /////////////////////////////////////

    if (generateChildrenAND(n,chi))
      return true; // no children

    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it)
      m_stack.push(*it);

  } else { // NODE_OR /////////////////////////////////////////////////

    if (generateChildrenOR(n,chi))
      return true; // no children

    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it) {
      m_stack.push(*it);
    } // for loop

  } // if over node type //////////////////////////////////////////////

  return false; // default false

}


SearchNode* ParallelManager::nextNode() {

  SearchNode* n = NULL;
  if (m_stack.size()) {
    n = m_stack.top();
    m_stack.pop();
  }
  return n;

}


void ParallelManager::setInitialBound(double d) const {
  assert(m_space);
  m_space->root->setValue(d);
}


#ifndef NO_ASSIGNMENT
void ParallelManager::setInitialSolution(const vector<val_t>& tuple) const {
  assert(m_space);
  m_space->root->setOptAssig(tuple);
}
#endif



ParallelManager::ParallelManager(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h)
  : Search(prob, pt, s, h), m_subprobCount(0), m_prop(prob, s)
{

  // create first node (dummy variable)
  PseudotreeNode* ptroot = m_pseudotree->getRoot();
  SearchNode* node = new SearchNodeOR(NULL, ptroot->getVar() );
  m_space->root = node;

  // create dummy variable's AND node (domain size 1)
  SearchNode* next = new SearchNodeAND(m_space->root, 0, prob->getGlobalConstant());
  // put global constant into dummy AND node label
  m_space->root->addChild(next);

  // one more OR node for OPEN list
  node = next;
  next = new SearchNodeOR(node, ptroot->getVar()) ;
  node->addChild(next);
#ifndef NO_HEURISTIC
  heuristicOR(next);
#endif

  // add to OPEN list // todo
  m_open.push(make_pair(1.0, next));

}

#endif /* PARALLEL_STATIC */

