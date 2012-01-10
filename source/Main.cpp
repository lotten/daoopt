/*
 * Main.cpp
 *
 *  Copyright (C) 2008-2012 Lars Otten
 *  This file is part of DAOOPT.
 *
 *  DAOOPT is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DAOOPT is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DAOOPT.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  Created on: Oct 18, 2011
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "Main.h"

#include "UAI2012.h"
string UAI2012::filename = "";

#define VERSIONINFO "0.99.6b-UAI12"

time_t _time_start, _time_pre;

bool Main::parseOptions(int argc, char** argv) {
  // Reprint command line
  for (int i=0; i<argc; ++i)
    cout << argv[i] << ' ';
  cout << endl;
  // parse command line
  ProgramOptions* opt = parseCommandLine(argc, argv);
  if (!opt) {
    err_txt("Error parsing command line.");
    return false;
  }

  if (opt->seed == NONE)
    opt->seed = time(0);
  rand::seed(opt->seed);

  m_options.reset(opt);

  size_t idx = m_options->in_problemFile.find_last_of('/');
  UAI2012::filename = m_options->in_problemFile.substr(idx + 1) + ".MPE";

  return true;
}


bool Main::loadProblem() {
  m_problem.reset(new Problem);

  // load problem file
  m_problem->parseUAI(m_options->in_problemFile, m_options->in_evidenceFile);
  cout << "Created problem with " << m_problem->getN()
       << " variables and " << m_problem->getC() << " functions." << endl;

  // Remove evidence variables
  m_problem->removeEvidence();
  cout << "Removed evidence, now " << m_problem->getN()
       << " variables and " << m_problem->getC() << " functions." << endl;

#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  // Re-output problem file for parallel processing
  m_options->out_reducedFile = string("temp_prob.") + m_options->problemName
      + string(".") + m_options->runTag + string(".gz");
  m_problem->writeUAI(m_options->out_reducedFile);
  cout << "Saved problem to file " << m_options->out_reducedFile << endl;
#else
  // Output reduced network?
  if (!m_options->out_reducedFile.empty()) {
    cout << "Writing reduced network to file " << m_options->out_reducedFile << endl;
    m_problem->writeUAI(m_options->out_reducedFile);
  }
#endif

  // Some statistics
  cout << "Global constant:\t" << m_problem->getGlobalConstant() << endl;
  cout << "Max. domain size:\t" << (int) m_problem->getK() << endl;
  cout << "Max. function arity:\t" << m_problem->getR() << endl;

  return true;
}


bool Main::findOrLoadOrdering() {
  // Create primal graph of *reduced* problem
  Graph g(m_problem->getN());
  const vector<Function*>& fns = m_problem->getFunctions();
  for (vector<Function*>::const_iterator it = fns.begin(); it != fns.end(); ++it) {
    g.addClique((*it)->getScope());
  }
  cout << "Graph with " << g.getStatNodes() << " nodes and "
       << g.getStatEdges() << " edges created." << endl;

#ifdef PARALLEL_STATIC
  // for static parallelization post-processing mode, look for
  // ordering from preprocessing step
  if (m_options->par_postOnly) {
    m_options->in_orderingFile = string("temp_elim.") + m_options->problemName
        + string(".") + m_options->runTag + string(".gz");
    m_options->order_iterations = 0;
  }
#endif

  // Find variable ordering
  vector<int> elim;
  int w = numeric_limits<int>::max();
  bool orderFromFile = false;
  if (!m_options->in_orderingFile.empty()) {
    orderFromFile = m_problem->parseOrdering(m_options->in_orderingFile, elim);
  }

  // Init. pseudo tree
  m_pseudotree.reset(new Pseudotree(m_problem.get(), m_options->subprobOrder));

  if (orderFromFile) { // Reading from file succeeded (i.e. file exists)
    m_pseudotree->build(g, elim, m_options->cbound);
    w = m_pseudotree->getWidth();
    cout << "Read elimination ordering from file " << m_options->in_orderingFile
         << " (" << w << '/' << m_pseudotree->getHeight() << ")." << endl;
  } else {
    if (m_options->order_timelimit == NONE)
      // compute at least one
      m_options->order_iterations = max(1, m_options->order_iterations);
  }

  time_t time_order_start, time_order_cur;
  double timediff = 0.0;
  time(&time_order_start);

  // Search for variable elimination ordering, looking for min. induced
  // width, breaking ties via pseudo tree height
  cout << "Searching for elimination ordering,";
  if (m_options->order_iterations != NONE)
    cout << " " << m_options->order_iterations << " iterations";
  if (m_options->order_timelimit != NONE)
    cout << " " << m_options->order_timelimit << " seconds";
  cout << ":" << flush;

  int iterCount=0, sinceLast=0;
  int remaining = m_options->order_iterations;

  while (true) {

    if (m_options->order_iterations != NONE && remaining == 0)
      break;

    vector<int> elimCand;  // new ordering candidate
    bool improved = false;  // improved in this iteration?
    int new_w = m_pseudotree->eliminate(g,elimCand,w);
    if (new_w < w) {
      elim = elimCand; w = new_w; improved = true;
      m_pseudotree->build(g, elimCand, m_options->cbound);
      cout << " " << iterCount << ':' << w <<'/' << m_pseudotree->getHeight() << flush;
    } else if (new_w == w) {
      Pseudotree ptCand(m_problem.get(), m_options->subprobOrder);
      ptCand.build(g, elimCand, m_options->cbound);
      if (ptCand.getHeight() < m_pseudotree->getHeight()) {
        elim = elimCand; improved = true;
        m_pseudotree->build(g, elim, m_options->cbound);
        cout << " " << iterCount << ':' << w << '/' << m_pseudotree->getHeight() << flush;
      }
    }
    ++iterCount, ++sinceLast, --remaining;

    // Adaptive ordering scheme
    if (improved && m_options->autoIter && remaining > 0) {
      remaining = max(remaining, sinceLast+1);
      sinceLast = 0;
    }

    // check termination conditions
    time(&time_order_cur);
    timediff = difftime(time_order_cur, time_order_start);
    if (m_options->order_timelimit != NONE && timediff > m_options->order_timelimit)
      break;
  }
  time(&time_order_cur);
  timediff = difftime(time_order_cur, time_order_start);
  cout << endl << "Ran " << iterCount << " iterations (" << int(timediff)
       << " seconds), lowest width/height found: "
       << w << '/' << m_pseudotree->getHeight() << '\n';

  // Save order to file?
  if (!m_options->in_orderingFile.empty() && !orderFromFile) {
    m_problem->saveOrdering(m_options->in_orderingFile, elim);
    cout << "Saved ordering to file " << m_options->in_orderingFile << endl;
  }
#if defined PARALLEL_DYNAMIC or defined PARALLEL_STATIC
#if defined PARALLEL_STATIC
  if (!m_options->par_postOnly) // no need to write ordering in post processing
#endif
  {
    m_options->in_orderingFile = string("temp_elim.") + m_options->problemName
        + string(".") + m_options->runTag + string(".gz");
    m_problem->saveOrdering(m_options->in_orderingFile,elim);
    cout << "Saved ordering to file " << m_options->in_orderingFile << endl;
  }
#endif

  // OR search?
  if (m_options->orSearch) {
    cout << "Rebuilding pseudo tree as chain." << endl;
    m_pseudotree->buildChain(g, elim, m_options->cbound);
  }

  // Pseudo tree has dummy node after build(), add to problem
  m_problem->addDummy(); // add dummy variable to problem, to be in sync with pseudo tree
  m_pseudotree->resetFunctionInfo(m_problem->getFunctions());
#if defined PARALLEL_DYNAMIC //or defined PARALLEL_STATIC
  int cutoff = m_pseudotree->computeComplexities(m_options->threads);
  cout << "Suggested cutoff:\t" << cutoff << " (ignored)" << endl;
//  if (opt.autoCutoff) {
//    cout << "Auto cutoff:\t\t" << cutoff << endl;
//    opt.cutoff_depth = cutoff;
//  }
#endif

  // Output pseudo tree to file for plotting?
  if (!m_options->out_pstFile.empty()) {
    m_pseudotree->outputToFile(m_options->out_pstFile);
  }

  if (m_options->maxWidthAbort != NONE &&
      m_options->maxWidthAbort < m_pseudotree->getWidth()) {
    oss msg;
    msg << "Problem instance with width w="<< m_pseudotree->getWidth()
        << " is too complex, aborting (limit is w="
        << m_options->maxWidthAbort << ")";
    err_txt(msg.str());
    return false;
  }
  return true;
}


bool Main::runSLS() {
#ifdef ENABLE_SLS
  if (!m_options->in_subproblemFile.empty())
    return true;  // no SLS in case of subproblem processing
  if (m_options->slsIter <= 0)
    return true;
  cout << "Running SLS " << m_options->slsIter << " times for "
       << m_options->slsTime << " seconds" << endl;
  m_slsWrapper.reset(new SLSWrapper());
  m_slsWrapper->init(m_problem.get(), m_options->slsIter, m_options->slsTime);
  m_slsWrapper->run();
  cout << "SLS finished" << endl;
#endif
  return true;
}


bool Main::initDataStructs() {

  // The main search space
#ifdef PARALLEL_DYNAMIC
  m_space.reset( new SearchSpaceMaster(m_pseudotree.get(), m_options.get()) );
#else
  m_space.reset( new SearchSpace(m_pseudotree.get(), m_options.get()) );
#endif

  // Heuristic is initialized here, built later in compileHeuristic()
#ifdef NO_HEURISTIC
  m_heuristic.reset(new Unheuristic);
#else
  if (m_options->match >= 0 || m_options->mplp >= 0 || m_options->jglp >= 0) {
    m_heuristic.reset(new MiniBucketElimMplp(m_problem.get(), m_pseudotree.get(),
					    m_options.get(), m_options->ibound) );
  } else {
    m_heuristic.reset(new MiniBucketElim(m_problem.get(), m_pseudotree.get(),
					 m_options.get(), m_options->ibound) );
  }
#endif

  // Main search engine
#if defined PARALLEL_DYNAMIC
  m_search.reset(new BranchAndBoundMaster(m_problem.get(), m_pseudotree.get(),
                                          m_space.get(), m_heuristic.get())); // TODO
#elif defined PARALLEL_STATIC
  m_search.reset(new ParallelManager(m_problem.get(), m_pseudotree.get(),
                                     m_space.get(), m_heuristic.get()));
#else
  if (m_options->rotate) {
    m_search.reset(new BranchAndBoundRotate(
        m_problem.get(), m_pseudotree.get(), m_space.get(), m_heuristic.get()));
  } else {
    m_search.reset(new BranchAndBound(
        m_problem.get(), m_pseudotree.get(), m_space.get(), m_heuristic.get()));
  }
#endif

  // Subproblem specified? If yes, restrict.
  if (!m_options->in_subproblemFile.empty()) {
    if (m_options->in_orderingFile.empty()) {
      err_txt("Subproblem specified but no ordering given.");
      return false;
    }else {
      m_problem->setSubprobOnly();
      m_options->order_iterations = 0;
      cout << "Reading subproblem from file " << m_options->in_subproblemFile << '.' << endl;
      if (!m_search->restrictSubproblem(m_options->in_subproblemFile) ) {
        err_txt("Subproblem restriction failed.");
        return false;
      }
    }
  }

  cout << "Induced width:\t\t" << m_pseudotree->getWidthCond()
       << " / " << m_pseudotree->getWidth() << endl;
  cout << "Pseudotree depth:\t" << m_pseudotree->getHeightCond()
       << " / " << m_pseudotree->getHeight() << endl;
  cout << "Problem variables:\t" << m_pseudotree->getSizeCond()
       <<  " / " << m_pseudotree->getSize() << endl;
  cout << "Disconn. components:\t" << m_pseudotree->getComponentsCond()
       << " / " << m_pseudotree->getComponents() << endl;

#ifdef MEMDEBUG
  malloc_stats();
#endif

  return true;
}


bool Main::preprocessHeuristic() {
  m_options->ibound = min(m_options->ibound, m_pseudotree->getWidthCond());
  size_t sz = 0;
  if (m_options->memlimit != NONE) {
    sz = m_heuristic->limitSize(m_options->memlimit,
                                & m_search->getAssignment() );
    sz *= sizeof(double) / (1024*1024.0);
    cout << "Enforcing memory limit resulted in i-bound " << m_options->ibound
         << " with " << sz << " MByte." << endl;
  }
  if (m_options->nosearch) {
    cout << "Skipping heuristic preprocessing..." << endl;
    return false;
  }

  if (m_heuristic->preprocess(& m_search->getAssignment()))
    m_pseudotree->resetFunctionInfo(m_problem->getFunctions());

  return true;
}


bool Main::compileHeuristic() {
  size_t sz = 0;
  if (m_options->nosearch) {
    cout << "Simulating mini bucket heuristic..." << endl;
    sz = m_heuristic->build(& m_search->getAssignment(), false); // false = just compute memory estimate
  } else {
    time(&_time_pre);
    bool mbFromFile = false;
    if (m_options->in_minibucketFile.empty()) {
      mbFromFile = m_heuristic->readFromFile(m_options->in_minibucketFile);
      sz = m_heuristic->getSize();
    }
    if (!mbFromFile) {
      cout << "Computing mini bucket heuristic..." << endl;
      sz = m_heuristic->build(& m_search->getAssignment(), true); // true =  actually compute heuristic
      time_t cur_time;
      time(&cur_time);
      double time_passed = difftime(cur_time, _time_pre);
      cout << "\tMini bucket finished in " << time_passed << " seconds" << endl;
    }
    if (!mbFromFile && !m_options->in_minibucketFile.empty()) {
      cout << "\tWriting mini bucket to file " << m_options->in_minibucketFile << " ..." << flush;
      m_heuristic->writeToFile(m_options->in_minibucketFile);
      cout << " done" << endl;
    }
  }
  cout << '\t' << (sz / (1024*1024.0)) * sizeof(double) << " MBytes" << endl;

  // heuristic might have changed problem functions, pseudotree needs remapping
  m_pseudotree->resetFunctionInfo(m_problem->getFunctions());

  // set initial lower bound if provided (but only if no subproblem was specified)
  if (m_options->in_subproblemFile.empty() ) {
    if (m_options->in_boundFile.size()) {
      cout << "Loading initial lower bound from file " << m_options->in_boundFile << '.' << endl;
      if (!m_search->loadInitialBound(m_options->in_boundFile)) {
        err_txt("Loading initial bound failed");
        return false;
      }
    } else if (!ISNAN ( m_options->initialBound )) {
#ifdef NO_ASSIGNMENT
      cout <<  "Setting external lower bound " << m_options->initialBound << endl;
      m_search->setInitialSolution(m_options->initialBound);
#else
      err_txt("Compiled with tuple support, value-based bound not possible.");
      return false;
#endif
    }
  }

#ifndef NO_HEURISTIC
  if (m_search->getCurOptValue() >= m_heuristic->getGlobalUB()) {
    m_solved = true;
    cout << endl << "--------- Solved during preprocessing ---------" << endl;
  }
  else if (m_heuristic->isAccurate()) {
    cout << endl << "Heuristic is accurate!" << endl;
    m_options->lds = 0; // set LDS to 0 (sufficient given perfect heuristic)
    m_solved = true;
  }
#endif

  return true;
}


bool Main::runLDS() {

  // Run LDS if specified
  if (m_options->lds != NONE) {
    cout << "Running LDS with limit " << m_options->lds << endl;
    scoped_ptr<SearchSpace> spaceLDS(new SearchSpace(m_pseudotree.get(), m_options.get()));
    LimitedDiscrepancy lds(m_problem.get(), m_pseudotree.get(), spaceLDS.get(),
                           m_heuristic.get(), m_options->lds);
    if (!m_options->in_subproblemFile.empty()) {
      if (!lds.restrictSubproblem(m_options->in_subproblemFile))
        err_txt("Subproblem restriction for LDS failed.");
        return false;
    }

#ifdef ENABLE_SLS
    if (m_options->slsIter > 0) {
      vector<val_t> slsTup;
      double slsSol = m_slsWrapper->getSolution(&slsTup);
      lds.setInitialSolution(slsSol
#ifndef NO_ASSIGNMENT
          ,slsTup
#endif
      );
      cout << "LDS: Solution from SLS loaded: " << slsSol << endl;
    }
#endif

    BoundPropagator propLDS(m_problem.get(), spaceLDS.get(), false);  // doCaching = false
    SearchNode* n = lds.nextLeaf();
    while (n) {
      propLDS.propagate(n,true); // true = report solution
      n = lds.nextLeaf();
    }
    cout << "LDS: explored " << spaceLDS->nodesOR << '/' << spaceLDS->nodesAND
         << " OR/AND nodes" << endl;
    cout << "LDS: solution cost " << lds.getCurOptValue() << endl;
    if (lds.getCurOptValue() > m_search->curLowerBound()) {
      cout << "LDS: updating initial lower bound" << endl;
      m_search->setInitialSolution(lds.getCurOptValue()
#ifndef NO_ASSIGNMENT
          , lds.getCurOptTuple()
#endif
      );
    }
#ifndef NO_HEURISTIC
    if (m_search->getCurOptValue() >= m_heuristic->getGlobalUB()) {
      m_solved = true;
      cout << endl << "--------- Solved by LDS ---------" << endl;
    }
#endif
  }

  return true;
}


bool Main::finishPreproc() {

#ifdef ENABLE_SLS
  // pull solution from SLS, if applicable
  if (m_options->slsIter > 0) {
    vector<val_t> slsTup;
    double slsSol = m_slsWrapper->getSolution(&slsTup);
    m_search->setInitialSolution(slsSol
#ifndef NO_ASSIGNMENT
        ,slsTup
#endif
    );
    cout << "Passed SLS solution to search: " << slsSol << endl;
  }
#endif

  // output (sub)problem lower bound, mostly makes sense for conditioned subproblems
  // in parallelized setting
  double lb = m_search->curLowerBound();
  cout << "Initial problem lower bound: " << lb << endl;

  // Record time after preprocessing
  time(&_time_pre);
  double time_passed = difftime(_time_pre, _time_start);
  cout << "Preprocessing complete: " << time_passed << " seconds" << endl;

  return true;
}


#ifdef PARALLEL_DYNAMIC
/* dynamic master mode for distributed execution */
bool Main::runSearchDynamic() {

  if (!m_solved) {
    // The propagation engine
    BoundPropagatorMaster prop(m_problem.get(), m_space.get());

    // take care of signal handling
    sigset_t new_signal_mask;//, old_signal_mask;
    sigfillset( & new_signal_mask );
    pthread_sigmask(SIG_BLOCK, &new_signal_mask, NULL); // block all signals

    try {
    CondorSubmissionEngine cse(m_space.get());

    boost::thread thread_prop(boost::ref(prop));
    boost::thread thread_bab(boost::ref( *m_search ));
    boost::thread thread_cse(boost::ref(cse));

    // start signal handler
    SigHandler sigH(&thread_bab, &thread_prop, &thread_cse);
    boost::thread thread_sh(boost::ref(sigH));

    thread_bab.join();
    thread_prop.join();
    thread_cse.interrupt();
    thread_cse.join();
    cout << endl;
    } catch (...) {
      myerror("Caught signal during master execution, aborting.\n");
      return false;
    }

    // unblock signals
    pthread_sigmask(SIG_UNBLOCK, &new_signal_mask, NULL);

  } else {  // !m_solved
    BoundPropagator prop(m_problem.get(), m_space.get());
    SearchNode* n = m_search->nextLeaf();
    while (n) {
      prop.propagate(n, true);
      n = m_search->nextLeaf();
    }
  }
  return true;
}
#endif


