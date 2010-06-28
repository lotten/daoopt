/*
 * BranchAndBound.h
 *
 *  Created on: Nov 3, 2008
 *      Author: lars
 */

#ifndef BRANCHANDBOUND_H_
#define BRANCHANDBOUND_H_

#include "Search.h"


#ifdef ANYTIME
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
#endif


/* Branch and Bound search, implements pure virtual functions from
 * Search.h, most importantly expandNext() */
class BranchAndBound : virtual public Search {

protected:
#ifdef ANYTIME
  size_t m_stackCount;        // counter for current stack
  MyStack* m_rootStack;       // the root stack
  queue<MyStack*> m_stacks;   // the queue of active stacks
#else
  stack<SearchNode*> m_stack; // The DFS stack of nodes
#endif


protected:
  void expandNext();

  bool doExpand(SearchNode* n);

  void resetSearch(SearchNode* p);

  SearchNode* nextNode();

  bool isMaster() const { return false; }

public:

  void setInitialBound(double d) const;

#ifndef NO_ASSIGNMENT
  void setInitialSolution(const vector<val_t>&) const;
#endif

public:
  BranchAndBound(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) ;
};


/* Inline definitions */

inline void BranchAndBound::resetSearch(SearchNode* p) {
  assert(p);
#ifdef ANYTIME
  // TODO
  while (m_stacks.size()) {
    delete m_stacks.front();
    m_stacks.pop();
  }
  m_rootStack = new MyStack(NULL);
  m_rootStack->push(p);
#else
  while (!m_stack.empty())
      m_stack.pop();
  m_stack.push(p);
#endif
}


#endif /* BRANCHANDBOUND_H_ */
