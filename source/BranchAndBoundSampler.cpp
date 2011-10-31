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
  // check for existing child nodes
  const CHILDLIST& curChildren = n->getChildren();
  if (curChildren.size()) {
    chi.reserve(curChildren.size());
    for (CHILDLIST::const_iterator it=curChildren.begin(); it!=curChildren.end(); ++it)
      chi.push_back(*it);
  }
  */

  if (n->getType() == NODE_AND) {  // AND node

    if (generateChildrenAND(n,chi)) {
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

    if (generateChildrenOR(n,chi))
      return true; // no children
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

/*
BranchAndBoundSampler::BranchAndBoundSampler(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) :
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
*/
