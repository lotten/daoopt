/*
 * BranchAndBoundSampler.cpp
 *
 *  Created on: Oct 30, 2011
 *      Author: lars
 */

#include "BranchAndBoundSampler.h"


bool BranchAndBoundSampler::doExpand(SearchNode* n) {

  assert(n);
  vector<SearchNode*> chi;

  /*
  NodeP* children = n->getChildren();
  if (children) {
    chi.reserve(n->getChildCountAct());
    for (size_t i = 0; i < n->getChildCountFull(); ++i) {
      if (children[i])
        chi.push_back(children[i]);
    }
  }
  */

  if (n->getType() == NODE_AND) {  // AND node

    if (chi.size() == 0 && generateChildrenAND(n,chi)) {
      return true; // no children
    }

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

    if (chi.size() == 0 && generateChildrenOR(n,chi)) {
      return true; // no children
    }

    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it) {
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

