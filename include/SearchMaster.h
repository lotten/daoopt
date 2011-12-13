/*
 * SearchMaster.h
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

#ifndef SEARCHMASTER_H_
#define SEARCHMASTER_H_

#include "Search.h"

#ifdef PARALLEL_DYNAMIC

#include "SubproblemHandler.h"
#include "SubproblemCondor.h"
#include "Statistics.h"


/* all search implementations for the master process should inherit this */
class SearchMaster : virtual public Search {

protected:
  count_t m_nextThreadId;           // Next subproblem thread id
  SearchSpaceMaster* m_spaceMaster; // master search space

public:
  /* operator for threading */
  void operator () ();

  /* performs initialization, returns true iff problem is solved in the process */
  bool init() ;

  /* implemented from Search class */
  size_t getSubproblemCount() const { return m_nextThreadId; }

protected:

  bool isMaster() const { return true; }

  /* returns true if the subproblem rooted at this node was simple enough to
   * be solved locally, In which case the solution is already stored in the node */
  bool doSolveLocal(SearchNode*);

  /* returns true if the subproblem rooted at this node should be outsourced/parallelized
   * (implemented in derived classes) */
  bool doParallelize(SearchNode*);

  /* finds the initial cutoff depth by exploring at most N nodes,
   * writes the actual value of N back and returns true iff problem was
   * fully solved already */
  virtual bool findInitialParams(count_t& N) const = 0;

  /* solves the subproblem under the node locally through sequential search */
  virtual void solveLocal(SearchNode*) const = 0;

  SearchMaster(Problem* prob, Pseudotree* pt, SearchSpaceMaster* s, Heuristic* h);

protected:

  /* computes and returns the average increment (= AND labels) on the path from
   * a node to the root (excluding the dummy label, which is the global constant) */
  double avgIncrement(SearchNode*) const;

  /* computes and returns an estimate for the average increment in a subproblem below
   * a given node: collects all relevant functions, computes their respective 'average'
   * cost, and combines these into an overall prediction */
  double estimateIncrement(SearchNode*) const;

};


inline SearchMaster::SearchMaster(Problem* prob, Pseudotree* pt, SearchSpaceMaster* s, Heuristic* h) :
          Search(prob, pt, s, h), m_nextThreadId(0), m_spaceMaster(s)
{
  m_spaceMaster->stats = new Statistics();
}


#endif /* PARALLEL_DYNAMIC */

#endif /* SEARCHMASTER_H_ */
