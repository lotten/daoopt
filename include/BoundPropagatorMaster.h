/*
 * BoundPropagatorMaster.h
 *
 *  Created on: Feb 14, 2010
 *      Author: lars
 */

#ifndef BOUNDPROPAGATORMASTER_H_
#define BOUNDPROPAGATORMASTER_H_

#include "BoundPropagator.h"

#ifdef PARALLEL_MODE


class BoundPropagatorMaster : public BoundPropagator {

protected:
  // same as m_space, but different type
  SearchSpaceMaster* m_spaceMaster;

public:
  // threading operator for master process
  void operator () ();

protected:
  bool isMaster() const { return true; }

public:
  // simple constructor
  BoundPropagatorMaster(SearchSpaceMaster* s) : BoundPropagator(s), m_spaceMaster(s) {}

};


#endif /* PARALLEL_MODE */

#endif /* BOUNDPROPAGATORMASTER_H_ */
