/*
 * Heuristic.h
 *
 *  Copyright (C) 2008-2012 Lars Otten
 *  This file is part of DAOOPT.
 *
 *  DAOOPT is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DAOOPT is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DAOOPT.  If not, see <http://www.gnu.org/licenses/>.
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

namespace daoopt {

/* forward class declarations */
class ProgramOptions;
class Problem;
class Pseudotree;

/*
 * Base class for all heuristic implementations.
 * The following are the three central functions that need to be implemented
 * in subclasses (other functions can have empty implementations):
 * - build(...) : to actually build the heuristic.
 * - getHeur(...) : to query heuristic values
 * - getGlobalUB() : to retrieve the problem upper bound
 */
class Heuristic {

protected:
  Problem* m_problem;            // The problem instance
  Pseudotree* m_pseudotree;      // The underlying pseudotree
  ProgramOptions* m_options;     // Program options instance

public:

  /* Limits the memory size of the heuristic (e.g. through lowering
   * the mini bucket i-bound)
   */
  virtual size_t limitSize(size_t limit, const std::vector<val_t> * assignment) = 0;

  /* Returns the memory size of the heuristic
   */
  virtual size_t getSize() const = 0;

  /* Builds the heuristic. The first optional parameter can be used to specify
   * a partial assignment (when solving a conditioned subproblem,for instance),
   * the second optional parameter signals simulation-only mode (i.e. the
   * heuristic compilation is just simulated). The basic heuristic will work
   * without these two features, though.
   * The function should return the memory size of the heuristic instance.
   */
  virtual size_t build(const std::vector<val_t>* = NULL,
                       bool computeTables = true) = 0;

  /* Reads and writes heuristic from/to specified file.
   * Should return true/false on success or failure, respectively.
   */
  virtual bool readFromFile(std::string filename) = 0;
  virtual bool writeToFile(std::string filename) const = 0;

  /* Returns the global upper bound on the problem solution (e.g., after
   * marginalizing out the root bucket)
   */
  virtual double getGlobalUB() const = 0;

  /* Compute and return the heuristic estimate for the given variable.
   * The second argument is an assignment of all problem variables (i.e.
   * assignment.size() = number of problem variables incl. the dummy) from
   * from which the relevant assignments (i.e. the context of the current
   * variable) will be extracted.
   */
  virtual double getHeur(int var, const std::vector<val_t>& assignment) const = 0;

  /* same as above, but writes heuristic values for all instantiations
   * of var into out. */
  virtual void getHeurAll(int var, const std::vector<val_t>& assignment,
      std::vector<double>& out) const = 0;

  /* Returns true if the heuristic is provably accurate. Default false,
   * override in child class.
   */
  virtual bool isAccurate() { return false; }
  
protected:
  Heuristic(Problem* p, Pseudotree* pt, ProgramOptions* po) :
      m_problem(p), m_pseudotree(pt), m_options(po) { /* nothing */ }
public:
  virtual ~Heuristic() { /* nothing */ }
};


/* "Empty" heuristic, for testing and debugging */
class UnHeuristic : public Heuristic {
public:
  size_t limitSize(size_t, ProgramOptions*, const std::vector<val_t> *) { return 0 ; }
  size_t getSize() const { return 0; }
  size_t build(const std::vector<val_t>*, bool) { return 0; }
  bool readFromFile(std::string) { return true; }
  bool writeToFile(std::string) const { return true; }
  double getGlobalUB() const { assert(false); return 0; }
  double getHeur(int, const std::vector<val_t>&) const { assert(false); return 0; }
  bool isAccurate() { return false; }
  UnHeuristic() : Heuristic(NULL, NULL, NULL) {}
  virtual ~UnHeuristic() {}
};

}  // namespace daoopt

#endif /* HEURISTIC_H_ */
