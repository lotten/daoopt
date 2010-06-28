/*
 * LimitedDiscrepancy.h
 *
 *  Created on: Apr 25, 2010
 *      Author: lars
 */

#ifndef LIMITEDDISCREPANCY_H_
#define LIMITEDDISCREPANCY_H_

#include "Search.h"

/* implements a limited discrepancy search, which explores every solution
 * that deviates from the heuristic preference at most k times */
class LimitedDiscrepancy : virtual public Search {

protected:
  size_t m_maxDisc; // max. discrepancy
  size_t m_discCache; // caches the discrepancy of the last "next node"

  /* the stack of nodes, each with its discrepancy value */
  stack<pair<SearchNode*,size_t> > m_stack;

protected:
  void expandNext();

  bool doExpand(SearchNode*);

  void resetSearch(SearchNode*);

  SearchNode* nextNode();

  bool isMaster() const { return false; }

public:
  void setInitialBound(double d) const;

#ifndef NO_ASSIGNMENT
  void setInitialSolution(const vector<val_t>&) const;
#endif

public:
  LimitedDiscrepancy(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur, size_t disc);

  // to compare pair<double,size_t>
  struct PairComp {
    bool operator () (pair<double,size_t>& a, pair<double, size_t>& b) {
      return a.first > b.first;
    }
  };

};

/* INLINE DEFINITIONS */

inline SearchNode* LimitedDiscrepancy::nextNode() {
  SearchNode* n;
  if (m_stack.empty())
    n = NULL;
  else {
    n = m_stack.top().first;
    m_discCache = m_stack.top().second;
    m_stack.pop();
  }
  return n;
}

inline void LimitedDiscrepancy::resetSearch(SearchNode* n) {
  assert(n);
  while (m_stack.size())
    m_stack.pop();
  m_stack.push(make_pair(n,0));
}

#endif /* LIMITEDDISCREPANCY_H_ */
