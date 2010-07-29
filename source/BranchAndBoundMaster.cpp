/*
 * BranchAndBoundMaster.cpp
 *
 *  Created on: Feb 24, 2010
 *      Author: lars
 */

#undef DEBUG

#include "BranchAndBoundMaster.h"

#ifdef PARALLEL_MODE

bool BranchAndBoundMaster::findInitialParams(count_t& limitN) const {

  assert(limitN > 0 && m_space->root );

  Pseudotree pt(*m_pseudotree);
  SearchSpace sp(&pt, m_space->options);

  BranchAndBound bab(m_problem, &pt, &sp, m_heuristic);

  bab.setInitialBound( lowerBound(m_space->root) );

  BoundPropagator prop(m_problem,&sp);

  int maxSubRootDepth = pt.getHeight();
  int maxSubRootHeight = 0;
  count_t maxSubCount = 0;
  count_t maxSubLeaves = 0;
  count_t maxSubLeafD = 0;
  double lbound=ELEM_ONE, ubound=ELEM_ONE;

  SearchNode* parent = NULL;

  do {

    parent = bab.nextLeaf(); // temp. assignment to parent
    if (!parent) break; // search complete!
    parent = prop.propagate(parent);

    count_t subCount = prop.getSubCountCache();

    if (subCount*20 > limitN && subCount > maxSubCount) {
      maxSubCount = subCount;

      int rootvar = prop.getSubRootvarCache();
      maxSubRootDepth = pt.getNode(rootvar)->getDepth();
      maxSubRootHeight = pt.getNode(rootvar)->getSubHeight();

      maxSubLeaves = prop.getSubLeavesCache();
      maxSubLeafD = prop.getSubLeafDCache();

      const pair<double,double>& bounds = prop.getBoundsCache();//bab.getBounds().at(pd);
      lbound = bounds.first;
      ubound = bounds.second;

      cout << "Root " << rootvar << " d:" << maxSubRootDepth << " h:" << maxSubRootHeight
           << " N:"<< maxSubCount << " L:" << maxSubLeaves << " D:" << maxSubLeafD
           << " avgD:" << (maxSubLeafD/(double)maxSubLeaves)
           << "\t" << lbound << '/' << ubound << endl;

    }

  } while ( (sp.nodesAND < limitN) || (maxSubCount==0) ) ;

  limitN = sp.nodesAND;

  // initialize stats using max. subproblem except for full leaf profile
  m_spaceMaster->stats->init(maxSubRootDepth, maxSubRootHeight, maxSubCount, maxSubLeaves, maxSubLeafD, lbound, ubound);

  if (! bab.nextLeaf())
    return true; // solved within time limit

  return false; // not solved

}



void BranchAndBoundMaster::solveLocal(SearchNode* node) const {
  assert(node && node->getType() == NODE_OR);

  Pseudotree pt(*m_pseudotree);
  SearchSpace sp(&pt, m_space->options);

  BranchAndBound bab(m_problem, &pt, &sp, m_heuristic);

  vector<double> pst;
  node->getPST(pst); // gets the partial solution tree (bottom-up)
  reverse(pst.begin(), pst.end()); // reverse to make it top-down

  bab.restrictSubproblem(node->getVar(), m_assignment , pst );

  BoundPropagator prop(m_problem,&sp);

  SearchNode* n = bab.nextLeaf();
  while (n) {
    prop.propagate(n);
    n = bab.nextLeaf();
  }

  double mpeCost = bab.getCurOptValue();

  node->setValue(mpeCost);
  node->setLeaf();

//  cout << "Local solver done: " << mpeCost << " " << bab.getNoNodesOR() << '/' << bab.getNoNodesAND() << " nodes" << endl;

}

#endif /* PARALLEL_MODE */
