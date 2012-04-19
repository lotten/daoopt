/*
 * BranchAndBound.cpp
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
 *  Created on: Nov 4, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#undef DEBUG

#include "BranchAndBound.h"

#if defined(PARALLEL_DYNAMIC) || defined(PARALLEL_STATIC)
#undef DEBUG
#endif

typedef PseudotreeNode PtNode;



void BranchAndBound::reset(SearchNode* p) {
  if (!p) {
    p = m_space->getTrueRoot();
    if (p->getChildren())
      p = p->getChildren()[0];
  }
#ifdef ANYTIME_DEPTH
  while (!m_stackDive.empty())
    m_stackDive.pop();
#endif
  while (!m_stack.empty())
      m_stack.pop();
  m_stack.push(p);
}



SearchNode* BranchAndBound::nextNode() {
  SearchNode* n = NULL;
#ifdef ANYTIME_DEPTH
  if (m_stackDive.size()) {
    n = m_stackDive.top();
    m_stackDive.pop();
  }
#endif
  if (!n && m_stack.size()) {
    n = m_stack.top();
    m_stack.pop();
  }
  return n;
}


bool BranchAndBound::doExpand(SearchNode* n) {
  assert(n);
  m_expand.clear();

  if (n->getType() == NODE_AND) {  // AND node

    if (generateChildrenAND(n, m_expand))
      return true; // no children

#ifdef DEBUG
    ostringstream ss;
    for (vector<SearchNode*>::iterator it=m_expand.begin(); it!=m_expand.end(); ++it)
      ss << '\t' << *it << ": " << *(*it) << endl;
    myprint (ss.str());
#endif

#if defined(ANYTIME_DEPTH)
    // reverse iterator needed since dive step reverses subproblem order
    for (vector<SearchNode*>::reverse_iterator it=m_expand.rbegin(); it!=m_expand.rend(); ++it)
      m_stackDive.push(*it);
#else
    for (vector<SearchNode*>::iterator it = m_expand.begin(); it != m_expand.end(); ++it)
      m_stack.push(*it);
#endif

  } else {  // OR node

    if (generateChildrenOR(n, m_expand))
      return true; // no children
    for (vector<SearchNode*>::iterator it = m_expand.begin(); it != m_expand.end(); ++it) {
      m_stack.push(*it);
      DIAG( ostringstream ss; ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; myprint(ss.str()); )
    } // for loop
    DIAG (ostringstream ss; ss << "\tGenerated " << n->getChildCountFull() <<  " child AND nodes" << endl; myprint(ss.str()); )
#ifdef ANYTIME_DEPTH
    // pull last node from normal stack for dive
    m_stackDive.push(m_stack.top());
    m_stack.pop();
#endif

  } // if over node type

  return false; // default false
}


BranchAndBound::BranchAndBound(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) :
   Search(prob,pt,space,heur) {
#ifndef NO_CACHING
  // Init context cache table
  if (!m_space->cache)
    m_space->cache = new CacheTable(prob->getN());
#endif

  SearchNode* first = this->initSearch();
  if (first) {
    m_stack.push(first);
  }
}

