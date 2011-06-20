/*
 * ParallelManager.h
 *
 *  Created on: Apr 11, 2010
 *      Author: lars
 */

#ifndef PARALLELMANAGER_H_
#define PARALLELMANAGER_H_

#include "_base.h"

#ifdef PARALLEL_STATIC

#include "ProgramOptions.h"
#include "Search.h"
#include "LimitedDiscrepancy.h"
#include "BoundPropagator.h"
#include "utils.h"

class ParallelManager : virtual public Search {

protected:

  /* counter for subproblem IDs */
  size_t m_subprobCount;

  /* for LDS */
  SearchSpace* m_ldsSpace;
  LimitedDiscrepancy* m_ldsSearch;
  BoundPropagator* m_ldsProp;

  /* stack for local solving */
  stack<SearchNode*> m_stack;

  /* subproblems that are easy enough for local solving */
  vector<SearchNode*> m_local;

  /* subproblems that have been submitted to the grid */
  vector<SearchNode*> m_external;

  /* for propagating leaf nodes */
  BoundPropagator m_prop;

protected:
  /* implemented from Search class */
  bool isDone() const;
  bool doExpand(SearchNode* n);
  void resetSearch(SearchNode* p);
  SearchNode* nextNode();
  bool isMaster() const { return true; }

public:
  void setInitialBound(double d) const;

protected:
  /* moves the frontier one step deeper by splitting the given node */
  bool deepenFrontier(SearchNode*, vector<SearchNode*>& out);
  /* evaluates a node (wrt. order in the queue) */
  double evaluate(const SearchNode*) const;
  /* filters out easy subproblems */
  bool isEasy(const SearchNode*) const;
  /* synchs the global assignment with the given node */
  void syncAssignment(const SearchNode*);
  /* applies LDS to the subproblem, i.e. mini bucket forward pass;
   * returns true if subproblem was solved fully */
  bool applyLDS(SearchNode*);

  /* compiles the name of a temporary file */
  string filename(const char* pre, const char* ext, int count = NONE) const;
  /* submits jobs to the grid system (Condor) */
  bool submitToGrid() const;
  /* waits for all external jobs to finish */
  bool waitForGrid() const;

  /* creates the encoding of subproblems for the condor submission */
  string encodeJobs(const vector<SearchNode*>&) const;
  /* solves a subproblem locally through AOBB */
  void solveLocal(SearchNode*);


public:
  /* computes the parallel frontier and , returns true iff successful */
  bool findFrontier();
  /* writes subproblem information to disk */
  bool writeJobs() const;
  /* initiates parallel subproblem computation through Condor */
  bool runCondor() const;
  /* parses the results from external subproblems */
  bool readExtResults();
  /* recreates the frontier given a previously written subproblem file */
  bool restoreFrontier();

#ifndef NO_ASSIGNMENT
  void setInitialSolution(const vector<val_t>&) const;
#endif

public:
  ParallelManager(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h);
  ~ParallelManager();

};


/* Inline definitions */

inline bool ParallelManager::isDone() const {
  assert(false); // should never be called
  return false;
}

inline void ParallelManager::resetSearch(SearchNode* n) {
  m_external.clear();
  m_local.clear();
  m_external.push_back(n);
}

inline ParallelManager::~ParallelManager() {
  if (m_ldsSpace) delete m_ldsSpace;
  if (m_ldsSearch) delete m_ldsSearch;
  if (m_ldsProp) delete m_ldsProp;
}


#endif /* PARALLEL_STATIC */

#endif /* PARALLELMANAGER_H_ */


