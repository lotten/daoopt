/*
 * Function.cpp
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

#include "Function.h"
#include "Problem.h"

#undef DEBUG

/* Constructor */
Function::Function(const int& id, Problem* p, const set<int>& scope, double* T, const size_t& size) :
  m_id(id), m_problem(p), m_table(T), m_tableSize(size), m_scope(scope) {
#ifdef PRECOMP_OFFSETS
  m_offsets.resize(scope.size());
  size_t offset = 1;
  set<int>::reverse_iterator rit; vector<size_t>::reverse_iterator ritOff;
  for (rit=m_scope.rbegin(), ritOff=m_offsets.rbegin(); rit!=m_scope.rend(); ++rit, ++ritOff) {
    *ritOff = offset;
    offset *= m_problem->getDomainSize(*rit);
  }
#endif
}


/* returns the table entry for the assignment (input is vector of val_t) */
double Function::getValue(const vector<val_t>& assignment) const {
//  assert(isInstantiated(assignment)); // make sure scope is fully instantiated
#ifdef PRECOMP_OFFSETS
  size_t idx = 0;
  set<int>::const_iterator it=m_scope.begin();
  vector<size_t>::const_iterator itOff=m_offsets.begin();
  for (; it!=m_scope.end(); ++it, ++itOff)
    idx += assignment[*it] * (*itOff);
#else
  size_t idx = 0, offset=1;
  for (set<int>::reverse_iterator rit=m_scope.rbegin(); rit!=m_scope.rend(); ++rit) {
    idx += assignment[*rit] * offset;
    offset *= m_problem->getDomainSize(*rit);
  }
#endif
  assert(idx < m_tableSize);
  return m_table[idx];
}


/* returns the table entry for the assignment (input is vector of pointers to val_t) */
double Function::getValuePtr(const vector<val_t*>& tuple) const {
  assert(tuple.size() == m_scope.size()); // make sure tuple size matches scope size
#ifdef PRECOMP_OFFSETS
  size_t idx = 0;
  for (size_t i=0; i<m_scope.size(); ++i)
    idx += *(tuple[i]) * m_offsets[i];
#else
  size_t idx = 0, offset=1;
  set<int>::reverse_iterator rit=m_scope.rbegin();
  vector<val_t*>::const_reverse_iterator ritTup = tuple.rbegin();
  for (; rit!=m_scope.rend(); ++rit,++ritTup) {
    idx += *(*ritTup) * offset;
    offset *= m_problem->getDomainSize(*rit);
  }
#endif
  assert(idx < m_tableSize);
  return m_table[idx];
}


void Function::translateScope(const map<int, int>& translate) {
  //cout << "Translating " << m_id << " :";
  set<int> newScope;
  for (set<int>::iterator it = m_scope.begin(); it != m_scope.end(); ++it) {
    newScope.insert(translate.find(*it)->second);
    //cout << ' ' << *it << "->" << translate.find(*it)->second;
  }
  //cout << endl;
  m_scope = newScope;
}

/* Main substitution function.
 * !! Changes the values of the newScope, newTable, newTableSize reference arguments !!
 */
void Function::substitute_main(const map<int,val_t>& assignment, set<int>& newScope,
                               double*& newTable, size_t& newTableSize) const {

  map<int,val_t> localElim; // variables in scope to be substituted
  newTableSize = 1; // size of resulting table

  for (set<int>::const_iterator it=m_scope.begin(); it != m_scope.end(); ++it) {
    // assignment contains evidence and unary variables
    map<int,val_t>::const_iterator s = assignment.find(*it);
    if (s != assignment.end()) {
      localElim.insert(*s); // variable will be instantiated
    } else {
      newScope.insert(*it);
      newTableSize *= m_problem->getDomainSize(*it);
    }
  }

  if (newTableSize == m_tableSize) {
    // no reduction, just return the original table
    newTable = new double[newTableSize];
    for (size_t i=0; i<newTableSize; ++i)
      newTable[i] = m_table[i];
    return;
  }

  // Collect domain sizes of variables in scope and
  // compute resulting table size
  vector<val_t> domains(m_scope.size(),UNKNOWN);
  int i=0;
  for (set<int>::iterator it=m_scope.begin(); it != m_scope.end(); ++it ) {
    domains[i] = m_problem->getDomainSize(*it);
    ++i;
  }

  // initialize the new table
  newTable = new double[newTableSize];

  // keeps track of assignment while going over table
  vector<val_t> tuple(m_scope.size(),0);
  size_t idx=0, newIdx=0;
  do {
    // does it match?
    bool match = true;
    set<int>::iterator itSco = m_scope.begin();
    vector<val_t>::iterator itTup = tuple.begin();
    map<int,val_t>::iterator itEli = localElim.begin();
    for ( ; match && itEli != localElim.end(); ++itEli) {
      while (itEli->first != *itSco) { ++itSco; ++itTup; }
      if (*itTup != itEli->second) match = false; // current tuple does not match evidence
    }

    // tuple is compatible with evidence, record in new table
    if ( match ) {
      newTable[newIdx] = m_table[idx];
      ++newIdx;
    }

  } while (increaseTuple(idx,tuple,domains)); // increases idx and the tuple

}