#ifdef PARALLEL_STATIC
/* static master mode for distributed execution */
bool Main::runSearchStatic() {
  bool success = true;
  bool preOnly = m_options->par_preOnly, postOnly = m_options->par_postOnly;

  if (!postOnly) { // full or pre-only mode
    success = success && m_search->doLearning();
    /* find frontier from scratch */
    success = success && m_search->findFrontier();
    /* generate files for subproblems */
    success = success && m_search->writeJobs();
    if (m_search->getSubproblemCount()==0) m_solved = true;
  } else { // post-only mode
    /* restore frontier from file */
    success = success && m_search->findFrontier(); // TODO
    //success = success && search.restoreFrontier();
  }
  if (!preOnly && !postOnly) { // full mode
    /* run Condor and wait for results */
    success = success && m_search->runCondor();
  }
  if (!preOnly) { // full or post-only mode
    /* reads external results */
    success = success && m_search->readExtResults();
  }

  if (!success) {
    myprint("!!! Search failed. !!!\n");
    err_txt("Main search routine failed.");
    return false;
  }

  return true;
}
#endif


/* sequential mode or worker mode for distributed execution */
bool Main::runSearchWorker() {
  BoundPropagator prop(m_problem.get(), m_space.get());
  SearchNode* n = m_search->nextLeaf();
  while (n) {
    prop.propagate(n, true); // true = report solutions
    n = m_search->nextLeaf();
  }

  m_solved = true;
  return true;
}


