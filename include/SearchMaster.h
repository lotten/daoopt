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


/* all search implementations for the master process should inherit this */
class SearchMaster : virtual public Search {

protected:
  SearchSpaceMaster* m_spaceMaster; // master search space

public:
  void operator () ();

protected:

  bool isMaster() const { return true; }

  SearchMaster(Problem* prob, Pseudotree* pt, SearchSpaceMaster* s, Heuristic* h) :
      Search(prob, pt, s, h), m_spaceMaster(s) {}

  // returns true if the subproblem rooted at this node should be outsourced/parallelized
  // (implemented in derived classes)
  bool doParallelize(SearchNode*);

  // solves the subproblem under the node locally through sequential search
  virtual void solveLocal(SearchNode*) const = 0;

protected:

  // computes and returns the average increment (= AND labels) on the path from
  // a node to the root (excluding the dummy label, which is the global constant)
  double avgIncrement(SearchNode*) const;

  // computes and returns an estimate for the average increment in a subproblem below
  // a given node: collects all relevant functions, computes their respective 'average'
  // cost, and combines these into an overall prediction
  double estimateIncrement(SearchNode*) const;

};


#endif /* PARALLEL_MODE */

#endif /* SEARCHMASTER_H_ */
