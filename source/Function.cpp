/*
 * Function.cpp
 *
 *  Created on: Oct 9, 2008
 *      Author: lars
 */

#include "Function.h"
#include "Problem.h"

// Constructor
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


// returns the table entry for the assignment
// (input is vector of val_t)
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

// returns the table entry for the assignment
// (input is vector of pointers to val_t)
double Function::getValuePtr(const vector<val_t*>& tuple) const {

//  cout << 'f' << m_id << ": Tuple " << tuple.size() << endl;
//  cout << 'f' << m_id << ' ' << tuple << endl;

  assert(tuple.size() == m_scope.size()); // make sure tuple size matches scope size
#ifdef PRECOMP_OFFSETS
  size_t idx = 0;
  for (size_t i=0; i<m_scope.size(); ++i)
    idx += *(tuple[i]) * m_offsets[i];
#else
  size_t idx = 0, offset=1;
  set<int>::reverse_iterator rit=m_scope.rbegin()
  vector<int*>::const_reverse_iterator ritTup = tuple.rbegin();
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

// Main substitution function.
// !! Changes the values of the newScope, newTable, newTableSize reference arguments !!
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

#ifdef TIGHTNESS
size_t FunctionBayes::getTightness(const set<int>& proj, const set<int>& inst) {

  // Compute intersection of 'scope' and m_argv
  set<int> s;
  set_intersection(m_scope.begin(), m_scope.end(), proj.begin(), proj.end(), inserter(s,s.begin()));
  assert(!s.empty()); // make sure function is relevant

  // not actually projected, return full tightness
  if (s.size() == m_scope.size())
    return this->getTightness();

  size_t idx = 0;
  int* tuple = new int[m_scope.size()];
  for (int i=0; i<m_scope.size();++i)
    tuple[i] = 0;
  // collect domain sizes
  vector<int> domains;
  domains.reserve(m_scope.size());
  for (set<int>::const_iterator sit = m_scope.begin(); sit!=m_scope.end(); ++sit) {
    domains.push_back(m_problem->getDomainSize(*sit));
  }


  ////// update below //////


  // find the indexes of the relevant projected variables
  int* idxs = new int [s.size()];
  int c =0;
  for (set<int>::iterator it = s.begin(); it != s.end(); ++it) {
    for (int i=0; i<m_argc; ++i)
      if (m_argv[i]==*it) { idxs[c]=i; break; }
    ++c;
  }

  // Has the value assignment (of relevant variables) changed?
  bool valsChanged = false;
  // Is any variable in the (projected) scope assigned?
  bool restricted = false;

  for (int i=0; i<s.size(); ++i) {
    int curVal = m_problem->getValue(m_argv[idxs[i]]);
    if (curVal != -1) restricted = true;
    if (curVal != m_assignmentCached[idxs[i]]) {
      valsChanged = true;
      m_assignmentCached[i] = curVal;
    }
  }

  // function is fully covered by cluster and no relevant variables
  // are assigned yet
  if (s.size() == m_scope.size() && !restricted)
    return getTightness();

  // If match with last scope AND no relevant value changes,
  // return cached value
  if (m_tightnessCacheScope == s && !valsChanged) {
    switch (m_type) {
    case FUN_PROBABILITY : return m_tightnessCache;
    case FUN_CONSTRAINT : return m_tightnessCacheTableSize - m_tightnessCache;
    default: assert(false);
    }
  }

  // keep track of the complete tuple
  int* vals = new int[m_argc]; // variable value for iterating over table
  int* doms = new int[m_argc]; // max. domain size (static)
  for (int i=0; i<m_argc;++i) // init. values
    vals[i]=0, doms[i]=m_problem->getStaticDomainSize(m_argv[i]);

  set<int> positives; // collect the new indices
  for (int i=0; i<m_tableSize; increaseTupleCount(i,vals,doms,m_argc)) {
    // i is index of *complete* tuple
    if (m_table[i] && (!restricted ||
        compliesWithAssignment(vals, m_assignmentCached, idxs, s.size()) ) ) {
      int id = 0; // compute *projected* index
      for (int j=0; j<s.size(); ++j) {
        id += vals[idxs[j]] * prod(doms, idxs, j+1, s.size());
      }
      /*cout << i << ":";
      for (int k=0; k<s.size(); ++k)
        cout << ' ' << vals[idxs[k]];
      cout << endl;*/
      positives.insert(id);
    }
  }
  delete[] vals;
  delete[] doms;
  delete[] idxs;

  // compute max. possible projected table size
  m_tightnessCacheTableSize = 1;
  for (set<int>::iterator it = s.begin(); it != s.end(); ++it)
    m_tightnessCacheTableSize *= m_problem->getStaticDomainSize(*it);
  m_tightnessCache = positives.size();
  m_tightnessCacheScope = s;

  switch (m_type) {
  case FUN_PROBABILITY : return m_tightnessCache;
  case FUN_CONSTRAINT : return m_tightnessCacheTableSize - m_tightnessCache;
  default: assert(false);
  }


}
#endif
