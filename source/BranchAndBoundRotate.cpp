/*
 * BranchAndBoundRotate.cpp
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
 *  Created on: Oct 30, 2011
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#undef DEBUG

#include "BranchAndBoundRotate.h"

typedef PseudotreeNode PtNode;


SearchNode* BranchAndBoundRotate::nextNode() {
  SearchNode* n = NULL;
  while (m_stacks.size()) {
    MyStack* st = m_stacks.front();
    if (st->getChildren()) {
      m_stacks.push(st);
      m_stackCount = 0;
    } else if (st->empty()) {
      if (st->getParent())
        st->getParent()->delChild();
      delete st;
      m_stackCount = 0;
    } else if (m_stackLimit && m_stackCount++ == m_stackLimit) {
      m_stacks.push(st);
      m_stackCount = 0;
    } else {
      n = st->top();
      st->pop();
      break; // while loop
    }
    m_stacks.pop();
  }
  return n;
}


bool BranchAndBoundRotate::doExpand(SearchNode* n) {
  assert(n);
  vector<SearchNode*> chi;

  MyStack* stack = m_stacks.front();
  if (n->getType() == NODE_AND) {  // AND node
    if (generateChildrenAND(n,chi))
      return true; // no children

#ifdef DEBUG
    ostringstream ss;
    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it)
      ss << '\t' << *it << ": " << *(*it) << endl;
    myprint (ss.str());
#endif

    if (chi.size() == 1) {  // no decomposition
      stack->push(chi.at(0));
    } else {  // decomposition, split stacks
      // reverse iterator needed since new stacks are put in queue (and not depth-first stack)
      for (vector<SearchNode*>::reverse_iterator it=chi.rbegin(); it!=chi.rend(); ++it) {
        MyStack* s = new MyStack(stack);
        stack->addChild();
        s->push(*it);
        m_stacks.push(s);
      }
    }
  } else {  // OR node
    if (generateChildrenOR(n,chi))
      return true; // no children

    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it) {
      stack->push(*it);
      DIAG( ostringstream ss; ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; myprint(ss.str()); )
    }  // for loop
    DIAG (ostringstream ss; ss << "\tGenerated " << n->getChildCountFull() <<  " child AND nodes" << endl; myprint(ss.str()); )
  }  // if over node type

  return false; // default false
}


BranchAndBoundRotate::BranchAndBoundRotate(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) :
   Search(prob,pt,space,heur) {
#ifndef NO_CACHING
  // Init context cache table
  if (!m_space->cache)
    m_space->cache = new CacheTable(prob->getN());
#endif
  m_stackLimit = m_space->options->rotateLimit;
  SearchNode* first = this->initSearch();
  if (first) {
    m_rootStack = new MyStack(NULL);
    m_rootStack->push(first);
    m_stacks.push(m_rootStack);
    m_stackCount = 0;
  }
}

