/*
 * BranchAndBoundMaster.cpp
 *
 *  Created on: Feb 24, 2010
 *      Author: lars
 */

#include "BranchAndBoundMaster.h"

#ifdef PARALLEL_MODE

#undef DEBUG

bool BranchAndBoundMaster::findInitialParams(count_t& limitN) const {

  assert(limitN > 0 && m_space->root );

  Pseudotree pt(*m_pseudotree);
  SearchSpace sp(&pt, m_space->options);

  BranchAndBound bab(m_problem, &pt, &sp, m_heuristic);

  bab.setInitialBound( lowerBound(m_space->root) );

  BoundPropagator prop(&sp);

  int minDepth = pt.getHeight();
  count_t nodeCount = 0;
//  vector<count_t> leafProf;
  double lbound=ELEM_ONE, ubound=ELEM_ONE;

  SearchNode* parent = NULL;

  count_t step = 100000;

  do {

    parent = prop.propagate(bab.nextLeaf());
    if (parent->getType() == NODE_OR) continue; // ignore AND nodes

    int pd = pt.getNode(parent->getVar())->getDepth() + 1;
    count_t N = bab.getNoNodesAND();

    if ( N*20>limitN &&  pd < minDepth) {
      minDepth = pd;
      nodeCount = N;
//      leafProf = bab.getLeafProfile();

      const pair<double,double>& bounds = prop.getBoundsCache();//bab.getBounds().at(pd);
      lbound = bounds.first;
      ubound = bounds.second;

      cout << "Max. subproblem at depth " << pd << " N:"<< nodeCount
           << "\t" << lbound << '/' << ubound << endl;

    }

    /*
    if (N > step) {
      step += 100000;
      cout << N << '\t'<< minDepth << '\t' << lbound << ' ' << ubound;
      const vector<count_t>& ls = bab.getLeafProfile();
      cout << '\t' << ls.size();
      for (vector<count_t>::const_iterator it=ls.begin(); it!=ls.end(); ++it)
        cout << ' ' << *it;
      cout << endl;
    }
    */

  } while (!bab.isDone() && ( (bab.getNoNodesAND() < limitN) || (nodeCount==0) ) ) ;

  limitN = bab.getNoNodesAND();

  // initialize stats using max. subproblem except for full leaf profile
  m_spaceMaster->stats->init(minDepth, nodeCount, bab.getLeafProfile(), lbound, ubound);

  if (bab.isDone())
    return true; // solved within time limit

  return false; // not solved

}



void BranchAndBoundMaster::solveLocal(SearchNode* node) const {
  assert(node && node->getType() == NODE_OR);

  Pseudotree pt(*m_pseudotree);
  SearchSpace sp(&pt, m_space->options);

  BranchAndBound bab(m_problem, &pt, &sp, m_heuristic);
  vector<double> pst;
  node->getPST(pst); // gets the partial solution tree
  bab.restrictSubproblem(node->getVar(), m_assignment , pst );

  BoundPropagator prop(&sp);

  while (!bab.isDone()) {
    prop.propagate(bab.nextLeaf());
  }

  double mpeCost = bab.getCurOptValue();

  node->setValue(mpeCost);
  node->setLeaf();


}

#endif /* PARALLEL_MODE */
