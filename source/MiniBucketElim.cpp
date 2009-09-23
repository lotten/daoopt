/*
 * MiniBucket.cpp
 *
 *  Created on: Nov 8, 2008
 *      Author: lars
 */

#include "MiniBucketElim.h"

// disables DEBUG output
#undef DEBUG


#ifdef DEBUG
// ostream operator for debugging
ostream& operator <<(ostream& os, const list<Function*>& l) {
  list<Function*>::const_iterator it = l.begin();
  os << '[';
  while (it!=l.end()) {
    os << (**it);
    if (++it != l.end()) os << ',';
  }
  os << ']';
  return os;
}
#endif


// computes the augmented part of the heuristic estimate
double MiniBucketElim::getHeur(int var, const vector<val_t>& assignment) const {

  assert( var >= 0 && var < m_problem->getN());

  double h = ELEM_ONE;

  // go over augmented and intermediate lists and combine all values
  list<Function*>::const_iterator itF = m_augmented[var].begin();
  for (; itF!=m_augmented[var].end(); ++itF) {
    h OP_TIMESEQ (*itF)->getValue(assignment);
  }

  itF = m_intermediate[var].begin();
  for (; itF!=m_intermediate[var].end(); ++itF) {
    h OP_TIMESEQ (*itF)->getValue(assignment);
  }

  return h;
}


void MiniBucketElim::reset() {

  vector<list<Function*> > empty;
  m_augmented.swap(empty);

  vector<list<Function*> > empty2;
  m_intermediate.swap(empty2);

}


size_t MiniBucketElim::build(const vector<val_t> * assignment, bool computeTables) {

#ifdef DEBUG
  cout << "$ Building MBE(" << m_ibound << ")" << endl;
#endif

  this->reset();

  vector<int> elimOrder; // will hold dfs order
  findDfsOrder(elimOrder); // computes dfs ordering of relevant subtree

  m_augmented.resize(m_problem->getN());
  m_intermediate.resize(m_problem->getN());

  // keep track of total memory consumption
  size_t memSize = 0;

  // ITERATES OVER BUCKETS, FROM LEAVES TO ROOT
  for (vector<int>::reverse_iterator itV=elimOrder.rbegin(); itV!=elimOrder.rend(); ++itV) {

#ifdef DEBUG
    cout << "$ Bucket for variable " << *itV << endl;
#endif

    // collect relevant functions in funs
    vector<Function*> funs;
    const list<Function*>& fnlist = m_pseudotree->getNode(*itV)->getFunctions();
    funs.insert(funs.end(), fnlist.begin(), fnlist.end());
    funs.insert(funs.end(), m_augmented[*itV].begin(), m_augmented[*itV].end());
#ifdef DEBUG
    for (vector<Function*>::iterator itF=funs.begin(); itF!=funs.end(); ++itF)
      cout << ' ' << (**itF);
    cout << endl;
#endif

    // compute global upper bound for root (dummy) bucket
    if (*itV == elimOrder[0]) {// variable is dummy root variable
      if (computeTables && assignment) { // compute upper bound if assignment is given
        for (vector<Function*>::iterator itF=funs.begin(); itF!=funs.end(); ++itF)
          m_globalUB OP_TIMESEQ (*itF)->getValue(*assignment); // all constant functions, tablesize==1

//#ifdef DEBUG
        double mbval = m_globalUB OP_TIMES m_problem->getGlobalConstant();

        cout << "    MBE-UB = " << SCALE_LOG(mbval) << " (" << SCALE_NORM(mbval) << ")" << endl;
//#endif

      }
      continue; // skip the dummy variable's bucket
    }

    // sort functions by decreasing scope size
    sort(funs.begin(), funs.end(), scopeIsLarger);

    // partition functions into minibuckets
    vector<MiniBucket> minibuckets;
//    vector<Function*>::iterator itF; bool placed;
    for (vector<Function*>::iterator itF = funs.begin(); itF!=funs.end(); ++itF) {
//    while (funs.size()) {
      bool placed = false;
      for (vector<MiniBucket>::iterator itB=minibuckets.begin();
            !placed && itB!=minibuckets.end(); ++itB)
      {
        if (itB->allowsFunction(*itF)) { // checks if function fits into bucket
          itB->addFunction(*itF);
          placed = true;
        }
      }
      if (!placed) { // no fit, need to create new bucket
        MiniBucket mb(*itV,m_ibound,this);
        mb.addFunction(*itF);
        minibuckets.push_back(mb);
      }
//      funs.pop_front();
      //funs.erase(itF);
    }

    // minibuckets for current bucket are now ready, process each
    // and place resulting function
    for (vector<MiniBucket>::iterator itB=minibuckets.begin();
          itB!=minibuckets.end(); ++itB)
    {
      Function* newf = itB->eliminate(computeTables); // process the minibucket
      const set<int>& newscope = newf->getScope();
      memSize += newf->getTableSize();
      // go up in tree to find target bucket
      PseudotreeNode* n = m_pseudotree->getNode(*itV)->getParent();
      while (newscope.find(n->getVar()) == newscope.end() && n != m_pseudotree->getRoot() ) {
        m_intermediate[n->getVar()].push_back(newf);
        n = n->getParent();
      }
      // matching bucket found OR root of pseudo tree reached
      m_augmented[n->getVar()].push_back(newf);
    }
    // all minibuckets processed and resulting functions placed
  }

#ifdef DEBUG
  // output augmented and intermediate buckets
  if (computeTables)
    for (int i=0; i<m_problem->getN(); ++i) {
      cout << "$ AUG" << i << ": " << m_augmented[i] << " + " << m_intermediate[i] << endl;
    }
#endif

  // clean up for estimation mode
  if (!computeTables) {
    for (vector<list<Function*> >::iterator itA = m_augmented.begin(); itA!=m_augmented.end(); ++itA)
      for (list<Function*>::iterator itB = itA->begin(); itB!=itA->end(); ++itB)
        delete *itB;
    m_augmented.clear();
    m_augmented.clear();
  }

  return memSize;

}


