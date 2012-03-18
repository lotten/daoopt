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

int BranchAndBoundRotate::HAS_CHILDREN = 0;
int BranchAndBoundRotate::STACK_EMPTY = 1;
int BranchAndBoundRotate::LIMIT_REACHED = 2;


SearchNode* BranchAndBoundRotate::nextNode() {
  SearchNode* n = NULL;
  while (m_stacks.size()) {
    MyStack* st = m_stacks.front();
    if (st->getChildren()) {  // move on if this stack has child stacks
      m_stacks.push(st);
      resetRotateCount(HAS_CHILDREN);
    } else if (st->empty()) {  // clean up empty stack
      if (st->getParent())
        st->getParent()->delChild();
      delete st;
      resetRotateCount(STACK_EMPTY);
    } else if (m_stackLimit > 0 && m_stackCount++ == m_stackLimit) {  // stack limit reached
      m_stacks.push(st);
      resetRotateCount(LIMIT_REACHED);
    } else {  // remain in current stack
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
  m_expand.clear();

  MyStack* stack = m_stacks.front();
  if (n->getType() == NODE_AND) {  // AND node
    if (generateChildrenAND(n, m_expand))
      return true; // no children

#ifdef DEBUG
    ostringstream ss;
    for (vector<SearchNode*>::iterator it = m_expand.begin(); it != m_expand.end(); ++it)
      ss << '\t' << *it << ": " << *(*it) << endl;
    myprint (ss.str());
#endif

    if (m_expand.size() == 1) {  // no decomposition
      stack->push(m_expand.at(0));
    } else {  // decomposition, split stacks
      // reverse iterator needed since new stacks are put in queue (and not depth-first stack)
      for (vector<SearchNode*>::reverse_iterator it = m_expand.rbegin();
           it != m_expand.rend(); ++it) {
        MyStack* s = new MyStack(stack);
        stack->addChild();
        s->push(*it);
        m_stacks.push(s);
      }
    }
  } else {  // OR node
    if (generateChildrenOR(n, m_expand))
      return true; // no children

    for (vector<SearchNode*>::iterator it = m_expand.begin(); it != m_expand.end(); ++it) {
      stack->push(*it);
      DIAG( ostringstream ss; ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; myprint(ss.str()); )
    }  // for loop
    DIAG (ostringstream ss; ss << "\tGenerated " << n->getChildCountFull() <<  " child AND nodes" << endl; myprint(ss.str()); )
  }  // if over node type

  return false; // default false
}


void BranchAndBoundRotate::printStats() const {
  oss ss;
  ss << "rotateCnt:";
  BOOST_FOREACH(size_t n, m_rotateCnt) ss << ' ' << n;
  ss << endl;
  myprint (ss.str());
  ss.str("");

  ss << "reasonCnt:";
  BOOST_FOREACH(size_t n, m_reasonCnt) ss << ' ' << n;
  ss << endl;
  myprint(ss.str());
  ss.str("");

  double avgRotate = 0;
  size_t total = 0;
  for (size_t i = 0; i < m_rotateCnt.size(); ++i) {
    total += m_rotateCnt[i];
    avgRotate += i * m_rotateCnt[i];
  }
  avgRotate /= total;
  ss << "avgRotate: " << avgRotate << " maxRotate: " << m_rotateCnt.size()-1 << endl;
  myprint(ss.str());
  ss.str("");
}


BranchAndBoundRotate::BranchAndBoundRotate(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) :
   Search(prob,pt,space,heur), m_reasonCnt(3,0) {
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

