/*
 * MiniBucket.h
 *
 *  Created on: Nov 8, 2008
 *      Author: lars
 */

#ifndef MINIBUCKETELIMNEW_H_
#define MINIBUCKETELIMNEW_H_

#include "Heuristic.h"
#include "Function.h"
#include "Problem.h"
#include "ProgramOptions.h"
#include "Pseudotree.h"
#include "utils.h"

#include "MiniBucket.h"

#include "mex/mbe.h"


/* The overall minibucket elimination */
class MiniBucketElimNew : public Heuristic {
public:

  // checks if the given i-bound would exceed the memlimit, in that
  // case compute highest possible i-bound that 'fits'
  //int limitIbound(int ibound, size_t memlimit, const vector<val_t> * assignment=NULL) {
  size_t limitSize(size_t memlimit, ProgramOptions* options, const vector<val_t> *assignment) {
		size_t ibound = options->ibound;
		_memlimit = memlimit;
		_options  = options;
		memlimit *= 1024 *1024 / sizeof(double);						// convert to # of table entries
		cout<<"Adjusting mini-bucket i-bound (ati)...\n";
		this->setIbound(ibound);                            // try initial ibound
		size_t mem = _mbe.simulateMemory();
		cout << " i=" << ibound << " -> " << ((mem / (1024*1024.0)) * sizeof(double) ) << " MBytes\n";
		while (mem > memlimit && ibound > 1) {
			this->setIbound(--ibound); 
			mem = _mbe.simulateMemory();
		  cout << " i=" << ibound << " -> " << ((mem / (1024*1024.0)) * sizeof(double) ) << " MBytes\n";
		}
		options->ibound = ibound; 
		return mem;
	}

  // builds the heuristic, limited to the relevant subproblem, if applicable.
  // if computeTables=false, only returns size estimate (no tables computed)
  size_t build(const vector<val_t>* assignment = NULL, bool computeTables = true) { 

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
			if (_memlimit) this->limitSize(_memlimit,_options,NULL);   // re-check ibound limit
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

  // returns the global upper bound
  double getGlobalUB() const { return _mbe.ub() + _p->getGlobalConstant(); }

  // computes the heuristic for variable var given a (partial) assignment
  double getHeur(int var, const vector<val_t>& assignment) const {
	  //double res=std::log( _mbe.heurToGo(_mbe.var(var),assignment) ); 
	  double res=_mbe.logHeurToGo(_mbe.var(var),assignment); 
		//std::cout<<"Requested "<<var<<"("<<(int)assignment[var]<<"): "<<res<<"\n";
		return res;
	}

  // reset the i-bound
  void setIbound(int ibound) { _mbe.setIBound(ibound); }
  // gets the i-bound
  int getIbound() const { return _mbe.getIBound(); }

  // gets sum of tables sizes
  size_t getSize() const {  
		size_t sz=0;
	  const mex::vector<mex::Factor> flist = _mbe.factors();
		for (size_t i=0;i<flist.size();++i) sz += flist[i].numel();
		return sz;
	}

  bool writeToFile(string fn) const { return true; }
  bool readFromFile(string fn)			{ return false; }

public:
  MiniBucketElimNew(Problem* p, Pseudotree* pt, int ib) : Heuristic(p,pt), _p(p), _pt(pt), 
		_mbe(), _memlimit(0), _options(NULL) {
		mex::vector<mex::Factor> orig(p->getC());
		for (int i=0;i<p->getC(); ++i) orig[i] = p->getFunctions()[i]->asFactor().exp();
		mex::graphModel gm(orig);
		
		_mbe = mex::mbe(gm);
		//_mbe.setProperties("DoMatch=1,DoFill=1,DoMplp=1");
		//_mbe.setProperties("DoMatch=1,DoFill=0,DoMplp=1");
		//_mbe.setProperties("DoMatch=1,DoFill=0,DoMplp=0");
		_mbe.setProperties("DoMatch=0,DoFill=0,DoMplp=0");
		mex::VarOrder ord(pt->getElimOrder().begin(),--pt->getElimOrder().end());		// -- to remove dummy root
		_mbe.setOrder(ord); _mbe.setIBound(ib);
	}
  ~MiniBucketElimNew() { }

protected:
	Problem* _p;
	Pseudotree* _pt;
	mex::mbe _mbe;
	size_t _memlimit;
	ProgramOptions* _options;

};



#endif /* MINIBUCKETELIM_H_ */
