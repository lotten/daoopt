/*
 * MiniBucket.h
 *
 *  Created on: Nov 8, 2008
 *      Author: lars
 */

#ifndef MINIBUCKET_H_
#define MINIBUCKET_H_

#include "Heuristic.h"
#include "Function.h"
#include "Problem.h"
#include "Pseudotree.h"
#include "utils.h"


// forward declaration
class MiniBucket;


// The overall minibucket elimination
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
  //bool scopeIsLarger(Function*, Function*) const;

public:
  // builds the heuristic, limited to the relevant subproblem, if applicable.
  // if computeTables=false, only returns size estimate (no tables computed)
  size_t build(const vector<val_t>* assignment = NULL, bool computeTables = true);

  // gets and sets the global upper bound
  double getGlobalUB() const { return m_globalUB; }

  // computes the heuristic for variable var given a (partial) assignment
  double getHeur(int var, const vector<val_t>& assignment) const;

public:
  MiniBucketElim(Problem* p, Pseudotree* pt, int ib);
  ~MiniBucketElim();

};


// A single minibucket, i.e. a collection of functions
class MiniBucket {

protected:
  int m_bucketVar;               // the bucket variable
  int m_ibound;                  // the ibound
  MiniBucketElim* m_mbElim;      // pointer to the bucket elimination structure
  vector<Function*> m_functions; // the functions in the MB
  set<int> m_jointScope;         // keeps track of the joint scope if the functions

public:
  // checks whether the MB has space for a function
  bool allowsFunction(Function*);
  // adds a function to the minibucket
  void addFunction(Function*);
  // Joins the MB functions, eliminate the bucket variable, and returns the resulting function
  // set buildTable==false to get only size estimate (table will not be computed)
  Function* eliminate(bool buildTable=true);

public:
  MiniBucket(int var, int bound, MiniBucketElim* mb);

};



// Inline definitions


inline MiniBucketElim::MiniBucketElim(Problem* p, Pseudotree* pt, int ib) :
#ifdef USE_LOG
  m_ibound(ib), m_globalUB(0.0), m_problem(p), m_pseudotree(pt)
#else
  m_ibound(ib), m_globalUB(1.0), m_problem(p), m_pseudotree(pt)
#endif
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

inline void MiniBucket::addFunction(Function* f) {
  assert(f);
  // insert function
  m_functions.push_back(f);
  // update joint scope
  m_jointScope.insert(f->getScope().begin(), f->getScope().end() );
}

inline MiniBucket::MiniBucket(int v, int b, MiniBucketElim* mb) :
  m_bucketVar(v), m_ibound(b), m_mbElim(mb) {}

#endif /* MINIBUCKET_H_ */
