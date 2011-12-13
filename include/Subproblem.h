/*
 * Subproblem.h
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
 *  Created on: Mar 2, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef SUBPROBLEM_H_
#define SUBPROBLEM_H_

#include "_base.h"

#ifdef PARALLEL_DYNAMIC

// forward declaration
#include "SearchNode.h"

/* simple container for an external subproblem and its attributes (for parallelization) */
struct Subproblem {
protected:
  bool        solved;     // true iff subproblem has been solved
public:
  int         width;      // induced width of the conditioned problem
  int         depth;      // depth of the root node in original search space
  int         ptHeight;   // height of the subproblem pseudo tree
  size_t      threadId;   // thread ID
  double      lowerBound; // lower bound on solution (from previous subproblems)
  double      upperBound; // heuristic estimate of the subproblem solution (upper bound)
  double      avgInc;     // average label value along the path from the root to the subproblem
  double      estInc;     // estimated increment for the subproblem, using the resp. function tables
  SearchNode* root;       // subproblem root node in master search space
  count_t     nodesAND;   // number of AND nodes generated in subproblem
  count_t     nodesOR;    // number of OR nodes generated in subproblem
  count_t     estimate;   // precomputed node estimate
  bigint      hwb;        // hwb bound for subproblem
  bigint      twb;        // twb bound for subproblem
  vector<count_t> leafP;  // leaf node profile of subproblem solution
  vector<count_t> nodeP;  // full node profile of subproblem solution

  Subproblem(SearchNode* n, int wi, int de, int ptHei, size_t id, double lBou, double uBou, double avg, double estI, count_t estN, bigint hw, bigint tw) :
    solved(false), width(wi), depth(de), ptHeight(ptHei), threadId(id),
    lowerBound(lBou), upperBound(uBou), avgInc(avg), estInc(estI),
    root(n), nodesAND(0), nodesOR(0), estimate(estN), hwb(hw), twb(tw) {};

  void setSolved() { solved = true; }
  bool isSolved() const { return solved; }

};

#endif /* PARALLEL_DYNAMIC */

#endif /* SUBPROBLEM_H_ */