bool Main::outputStats() const {
  // Output cache statistics
  m_space->cache->printStats();

  cout << endl << "--------- Search done ---------" << endl;
  cout << "Problem name:  " << m_problem->getName() << endl;
#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  cout << "Condor jobs:   " << m_search->getSubproblemCount() << endl;
#endif
  cout << "OR nodes:      " << m_space->nodesOR << endl;
  cout << "AND nodes:     " << m_space->nodesAND << endl;
#if defined PARALLEL_STATIC
  cout << "OR external:   " << m_space->nodesORext << endl;
  cout << "AND external:  " << m_space->nodesANDext << endl;
#endif

#ifdef PARALLEL_STATIC
  if (m_options->par_preOnly && m_solved) {
    ofstream slvd("SOLVED");
    slvd << "SOLVED" << endl;
    slvd.close();
  }
#endif

  time_t time_end;
  time(&time_end);
  double time_passed = difftime(time_end, _time_start);
  cout << "Time elapsed:  " << time_passed << " seconds" << endl;
  time_passed = difftime(_time_pre, _time_start);
  cout << "Preprocessing: " << time_passed << " seconds" << endl;
  cout << "-------------------------------" << endl;

#ifdef PARALLEL_STATIC
  if (!m_options->par_preOnly || m_solved) { // parallel static: only output if solved
#endif

  double mpeCost = m_search->getCurOptValue();
  cout << SCALE_LOG(mpeCost) << " (" << SCALE_NORM(mpeCost) << ')' << endl;

  // Output node and leaf profiles per depth
  const vector<count_t>& prof = m_search->getNodeProfile();
  cout << endl << "p " << prof.size();
  for (vector<count_t>::const_iterator it=prof.begin(); it!=prof.end(); ++it) {
    cout << ' ' << *it;
  }
  const vector<count_t>& leaf = m_search->getLeafProfile();
  cout << endl << "l " << leaf.size();
  for (vector<count_t>::const_iterator it=leaf.begin(); it!=leaf.end(); ++it) {
    cout << ' ' << *it;
  }
  cout << endl;

  pair<size_t,size_t> noNodes = make_pair(m_space->nodesOR, m_space->nodesAND);
  m_problem->outputAndSaveSolution(m_options->out_solutionFile, noNodes,
      m_search->getNodeProfile(), m_search->getLeafProfile(),
      !m_options->in_subproblemFile.empty() );
#ifdef PARALLEL_STATIC
  }
#endif

  cout << endl;
  return true;
}


