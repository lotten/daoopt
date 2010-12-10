/*
 * ParallelManager.cpp
 *
 *  Created on: Apr 11, 2010
 *      Author: lars
 */

#include "ParallelManager.h"

#undef DEBUG

#ifdef PARALLEL_STATIC

/* expands a node -- assumes OR node! Will generate descendant OR nodes
 * (via intermediate AND nodes) and put them on the OPEN list */
bool ParallelManager::doExpand(SearchNode* n) {

  assert(n && n->getType() == NODE_OR);
  vector<SearchNode*> chi, chi2;

  if (generateChildrenOR(n,chi))
    return true; // no children

  size_t counter=0;
  for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it) {
    DIAG( ostringstream ss; ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; myprint(ss.str()); )
    generateChildrenAND(*it,chi2);
#ifdef DEBUG
    ostringstream ss;
    for (vector<SearchNode*>::iterator it=chi2.begin(); it!=chi2.end(); ++it)
      ss << '\t' << *it << ": " << *(*it) << endl;
    myprint (ss.str());
#endif
    counter += chi2.size();
    for (vector<SearchNode*>::iterator it=chi2.begin(); it!=chi2.end(); ++it)
      m_open.push(make_pair(evaluate(*it),*it));
    chi2.clear();
  }

  if (!counter)
    return true; // no children

  return false; // default false

}


double ParallelManager::evaluate(SearchNode* n) const {
  assert(false); // TODO
  return -1;
}

SearchNode* ParallelManager::nextNode() {

  SearchNode* n = NULL;
  if (m_open.size()) {
    n = m_open.top().second;
    m_open.pop();
  }
  return n;

}


void ParallelManager::setInitialBound(double d) const {
  assert(m_space);
  m_space->root->setValue(d);
}


#ifndef NO_ASSIGNMENT
void ParallelManager::setInitialSolution(const vector<val_t>& tuple) const {
  assert(m_space);
  m_space->root->setOptAssig(tuple);
}
#endif



ParallelManager::ParallelManager(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h)
  : Search(prob, pt, s, h)
{

  // create first node (dummy variable)
  PseudotreeNode* ptroot = m_pseudotree->getRoot();
  SearchNode* node = new SearchNodeOR(NULL, ptroot->getVar() );
  m_space->root = node;

  // create dummy variable's AND node (domain size 1)
  SearchNode* next = new SearchNodeAND(m_space->root, 0, prob->getGlobalConstant());
  // put global constant into dummy AND node label
  m_space->root->addChild(next);

  // one more OR node for OPEN list
  node = next;
  next = new SearchNodeOR(node, ptroot->getVar()) ;
  node->addChild(next);

  // add to OPEN list // todo
  m_open.push(make_pair(1.0, next));

}

#endif /* PARALLEL_STATIC */

