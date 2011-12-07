/*
 * BranchAndBoundRotate.h
 *
 *  Copyright (C) 2011 Lars Otten
 *  Licensed under the MIT License, see LICENSE.TXT
 *  
 *  Created on: Oct 30, 2011
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef BRANCHANDBOUNDROTATE_H_
#define BRANCHANDBOUNDROTATE_H_

#include "Search.h"

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
  size_t m_stackCount;        // counter for current stack
  size_t m_stackLimit;        // expansion limit for stack rotation
  MyStack* m_rootStack;       // the root stack
  queue<MyStack*> m_stacks;   // the queue of active stacks

protected:
  bool isDone() const;
  bool doExpand(SearchNode* n);
  void reset(SearchNode* p);
  SearchNode* nextNode();
  bool isMaster() const { return false; }

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

#endif /* BRANCHANDBOUNDROTATE_H_ */
