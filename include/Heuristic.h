/*
 * Heuristic.h
 *
 *  Created on: Nov 18, 2008
 *      Author: lars
 */

#ifndef HEURISTIC_H_
#define HEURISTIC_H_

#include "DEFINES.h"
#include <vector>

class Heuristic {

public:

  // initialises and builds the heuristic, if required
  virtual size_t build(const std::vector<val_t>* = NULL, bool computeTables = true) = 0;

  // gets and sets the global upper bound
  virtual double getGlobalUB() const = 0;

  // computes the heuristic for variable var given a (partial) assignment
  virtual double getHeur(int var, const std::vector<val_t>& assignment) const = 0;


protected:
  Heuristic() {}
  virtual ~Heuristic() {}

};


#endif /* HEURISTIC_H_ */
