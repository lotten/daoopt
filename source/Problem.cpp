/*
 * Problem.cpp
 *
 *  Created on: Oct 10, 2008
 *      Author: lars
 */

#include "Problem.h"
#include <iostream>
#include <sstream>
#include <fstream>


void Problem::removeEvidence() {

  assert(m_n!=UNKNOWN);

  // record original no. of variables
  m_nOrg = m_n;

  // Declare aux. variables.
  int idx, i, new_r, new_n;
  val_t k, new_k;
  //map<uint,uint> evidence(m_evidence);
  vector<val_t> new_domains;
  vector<Function*> new_funs;

  // evidFlag[i]==TRUE for evidence variables i
  vector<bool> evidFlag(m_n,false);
  for (map<int,val_t>::iterator it = m_evidence.begin(); it != m_evidence.end(); ++it) {
    evidFlag[it->first] = true;
  }

  // Identify evidence variables and filter remaining variables
  idx = new_n = 0;
  new_k = 0;
  for (i = 0; i < m_n; ++i)
  {
    // Check for evidence variable.
    if (evidFlag[i]) {
      // nothing here, variable is already evidence
    }
    else if (m_domains[i] == 1) { // regard unary domains as evidence
      m_evidence.insert(make_pair(i, 0));
      ++m_e;
      evidFlag[i] = true;
    }
    else { // not evidence, keep variable
      m_old2new[i] = idx; // record translation
//      cout << "Var. " << i << " -> " << idx << endl;
      k = m_domains[i];
      new_k = max(new_k, k);
      new_domains.push_back(k);

      ++idx;
      ++new_n;
    }
  }

  // Substitute evidence to get reduced functions
  double globalConstant = ELEM_ONE;
  new_r = 0; // max. arity
  vector<Function*>::iterator fi = m_functions.begin();
  for (; fi != m_functions.end(); ++fi) {
    Function *fn = (*fi);
    // Substitute evidence variables
    Function* new_fn = fn->substitute(m_evidence);
    if (new_fn->isConstant()) {
      globalConstant OP_TIMESEQ new_fn->getTable()[0];
      delete new_fn;
    } else {
      new_fn->translateScope(m_old2new); // translate variable indexes
      new_funs.push_back(new_fn); // record new function
      new_r = max(new_r, (int) new_fn->getScope().size());
    }
    delete fn; // delete old function

  }

  // save global constant
  m_globalConstant = globalConstant;

  // update variable information
  m_domains = new_domains;
  m_n = new_n;
  m_k = new_k;

  // update function information
  m_functions = new_funs;
  m_r = new_r;
  m_c = m_functions.size();

  return;
}


bool Problem::parseOrdering(const string& file, vector<int>& elim) const {

  assert(m_n!=UNKNOWN);

  ifstream inTemp(file.c_str());
  inTemp.close();

  if (inTemp.fail()) { // file not existent yet
    return false;
  }

  igzstream in(file.c_str());

  int n=0,x=UNKNOWN;
  vector<bool> check(m_n, false);

  while (!in.eof()) {
    in >> x; ++n;
    if (x<0 || x>=m_n) {
      cerr << "Problem reading ordering, variable index " << x << " out of range" << endl;
      in.close();
      exit(1);
    }
    if (check[x]) {
      cerr << "Problem reading ordering, variable " << x << " appears more than once." << endl;
      in.close();
      exit(1);
    } else check[x] = true;
    elim.push_back(x);
    if (n==m_n) break;
  }

  if (n!=m_n) {
    cerr << "Problem reading ordering, number of variables doesn't match." << endl;
    in.close();
    exit(1);
  }

  in.close();
  return true;
}


void Problem::saveOrdering(const string& file, const vector<int>& elim) const {
  assert( (int) elim.size() == m_n);

  ogzstream out(file.c_str());

  if (!out) {
    cerr << "Error writing ordering to file " << file << endl;
    exit(1);
  }

  for (vector<int>::const_iterator it=elim.begin(); it!=elim.end(); ++it)
    out << ' ' << *it;

  out.close();

}



