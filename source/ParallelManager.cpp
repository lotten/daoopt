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
#define PREFIX_SOL "temp_sol."
#define PREFIX_EST "temp_est."
#define PREFIX_SUB "temp_sub."
#define PREFIX_LOG "temp_log."
#define PREFIX_STATS "temp_stats."

#define PREFIX_JOBS "temp_jobs."
#define PREFIX_SUBPROB "temp_subproblems.gz"

#define TEMPLATE_FILE "daoopt-template.condor"

/* custom attributes for generated condor jobs */
#define CONDOR_ATTR_PROBLEM "daoopt_problem"
#define CONDOR_ATTR_THREADID "daoopt_threadid"

/* default job submission template */
const string default_job_template =
    "universe = vanilla \n\
    notification = never \n\
    should_transfer_files = yes \n\
    when_to_transfer_output = always \n\
    executable = daoopt.$$(Arch) \n\
    output = temp_std.%PROBLEM%.%TAG%.$(Process).txt \n\
    error = temp_err.%PROBLEM%.%TAG%.$(Process).txt \n\
    log = temp_log.%PROBLEM%.%TAG%.txt \n\
    +daoopt_problem = daoopt_%PROBLEM%_%TAG% \n\
    requirements = ( Arch == \"X86_64\" || Arch == \"INTEL\" ) \n\
    ";
bool ParallelManager::run() {

  SearchNode* node;
  double eval;

#ifndef NO_HEURISTIC
  // precompute heuristic of initial dummy OR node (couldn't be done earlier)
  heuristicOR(m_open.top().second);
#endif

  // split subproblems
  while (m_open.size()
      && (m_space->options->threads == NONE
          || (int) m_open.size() < m_space->options->threads)) {

    eval = m_open.top().first;
    node = m_open.top().second;
    m_open.pop();
    DIAG(oss ss; ss << "Top " << node <<"(" << *node << ") with eval " << eval << endl; myprint(ss.str());)

    if (m_space->options->cutoff_depth != NONE) {
      PseudotreeNode* ptnode = m_pseudotree->getNode(node->getVar());
      int d = ptnode->getDepth();
      if (d == m_space->options->cutoff_depth) {
        node->setExtern();
        m_external.push_back(node);
        continue;
      }
    }

    syncAssignment(node);
    deepenFrontier(node);

  }

  // move from open stack to external queue
  m_external.reserve(m_open.size());
  m_nextThreadId = m_open.size(); // thread count
  while (m_open.size()) {
    m_open.top().second->setExtern();
    m_external.push_back(m_open.top().second);
    m_open.pop();
  }

  ostringstream ss;
  ss << "Generated " << m_external.size() << " external subproblems, " << m_local.size() << " local" << endl;
  myprint(ss.str());

#ifdef DEBUG
  sleep(5);
#endif

  // generates files for grid jobs and submits
  if (m_external.size()) {
    if (!submitToGrid())
      return false;
  }

  for (vector<SearchNode*>::iterator it = m_local.begin(); it!=m_local.end(); ++it) {
    solveLocal(*it);
  }
  myprint("Local jobs (if any) done.\n");

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

  // include LDS node count into overall
//  m_space->nodesAND += m_ldsSpace->nodesAND;
//  m_space->nodesOR += m_ldsSpace->nodesOR;

  return true; // default: success

}


