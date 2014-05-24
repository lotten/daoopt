/*
 * MiniBucket.h
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
 *  Created on: Nov 8, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef MINIBUCKETELIM_H_
#define MINIBUCKETELIM_H_

#include "Heuristic.h"
#include "Function.h"
#include "Problem.h"
#include "ProgramOptions.h"
#include "Pseudotree.h"
#include "utils.h"

#include "MiniBucket.h"

namespace daoopt {

/* The overall minibucket elimination */
class MiniBucketElim : public Heuristic {

  friend class MiniBucket;

protected:
  int m_ibound;                  // The ibound for this MB instance
  double m_globalUB;             // The global upper bound

  // The augmented buckets that will store the minibucket functions (but not the original ones)
  vector<vector<Function*> > m_augmented;
  // Precompute and store, for each variable v, the relevant intermediate functions that are
  // generated in a pseudotree descendant and passed to an ancestor of v
  // (points to the same function objects as m_augmented)
  vector<vector<Function*> > m_intermediate;

protected:
  // Computes a dfs order of the pseudo tree, for building the bucket structure
  void findDfsOrder(vector<int>&) const;

  // Compares the size of the scope of two functions
//  bool scopeIsLarger(Function*, Function*) const;

  // reset the data structures
  void reset() ;

public:

  // checks if the given i-bound would exceed the memlimit and lowers
  // it accordingly.
  size_t limitSize(size_t memlimit, const vector<val_t> * assignment);

  // builds the heuristic, limited to the relevant subproblem, if applicable.
  // if computeTables=false, only returns size estimate (no tables computed)
  size_t build(const vector<val_t>* assignment = NULL, bool computeTables = true);

  // returns the global upper bound
  double getGlobalUB() const { return m_globalUB; }

  // computes the heuristic for variable var given a (partial) assignment
  double getHeur(int var, const vector<val_t>& assignment) const;
  // computes heuristic values for all instantiations of var, given context assignment
  void getHeurAll(int var, const vector<val_t>& assignment, vector<double>& out) const;

  // reset the i-bound
  void setIbound(int ibound) { m_ibound = ibound; }
  // gets the i-bound
  int getIbound() const { return m_ibound; }

  // gets sum of tables sizes
  size_t getSize() const;

  bool writeToFile(string fn) const;
  bool readFromFile(string fn);

  bool isAccurate();

public:
  MiniBucketElim(Problem* p, Pseudotree* pt, ProgramOptions* po, int ib);
  virtual ~MiniBucketElim();

};

/* Inline definitions */

inline bool MiniBucketElim::isAccurate() {
  assert(m_pseudotree);
  return (m_pseudotree->getWidthCond() <= m_ibound);
}

inline MiniBucketElim::MiniBucketElim(Problem* p, Pseudotree* pt,
				      ProgramOptions* po, int ib) :
    Heuristic(p, pt, po), m_ibound(ib), m_globalUB(ELEM_ONE)
// , m_augmented(p->getN()), m_intermediate(p->getN())
  { }

inline MiniBucketElim::~MiniBucketElim() {
  // make sure to delete each function only once
  for (vector<vector<Function*> >::iterator it=m_augmented.begin() ;it!=m_augmented.end(); ++it)
    for (vector<Function*>::iterator it2=it->begin(); it2!=it->end(); ++it2)
      delete (*it2);
}

inline bool scopeIsLarger(Function* p, Function* q) {
  assert(p && q);
  if (p->getArity() == q->getArity())
    return (p->getId() > q->getId());
  else
    return (p->getArity() > q->getArity());
}

}  // namespace daoopt

#endif /* MINIBUCKETELIM_H_ */
