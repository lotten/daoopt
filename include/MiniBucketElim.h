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
#include "ProgramOptions.h"
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
  // (points to the same function objects as m_augmented)
  vector<list<Function*> > m_intermediate;

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
  size_t limitSize(size_t memlimit, ProgramOptions* options, const vector<val_t> * assignment);

  // builds the heuristic, limited to the relevant subproblem, if applicable.
  // if computeTables=false, only returns size estimate (no tables computed)
  size_t build(const vector<val_t>* assignment = NULL, bool computeTables = true);

  // returns the global upper bound
  double getGlobalUB() const { return m_globalUB; }

  // computes the heuristic for variable var given a (partial) assignment
  double getHeur(int var, const vector<val_t>& assignment) const;

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
  MiniBucketElim(Problem* p, Pseudotree* pt, int ib);
  virtual ~MiniBucketElim();

};

/* Inline definitions */

inline bool MiniBucketElim::isAccurate() {
  assert(m_pseudotree);
  return (m_pseudotree->getWidthCond() <= m_ibound);
}

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
  if (p->getArity() == q->getArity())
    return (p->getId() > q->getId());
  else
    return (p->getArity() > q->getArity());
}

#endif /* MINIBUCKETELIM_H_ */
