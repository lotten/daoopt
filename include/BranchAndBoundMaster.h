/*
 * BranchAndBoundMaster.h
 *
 *  Created on: Feb 14, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef BRANCHANDBOUNDMASTER_H_
#define BRANCHANDBOUNDMASTER_H_

#include "BranchAndBound.h"

#ifdef PARALLEL_DYNAMIC

#include "SearchMaster.h"

#include "BoundPropagator.h"

/* Branch and Bound for master process, inherits:
 *  - operator () from SearchMaster
 *  - BaB functionality from BranchAndBound
 */
class BranchAndBoundMaster : public SearchMaster, public BranchAndBound {

protected:
  bool isMaster() const { return true; }

  bool findInitialParams(count_t& limitN) const;

  void solveLocal(SearchNode*) const;

public:
  BranchAndBoundMaster(Problem* prob, Pseudotree* pt, SearchSpaceMaster* space, Heuristic* heur) ;

};

/* Inline definitions */

inline BranchAndBoundMaster::BranchAndBoundMaster(Problem* prob, Pseudotree* pt, SearchSpaceMaster* space, Heuristic* heur) :
    Search(prob, pt,space, heur), SearchMaster(prob,pt,space,heur), BranchAndBound(prob,pt,space,heur) {}


#endif /* PARALLEL_DYNAMIC */

#endif /* BRANCHANDBOUNDMASTER_H_ */
