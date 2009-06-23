/*
 * Search.h
 *
 *  Created on: Nov 3, 2008
 *      Author: lars
 */

#ifndef SEARCH_H_
#define SEARCH_H_

#include "SearchSpace.h"
#include "SearchNode.h"
#include "Heuristic.h"
#include "Problem.h"
#include "Pseudotree.h"
#include "utils.h"

// All search algorithms should inherit this
class Search {

protected:
  Problem* m_problem;
  Pseudotree* m_pseudotree;
  SearchSpace* m_space;
  Heuristic* m_heuristic;

public:
#ifdef USE_THREADS
  virtual void operator () () = 0;
#else
  virtual SearchNode* nextLeaf() = 0;
#endif

protected:
  Search(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h) :
    m_problem(prob), m_pseudotree(pt), m_space(s), m_heuristic(h) {}

};


#endif /* SEARCH_H_ */
