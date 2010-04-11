/*
 * SearchMaster.h
 *
 *  Created on: Feb 14, 2010
 *      Author: lars
 */

#ifndef SEARCHMASTER_H_
#define SEARCHMASTER_H_

#include "Search.h"

#ifdef PARALLEL_MODE

#include "SubproblemHandler.h"
#include "SubproblemCondor.h"
#include "Statistics.h"


/* all search implementations for the master process should inherit this */
class SearchMaster : virtual public Search {

protected:
  SearchSpaceMaster* m_spaceMaster; // master search space

public:
  void operator () ();

protected:

  bool isMaster() const { return true; }

  // returns true if the subproblem rooted at this node was simple enough to
  // be solved locally, In which case the solution is already stored in the node
  bool doSolveLocal(SearchNode*);

  // returns true if the subproblem rooted at this node should be outsourced/parallelized
  // (implemented in derived classes)
  bool doParallelize(SearchNode*);

  // finds the initial cutoff depth by exploring at most N nodes,
  // writes the actual value of N back and returns true iff problem was
  // fully solved already
  virtual bool findInitialParams(count_t& N) const = 0;

  // solves the subproblem under the node locally through sequential search
  virtual void solveLocal(SearchNode*) const = 0;

  SearchMaster(Problem* prob, Pseudotree* pt, SearchSpaceMaster* s, Heuristic* h);

protected:

  // computes and returns the average increment (= AND labels) on the path from
  // a node to the root (excluding the dummy label, which is the global constant)
  double avgIncrement(SearchNode*) const;

  // computes and returns an estimate for the average increment in a subproblem below
  // a given node: collects all relevant functions, computes their respective 'average'
  // cost, and combines these into an overall prediction
  double estimateIncrement(SearchNode*) const;

};


inline SearchMaster::SearchMaster(Problem* prob, Pseudotree* pt, SearchSpaceMaster* s, Heuristic* h) :
          Search(prob, pt, s, h), m_spaceMaster(s)
{
  m_spaceMaster->stats = new Statistics();
}


#endif /* PARALLEL_MODE */

#endif /* SEARCHMASTER_H_ */
