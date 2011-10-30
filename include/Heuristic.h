/*
 * Heuristic.h
 *
 *  Created on: Nov 18, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef HEURISTIC_H_
#define HEURISTIC_H_

#include "assert.h"
#include "DEFINES.h"
#include <string>
#include <vector>

using std::string;
using std::vector;
class ProgramOptions;

class Heuristic {

public:

  // Limits the memory size of the heuristic (e.g. through lowering the mini bucket i-bound)
  virtual size_t limitSize(size_t limit, ProgramOptions* options, const vector<val_t> * assignment) = 0;

  // Returns the memory size of the heuristic
  virtual size_t getSize() const = 0;

  // Initialises and builds the heuristic, if required
  virtual size_t build(const std::vector<val_t>* = NULL, bool computeTables = true) = 0;

  // Reads and writes heuristic from/to specificd file
  virtual bool readFromFile(string fn) = 0;
  virtual bool writeToFile(string fn) const = 0;

  // Gets and sets the global upper bound
  virtual double getGlobalUB() const = 0;

  // Computes the heuristic for variable var given a (partial) assignment
  virtual double getHeur(int var, const std::vector<val_t>& assignment) const = 0;

  virtual bool isAccurate() = 0;

protected:
  Heuristic() {}
public:
  virtual ~Heuristic() {}

};


class UnHeuristic : public Heuristic {
public:
  size_t limitSize(size_t, ProgramOptions*, const vector<val_t> *) { return 0 ; }
  size_t getSize() const { return 0; }
  size_t build(const std::vector<val_t>*, bool) { return 0; }
  bool readFromFile(string) { return true; }
  bool writeToFile(string) const { return true; }
  double getGlobalUB() const { assert(false); return 0; }
  double getHeur(int, const std::vector<val_t>&) const { assert(false); return 0; }
  bool isAccurate() { return false; }
  UnHeuristic() {}
  virtual ~UnHeuristic() {}
};


#endif /* HEURISTIC_H_ */
