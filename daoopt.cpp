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
#include "BranchAndBound.h"
#include "BoundPropagator.h"
#include "SubproblemHandler.h"
#include "ProgramOptions.h"
#include "MiniBucketElim.h"
#include "SigHandler.h"

#include <ctime>

#define VERSIONINFO "0.93c alpha"

// define to enable diagnostic output of memory stats
//#define MEMDEBUG

int main(int argc, char** argv) {

  // Record start time
  time_t time_start, time_end;
  time(&time_start);

  cout << "DAOOPT " << VERSIONINFO ;
#ifdef USE_THREADS
  cout << " PARALLEL (";
#else
  cout << " STANDALONE (";
#endif
  {
    val_t temp=0;
    cout << typeid(temp).name() << ")" << endl;
  }

  ProgramOptions opt = parseCommandLine(argc,argv);

  /////////////////////

  cout
  << "+ i-bound:\t" << opt.ibound << endl
  << "+ j-bound:\t" << opt.cbound << endl
#ifdef USE_THREADS
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

  // Some statistics
  cout << "Max. domain size:\t" << (int) p.getK() << endl;
  cout << "Max. function arity:\t" << p.getR() << endl;

  // Create primal graph
  Graph g(p.getN());
  const vector<Function*>& fns = p.getFunctions();
  for (vector<Function*>::const_iterator it = fns.begin(); it != fns.end(); ++it) {
    g.addClique((*it)->getScope());
  }
  cout << "Graph with " << g.getStatNodes() << " nodes and " << g.getStatEdges() << " edges created." << endl;

  rand::seed(time(0));

  // Find variable ordering
  vector<int> elim;
  bool orderFromFile = false;
  if (!opt.in_orderingFile.empty()) {
    orderFromFile = p.parseOrdering(opt.in_orderingFile, elim);
  }

  if (orderFromFile) { // Reading from file succeeded
    cout << "Read elimination ordering from file " << opt.in_orderingFile << '.' << endl;
  } else {
    // Search for variable elimination ordering
    cout << "Searching for elimination ordering..." << flush;
    int w = INT_MAX;
    for (int i=opt.order_iterations; i; --i) {
      vector<int> elimCand;
      int new_w = Pseudotree::eliminate(g,elimCand);
      if (new_w < w) {
        elim = elimCand; w = new_w;
        cout << ' ' << w << flush;
      }
    }
    cout << endl<< "Lowest induced width found: " << w << endl;
  }

  // Save order to file?
#ifdef USE_THREADS
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

  // Build pseudo tree
  Pseudotree pt(p.getN());
  pt.build(g,elim,opt.cbound); // will add dummy variable
  p.addDummy(); // add dummy variable to problem, to be in sync with pseudo tree
  pt.addFunctionInfo(p.getFunctions());
#ifdef USE_THREADS
  pt.computeComplexities(p);
#endif

#ifdef DEBUG
  //cout << "Elimination: " << elim << endl;
#endif

  cout << "Induced width:\t\t" << pt.getWidth() << endl;
  cout << "Pseudotree depth:\t" << pt.getDepth() << endl;
  cout << "Disconn. components:\t" << pt.getComponents() << endl;


  // The central search space
  SearchSpace* space = new SearchSpace(&opt);

  // Mini bucket heuristic
  MiniBucketElim mbe(&p,&pt,opt.ibound);

  // The search engine
  BranchAndBound bab(&p,&pt,space,&mbe);

  // Subproblem specified? If yes, restrict.
  if (!opt.in_subproblemFile.empty()) {
    bab.restrictSubproblem(opt.in_subproblemFile);
    cout << "Read subproblem from file " << opt.in_subproblemFile << '.' << endl;
  }

#ifdef MEMDEBUG
  malloc_stats();
#endif

  // Build the MBE *after* restricting the subproblem
  size_t sz = 0;
  if (opt.nosearch) {
    cout << "Simulating mini bucket heuristic..." << endl;
    sz = mbe.build(& bab.getAssignment(), false); // just compute memory estimate
  }
  else {
    cout << "Computing mini bucket heuristic..." << endl;
    sz = mbe.build(& bab.getAssignment(), true); // actually compute heuristic
  }
  cout << '\t' << (sz / (1024*1024.0)) * sizeof(double) << " MBytes" << endl;

#ifdef MEMDEBUG
  malloc_stats();
#endif

  // abort before actual search?
  if (opt.nosearch) {
    cout << "'No search' mode requested, exiting." << endl;
    delete space;
    return 0;
  }

  // The propagation engine
  BoundPropagator prop(space);

  cout << "--- Starting search ---" << endl;

#ifdef USE_THREADS


  // take care of signal handling
  sigset_t new_signal_mask;//, old_signal_mask;
  sigfillset( & new_signal_mask );
  pthread_sigmask(SIG_BLOCK, &new_signal_mask, NULL); // block all signals

//  try {

    CondorSubmissionEngine cse(space);

    boost::thread thread_prop(boost::ref(prop));
    boost::thread thread_bab(boost::ref(bab));
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
  while (!bab.isDone()) {
    prop.propagate(bab.nextLeaf());
  }
#endif

  cout << endl << "--- Search done ---" << endl;
#ifdef USE_THREADS
  cout << "Condor jobs:  " << bab.getThreadCount() << endl;
#endif
  cout << "OR nodes:     " << bab.getNodesOR() << endl;
  cout << "AND nodes:    " << bab.getNodesAND() << endl;

  time(&time_end);
  double time_passed = difftime(time_end,time_start);
  cout << "Time elapsed: " << time_passed << " seconds" << endl;

  double mpeCost = space->getSolutionCost();

  // account for global constant (but not if solving subproblem)
  if (opt.in_subproblemFile.empty()) {
#ifdef USE_LOG
    mpeCost += p.getGlobalConstant();
#else
    mpeCost *= p.getGlobalConstant();
#endif
  }

  cout << "--------------------------\n" <<
#ifdef USE_LOG
  mpeCost << " (" << EXPFUN( mpeCost ) << ')'
#else
  log10(mpeCost) << " (" << mpeCost << ')'
#endif
  << endl;

  // write solution file, if requested
  if (!opt.out_solutionFile.empty()) {
    ogzstream sol(opt.out_solutionFile.c_str(), ios::out | ios::trunc | ios::binary);
    //sol.precision(16);
    if (!sol)
      cerr << "Error writing optimal cost to file " << opt.out_solutionFile <<"." << endl;
    else {
#ifdef USE_LOG
      double aa = mpeCost;
      BINWRITE(sol, aa);
      aa = EXPFUN(mpeCost);
      BINWRITE(sol, aa);
#else
      double aa = log10(mpeCost);
      BINWRITE(sol, aa);
      aa = mpeCost;
      BINWRITE(sol, aa);
#endif
      sol.close();
    }
  }

#ifdef MEMDEBUG
  malloc_stats();
#endif

  delete space;

  return EXIT_SUCCESS;
}
