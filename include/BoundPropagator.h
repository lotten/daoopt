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

#include "debug.h"

class BoundPropagator {
protected:
  SearchSpace* m_space;
public:

#ifdef USE_THREADS
  // operator for multithreading
  void operator () ();
#endif

 void propagate(SearchNode*);

public:
 BoundPropagator(SearchSpace* s) : m_space(s) {}
};


#endif /* BOUNDSPROPAGATOR_H_ */
