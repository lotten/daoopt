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

  // propagates the value of the specified search node and removes unneeded nodes
  void propagate(SearchNode*);

#ifndef NO_ASSIGNMENT
private:
  void propagateTuple(SearchNode* start, SearchNode* end);
#endif

protected:
  virtual bool isMaster() const { return false; }

public:
  BoundPropagator(SearchSpace* s) : m_space(s) {}
};


#endif /* BOUNDSPROPAGATOR_H_ */
