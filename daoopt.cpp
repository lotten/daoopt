/*
 * daoopt.cpp
 *
 *  Created on: Oct 13, 2008
 *      Author: lars
 */

#include "Problem.h"
#include "Function.h"
#include "Graph.h"
#include "Pseudotree.h"
#include "ProgramOptions.h"
#include "MiniBucketElim.h"

#ifdef PARALLEL_MODE
//#include "ParallelManager.h"
#include "BranchAndBoundMaster.h"
#include "BoundPropagatorMaster.h"
#include "SubproblemHandler.h"
#include "SigHandler.h"
#else
#include "BranchAndBound.h"
#include "BoundPropagator.h"
#endif

#include "BestFirst.h"
#include "LimitedDiscrepancy.h"

#include <ctime>

#define VERSIONINFO "0.98.6"

/* define to enable diagnostic output of memory stats */
//#define MEMDEBUG

int main(int argc, char** argv) {

  // Record start time, end time, and preprocessing time
  time_t time_start, time_end, time_pre;
  time(&time_start);
  double time_passed;

  cout << "-----------------------------------------------" << endl
       << "DAOOPT " << VERSIONINFO;
#ifdef PARALLEL_MODE
  cout << " PARALLEL (";
#else
  cout << " STANDALONE (";
#endif
  cout << sizeof(val_t)*8 << ")";
#ifdef LIKELIHOOD
  cout << " for likelihood" << endl;
#else
#ifdef NO_ASSIGNMENT
  cout << " w/o assig." << endl;
#else
  cout << " w. assig." << endl;
#endif
#endif /* LIKELIHOOD */
  cout << "  by Lars Otten, UC Irvine <lotten@ics.uci.edu>" << endl
       << "-----------------------------------------------" << endl;

  // Reprint command line
  for (int i=0; i<argc; ++i)
    cout << argv[i] << ' ';
  cout << endl;

  ProgramOptions opt = parseCommandLine(argc,argv);

  /////////////////////

  cout
  << "+ i-bound:\t" << opt.ibound << endl
  << "+ j-bound:\t" << opt.cbound << endl
  << "+ Memory limit:\t" << opt.memlimit << endl
#ifdef PARALLEL_MODE
  << "+ Cutoff depth:\t" << opt.cutoff_depth << endl
  << "+ Cutoff size:\t" << opt.cutoff_size << endl
  << "+ Max. workers:\t" << opt.threads << endl
#endif
  ;
  /////////////////////

  Problem p;

  // Load problem instance
  p.parseUAI(opt.in_problemFile, opt.in_evidenceFile);
  cout << "Created problem with " << p.getN() << " variables and " << p.getC() << " functions." << endl;

  // Remove evidence variables
  p.removeEvidence();
  cout << "Removed evidence, now " << p.getN() << " variables and " << p.getC() << " functions." << endl;

  // Output reduced network?
  if (!opt.out_reducedFile.empty()) {
    cout << "Writing reduced network to file " << opt.out_reducedFile << endl;
    p.writeUAI(opt.out_reducedFile);
  }

  // Some statistics
  cout << "Max. domain size:\t" << (int) p.getK() << endl;
  cout << "Max. function arity:\t" << p.getR() << endl;

  // Create primal graph of *reduced* problem
  Graph g(p.getN());
  const vector<Function*>& fns = p.getFunctions();
  for (vector<Function*>::const_iterator it = fns.begin(); it != fns.end(); ++it) {
    g.addClique((*it)->getScope());
  }
  cout << "Graph with " << g.getStatNodes() << " nodes and " << g.getStatEdges() << " edges created." << endl;
//  cout << "Graph has " << g.noComponents() << " disconnected components." << endl;

  if (opt.seed != NONE)
    rand::seed(opt.seed);
  else
    rand::seed(time(0));

  // Find variable ordering
  vector<int> elim;
  bool orderFromFile = false;
  if (!opt.in_orderingFile.empty()) {
    orderFromFile = p.parseOrdering(opt.in_orderingFile, elim);
  }

  // Init. pseudo tree
  Pseudotree pt(&p);

  if (orderFromFile) { // Reading from file succeeded
    cout << "Read elimination ordering from file " << opt.in_orderingFile << '.' << endl;
    pt.build(g,elim,opt.cbound);
  } else {
    // Search for variable elimination ordering, looking for min. induced
    // width, breaking ties via pseudo tree height
    cout << "Searching for elimination ordering..." << flush;
    int w = INT_MAX;
    for (int i=opt.order_iterations; i; --i) {
      vector<int> elimCand;
      int new_w = Pseudotree::eliminate(g,elimCand,w);
      if (new_w < w) {
        elim = elimCand; w = new_w;
        pt.build(g,elimCand,opt.cbound);
        cout << ' ' << w <<'/' << pt.getHeight() << flush;
      } else if (new_w == w) {
        Pseudotree ptCand(&p);
        ptCand.build(g,elimCand,opt.cbound);
        if (ptCand.getHeight() < pt.getHeight()) {
          elim = elimCand;
          pt.build(g,elim,opt.cbound);
          cout << ' ' << w << '/' << pt.getHeight() << flush;
        }

      }
    }
    cout << endl<< "Lowest induced width found: " << w << endl;
    cout << "Lowest height for width: " << pt.getHeight() << endl;
  }

  // Save order to file?
#ifdef PARALLEL_MODE
  if (!orderFromFile) {
    if (opt.in_orderingFile.empty())
      opt.in_orderingFile = string("elim_") + p.getName() + string(".gz");
    p.saveOrdering(opt.in_orderingFile,elim);
  }
#else
  if (!opt.in_orderingFile.empty() && !orderFromFile) {
    p.saveOrdering(opt.in_orderingFile,elim);
  }
#endif

  // Complete pseudo tree init.
//  Pseudotree pt(&p);
//  pt.build(g,elim,opt.cbound); // will add dummy variable
  p.addDummy(); // add dummy variable to problem, to be in sync with pseudo tree
  pt.addFunctionInfo(p.getFunctions());
#ifdef PARALLEL_MODE
  int cutoff = pt.computeComplexities(opt.threads);
  cout << "Suggested cutoff:\t" << cutoff << " (ignored)" << endl;
//  if (opt.autoCutoff) {
//    cout << "Auto cutoff:\t\t" << cutoff << endl;
//    opt.cutoff_depth = cutoff;
//  }
#endif

  // The main search space
#ifdef PARALLEL_MODE
  SearchSpaceMaster* space = new SearchSpaceMaster(&pt,&opt);
#else
  SearchSpace* space = new SearchSpace(&pt,&opt);
#endif

#ifdef NO_HEURISTIC
  UnHeuristic mbe;
#else
  // Mini bucket heuristic
  MiniBucketElim mbe(&p,&pt,opt.ibound);
#endif

  // The search engine
#ifdef PARALLEL_MODE
  BranchAndBoundMaster search(&p,&pt,space,&mbe);
#else
  BranchAndBound search(&p,&pt,space,&mbe);
#endif

  // Subproblem specified? If yes, restrict.
  if (!opt.in_subproblemFile.empty()) {
    cout << "Reading subproblem from file " << opt.in_subproblemFile << '.' << endl;
    if (!search.restrictSubproblem(opt.in_subproblemFile) ) {
      delete space; return 1;
    }
  }

  cout << "Induced width:\t\t" << pt.getWidthCond() << " / " << pt.getWidth() << endl;
  cout << "Pseudotree depth:\t" << pt.getHeightCond() << " / " << pt.getHeight() << endl;
  cout << "Problem variables:\t" << pt.getSizeCond() <<  " / " << pt.getSize() << endl;
  cout << "Disconn. components:\t" << pt.getComponentsCond() << " / " << pt.getComponents() << endl;

#ifdef MEMDEBUG
  malloc_stats();
#endif

#ifndef NO_HEURISTIC
  int ibound = opt.ibound;
  if (opt.memlimit != NONE) {
    ibound = mbe.limitIbound(ibound, opt.memlimit, & search.getAssignment() );
    cout << "Enforcing memory limit resulted in i-bound " << ibound << endl;
    opt.ibound = ibound; // Write back into options object
  }

  // Build the MBE *after* restricting the subproblem
  size_t sz = 0;
  if (opt.nosearch) {
    cout << "Simulating mini bucket heuristic..." << endl;
    sz = mbe.build(& search.getAssignment(), false); // false = just compute memory estimate
  }
  else {
    cout << "Computing mini bucket heuristic..." << endl;
    sz = mbe.build(& search.getAssignment(), true); // true =  actually compute heuristic
  }
  cout << '\t' << (sz / (1024*1024.0)) * sizeof(double) << " MBytes" << endl;
#endif /* NO_HEURISTIC */

#ifdef MEMDEBUG
  malloc_stats();
#endif

  // set initial lower bound if provided (but only if no subproblem was specified)
  if (opt.in_subproblemFile.empty() ) {
    if (opt.in_boundFile.size()) {
      cout << "Loading initial lower bound from file " << opt.in_boundFile << '.' << endl;
      if (!search.loadInitialBound(opt.in_boundFile)) {
        delete space; return 1;
      }
    } else if (!ISNAN ( opt.initialBound )) {
      cout <<  "Setting external lower bound " << opt.initialBound << endl;
      search.setInitialBound( opt.initialBound );
    }
  }

  // Run LDS if specified
  if (opt.lds != NONE) {
    cout << "Running LDS with limit " << opt.lds << endl;
    SearchSpace* spaceLDS = new SearchSpace(&pt, &opt);
    LimitedDiscrepancy lds(&p,&pt,spaceLDS,&mbe,opt.lds);
    BoundPropagator propLDS(spaceLDS);
    SearchNode* n = lds.nextLeaf();
    while (n) {
      propLDS.propagate(n);
      n = lds.nextLeaf();
    }
    cout << "LDS: explored " << lds.getNoNodesAND() << '/' << lds.getNoNodesOR() << " AND/OR nodes" << endl;
    cout << "LDS: found optimal cost " << lds.getCurOptValue() << endl;
    if (lds.getCurOptValue() > search.curLowerBound()) {
      search.setInitialBound(lds.getCurOptValue());
#ifndef NO_ASSIGNMENT
      search.setInitialSolution(lds.getCurOptTuple());
#endif
    }
    delete spaceLDS;
  }

  // output (sub)problem lower bound, mostly makes sense for conditioned subproblems
  // in parallelized setting
  double lb = search.curLowerBound();
  cout << "Initial problem lower bound: " << lb << endl;

  // Record time after preprocessing
  time(&time_pre);
  time_passed = difftime(time_pre, time_start);
  cout << "Preprocessing complete: " << time_passed << " seconds" << endl;

  // abort before actual search?
  if (opt.nosearch) {
    cout << "'No search' mode requested, exiting." << endl;
    delete space;
    return 0;
  }

  // The propagation engine
#ifdef PARALLEL_MODE
  BoundPropagatorMaster prop(space);
#else
  BoundPropagator prop(space);
#endif

  cout << "--- Starting search ---" << endl;

#ifdef PARALLEL_MODE

  /*
  ParallelManager manager(&search,&prop,space);
  manager.run();
  */

  // take care of signal handling
  sigset_t new_signal_mask;//, old_signal_mask;
  sigfillset( & new_signal_mask );
  pthread_sigmask(SIG_BLOCK, &new_signal_mask, NULL); // block all signals

//  try {

    CondorSubmissionEngine cse(space);

    boost::thread thread_prop(boost::ref(prop));
    boost::thread thread_bab(boost::ref(search));
    boost::thread thread_cse(boost::ref(cse));

    // start signal handler
    SigHandler sigH(&thread_bab, &thread_prop, &thread_cse);
    boost::thread thread_sh(boost::ref(sigH));

    thread_bab.join();
    thread_prop.join();
    thread_cse.interrupt();
    thread_cse.join();
    cout << endl;

//  } catch (...) {}

  // unblock signals
  pthread_sigmask(SIG_UNBLOCK, &new_signal_mask, NULL);


#else
  SearchNode* n = search.nextLeaf();
  while (n) {
    prop.propagate(n);
    n = search.nextLeaf();
  }
#endif

  cout << endl << "-------- Search done --------" << endl;
#ifdef PARALLEL_MODE
  cout << "Condor jobs:   " << search.getThreadCount() << endl;
#endif
  cout << "OR nodes:      " << search.getNoNodesOR() << endl;
  cout << "AND nodes:     " << search.getNoNodesAND() << endl;
#ifdef PARALLEL_MODE
//  cout << "Subcount:      " << search.getSubCount() << endl;
#endif

  time(&time_end);
  time_passed = difftime(time_end,time_start);
  cout << "Time elapsed:  " << time_passed << " seconds" << endl;
  time_passed = difftime(time_pre,time_start);
  cout << "Preprocessing: " << time_passed << " seconds" << endl;

  double mpeCost = search.getCurOptValue();//space->getSolutionCost();

  // account for global constant (but not if solving subproblem)
  if (opt.in_subproblemFile.empty()) {
//    mpeCost OP_TIMESEQ p.getGlobalConstant();
  }

  cout << "-----------------------------\n"
    << SCALE_LOG(mpeCost) << " (" << SCALE_NORM(mpeCost) << ')' << endl;

  /*
  vector<val_t> mpeTuple = bab.getCurOptTuple();
  for (size_t i=0; i<mpeTuple.size(); ++i) {
    cout << ' ' << (int) mpeTuple[i];
  }
  cout << endl;
  */


  // Output node and leaf profiles per depth
  const vector<count_t>& prof = search.getNodeProfile();
  cout << endl << "p " << prof.size();
  for (vector<count_t>::const_iterator it=prof.begin(); it!=prof.end(); ++it) {
    cout << ' ' << *it;
  }
  const vector<count_t>& leaf = search.getLeafProfile();
  cout << endl << "l " << leaf.size();
  for (vector<count_t>::const_iterator it=leaf.begin(); it!=leaf.end(); ++it) {
    cout << ' ' << *it;
  }
  cout << endl;


  p.outputAndSaveSolution(opt.out_solutionFile, mpeCost, search.getNoNodes(),
#ifndef NO_ASSIGNMENT
      search.getCurOptTuple(),
#endif
      search.getNodeProfile(),search.getLeafProfile(),!opt.in_subproblemFile.empty() );

  cout << endl;

#ifdef MEMDEBUG
  malloc_stats();
#endif

  delete space;

  return EXIT_SUCCESS;
}
