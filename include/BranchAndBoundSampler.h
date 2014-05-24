/*
 * BranchAndBoundSampler.h
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
 *  Created on: Oct 30, 2011
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef BRANCHANDBOUNDSAMPLER_H_
#define BRANCHANDBOUNDSAMPLER_H_

#include "BranchAndBound.h"

namespace daoopt {

class BranchAndBoundSampler : public BranchAndBound {
protected:
  bool doExpand(SearchNode*);
public:
  BranchAndBoundSampler(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur);
  virtual ~BranchAndBoundSampler() {}
};


/* Inline definitions */
inline BranchAndBoundSampler::BranchAndBoundSampler(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) :
    Search(prob, pt, space, heur),
    BranchAndBound(prob, pt, space, heur) {
  /* nothing here */
}

}  // namespace daoopt

#endif /* BRANCHANDBOUNDSAMPLER_H_ */
