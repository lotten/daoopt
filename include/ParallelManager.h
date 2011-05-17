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

class ParallelManager : virtual public Search {

protected:

  /* counter for subproblem IDs */
  size_t m_subprobCount;

  /* for LDS */
  SearchSpace* m_ldsSpace;
  LimitedDiscrepancy* m_ldsSearch;
  BoundPropagator* m_ldsProp;

  /* queue of subproblems, i.e. OR nodes, ordered according to
   * evaluation function */
  priority_queue<pair<double, SearchNode*> > m_open;

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
  bool deepenFrontier(SearchNode*);
  /* evaluates a node (wrt. order in the queue) */
  double evaluate(SearchNode*) const;
  /* filters out easy subproblems */
  bool isEasy(SearchNode*) const;
  /* synchs the global assignment with the given node */
  void syncAssignment(SearchNode*);
  /* applies LDS to the subproblem, i.e. mini bucket forward pass;
   * returns true if subproblem was solved fully */
  bool applyLDS(SearchNode*);

  /* submits jobs to the grid system (Condor) */
  bool submitToGrid();
  /* waits for all external jobs to finish */
  bool waitForGrid() const;
  /* parses the results from external subproblems */
  bool readExtResults() ;

  /* creates the encoding of a subproblem for the condor submission,
   * arguments are subproblem root and job id */
  string encodeJob(SearchNode*,size_t) const;
  /* solves a subproblem locally through AOBB */
  void solveLocal(SearchNode*);


public:
  /* runs the parallelization, returns true iff successful */
  bool run();

#ifndef NO_ASSIGNMENT
  void setInitialSolution(const vector<val_t>&) const;
#endif

public:
  ParallelManager(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h);
  ~ParallelManager();

};


/* Inline definitions */

inline bool ParallelManager::isDone() const {
  return m_open.empty();
}

inline void ParallelManager::resetSearch(SearchNode* p) {
  while (m_open.size())
      m_open.pop();
  m_open.push(make_pair(evaluate(p),p));
}

inline ParallelManager::~ParallelManager() {
  if (m_ldsSpace) delete m_ldsSpace;
  if (m_ldsSearch) delete m_ldsSearch;
  if (m_ldsProp) delete m_ldsProp;
}


#endif /* PARALLEL_STATIC */

#endif /* PARALLELMANAGER_H_ */


