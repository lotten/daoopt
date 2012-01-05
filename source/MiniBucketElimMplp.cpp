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

size_t MiniBucketElimMplp::build(const vector<val_t>* assignment, bool computeTables) {
  if (computeTables == false) return _mbe.simulateMemory();

  if (_options==NULL) std::cout<<"Warning (MBE-ATI): ProgramOptions not available!\n";
  if (_options!=NULL && _options->jglp > 0) {                  // Do the full thing:
    int ibound = _mbe.getIBound();
    mex::mbe mbe2=mex::mbe(_mbe.gmOrig().factors());           // make an initial mbe for the joingraph
    mbe2.setOrder(_mbe.getOrder());                            //   same elim order, etc.
    mbe2.setProperties("DoMatch=1,DoFill=0,DoJG=1");
    if (_options->mplp > 0) {
      char mplpIt[6]; sprintf(mplpIt,"%d",_options->mplp);
      mbe2.setProperties(std::string("DoMplp=").append(mplpIt));                    // if we're doing mplp do it here
    }
    mbe2.setIBound( _mbe.getIBound()/2 );                      // use a lower ibound than the max
    mbe2.init();
    mbe2.tighten(_options->jglp);                 // !!! redistributes logZ back into factors
    _mbe = mex::mbe( mbe2.factors() );                         // take tightened factors back
    _mbe.setOrder(mbe2.getOrder()); _mbe.setIBound(ibound);    // copy old parameters back
    //if (_memlimit) _memlimit -= 2*_mbe.memory()*sizeof(double)/1024/1024;
    mbe2 = mex::mbe();                                         // clear joingraph mbe structure
    _mbe.setProperties("DoMatch=1,DoFill=0,DoMplp=0,DoJG=0");  // final pass => matching only
    if (_memlimit) this->limitSize(_memlimit,NULL);   // re-check ibound limit
  } else if (_options!=NULL && _options->mplp>0) {
    char mplpIt[6]; sprintf(mplpIt,"%d",_options->mplp);
    _mbe.setProperties(std::string("DoMatch=1,DoFill=0,DoJG=0,DoMplp=").append(mplpIt));  // OR, mplp only
  } else if (_options!=NULL && _options->match>0) {
    std::cout<<"Using moment-matching only\n";
    _mbe.setProperties("DoMatch=1,DoFill=0,DoMplp=0,DoJG=0");  // OR, matching only
  }

  // Run mini-bucket to create initial bound
  _mbe.init();
  std::cout<<"Build Bound: "<<getGlobalUB()<<"\n";

  // Mini-bucket may have re-parameterized the original functions; if so, replace them
  const vector<mex::Factor>& factors = _mbe.gmOrig().factors();
  vector<Function*> newFunctions(factors.size()); // replacement, reparameterized functions
  for (size_t f=0;f<factors.size();++f) {
    double* tablePtr = new double[ factors[f].nrStates() ];
    std::set<int> scope;
    for (mex::VarSet::const_iterator v=factors[f].vars().begin(); v!=factors[f].vars().end(); ++v)
      scope.insert(v->label());
    newFunctions[f] = new FunctionBayes(f,_p,scope,tablePtr,factors[f].nrStates());
    newFunctions[f]->fromFactor( log(factors[f]) );
  }
  _p->replaceFunctions( newFunctions );      // replace them in the problem definition
  _pt->addFunctionInfo(_p->getFunctions());   // re-compute function assignments in pseudotree

  return this->getSize();
}

MiniBucketElimMplp::MiniBucketElimMplp(Problem* p, Pseudotree* pt, ProgramOptions* po, int ib)
    : Heuristic(p,pt,po), _p(p), _pt(pt), _mbe(), _memlimit(0), _options(po) {
  mex::vector<mex::Factor> orig(p->getC());
  for (int i=0;i<p->getC(); ++i) orig[i] = p->getFunctions()[i]->asFactor().exp();
  mex::graphModel gm(orig);

  _mbe = mex::mbe(gm);
  //_mbe.setProperties("DoMatch=1,DoFill=1,DoMplp=1");
  //_mbe.setProperties("DoMatch=1,DoFill=0,DoMplp=1");
  //_mbe.setProperties("DoMatch=1,DoFill=0,DoMplp=0");
  _mbe.setProperties("DoMatch=0,DoFill=0,DoMplp=0");
  mex::VarOrder ord(pt->getElimOrder().begin(),--pt->getElimOrder().end());   // -- to remove dummy root
  _mbe.setOrder(ord); _mbe.setIBound(ib);
}

