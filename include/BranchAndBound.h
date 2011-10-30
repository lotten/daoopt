/*
 * BranchAndBound.h
 *
 *  Created on: Nov 3, 2008
 *      Author: lars
 */

#ifndef BRANCHANDBOUND_H_
#define BRANCHANDBOUND_H_

#include "Search.h"


/* Branch and Bound search, implements pure virtual functions
 * from Search.h */
class BranchAndBound : virtual public Search {

protected:
#ifdef ANYTIME_DEPTH
  stack<SearchNode*> m_stackDive; // first stack for initial dives
#endif
  stack<SearchNode*> m_stack; // The DFS stack of nodes

protected:
  bool isDone() const;
  bool doExpand(SearchNode* n);
  void resetSearch(SearchNode* p);
  SearchNode* nextNode();
  bool isMaster() const { return false; }

public:
  BranchAndBound(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) ;
  virtual ~BranchAndBound() {}
};


/* Inline definitions */


inline bool BranchAndBound::isDone() const {
#if defined ANYTIME_DEPTH
  return (m_stack.empty() && m_stackDive.empty());
#else
  return m_stack.empty();
#endif
}

inline void BranchAndBound::resetSearch(SearchNode* p) {
  assert(p);
#ifdef ANYTIME_DEPTH
  while (!m_stackDive.empty())
    m_stackDive.pop();
#endif
  while (!m_stack.empty())
      m_stack.pop();
  m_stack.push(p);
}


#endif /* BRANCHANDBOUND_H_ */