Function* FunctionBayes::clone() const {
  double* newTable = new double[m_tableSize];
  for (size_t i=0; i<m_tableSize; ++i)
    newTable[i] = m_table[i];
  Function* f = new FunctionBayes(m_id, m_problem, m_scope, newTable, m_tableSize );
  return f;
}


Function* FunctionBayes::substitute(const map<int,val_t>& assignment) const {

  set<int> newScope;
  double* newTable = NULL;
  size_t newTableSize;
  // compute the new scope, table, and table size
  substitute_main(assignment, newScope, newTable, newTableSize);

  // Create new, modified function object
  Function* newF = new FunctionBayes(m_id, m_problem, newScope, newTable, newTableSize);
  return newF;

}


#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC

/* Computes tightness:
 * 1) Table is projected down on 'proj'.
 * 2) Every tuple is checked to match the values from 'assig' for the vars in 'cond'.
 * 3) Count matching (projected) tuples with non-zero weight
 */
size_t Function::getTightness(const set<int>& proj, const set<int>& cond,
                              const vector<val_t>* assig) {

#ifdef DEBUG
  cout << "Function::getTightness\t#" << *this << endl;
  cout << "Function::getTightness\tproj  = " << proj << endl;
  cout << "Function::getTightness\tcond  = " << cond << endl;
  cout << "Function::getTightness\tassig = ";
  if (assig)
    cout << *assig << endl;
  else
    cout << "NULL" << endl;
#endif

  size_t arity = m_scope.size();

  // Compute intersection of scope and proj
  set<int> s;
  set_intersection(m_scope.begin(), m_scope.end(), proj.begin(), proj.end(), inserter(s,s.begin()));
//  DIAG(cout << "Function::getTightness\tProjected to " << s << endl);
  assert(!s.empty()); // make sure function is relevant // TODO return 1?

  // not actually projected => return full tightness
  if (s.size() == arity) {
//    DIAG(cout << "Function::getTightness\t-> full scope, returning precomputed tightness." << endl );
    return this->getTightness();
  }

  size_t count = m_tCache;
  // Check cache scope (but caching only applies if no assignment was specified)
  if ( assig || s != m_tCacheScope ) {

    // find the indexes of the relevant projected variables
    // if i is index in reduced scope, idxs[i] is index in full scope
    vector<int> idxs;
    idxs.reserve(s.size());
    for (set<int>::iterator it = s.begin(); it != s.end(); ++it) {
      int c = 0;
      for (set<int>::iterator itSc = m_scope.begin(); itSc != m_scope.end(); ++itSc) {
        if (*itSc==*it) { idxs.push_back(c); break; }
        ++c;
      }
    }

    // keep track of the complete tuple (not projected)
    vector<val_t> vals(arity, 0);
    vector<val_t> doms; doms.reserve(arity);
    for (set<int>::iterator itSc = m_scope.begin(); itSc != m_scope.end(); ++itSc)
      doms.push_back(m_problem->getDomainSize(*itSc));

    set<size_t> positives; // collect the new indices (wrt. projected scope)
    for (size_t i=0; i<m_tableSize; increaseTuple(i,vals,doms) ) {
      // i is index of *complete* tuple
      if ( m_table[i] != ELEM_ZERO ) {

        // check if tuple complies with assignment (if given)
        if (assig) {

          // over the constraining assignment variables
          set<int>::const_iterator itCo = cond.begin();
          // over the current tuples
          set<int>::const_iterator itSc = m_scope.begin();
          vector<val_t>::const_iterator itV = vals.begin();
          bool compatible = true;

          while (compatible && itSc != m_scope.end() && itCo != cond.end() ) {

            if (*itCo < *itSc) ++itCo;
            else if (*itSc < *itCo) ++itSc, ++itV;
            else { // *itSc == *itCo , check assignment
              if (assig->at(*itSc) != UNKNOWN && assig->at(*itSc) != *itV )
                compatible = false; // tuple does not comply with assignment => skip it
              ++itCo; ++itSc; ++itV;
            }
          }
          if (!compatible)
            continue; // skip this tuple in outer for loop
        }

        // compute the index of the projected tuple and store it
        size_t id = 0; // *projected* index
        size_t prod = 1;
        for (int j=s.size()-1; j>=0; --j) {
          id += vals[idxs[j]] * prod; // vals is *full* tuple
          prod  *= doms[idxs[j]];
        }
        positives.insert(id);

      }
    }

    if (this->getType() == TYPE_BAYES)
      count = positives.size();
    else
      assert(false);

    // save result into cache (if no assignment given)
    if (assig==NULL) {
      m_tCacheScope = s;
      m_tCache = count;
    }

  }

  return count;

}
#endif /* PARALLEL_DYNAMIC */


