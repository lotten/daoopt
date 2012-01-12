/*
 * MiniBucketElimMplp.cpp
 * by Alex Ihler, December 2011
 *
 * Refactored from inline code in MiniBucketElimMplp.h
 */

#include "MiniBucketElimMplp.h"

size_t MiniBucketElimMplp::limitSize(size_t memlimit, const vector<val_t> *assignment) {
  size_t ibound = m_options->ibound;
  _memlimit = memlimit;
  memlimit *= 1024 *1024 / sizeof(double);            // convert to # of table entries
  cout<<"Adjusting mini-bucket i-bound (ati)...\n";
  this->setIbound(ibound);                            // try initial ibound
  size_t mem = _mbe.simulateMemory();
  cout << " i=" << ibound << " -> " << ((mem / (1024*1024.0)) * sizeof(double) ) << " MBytes\n";
  while (mem > memlimit && ibound > 1) {
    this->setIbound(--ibound);
    mem = _mbe.simulateMemory();
    cout << " i=" << ibound << " -> " << ((mem / (1024*1024.0)) * sizeof(double) ) << " MBytes\n";
  }
  m_options->ibound = ibound;
  return mem;
}

// Constructor: allow for blank entries?  if so, how to respecify later?
// MPLP function (with or without elimination ordering?)
//    either static function that looks like constructor, or enable blank constructor?
//    construct mplp factorgraph, pass messages (stop crit specified), write back to problem
// JGLP function


// Copy DaoOpt Function class into mex::Factor class structures
mex::vector<mex::Factor> MiniBucketElimMplp::copyFactors( void ) {
  mex::vector<mex::Factor> fs(_p->getC());
  for (int i=0;i<_p->getC(); ++i) fs[i] = _p->getFunctions()[i]->asFactor().exp();
  return fs;
}

// Mini-bucket may have re-parameterized the original functions; if so, replace them
void MiniBucketElimMplp::rewriteFactors( const vector<mex::Factor>& factors) {
  vector<Function*> newFunctions(factors.size()); // to hold replacement, reparameterized functions
  for (size_t f=0;f<factors.size();++f) {         // allocate memory, copy variables into std::set
    double* tablePtr = new double[ factors[f].nrStates() ];
    std::set<int> scope;
    for (mex::VarSet::const_iterator v=factors[f].vars().begin(); v!=factors[f].vars().end(); ++v)
      scope.insert(v->label());
    newFunctions[f] = new FunctionBayes(f,_p,scope,tablePtr,factors[f].nrStates());
    newFunctions[f]->fromFactor( log(factors[f]) );    // write in log factor functions
  }
  _p->replaceFunctions( newFunctions );                // replace them in the problem definition
}


bool MiniBucketElimMplp::preprocess(const vector<val_t>* assignment) {
  bool changedFunctions = doMPLP();
  _options->mplp = -1; _options->mplps = -1;  // disable MPLP in subsequent build()
  return changedFunctions;
}


// reparameterize problem using mplp on the original factors
bool MiniBucketElimMplp::doMPLP() {
  bool changedFunctions = false;

  if (_options!=NULL && (_options->mplp > 0 || _options->mplps > 0)) {
    //mex::mplp _mplp( copyFactors() );
    mex::mplp _mplp( _mbe.gmOrig().factors() );
    _mplp.setProperties("Schedule=Fixed,Update=Var,StopIter=100,StopObj=-1,StopMsg=-1,StopTime=-1");
    _mplp.init();
    char opt[50];
    if (_options->mplp > 0)  { sprintf(opt,"StopIter=%d",_options->mplp); _mplp.setProperties(opt); }
    if (_options->mplps > 0) { sprintf(opt,"StopTime=%f",_options->mplps); _mplp.setProperties(opt); }
    _mplp.run();
    rewriteFactors( _mplp.beliefs() );

    mex::VarOrder ord=_mbe.getOrder(); int iBound = _mbe.getIBound();
    _mbe = mex::mbe( _mplp.beliefs() );                         // take tightened factors back
    //_mbe = mex::mbe( copyFactors() );                         // take tightened factors back
    _mbe.setOrder(ord); _mbe.setIBound(iBound);

    changedFunctions = true;
  }

  return changedFunctions;
}

// reparameterize problem using a join-graph of the current ibound size
bool MiniBucketElimMplp::doJGLP() {
  bool changedFunctions = false;

  if (_options !=NULL && (_options->jglp > 0 || _options->jglps > 0)) {
    //mex::mbe _jglp(copyFactors()); _jglp.setOrder(_mbe.getOrder()); _jglp.setIBound(_mbe.getIBound());
    mex::mbe _jglp(_mbe.gmOrig().factors()); _jglp.setOrder(_mbe.getOrder()); _jglp.setIBound(_mbe.getIBound());
    _jglp.setProperties("DoMatch=1,DoFill=0,DoJG=1,DoMplp=0");
    _jglp.init();
    int iter; if (_options->jglp>0) iter=_options->jglp; else iter=100;
    _jglp.tighten(iter, _options->jglps);
    rewriteFactors( _jglp.factors() );
    _mbe = mex::mbe(_jglp.factors()); _mbe.setOrder(_jglp.getOrder()); _mbe.setIBound(_jglp.getIBound());
    changedFunctions = true;
  }

  return changedFunctions;
}


size_t MiniBucketElimMplp::build(const vector<val_t>* assignment, bool computeTables) {
  if (computeTables == false) return _mbe.simulateMemory();

  if (_options==NULL) std::cout<<"Warning (MBE-ATI): ProgramOptions not available!\n";

  doMPLP();

  int ib=_mbe.getIBound();
  _mbe.setIBound(ib/2);
  bool sizeChanged = doJGLP();  
  _mbe.setIBound(ib);

  if (_memlimit && sizeChanged) this->limitSize(_memlimit,NULL);   // re-check ibound limit

  // Run mini-bucket to create initial bound
  _mbe.init();
  std::cout<<"Build Bound: "<<getGlobalUB()<<"\n";

  return this->getSize();
}

MiniBucketElimMplp::MiniBucketElimMplp(Problem* p, Pseudotree* pt, ProgramOptions* po, int ib)
    : Heuristic(p,pt,po), _p(p), _pt(pt), _mbe(), _memlimit(0), _options(po) {

  _mbe = mex::mbe( copyFactors() );
  _mbe.setProperties("DoMatch=0,DoFill=0,DoMplp=0,DoJG=0");  // MPLP / JGLP done separately if needed
  if (_options->match > 0)  { _mbe.setProperties("DoMatch=1"); }
  mex::VarOrder ord(pt->getElimOrder().begin(),--pt->getElimOrder().end());   // -- to remove dummy root
  _mbe.setOrder(ord); _mbe.setIBound(ib);
}

