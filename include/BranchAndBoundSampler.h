/*
 * BranchAndBoundSampler.h
 *
 *  Copyright (C) 2011 Lars Otten
 *  Licensed under the MIT License, see LICENSE.TXT
 *  
 *  Created on: Oct 30, 2011
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef BRANCHANDBOUNDSAMPLER_H_
#define BRANCHANDBOUNDSAMPLER_H_

#include "BranchAndBound.h"

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

#endif /* BRANCHANDBOUNDSAMPLER_H_ */
