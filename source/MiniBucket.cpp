/*
 * MiniBucket.cpp
 *
 *  Created on: May 20, 2010
 *      Author: lars
 */

#include "MiniBucket.h"

#undef DEBUG


/* checks whether a function 'fits' in this MB */
bool MiniBucket::allowsFunction(Function* f) {

  const set<int>& a=m_jointScope, b=f->getScope();

  set<int>::const_iterator ita=a.begin(),
    itb = b.begin() ;

  int s = 0, d=0;

  while (ita != a.end() && itb != b.end()) {
    d = *ita - *itb;
    if (d>0) {
      ++s; ++itb;
    } else if (d<0) {
      ++s; ++ita;
    } else {
      ++s; ++ita; ++itb;
    }
  }

  while (ita != a.end()) {
    ++s; ++ita;
  }
  while (itb != b.end()) {
    ++s; ++itb;
  }

  // accept if no scope increase or new scope not greater than ibound
  return (s == (int) m_jointScope.size()) || (s <= m_ibound+1);
  // new scope would be greater then ibound?
  //return s <= m_ibound+1;

}


/* joins the functions in the MB while marginalizing out the bucket var.,
 * resulting function is returned */
Function* MiniBucket::eliminate(bool buildTable) {

#ifdef DEBUG
  cout << "  Marginalizing together:";
  for (vector<Function*>::iterator it=m_functions.begin();it!=m_functions.end();++it)
    cout << ' ' << (**it);
  cout << endl;
#endif

  set<int> scope;
  int i=0; size_t j=0; // loop variables

  vector<Function*>::const_iterator fit;
  set<int>::const_iterator sit;

  for (fit = m_functions.begin(); fit != m_functions.end(); ++fit) {
    scope.insert( (*fit)->getScope().begin() , (*fit)->getScope().end() );
  }

#ifdef DEBUG
  cout << "   Joint scope: " << scope << endl;
#endif

  // remove elimVar from the new scope
  scope.erase(m_bucketVar);
  int n = scope.size(); // new function arity

#ifdef DEBUG
  cout << "   Target scope: " << scope << endl;
#endif

  // compute new table size and collect domain sizes
  vector<val_t> domains;
  domains.reserve(n);
  size_t tablesize = 1;
  for (sit = scope.begin(); sit!=scope.end(); ++sit) {
//    if (*it != elimVar)
    tablesize *= m_problem->getDomainSize(*sit);
    domains.push_back(m_problem->getDomainSize(*sit));
  }

  double* newTable = NULL;
  if (buildTable) {
    newTable = new double[tablesize];
    for (j=0; j<tablesize; ++j) newTable[j] = ELEM_ZERO;

    // this keeps track of the tuple assignment
    val_t* tuple = new val_t[n+1];
    for (i=0; i<n; ++i) tuple[i] = 0; // i trough n index target variables
    val_t* elimVal = &tuple[n]; // n+1 is elimVar

    // maps each function scope assignment to the full tuple
    vector<vector<val_t*> > idxMap(m_functions.size());

    // holds iterators .begin() and .end() for all function scopes
    vector< pair< set<int>::const_iterator , set<int>::const_iterator > > iterators;
    iterators.reserve(m_functions.size());
    for (fit = m_functions.begin(); fit != m_functions.end(); ++fit) {
      // store begin() and end() for each function scope
      iterators.push_back( make_pair( (*fit)->getScope().begin(), (*fit)->getScope().end() ) );
    }

    // collect pointers to tuple values
    bool bucketVarPassed = false;
    for (i=0, sit=scope.begin(); i<n; ++i, ++sit) {
      if (!bucketVarPassed && *sit > m_bucketVar) { // just went past bucketVar
        for (j=0; j<m_functions.size(); ++j) {
          idxMap[j].push_back(elimVal);
          ++(iterators[j].first); // skip bucketVar in the original function scope
        }
        bucketVarPassed = true;
      }
      for (j=0; j<m_functions.size(); ++j) {
        //      cout << "  f" << funs[j]->getId() << ' ' << *sit << " == " << *(iterators[j].first) << endl;
        if (iterators[j].first != iterators[j].second // scope iterator != end()
            && *sit == *(iterators[j].first)) { // value found
          idxMap[j].push_back(&tuple[i]);
          ++(iterators[j].first);
        }
      }
    }

    if (!bucketVarPassed) { // bucketVar has highest index
      for (j=0; j<m_functions.size(); ++j) {
        idxMap[j].push_back(elimVal);
      }
    }

    // actual computation
    size_t idx; double z;
    // iterate over all values of elimVar
    for (*elimVal = 0; *elimVal < m_problem->getDomainSize(m_bucketVar); ++(*elimVal) ) {
      idx=0; // go over the full new table
      do {
        z = ELEM_ONE;
        for (j=0; j<m_functions.size(); ++j)
          z OP_TIMESEQ m_functions[j]->getValuePtr(idxMap[j]);
        newTable[idx] = max(newTable[idx],z);
      } while ( increaseTuple(idx,tuple,domains) );

    }
    // clean up
    delete[] tuple;
  }

  return new FunctionBayes(-m_bucketVar,m_problem,scope,newTable,tablesize);
}



