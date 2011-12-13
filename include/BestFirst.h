/*
 * BestFirst.h
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
 *  Created on: Sep 23, 2009
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef BESTFIRST_H_
#define BESTFIRST_H_

#include "Search.h"


class NodeComp {
public:
  bool operator() (const SearchNode* a, const SearchNode* b) const {
    return a->getHeur() < b->getHeur();
  }
};


class BestFirst : public Search {

protected:

  /* The queue of nodes, implementing A* search
   * TODO: replace by custom implementation? */
  typedef priority_queue<SearchNode*, vector<SearchNode*>, NodeComp > pqueue;
  pqueue m_queue;

protected:

  void reset(SearchNode* p);
  SearchNode* nextNode();

  /* inherited from Search class */
  bool doExpand(SearchNode*);

  /* updates the active assignment so that 'cur' can be expanded */
  void synchAssignment(SearchNode* cur);

public:
  BestFirst(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) ;

};

/* Inline definitions */


inline SearchNode* BestFirst::nextNode() {
  SearchNode* n = m_queue.top();
  m_queue.pop();
  return n;
}


inline void BestFirst::reset(SearchNode* p) {
  assert(p);
  while (!m_queue.empty())
      m_queue.pop();
  m_queue.push(p);
}


#endif /* BESTFIRST_H_ */