bool ParallelManager::submitToGrid() {

  const vector<SearchNode*>& nodes = m_external;

  /*
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
  */

  // Build the condor job description
  ostringstream jobstr;

  // Try to read template file, otherwise use default
  ifstream templ(TEMPLATE_FILE);
  if (templ) {
    string s;
    while (!templ.eof()) {
      getline(templ, s);
      s = str_replace(s,"%PROBLEM%",m_space->options->problemName);
      s = str_replace(s,"%TAG%",m_space->options->runTag);
      jobstr << s << endl;
    }
  } else {
    string s = default_job_template;
    s = str_replace(s,"%PROBLEM%",m_space->options->problemName);
    s = str_replace(s,"%TAG%",m_space->options->runTag);
    jobstr << s;
  }
  templ.close();

  // reset CSV file
  string csvfile = filename(PREFIX_STATS,".csv");
  ofstream csv(csvfile.c_str(),ios_base::out | ios_base::trunc);
  csv << "#id\troot\tdepth\tvars\tlb\tub\theight\twidth" << endl;
  csv.close();

  // encode actual subproblems
  string alljobs = encodeJobs(nodes);
  jobstr << alljobs;
  /*
  for (vector<SearchNode*>::const_iterator it=nodes.begin(); it!=nodes.end(); ++it) {
    syncAssignment(*it);
    string job = encodeJob(*it,m_subprobCount);
    if (job == "") return false;
    jobstr << job;
    m_subprobCount += 1;
  }
  */


  // write subproblem file
  string jobfile = filename(PREFIX_JOBS,".condor");
  ofstream file(jobfile.c_str());
  if (!file) {
    myerror("Error writing Condor job file.\n");
    return false;
  }
  file << jobstr.str();
  file.close();

  // Perform actual submission
  ostringstream submitCmd;
  submitCmd << CONDOR_SUBMIT << ' ' << jobfile;
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

  string logfile = filename(PREFIX_LOG,".txt");

  ostringstream waitCmd;
  waitCmd << CONDOR_WAIT << ' ' << logfile;
  waitCmd << " > /dev/null";

  // wait for the job to finish
  if ( system(waitCmd.str().c_str()) ) {
    cerr << "Error invoking "<< CONDOR_WAIT << endl;
    return false; // error!
  }

  return true; // default: success

}


