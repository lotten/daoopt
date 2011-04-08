/*
 * LimitedDiscrepancy.cpp
 *
 *  Created on: Apr 25, 2010
 *      Author: lars
 */

#undef DEBUG

#include "LimitedDiscrepancy.h"

#ifdef PARALLEL_DYNAMIC
#undef DEBUG
#endif


bool LimitedDiscrepancy::doExpand(SearchNode* node) {

  assert(node);
  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int depth = ptnode->getDepth();

  vector<SearchNode*> chi;

  if (node->getType() == NODE_AND) { /*******************************************/

    if (generateChildrenAND(node,chi))
      return true; // no children

    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it)
      m_stack.push( make_pair(*it,m_discCache) );

#if false
    // we need to generate *all* OR successors

    ++m_space->nodesAND; // count node
#ifdef PARALLEL_DYNAMIC
    node->setSubCount(1);
#endif

    if (depth >= 0) { // ignores dummy nodes
      m_nodeProfile.at(depth) += 1; // count node as expanded
    }

    // create new OR children
    for (vector<PseudotreeNode*>::const_iterator it=ptnode->getChildren().begin();
        it!=ptnode->getChildren().end(); ++it)
    {
      int vChild = (*it)->getVar();

      SearchNodeOR* n = new SearchNodeOR(node, vChild);

      // Compute and set heuristic estimate
      heuristicOR(n);

      node->addChild(n);
      // add node to stack with same disc. value as parent
      m_stack.push(make_pair(n,m_discCache));

#ifdef DEBUG
      ostringstream ss;
      ss << '\t' << n << ": " << *n;
      if (n->isCachable())
        ss << " - Cache: " << (*it)->getCacheContext();
      ss << endl;
      myprint (ss.str());
#endif

      // store initial lower bound on subproblem (needed for master init.)
      PAR_ONLY( n->setInitialBound(lowerBound(n)) );

    } // for loop

    if (node->getChildren().size()) {
      /* nothing right now */
    } else { // no children
      node->setLeaf(); // -> terminal node
      node->setValue(ELEM_ONE);
      if (depth!=-1) m_leafProfile.at(depth) += 1; // count leaf node
      PAR_ONLY( node->setSubLeaves(1) );
      return true;
    }

#endif
  } else { // NODE_OR /*********************************************************/

#if false
    if (generateChildrenOR(node,chi))
      return true;

    vector<SearchNode*>::iterator it = chi.begin();
    size_t offset = chi.size() - min(chi.size(),m_discCache+1);
    while (offset--) { // skip nodes with high discrepancy
      node->eraseChild(*it);
      ++it;
    }

    size_t c = 0;
    for (; it!=chi.end(); ++it, ++c) {
      m_stack.push( make_pair(*it,c) );
    }

#else
    // generate only successors whose total discrepancy is not higher
    // than the global limit

    ++m_space->nodesOR; // count node expansion

    // actually create new AND children
    double* heur = node->getHeurCache();

    vector<SearchNode*> newNodes;
    newNodes.reserve(m_problem->getDomainSize(var));

    // use heuristic cache to order child domain values
    priority_queue< pair<double,size_t>, vector<pair<double,size_t> >, PairComp > pqueue;

    for (val_t i=m_problem->getDomainSize(var)-1; i>=0; --i) {
      if (heur[2*i+1]!=ELEM_ZERO) // check precomputed label
        pqueue.push(make_pair(heur[2*i],i));
      else {
        m_leafProfile.at(depth) += 1;
        PAR_ONLY(node->addSubLeaves(1));
      }
    }

    // no child nodes left?
    if (pqueue.empty()) {
      node->setLeaf();
      node->setValue(ELEM_ZERO);
      return true;
    }

    size_t offset = min(pqueue.size(), m_discCache+1);
    while (pqueue.size()>offset)
      pqueue.pop(); // skip first entries

    while (pqueue.size()) {
      int i = pqueue.top().second;
      pqueue.pop();
      SearchNodeAND* n = new SearchNodeAND(node, i, heur[2*i+1]); // use cached label
      n->setHeur(heur[2*i]); // cached heur. value
      node->addChild(n);
      m_stack.push(make_pair(n, m_discCache-pqueue.size() ));
    }

//    node->clearHeurCache();

#endif
  } // if-else over node type

  return false; // default false

} // LimitedDiscrepancy::doExpand

void LimitedDiscrepancy::setInitialBound(double d) const {
  assert(m_space);
  m_space->root->setValue(d);
}

#ifndef NO_ASSIGNMENT
void LimitedDiscrepancy::setInitialSolution(const vector<val_t>& tuple) const {
  assert(m_space);
  m_space->root->setOptAssig(tuple);
}
#endif


LimitedDiscrepancy::LimitedDiscrepancy(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur, size_t disc)
  : Search(prob,pt,space,heur), m_maxDisc(disc)
{
  // create first node (dummy variable)
  PseudotreeNode* ptroot = m_pseudotree->getRoot();
  SearchNode* node = new SearchNodeOR(NULL, ptroot->getVar() );
  m_space->root = node;
  // create dummy variable's AND node (domain size 1)
  SearchNode* next = new SearchNodeAND(m_space->root, 0, prob->getGlobalConstant());
  // put global constant into dummy AND node label
  m_space->root->addChild(next);

  m_stack.push(make_pair(next,m_maxDisc));

  // need to replace cache table with bogus one
  delete m_space->cache;
  m_space->cache = new UnCacheTable();

}