bool Problem::parseUAI(const string& prob, const string& evid) {
  {
    ifstream inTemp(prob.c_str());
    inTemp.close();

    if (inTemp.fail()) { // file not existent yet
      cerr << "Error reading problem file " << prob << ", aborting." << endl;
      exit(1);
    }
  }
  if (!evid.empty()) {
    ifstream inTemp(evid.c_str());
    inTemp.close();

    if (inTemp.fail()) { // file not existent yet
      cerr << "Error reading evidence file " << evid << ", aborting." << endl;
      exit(1);
    }
  }

  igzstream in(prob.c_str());

  // Extract the filename without extension.
  string fname = prob;
  size_t len, start, pos1, pos2;
  static const basic_string <char>::size_type npos = -1;
#if defined(WINDOWS)
  pos1 = fname.find_last_of("\\");
#elif defined(LINUX)
  pos1 = fname.find_last_of("/");
#endif
  pos2 = fname.find_last_of(".");
  if (pos1 == npos) { len = pos2; start = 0; }
  else { len = (pos2-pos1-1); start = pos1+1; }
  m_name = fname.substr(start, len);

  cout << "Reading problem " << m_name << " ..." << endl;

  vector<int> arity;
  vector<vector<int> > scopes;
  string s;
  int x,y;
  val_t xs;
  uint z;

  in >> s; // Problem type
  if (s == "BAYES") {
    m_task = TASK_MAX;
    m_prob = PROB_MULT;
  } else if (s == "MARKOV") {
    m_task = TASK_MAX;
    m_prob = PROB_MULT;
  } else {
    cerr << "Unsupported problem type \"" << s << "\", aborting." << endl;
    in.close();
    exit(0);
  }

  in >> x; // No. of variables
  m_n = x;
  m_domains.resize(m_n,UNKNOWN);
  m_k = -1;
  for (int i=0; i<m_n; ++i) { // Domain sizes
    in >> x; // read into int first
    if (x > numeric_limits<val_t>::max()) {
      cerr << "Domain size " << x << " out of range for internal representation.\n"
           << "(Recompile with different type for variable values.)" << endl;
      in.close();
      exit(0);
    }
    xs = (val_t)x;
    m_domains[i] = xs;
    m_k = max(m_k,xs);
  }

  in >> x; // No. of functions
  m_c = x;
  scopes.reserve(m_c);

  // Scope information for functions
  m_r = -1;
  for (int i = 0; i < m_c; ++i)
  {
    vector<int> scope;
    in >> x; // arity

    m_r = max(m_r, x);
    for (int j=0; j<x; ++j) {
      in >> y; // the actual variables in the scope
      if(y>=m_n) {
        cerr << "Variable index " << y << " out of range." << endl;
        exit(EXIT_FAILURE);
      }
      scope.push_back(y); // preserve order from file
    }
    scopes.push_back(scope);
  }

  // Read functions
  for (int i = 0; i < m_c; ++i)
  {
    in >> z; // No. of entries
    size_t tab_size = 1;

    for (vector<int>::iterator it=scopes[i].begin(); it!=scopes[i].end(); ++it) {
      tab_size *= m_domains[*it];
    }

    assert(tab_size==z); // product of domain sizes matches no. of entries

    // create set version of the scope (ordered)
    set<int> scopeSet(scopes[i].begin(), scopes[i].end());
    z = scopeSet.size();

    // compute reindexing map from specified scope to ordered, internal one
    map<int,int> mapping; int k=0;
    for (vector<int>::const_iterator it=scopes[i].begin(); it!=scopes[i].end(); ++it)
      mapping[*it] = k++;
    vector<int> reidx(z);
    vector<int>::iterator itr = reidx.begin();
    for (set<int>::iterator it=scopeSet.begin(); itr!=reidx.end(); ++it, ++itr) {
      *itr = mapping[*it];
    }

    // read the full table into an temp. array (to allow reordering)
    vector<double> temp(tab_size);
    for (size_t j=0; j<tab_size; ++j) {
      in >> temp[j];
    }

    // get the variable domain sizes
    vector<val_t> limit; limit.reserve(z);
    for (vector<int>::const_iterator it=scopes[i].begin(); it!=scopes[i].end(); ++it)
      limit.push_back(m_domains[*it]);
    vector<val_t> tuple(z, 0);

    // create the new table (with reordering)
    double* table = new double[tab_size];
    for (size_t j=0; j<tab_size; ) {
      size_t pos=0, offset=1;
      // j is the index in the temp. table
      for (k=z-1; k>=0; --k) { // k goes backwards through the ordered scope
        pos += tuple[reidx[k]] * offset;
        offset *= m_domains[scopes[i][reidx[k]]];
      }
      table[pos] = ELEM_ENCODE( temp[j] );
      increaseTuple(j,tuple,limit);
    }

    Function* f = new FunctionBayes(i,this,scopeSet,table,tab_size);
    m_functions.push_back(f);

  } // All function tables read
  in.close();

  // Read evidence?
  if (evid.empty()) {
    m_e = 0;
    return true; // No evidence, return
  }

  cout << "Reading evidence..." << endl;

  in.open(evid.c_str());

  in >> x;
  m_e = x; // Number of evidence

  for (int i=0; i<m_e; ++i) {
    in >> x; // Variable index
    in >> y; // Variable value
    xs = (val_t) y;
    m_evidence.insert(make_pair(x,xs));
  }

  in.close();

  return true;

}

