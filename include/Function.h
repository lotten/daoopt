/*
 * Function.h
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
 *  Created on: Oct 9, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef FUNCTION_H_
#define FUNCTION_H_

/* define to enable precomputation of the offsets for value lookup */
#define PRECOMP_OFFSETS

#include "_base.h"
#include "utils.h"
//#include "Problem.h"

class Problem;

class Function {

protected:
  int m_id;               // Function id (number)

  Problem* m_problem;     // pointer to the problem

  double* m_table;        // the actual table of function values
  size_t  m_tableSize;    // size of the table

  set<int> m_scope;       // Scope of the function

#ifdef PRECOMP_OFFSETS
  vector<size_t> m_offsets;  // Precomputed offsets for value lookup
#endif

  /* Tightness related information */
  size_t m_tightness;     // number of valid entries in table
  size_t m_tCache;        // cached number of valid entries in reduced table
  set<int> m_tCacheScope; // cached scope for reduced tightness computation


public:
  int getId() const { return m_id; }
  size_t getTableSize() const { return m_tableSize; }
  double* getTable() const { return m_table; }
  const set<int>& getScope() const { return m_scope; }
  int getArity() const { return m_scope.size(); }

  /* returns true iff the function is constant */
  bool isConstant() const { return m_tableSize==1; }

  /* true iff var. i is in scope */
  bool hasInScope(const int& i) const { return (m_scope.find(i) != m_scope.end()); }

  /* true iff at least one var from S is in scope */
  bool hasInScope(const set<int>& S) const { return !intersectionEmpty(S,m_scope); }

  /* generates a new (smaller) function with reduced scope and the <var,val>
   * pairs from the argument factored into the new table.
   * ! HAS TO BE IMPLEMENTED IN SUBCLASSES !  */
  virtual Function* substitute(const map<int,val_t>& assignment) const = 0;

  /* translates the variables in the function scope */
  void translateScope(const map<int,int>& translate);

  /* checks if all variables in the function's scope are instantiated in assignment
   * (note that assignment is a full problem assignment, not just the function scope) */
  bool isInstantiated(const vector<val_t>& assignment) const;

  /* returns the table entry for an assignment */
  double getValue(const vector<val_t>& assignment) const;

  /* returns the function value for the tuple (which is pointered function scope) */
  double getValuePtr(const vector<val_t*>& tuple) const;

protected:
  /* main work for substitution: computes new scope, new table and table size
   * and stores them in the three non-const argument references */
  void substitute_main(const map<int,val_t>& assignment, set<int>&, double*&, size_t&) const;

public:
  /* generates a clone of this function object */
  virtual Function* clone() const = 0;

  /* gets static tightness */
  size_t getTightness() const { return m_tightness; }

#if defined PARALLEL_DYNAMIC or defined PARALLEL_STATIC
  /* tightness when projected down to 'proj' */
  size_t getTightness(const set<int>& proj, const set<int>& cond,
                       const vector<val_t>* assig = NULL);

  /* computes the gain ratio for greedy covering algorithm
   *  uncovered: the set of still uncovered variables in this iteration
   *  proj: Set of vars to project function scope to
   *  cond: Set of vars whose assignment is checked
   *  assig: Actual assignment to vars named in 'cond' */
  bigfloat gainRatio(const set<int>& uncovered, const set<int>& proj,
                      const set<int>& cond, const vector<val_t>* assig = NULL);
  /* computes and returns the average value, conditioned on the variables indicated
   * in the set, where the actual assignments are pulled from the assignment vector */
  double getAverage(const set<int>&, const vector<val_t>&);

#endif

public:
  virtual int getType() const = 0;

  virtual ~Function();

protected:
  Function(const int& id, Problem* p, const set<int>& scope, double* T, const size_t& size);

};

/***************************************************************************/

class FunctionBayes : public Function {

public:
  Function* substitute(const map<int,val_t>& assignment) const;
  Function* clone() const;
  inline int getType() const { return TYPE_BAYES; }

public:
  FunctionBayes(const int& id, Problem* p, const set<int>& scope, double* T, const size_t& size);
  virtual ~FunctionBayes() {}
};

# ifdef false
class FunctionWCSP : public Function {
  // TODO
};
#endif


/* Inline implementations */

inline Function::~Function() {
  if (m_table) delete[] m_table;
}

inline bool Function::isInstantiated(const vector<val_t>& assignment) const {
  for (set<int>::const_iterator it=m_scope.begin(); it!=m_scope.end(); ++it) {
    if (assignment[*it] == UNKNOWN)
      return false;
  }
  return true;
}


inline FunctionBayes::FunctionBayes(const int& id, Problem* p, const set<int>& scope, double* T, const size_t& size) :
  Function(id,p,scope,T,size) {
  size_t t = 0;
  if (T)
    for (size_t i=0; i<size; ++i) {
      if (T[i] != ELEM_ZERO) ++t;
    }
  m_tightness = t;
}


/* cout operator */
inline ostream& operator << (ostream& os, const Function& f) {
  os << 'f' << f.getId() << ':' << f.getScope();
  return os;
}


#endif /* FUNCTION_H_ */
