/*
 * LimitedDiscrepancy.h
 *
 *  Copyright (C) 2008-2012 Lars Otten
 *  This file is part of DAOOPT.
 *
 *  DAOOPT is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DAOOPT is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DAOOPT.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  Created on: Apr 25, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef LIMITEDDISCREPANCY_H_
#define LIMITEDDISCREPANCY_H_

#include "Search.h"

/* implements a limited discrepancy search, which explores every solution
 * that deviates from the heuristic preference at most k times */
class LimitedDiscrepancy : virtual public Search {

  // for access to resetSearch()
  friend class ParallelManager;

protected:
  size_t m_maxDisc; // max. discrepancy
  size_t m_discCache; // caches the discrepancy of the last "next node"

  /* the stack of nodes, each with its discrepancy value */
  stack<pair<SearchNode*,size_t> > m_stack;

protected:
  bool isDone() const;
  bool doExpand(SearchNode*);
  SearchNode* nextNode();
  bool isMaster() const { return false; }

public:
  void reset(SearchNode*);
  LimitedDiscrepancy(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur, size_t disc);
  virtual ~LimitedDiscrepancy() {}

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

inline bool LimitedDiscrepancy::isDone() const {
  return m_stack.empty();
}

inline void LimitedDiscrepancy::reset(SearchNode* n) {
  assert(n);
  while (m_stack.size())
    m_stack.pop();
  m_stack.push(make_pair(n,m_maxDisc));
}

#endif /* LIMITEDDISCREPANCY_H_ */
