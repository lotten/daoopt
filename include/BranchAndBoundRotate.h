/*
 * BranchAndBoundRotate.h
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

#ifndef BRANCHANDBOUNDROTATE_H_
#define BRANCHANDBOUNDROTATE_H_

#include "Search.h"

namespace daoopt {

/*
 * Minor extension of STL stack class, used by AOBB for the 'stack tree'
 */
class MyStack : public stack<SearchNode*> {
protected:
  size_t m_children;
  MyStack* m_parent;
public:
  size_t getChildren() const { return m_children; }
  void addChild() { m_children += 1; }
  void delChild() { m_children -= 1; }
  MyStack* getParent() const { return m_parent; }
public:
  MyStack(MyStack* p) : m_children(0), m_parent(p) {}
};


class BranchAndBoundRotate: public Search {
protected:
  // to differentiate rotation events
  static int HAS_CHILDREN;
  static int STACK_EMPTY;
  static int LIMIT_REACHED;

protected:
  size_t m_stackCount;        // counter for current stack
  size_t m_stackLimit;        // expansion limit for stack rotation
  MyStack* m_rootStack;       // the root stack
  vector<size_t> m_rotateCnt; // counts expansions per rotation
  vector<size_t> m_reasonCnt; // counts how often rotation occured for three conditions
  queue<MyStack*> m_stacks;   // the queue of active stacks

protected:
  bool isDone() const;
  bool doExpand(SearchNode* n);
  void reset(SearchNode* p);
  SearchNode* nextNode();
  bool isMaster() const { return false; }

  // resets the rotate count (after storing its value)
  void resetRotateCount(int reason);
  // outputs rotation stats
  void printStats() const;

public:
  void setStackLimit(size_t s) { m_stackLimit = s; }

public:
  BranchAndBoundRotate(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur);
  virtual ~BranchAndBoundRotate() {}
};


/* Inline definitions */

inline bool BranchAndBoundRotate::isDone() const {
  assert(false);  // TODO
  return false;
}

inline void BranchAndBoundRotate::reset(SearchNode* p) {
  assert(p);
  while (m_stacks.size()) {
    delete m_stacks.front();
    m_stacks.pop();
  }
  m_rootStack = new MyStack(NULL);
  m_rootStack->push(p);
  m_stacks.push(m_rootStack);
}


inline void BranchAndBoundRotate::resetRotateCount(int reason) {
  if (m_stackCount) {  // keep track for stats
    if (m_rotateCnt.size() <= m_stackCount)
      m_rotateCnt.resize(m_stackCount+1, 0);
    m_rotateCnt[m_stackCount] += 1;
    m_reasonCnt[reason] += 1;
  }
  m_stackCount = 0;  // reset count
}

}  // namespace daoopt

#endif /* BRANCHANDBOUNDROTATE_H_ */
