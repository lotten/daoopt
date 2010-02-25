/*
 * BranchAndBound.h
 *
 *  Created on: Nov 3, 2008
 *      Author: lars
 */

#ifndef BRANCHANDBOUND_H_
#define BRANCHANDBOUND_H_

#include "Search.h"

#include "debug.h"

/* Branch and Bound search, implements pure virtual functions from
 * Search.h, most importantly expandNext() */
class BranchAndBound : virtual public Search {

protected:
  stack<SearchNode*> m_stack; // The DFS stack of nodes


protected:
  void expandNext();

  bool doExpand(SearchNode* n);

  bool hasMoreNodes() const { return !m_stack.empty(); }

  void resetSearch(SearchNode* p);

  SearchNode* nextNode();

  bool isMaster() const { return false; }

public:
  bool isDone() const;

public:
  BranchAndBound(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) ;
};


/* Inline definitions */

inline bool BranchAndBound::isDone() const {
  return m_stack.empty();
}


inline SearchNode* BranchAndBound::nextNode() {
  SearchNode* n = m_stack.top();
  m_stack.pop();
  return n;
}


inline void BranchAndBound::resetSearch(SearchNode* p) {
  assert(p);
  while (!m_stack.empty())
      m_stack.pop();
  m_stack.push(p);
}


#endif /* BRANCHANDBOUND_H_ */