bool Main::start() const {

  time(&_time_start);

  // compile version string
  string version = "DAOOPT ";
  version += VERSIONINFO;

#ifdef PARALLEL_DYNAMIC
  version += " PARALLEL-DYNAMIC (";
#elif defined PARALLEL_STATIC
  version += " PARALLEL-STATIC (";
#else
  version += " STANDALONE (";
#endif
  version += boost::lexical_cast<std::string>(sizeof(val_t)*8);

#ifdef NO_ASSIGNMENT
  version += ") w/o assig.";
#else
  version += ") w. assig.";
#endif

#if defined(ANYTIME_DEPTH)
 version += " /dive";
#endif

#if defined LIKELIHOOD
  version += " /likelihood";
#endif

  ostringstream oss;

  oss << "------------------------------------------------------------------" << endl
      << version << endl
      << "  by Lars Otten, UC Irvine <lotten@ics.uci.edu>" << endl
      << "------------------------------------------------------------------" << endl;

  cout << oss.str();

  return true;
}

bool Main::outputInfo() const {
  assert(m_options.get());

#if defined PARALLEL_DYNAMIC or defined PARALLEL_STATIC
  if (m_options->runTag == "") {
    m_options->runTag = "notag";
  }
#endif

  ostringstream oss;
  oss
  << "+ i-bound:\t" << m_options->ibound << endl
  << "+ j-bound:\t" << m_options->cbound << endl
  << "+ Memory limit:\t" << m_options->memlimit << endl
  << "+ Suborder:\t" << m_options->subprobOrder << " ("
  << subprob_order[m_options->subprobOrder] <<")"<< endl
  << "+ Random seed:\t" << m_options->seed << endl
#if defined PARALLEL_DYNAMIC or defined PARALLEL_STATIC
  << "+ Cutoff depth:\t" << m_options->cutoff_depth << endl
  << "+ Cutoff size:\t" << m_options->cutoff_size << endl
  << "+ Max. workers:\t" << m_options->threads << endl
  << "+ Run tag:\t" << m_options->runTag << endl
#else
  << "+ rotate:\t" << ((m_options->rotate) ? "on" : "off") << endl
#endif
  ;

 cout << oss.str();
 return true;
}


// EoF