bool ParallelManager::readExtResults() {

  bool success = true;

  // read current stats CSV
  string line,csvheader;
  vector<string> stats;
  string csvfile = filename(PREFIX_STATS,".csv");
  fstream csv(csvfile.c_str(), ios_base::in);
  getline(csv,csvheader);
  for (size_t id=0; id<m_subprobCount; ++id) {
    getline(csv,line);
    stats.push_back(line);
  }
  csv.close();

  for (size_t id=0; id<m_subprobCount; ++id) {

    SearchNode* node = m_external.at(id);

    // Read solution from file
    string solutionFile = filename(PREFIX_SOL,".gz",id);
    {
      ifstream inTemp(solutionFile.c_str());
      inTemp.close();
      if (inTemp.fail()) {
        ostringstream ss;
        ss << "Problem reading subprocess solution for job " << id << '.' << endl;
        myerror(ss.str());
        success = false;
        continue; // skip rest of loop
      }
    }
    igzstream in(solutionFile.c_str(), ios::binary | ios::in);

    double optCost;
    BINREAD(in, optCost); // read opt. cost
//    if (ISNAN(optCost)) optCost = ELEM_ZERO;

    count_t nodesOR, nodesAND;
    BINREAD(in, nodesOR);
    BINREAD(in, nodesAND);

    m_space->nodesORext += nodesOR;
    m_space->nodesANDext += nodesAND;

    {
    // for CSV output
    stringstream ss;
    ss << '\t' << nodesOR << '\t' << nodesAND;
    stats.at(id) += ss.str();
    }

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

  // rewrite CSV file
  csv.open(csvfile.c_str(), ios_base::out | ios_base::trunc);
  csv << csvheader << "\tOR\tAND" << endl;
  for (vector<string>::iterator it=stats.begin(); it!=stats.end(); ++it) {
    csv << *it << endl;
  }
  csv.close();

  return success;

}


string ParallelManager::encodeJobs(const vector<SearchNode*>& nodes) {

  // open CSV file for stats
  string csvfile = filename(PREFIX_STATS,".csv");
  ofstream csv(csvfile.c_str(),ios_base::out | ios_base::app);

  // Will hold the condor job description for the submission file
  ostringstream job;

  string subprobsFile = filename(PREFIX_SUB,".gz");
  ogzstream subprobs(subprobsFile.c_str(), ios::out | ios::binary);
  if (!subprobs) {
    ostringstream ss;
    ss << "Problem writing subproblem file"<< endl;
    myerror(ss.str());
    return "";
  }


  // no. of subproblems to file (abuse of id variable)
  size_t id=nodes.size();
  BINWRITE(subprobs,id);

  id = m_subprobCount = 0;
  int z = NONE;

  for (vector<SearchNode*>::const_iterator it=nodes.begin(); it!=nodes.end(); ++it, ++id) {

    const SearchNode* node = (*it);
    assert(node);
    syncAssignment(node);

    // write current subproblem id
    BINWRITE(subprobs, id);

    int rootVar = node->getVar();
    PseudotreeNode* ptnode = m_pseudotree->getNode(rootVar);

    /* subproblem root variable */
    BINWRITE( subprobs, rootVar );
    /* subproblem context size */
    const set<int>& ctxt = ptnode->getFullContext();
    int sz = ctxt.size();
    BINWRITE( subprobs, sz );

    /* write context instantiation */
    for (set<int>::const_iterator it=ctxt.begin(); it!=ctxt.end(); ++it) {
      z = (int) m_assignment.at(*it);
      BINWRITE( subprobs, z);
    }

    /* write the PST:
     * first, number of entries = depth of root node
     * write negative number to signal bottom-up traversal
     * (positive number means top-down, to be compatible with old versions)
     */
    vector<double> pst;
    node->getPST(pst);
    int pstSize = ((int) pst.size()) / -2;
    BINWRITE( subprobs, pstSize); // write negative PST size

    for(vector<double>::iterator it=pst.begin();it!=pst.end();++it) {
      BINWRITE( subprobs, *it);
    }

    // Where the solution will be read from
    string solutionFile = filename(PREFIX_SOL,".gz",id);

    // Custom job attributes
    job
    << '+' << CONDOR_ATTR_THREADID << " = " << id << endl;

    // Job specifics
    job
    // Make sure input files get transferred
    << "transfer_input_files = " << m_space->options->in_problemFile
    << ", " << m_space->options->in_orderingFile
    << ", " << subprobsFile;
    if (!m_space->options->in_evidenceFile.empty())
      job << ", " << m_space->options->in_evidenceFile;
    job << endl;

    // Build the argument list for the worker invocation
    ostringstream command;
    command << " -f " << m_space->options->in_problemFile;
    if (!m_space->options->in_evidenceFile.empty())
      command << " -e " << m_space->options->in_evidenceFile;
    command << " -o " << m_space->options->in_orderingFile;
    command << " -s " << subprobsFile << ":" << id;
    command << " -i " << m_space->options->ibound;
    command << " -j " << m_space->options->cbound_worker;
    command << " -c " << solutionFile;
    command << " -t 0" ;

    // Add command line arguments to condor job
    job << "arguments = " << command.str() << endl;

    // Queue it
    job << "queue" << endl;

    // write CSV output
    int depth = ptnode->getDepth();
    int height = ptnode->getSubHeight();
    int width = ptnode->getSubWidth();
    size_t vars = ptnode->getSubprobSize();

    double lb = lowerBound(node);
    double ub = node->getHeur();

    csv << id
        << '\t' << rootVar
        << '\t' << depth
        << '\t' << vars
        << '\t' << lb
        << '\t' << ub
        << '\t' << height
        << '\t' << width;
    csv << endl;

  } // for over nodes

  // update subproblem count
  m_subprobCount = id;

  // close subproblem file
  subprobs.close();

  // close CSV file
  csv.close();
  return job.str();

}


void ParallelManager::syncAssignment(const SearchNode* node) {

  // only accept OR nodes
  assert(node && node->getType()==NODE_OR);

  while (node->getParent()) {
    node = node->getParent(); // AND node
    m_assignment.at(node->getVar()) = node->getVal();
    node = node->getParent(); // OR node
  }

}


bool ParallelManager::isEasy(const SearchNode* node) const {
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

  // first generate intermediate AND nodes
  if (generateChildrenOR(n,chi))
    return true; // no children

  // for each AND node, generate OR children
  for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it) {
    DIAG(oss ss; ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; myprint(ss.str());)
    if (generateChildrenAND(*it,chi2)) {
      m_prop.propagate(*it, true);
      continue;
    }
    DIAG( for (vector<SearchNode*>::iterator it=chi2.begin(); it!=chi2.end(); ++it) {oss ss; ss << "\t  " << *it << ": " << *(*it) << endl; myprint(ss.str());} )
    for (vector<SearchNode*>::iterator it=chi2.begin(); it!=chi2.end(); ++it) {

      if (applyLDS(*it)) {// apply LDS. i.e. mini bucket forward pass
        m_ldsProp->propagate(*it,true);
        continue; // skip to next
      }

      if (doCaching(*it)) {
        m_prop.propagate(*it,true); continue;
      } else if (doPruning(*it)) {
        m_prop.propagate(*it,true); continue;
      } else if (isEasy(*it)) {
        m_local.push_back(*it);
      } else {
        m_open.push(make_pair(evaluate(*it),*it));
      }
    }
    chi2.clear(); // prepare for next AND node
  }

  return false; // default false

}


