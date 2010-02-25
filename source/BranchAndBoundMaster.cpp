/*
 * BranchAndBoundMaster.cpp
 *
 *  Created on: Feb 24, 2010
 *      Author: lars
 */

#include "BranchAndBoundMaster.h"

#ifdef PARALLEL_MODE

void BranchAndBoundMaster::solveLocal(SearchNode* node) const {
  assert(node && node->getType() == NODE_OR);

  Pseudotree pt(*m_pseudotree);
  SearchSpace sp(&pt, m_space->options);

  BranchAndBound bab(m_problem, &pt, &sp, m_heuristic);
  bab.restrictSubproblem(node->getVar(), m_assignment , vector<double>() );

  BoundPropagator prop(&sp);

  while (!bab.isDone()) {
    prop.propagate(bab.nextLeaf());
  }

  double mpeCost = bab.getCurOptValue();

  node->setValue(mpeCost);
  node->setLeaf();


}

#endif /* PARALLEL_MODE */
