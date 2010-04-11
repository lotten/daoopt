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

#ifdef PARALLEL_MODE
  // caches the lower/upper bound of the last highest deleted node
  // (required for preprocessing within master process)
  pair<double,double> m_boundsCache;
#endif

public:

  // propagates the value of the specified search node and removes unneeded nodes
  // returns a pointer to the parent of the highest deleted node
  SearchNode* propagate(SearchNode*);

#ifdef PARALLEL_MODE
  const pair<double,double>& getBoundsCache() const { return m_boundsCache; }
#endif

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
