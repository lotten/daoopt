/*
 * BranchAndBound.cpp
 *
 *  Created on: Nov 4, 2008
 *      Author: lars
 */

#undef DEBUG

#include "BranchAndBound.h"

#if defined(PARALLEL_DYNAMIC) || defined(PARALLEL_STATIC)
#undef DEBUG
#endif

typedef PseudotreeNode PtNode;


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
  vector<SearchNode*> chi;
  if (n->getType() == NODE_AND) {  // AND node

    if (generateChildrenAND(n,chi))
      return true; // no children

#ifdef DEBUG
    ostringstream ss;
    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it)
      ss << '\t' << *it << ": " << *(*it) << endl;
    myprint (ss.str());
#endif

#if defined(ANYTIME_DEPTH)
    // reverse iterator needed since dive step reverses subproblem order
    for (vector<SearchNode*>::reverse_iterator it=chi.rbegin(); it!=chi.rend(); ++it)
      m_stackDive.push(*it);
#else
    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it)
      m_stack.push(*it);
#endif

  } else {  // OR node

    if (generateChildrenOR(n,chi))
      return true; // no children
    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it) {
      m_stack.push(*it);
      DIAG( ostringstream ss; ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; myprint(ss.str()); )
    } // for loop
    DIAG (ostringstream ss; ss << "\tGenerated " << n->getChildren().size() <<  " child AND nodes" << endl; myprint(ss.str()); )
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

