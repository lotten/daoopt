/*
 * MiniBucket.h
 *
 *  Created on: Nov 8, 2008
 *      Author: lars
 */

#ifndef MINIBUCKETELIM_H_
#define MINIBUCKETELIM_H_

#include "Heuristic.h"
#include "Function.h"
#include "Problem.h"
#include "Pseudotree.h"
#include "utils.h"

#include "MiniBucket.h"


/* The overall minibucket elimination */
class MiniBucketElim : public Heuristic {

  friend class MiniBucket;

protected:
  int m_ibound;                  // The ibound for this MB instance
  double m_globalUB;             // The global upper bound
  Problem* m_problem;            // The problem instance
  Pseudotree* m_pseudotree;      // The underlying pseudotree

  // The augmented buckets that will store the minibucket functions (but not the original ones)
  vector<list<Function*> > m_augmented;
  // Precompute and store, for each variable v, the relevant intermediate functions that are
  // generated in a pseudotree descendant and passed to an ancestor of v
  vector<list<Function*> > m_intermediate;

protected:
  // Computes a dfs order of the pseudo tree, for building the bucket structure
  void findDfsOrder(vector<int>&) const;

  // Compares the size of the scope of two functions
//  bool scopeIsLarger(Function*, Function*) const;

  // reset the data structures
  void reset() ;

public:

  // checks if the given i-bound would exceed the memlimit, in that
  // case compute highest possible i-bound that 'fits'
  int limitIbound(int ibound, size_t memlimit, const vector<val_t> * assignment);

  // builds the heuristic, limited to the relevant subproblem, if applicable.
  // if computeTables=false, only returns size estimate (no tables computed)
  size_t build(const vector<val_t>* assignment = NULL, bool computeTables = true);

  // gets and sets the global upper bound
  double getGlobalUB() const { return m_globalUB; }

  // computes the heuristic for variable var given a (partial) assignment
  double getHeur(int var, const vector<val_t>& assignment) const;

  // reset the i-bound
  void setIbound(int ibound) { m_ibound = ibound; }

public:
  MiniBucketElim(Problem* p, Pseudotree* pt, int ib);
  ~MiniBucketElim();

};




/* Inline definitions */


inline MiniBucketElim::MiniBucketElim(Problem* p, Pseudotree* pt, int ib) :
  m_ibound(ib), m_globalUB(ELEM_ONE), m_problem(p), m_pseudotree(pt)
// , m_augmented(p->getN()), m_intermediate(p->getN())
  { }

inline MiniBucketElim::~MiniBucketElim() {
  // make sure to delete each function only once
  for (vector<list<Function*> >::iterator it=m_augmented.begin() ;it!=m_augmented.end(); ++it)
    for (list<Function*>::iterator it2=it->begin(); it2!=it->end(); ++it2)
      delete (*it2);
}

inline bool scopeIsLarger(Function* p, Function* q) {
  assert(p && q);
  return (p->getArity() > q->getArity());
}

#endif /* MINIBUCKETELIM_H_ */