// finds a dfs order of the pseudotree (or the locally restricted subtree)
// and writes it into the argument vector
void MiniBucketElim::findDfsOrder(vector<int>& order) const {
  order.clear();
  stack<PseudotreeNode*> dfs;
  dfs.push(m_pseudotree->getRoot());
  PseudotreeNode* n = NULL;
  while (!dfs.empty()) {
    n = dfs.top();
    dfs.pop();
    order.push_back(n->getVar());
    for (vector<PseudotreeNode*>::const_iterator it=n->getChildren().begin();
          it!=n->getChildren().end(); ++it) {
      dfs.push(*it);
    }
  }
}


// checks whether a function 'fits' in this MB
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
  //return s==m_jointScope.size() || s <= m_ibound;
  // new scope would be greater then ibound?
  return s <= m_ibound+1;

}


// joins the functions in the MB while marginalizing out the bucket var., resulting
// function is returned
Function* MiniBucket::eliminate(bool buildTable) {

  Problem* problem = m_mbElim->m_problem;

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
    tablesize *= problem->getDomainSize(*sit);
    domains.push_back(problem->getDomainSize(*sit));
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
    for (*elimVal = 0; *elimVal < problem->getDomainSize(m_bucketVar); ++(*elimVal) ) {
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

  return new FunctionBayes(-m_bucketVar,problem,scope,newTable,tablesize);
}


int MiniBucketElim::limitIbound(int ibound, size_t memlimit, const vector<val_t> * assignment) {

  // convert to bits
  memlimit *= 1024 *1024 / sizeof(double);

  cout << "Adjusting mini bucket i-bound..." << endl;
  this->setIbound(ibound);
  size_t mem = this->build(assignment, false);
  cout << " i=" << ibound << " -> " << ((mem / (1024*1024.0)) * sizeof(double) ) << " MBytes" << endl;

  while (mem > memlimit && ibound > 1) {
    this->setIbound(--ibound);
    mem = this->build(assignment, false);
    cout << " i=" << ibound << " -> " << ((mem / (1024*1024.0)) * sizeof(double) ) << " MBytes" << endl;
  }

  return ibound;

}


