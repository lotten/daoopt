/*
 * BranchAndBoundMaster.h
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
 *  Created on: Feb 14, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef BRANCHANDBOUNDMASTER_H_
#define BRANCHANDBOUNDMASTER_H_

#include "BranchAndBound.h"

#ifdef PARALLEL_DYNAMIC

#include "SearchMaster.h"

#include "BoundPropagator.h"

namespace daoopt {

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

}  // namespace daoopt

#endif /* PARALLEL_DYNAMIC */

#endif /* BRANCHANDBOUNDMASTER_H_ */