#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
bigfloat Function::gainRatio(const set<int>& uncovered, const set<int>& proj,
                              const set<int>& cond, const vector<val_t>* assig) {

  bigint csize = 1;

  set<int>::const_iterator it1 = m_scope.begin();
  set<int>::const_iterator it2 = uncovered.begin();

  // compute product of the newly covered domains
  while (it1 != m_scope.end() && it2 != uncovered.end()) {
    if (*it1 < *it2)
      ++it1;
    else if (*it2 < *it1)
      ++it2;
    else { // *it1==*it2
      csize *= m_problem->getDomainSize(*it1);
      ++it1, ++it2;
    }
  }

  if (csize==1) // nothing new covered
          return UNKNOWN;

  // return ratio of tightness of function (projected on cluster)
  // over product of newly covered domains (might be 0)
  // TODO use uncovered instead of cluster?

  size_t t = getTightness(proj, cond, assig);
//  DIAG( cout << "\t\tt = " << t << endl );
  return t / ((bigfloat) csize);

}
#endif /* PARALLEL_DYNAMIC */


#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
double Function::getAverage(const set<int>& cond, const vector<val_t>& assig) {

  double sum = ELEM_ONE;
  size_t count = 0;

  // make sure assignment is actually specified
  for (set<int>::const_iterator it=cond.begin(); it!=cond.end(); ++it) {
    assert (assig.at(*it) != UNKNOWN );
  }

  // keep track of the complete tuple (i.e., tuple size = scope size)
  vector<val_t> vals(m_scope.size(), 0);
  // cache domain sizes of scope variables
  vector<val_t> doms; doms.reserve( m_scope.size() );
  for (set<int>::iterator itSc = m_scope.begin(); itSc != m_scope.end(); ++itSc)
    doms.push_back(m_problem->getDomainSize(*itSc));

  // iterate over all tuples
  for (size_t i=0; i<m_tableSize; increaseTuple(i,vals,doms) ) {

    // over the constraining assignment variables
    set<int>::const_iterator itCo = cond.begin();
    // over the current tuples
    set<int>::const_iterator itSc = m_scope.begin(); // should be kept in sync with the next
    vector<val_t>::const_iterator itV = vals.begin();

    bool compatible = true;

    while (compatible && itSc != m_scope.end() && itCo != cond.end() ) {

      if (*itCo < *itSc) ++itCo;
      else if (*itSc < *itCo) ++itSc, ++itV; // keep sync
      else { // *itSc == *itCo , check assignment
        if (assig.at(*itSc) != *itV )
          compatible = false; // tuple does not comply with assignment => skip it
        ++itCo; ++itSc; ++itV;
      }
    }
    if (!compatible)
      continue; // skip this tuple in for loop over tuples

//    if (m_table[i] != ELEM_ZERO) { // don't count zeros
    if (m_table[i] != ELEM_ZERO && m_table[i] != ELEM_ONE) { // don't count zeros or ones
      sum OP_TIMESEQ m_table[i];
      count += 1;
    }

  }

  if (count) {
#ifdef USE_LOG
    return sum /count ;
#else
    return pow(sum, 1.0/count) ;
#endif
  } else {
    return ELEM_ONE;
  }

#ifdef USE_LOG
  return (count) ? (sum / count) : ELEM_ONE;
#else
  return (count) ? pow(sum, 1.0/count) : ELEM_ONE;
#endif

}
#endif /* PARALLEL_DYNAMIC */

