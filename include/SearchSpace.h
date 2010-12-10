/*
 * SearchSpace.h
 *
 *  Created on: Nov 4, 2008
 *      Author: lars
 */

#ifndef SEARCHSPACE_H_
#define SEARCHSPACE_H_

/* max. number of concurrent threads for subproblem processing */
#define MAX_THREADS 7

//#include <set>

#include "SearchNode.h"

#ifdef PARALLEL_DYNAMIC
#include "Subproblem.h"
#include "Statistics.h"
#endif /* PARALLEL_DYNAMIC */

#ifndef NO_CACHING
#include "CacheTable.h"
#endif

/* forward declarations */
#ifdef PARALLEL_DYNAMIC
namespace boost {
  class thread;
}
struct CondorSubmission;
#endif

/* declare ProgramOptions */
#ifdef PARALLEL_DYNAMIC
#include "ProgramOptions.h"
#else
struct ProgramOptions;
#endif

/* forward declarations */
class Pseudotree;


/* main search space structure for worker nodes */
struct SearchSpace{

  size_t nodesOR;               // number of OR nodes expanded
  size_t nodesAND;              // number of AND nodes expanded

  SearchNode* root;             // true root of the search space, always a dummy OR node
  SearchNode* subproblemLocal;  // pseudo root node when processing subproblem (NULL otherwise)
  ProgramOptions* options;      // Pointer to instance of program options container
  Pseudotree* pseudotree;       // Guiding pseudotree
  CacheTable* cache;            // Cache table (not used when caching is disabled through preprocessor flag)

  SearchNode* getTrueRoot() const;
  SearchSpace(Pseudotree* pt, ProgramOptions* opt);
  ~SearchSpace();
};



inline SearchSpace::SearchSpace(Pseudotree* pt, ProgramOptions* opt) :
    nodesOR(0), nodesAND(0),
    root(NULL), subproblemLocal(NULL), options(opt), pseudotree(pt), cache(NULL)
{ /* intentionally empty at this point */ }

inline SearchSpace::~SearchSpace() {
  if (root) delete root;
  if (cache) delete cache;
}

/* returns the relevant root node in both conditioned and unconditioned cases */
inline SearchNode* SearchSpace::getTrueRoot() const {
  assert(root);
  if (subproblemLocal)
    return subproblemLocal;
  else
    return root;
}



#ifdef PARALLEL_DYNAMIC
/* search space for master node, extends worker search space to allow threading */
struct SearchSpaceMaster : public SearchSpace {

  Statistics* stats; // keeps track of the statistics

  /* queue for information exchange between components */
  queue< Subproblem* > solved; // for externally solved subproblems
  queue< SearchNode* > leaves; // for fully solved leaf nodes
  map< Subproblem*, boost::thread* > activeThreads;

  /* for pipelining the condor submissions */
  queue< CondorSubmission* > condorQueue;
  boost::mutex mtx_condorQueue;
  boost::condition_variable_any cond_condorQueue;
  /* to signal SubproblemHandlers that their job has been submitted */
  boost::condition_variable_any cond_jobsSubmitted;

  /* limits the number of processing leaves/threads at any time */
  volatile size_t allowedThreads;
  boost::mutex mtx_allowedThreads;
  boost::condition_variable_any cond_allowedThreads;

  /* counts the number of active threads */
  volatile bool searchDone;
  boost::mutex mtx_searchDone;

  /* mutex and condition variable for search space *and* cache */
  boost::mutex mtx_space;

  /* mutex for Statistics object */
  boost::mutex mtx_stats;

  /* mutex and condition var. for the list of solved leaf nodes */
  boost::mutex mtx_solved;
  boost::condition_variable_any cond_solved;

  /* mutex for active threads */
  boost::mutex mtx_activeThreads;

  /* mutex for condor */
  boost::mutex mtx_condor;

  SearchSpaceMaster(Pseudotree* pt, ProgramOptions* opt);
  ~SearchSpaceMaster();

};


inline SearchSpaceMaster::SearchSpaceMaster(Pseudotree* pt, ProgramOptions* opt) :
    SearchSpace(pt, opt), allowedThreads(MAX_THREADS), searchDone(false)
{
  if (options) allowedThreads = opt->threads;
}

inline SearchSpaceMaster::~SearchSpaceMaster() {
  if (stats) delete stats;
}

#endif /* PARALLEL_DYNAMIC */


#endif /* SEARCHSPACE_H_ */
