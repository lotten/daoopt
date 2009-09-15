/*
 * BoundPropagator.h
 *
 *  Created on: Nov 7, 2008
 *      Author: lars
 */

#ifndef BOUNDPROPAGATOR_H_
#define BOUNDPROPAGATOR_H_

#include "SearchSpace.h"
#include "SearchNode.h"
#include "Pseudotree.h"

#include "debug.h"

class BoundPropagator {
protected:
  SearchSpace* m_space;
public:

#ifdef PARALLEL_MODE
  // operator for multithreading
  void operator () ();
#endif

  void propagate(SearchNode*);

#ifndef NO_ASSIGNMENT
private:
//  void mergePrevAssignment(SearchNode* prev, SearchNode* cur);

  void propagateTuple(SearchNode* start, SearchNode* end);

#endif

public:
 BoundPropagator(SearchSpace* s) : m_space(s) {}
};


#endif /* BOUNDSPROPAGATOR_H_ */