#ifndef NO_ASSIGNMENT
void Problem::outputAndSaveSolution(const string& file, double mpe, pair<size_t,size_t> noNodes, const vector<val_t>& sol, bool subprobOnly) const {
#else
void Problem::outputAndSaveSolution(const string& file, double mpe, pair<size_t,size_t> noNodes, bool subprobOnly) const {
#endif

#ifndef NO_ASSIGNMENT
	  assert( subprobOnly || (int) sol.size() == m_n );
#endif

	  bool writeFile = false;
	  if (! file.empty())
	    writeFile = true;

	  ogzstream out;
	  if (writeFile) {
	    out.open(file.c_str(), ios::out | ios::trunc | ios::binary);
	    if (!out) {
	      cerr << "Error writing optimal solution to file " << file << endl;
	      exit(1);
	    }
	  }

	  cout << "s " << SCALE_LOG(mpe);
#ifndef NO_ASSIGNMENT
	  cout << ' ' << m_nOrg;
#endif


#ifndef NO_ASSIGNMENT

	  int fileAssigSize = UNKNOWN;
	  if (subprobOnly)
	    fileAssigSize = (int) sol.size() -1; // -1 for dummy var.
	  else
	    fileAssigSize = m_nOrg;

    if (writeFile) {
      BINWRITE(out,mpe); // mpe solution cost
      BINWRITE(out, noNodes.first); // no. of OR nodes expanded
      BINWRITE(out, noNodes.second); // no. of AND nodes expanded
      BINWRITE(out,fileAssigSize); // no. of variables in opt. assignment
    }

    val_t v = UNKNOWN;

	  for (int i=0; i<fileAssigSize; ++i) {
	    if (subprobOnly) { // only output subproblem assignment
	      v = sol.at(i);
	      cout << ' ' << (int) v;
	      if (writeFile) BINWRITE(out, v);
	    } else { // ouptut full assignment (incl. evidence)
	      map<int,int>::const_iterator itRen = m_old2new.find(i);
	      if (itRen != m_old2new.end()) {
	        v = sol.at( itRen->second);
	        cout << ' ' << (int) v;
	        if (writeFile) BINWRITE(out,v);
	      } else {
	        map<int,val_t>::const_iterator itEvid = m_evidence.find(i);
	        if (itEvid != m_evidence.end()) { // var. i was evidence
	          v = itEvid->second;
	          cout << ' ' << (int) v;
	          if (writeFile) BINWRITE(out,v);
	        } else { // var. had unary domain and was removed
	          v = 0;
	          cout << ' ' << (int) v;
	          if (writeFile) BINWRITE(out,v);
	        }
	      }
	    }
	  }
#else
    if (writeFile) {
      BINWRITE(out,mpe); // mpe solution cost
      BINWRITE(out, noNodes.first); // no. of OR nodes expanded
      BINWRITE(out, noNodes.second); // no. of AND nodes expanded
    }
#endif

	  cout << endl;
	  if (writeFile)
	    out.close();

}


void Problem::writeUAI(const string& prob) const {
  assert (prob.size());

  ogzstream out;
  out.open(prob.c_str(), ios::out | ios::trunc);

  if (!out) {
    cerr << "Error writing reduced network to file " << prob << endl;
    exit(1);
  }

  out << "MARKOV" << endl; // TODO hard-coding is not optimal

  // variable info
  out << (m_n) << endl;
  for (vector<val_t>::const_iterator it=m_domains.begin(); it!=m_domains.end(); ++it)
    out << ' ' << ((int) *it) ;

  // function information
  // NOTE: introduces a dummy function to account for a possible global constant
  // introduced by eliminating evidence and unary variables earlier
  out << endl << m_functions.size()+1 << endl; // no. of functions +1 (for dummy function)
  for (vector<Function*>::const_iterator it=m_functions.begin(); it!=m_functions.end(); ++it) {
    const set<int>& scope = (*it)->getScope();
    out << scope.size() << '\t'; // scope size
    for (set<int>::const_iterator itS=scope.begin(); itS!=scope.end(); ++itS)
      out << *itS << ' '; // variables in scope
    out << endl;
  }
  out << "0" << endl; // dummy function (constant, thus empty scope)
  out << endl;

  // write the function tables
  for (vector<Function*>::const_iterator it=m_functions.begin(); it!=m_functions.end(); ++it) {
    double * T = (*it)->getTable();
    out << (*it)->getTableSize() << endl; // table size
    for (size_t i=0; i<(*it)->getTableSize(); ++i)
      out << ' ' << SCALE_NORM( T[i] ); // table entries
    out << endl;
  }
  // and the dummy function table
  out << '1' << endl << ' ' << SCALE_NORM(m_globalConstant) << endl;

  // done
  out << endl;
  out.close();

}