double ParallelManager::evaluate(const SearchNode* node) const {
  assert(node && node->getType() == NODE_OR);

  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int d = ptnode->getDepth();
  int h = ptnode->getSubHeight();
  int w = ptnode->getSubWidth();
  int n = ptnode->getSubprobSize();

  double L = lowerBound(node);
  double U = node->getHeur();

  //double z = (ub-lb) * pow(height,.5) * pow(width,.5);                                             
  double z = 2*(U-L) + h + 0.5 * log10(n);

  DIAG(oss ss; ss<< "eval " << *node << " : "<< z<< endl; myprint(ss.str()))

  return z;

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


bool ParallelManager::applyLDS(SearchNode* node) {

  assert(node);

  bool complete = false;
  PseudotreeNode* ptnode = m_pseudotree->getNode(node->getVar());
  if (ptnode->getSubWidth() <= m_space->options->ibound) {
    complete = true; // subproblem solved fully
  }

  m_ldsSearch->resetSearch(node);
  SearchNode* n = m_ldsSearch->nextLeaf();
  while (n) {
    m_ldsProp->propagate(n,true,node);
    n = m_ldsSearch->nextLeaf();
  }

  return complete;

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


string ParallelManager::filename(const char* pre, const char* ext, int count) const {
  ostringstream ss;
  ss << pre;
  ss << m_space->options->problemName;
  ss << ".";
  ss << m_space->options->runTag;
  if (count!=NONE) {
    ss << ".";
    ss << count;
  }
  ss << ext;

  return ss.str();
}


ParallelManager::ParallelManager(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h)
  : Search(prob, pt, s, h), m_subprobCount(0), m_prop(prob, s)
{

#ifndef NO_CACHING
  // Init context cache table
  m_space->cache = new CacheTable(prob->getN());
#endif

  // create first node (dummy variable)
  PseudotreeNode* ptroot = m_pseudotree->getRoot();
  SearchNode* node = new SearchNodeOR(NULL, ptroot->getVar() );
  m_space->root = node;

  // create dummy variable's AND node (unary domain)
  // and put global constant into node label
  SearchNode* next = new SearchNodeAND(m_space->root, 0, prob->getGlobalConstant());
  m_space->root->addChild(next);

  // one more dummy OR node for OPEN list
  node = next;
  next = new SearchNodeOR(node, ptroot->getVar()) ;
  node->addChild(next);

  // add to OPEN list // TODO
  m_open.push(make_pair(42.0, next));

  // set up LDS
  m_ldsSpace = new SearchSpace(m_pseudotree, m_space->options);
  m_ldsSpace->root = m_space->root;
  m_ldsSearch = new LimitedDiscrepancy(prob, pt, m_ldsSpace, h, 0);
  m_ldsProp = new BoundPropagator(prob, m_ldsSpace);

}

#endif /* PARALLEL_STATIC */

