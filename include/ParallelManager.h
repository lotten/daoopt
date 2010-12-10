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

#include "Search.h"

class ParallelManager : virtual public Search {

protected:

  /* queue of subproblems, i.e. OR nodes, ordered according to
   * evaluation function */
  priority_queue<pair<double, SearchNode*> > m_open;

protected:

  bool isDone() const;

  bool doExpand(SearchNode* n);

  void resetSearch(SearchNode* p);

  SearchNode* nextNode();

  bool isMaster() const { return true; }

  double evaluate(SearchNode* n) const;

public:

  void setInitialBound(double d) const;

#ifndef NO_ASSIGNMENT
  void setInitialSolution(const vector<val_t>&) const;
#endif

public:
  ParallelManager(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h);

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


#endif /* PARALLEL_STATIC */

#endif /* PARALLELMANAGER_H_ */


