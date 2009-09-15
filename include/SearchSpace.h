/*
 * SearchSpace.h
 *
 *  Created on: Nov 4, 2008
 *      Author: lars
 */

#ifndef SEARCHSPACE_H_
#define SEARCHSPACE_H_

//max. number of concurrent threads for subproblem processing
#define MAX_THREADS 7

//#include <set>

#include "SearchNode.h"

#ifndef NO_CACHING
#include "CacheTable.h"
#endif

// forward declarations
#ifdef PARALLEL_MODE
namespace boost {
  class thread;
}
struct CondorSubmission;
#endif

// declare ProgramOptions
#ifdef PARALLEL_MODE
#include "ProgramOptions.h"
#else
struct ProgramOptions;
#endif

// forward declaration
class Pseudotree;

struct SearchSpace {
  friend class Search;
  friend class BranchAndBound;
  friend class BoundPropagator;
  friend class SubproblemHandler;
  friend class SubproblemStraight;
  friend class SubproblemCondor;
  friend class CondorSubmissionEngine;

protected:
  SearchNode* root; // true root node of search space, always a dummy OR node
  SearchNode* subproblem; // pseudo root node when processing subproblem (NULL otherwise)
  ProgramOptions* options;
  Pseudotree* pseudotree;

#ifndef NO_CACHING
protected:
  // Caching of subproblem solutions
  CacheTable* cache;
#endif

public:
  // returns a pointer to the the true root node (potentially a subproblem)
  SearchNode* getTrueRoot() const;
/*
  // returns the current cost of the root node, i.e. the current optimal solution
  double getSolutionCost() const;
*/

#ifdef PARALLEL_MODE

protected:
  // queue for information exchange between components
  queue< SearchNode* > solved;
  map< SearchNode*, boost::thread* > activeThreads;

  // for pipelining the condor submissions
  queue< CondorSubmission* > condorQueue;
  boost::mutex mtx_condorQueue;
  boost::condition_variable_any cond_condorQueue;
  // to signal SubproblemHandlers that their job has been submitted
  boost::condition_variable_any cond_jobsSubmitted;

  // limits the number of processing leaves/threads at any time
  volatile size_t allowedThreads;
  boost::mutex mtx_allowedThreads;
  boost::condition_variable_any cond_allowedThreads;

  // counts the number of active threads
  volatile bool searchDone;
  boost::mutex mtx_searchDone;

  // mutex and condition variable for search space *and* cache
  boost::mutex mtx_space;

  // mutex and condition var. for the list of solved leaf nodes
  boost::mutex mtx_solved;
  boost::condition_variable_any cond_solved;

  // mutex for active threads
  boost::mutex mtx_activeThreads;

  // mutex for condor
  boost::mutex mtx_condor;

#endif

public:
#ifdef PARALLEL_MODE
  SearchSpace(Pseudotree* pt, ProgramOptions* opt) : root(NULL), subproblem(NULL), options(opt), pseudotree(pt), allowedThreads(MAX_THREADS), searchDone(false) {
#ifndef NO_CACHING
    cache = NULL;
#endif
    if (options) allowedThreads = opt->threads;
  }
#else
  SearchSpace(Pseudotree* pt, ProgramOptions* opt) : root(NULL), subproblem(NULL), options(opt), pseudotree(pt) {
#ifndef NO_CACHING
    cache = NULL;
#endif
  }
#endif
  ~SearchSpace() {
    if (root) delete root;
#ifndef NO_CACHING
    if (cache) delete cache;
#endif
  }
};


inline SearchNode* SearchSpace::getTrueRoot() const {
  assert(root);
  if (subproblem)
    return subproblem;
  else
    return root;
}
/*
inline double SearchSpace::getSolutionCost() const {
  assert(root);
  if (subproblem)
    return subproblem->getValue();
  else
    return root->getValue();
}
*/
#endif /* SEARCHSPACE_H_ */
