/*
 * LimitedDiscrepancy.cpp
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
 *  Created on: Apr 25, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#undef DEBUG

#include "LimitedDiscrepancy.h"

#ifdef PARALLEL_DYNAMIC
#undef DEBUG
#endif


bool LimitedDiscrepancy::doExpand(SearchNode* node) {
  assert(node);
  m_expand.clear();

  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int depth = ptnode->getDepth();

  if (node->getType() == NODE_AND) { /*******************************************/

    if (generateChildrenAND(node, m_expand))
      return true; // no children

    for (vector<SearchNode*>::iterator it = m_expand.begin(); it != m_expand.end(); ++it)
      m_stack.push( make_pair(*it,m_discCache) );

  } else { // NODE_OR /*********************************************************/

#if false
    if (generateChildrenOR(node, m_expand))
      return true;

    vector<SearchNode*>::iterator it = m_expand.begin();
    size_t offset = m_expand.size() - min(m_expand.size(), m_discCache+1);
    while (offset--) { // skip nodes with high discrepancy
      node->eraseChild(*it);
      ++it;
    }

    size_t c = 0;
    for (; it != m_expand.end(); ++it, ++c) {
      m_stack.push( make_pair(*it,c) );
    }

#else
    // generate only successors whose total discrepancy is not higher
    // than the global limit

    m_space->stats.numOR += 1; // count node expansion

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
        m_space->stats.numDead += 1;
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

    vector<NodeP> vec;  // collect nodes in here
    vec.reserve(pqueue.size());

    while (pqueue.size()) {
      int i = pqueue.top().second;
      pqueue.pop();
      SearchNodeAND* n = new SearchNodeAND(node, i, heur[2*i+1]); // use cached label
      n->setHeur(heur[2*i]); // cached heur. value
      vec.push_back(n);
      m_stack.push(make_pair(n, m_discCache-pqueue.size() ));
    }

    node->addChildren(vec);
//    node->clearHeurCache();

#endif
  } // if-else over node type

  return false; // default false

} // LimitedDiscrepancy::doExpand


LimitedDiscrepancy::LimitedDiscrepancy(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur, size_t disc)
  : Search(prob,pt,space,heur), m_maxDisc(disc)
{

  SearchNode* first = this->initSearch();
  if (first) {
    m_stack.push(make_pair(first, m_maxDisc));
  }

  // need to replace cache table with bogus one
  if (!m_space->cache)
    m_space->cache = new UnCacheTable();

}
