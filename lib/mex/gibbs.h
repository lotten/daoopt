#ifndef __MEX_GIBBS_H
#define __MEX_GIBBS_H

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <cstdlib>
#include <cstring>

#include "enum.h"
#include "factorgraph.h"
#include "alg.h"

/*
*/

namespace mex {

// Factor graph algorithm specialization for gibbs sampling
// 

class gibbs : public graphModel, public gmAlg, virtual public mxObject {
public:
	typedef factorGraph::findex        findex;				// factor index
	typedef factorGraph::vindex        vindex;				// variable index
	typedef factorGraph::flist				 flist; 				// collection of factor indices

public:
	gibbs() : graphModel() { setProperties(); }
	gibbs(const graphModel& fg) : graphModel(fg) { setProperties(); }
	gibbs* clone() const { gibbs* fg = new gibbs(*this); return fg; }


  /*
  struct Property {
    enum Type { TempMin, TempMax, Best, Beliefs, nIter, nSamples, nType };
    Type t_;
    Property(Type t) : t_(t) {}
    Property(const char* s) : t_() {
      for (int i=0;i<nType;++i) if (strcasecmp(names()[i],s)==0) { t_=Type(i); return; }
      throw std::runtime_error("Unknown type string");
    }
    operator Type () const { return t_; }
    operator char const* () const { return names()[t_]; }
  private:
    template<typename T> operator T() const;
    static char const* const* names() {
      static char const* const str[] = {"TempMin","TempMax","Best","Beliefs","nIter","nSamples",0};
      return str;
    }
  };
	*/
	MEX_ENUM( Property , TempMin,TempMax,Best,Beliefs,nIter,nSamples );

  virtual void setProperties(std::string opt=std::string()) {
    if (opt.length()==0) {
      setProperties("TempMin=1.0,TempMax=1.0,Best=0,Beliefs=0,nIter=1000,nSamples=100");
		  _order.clear(); _order.reserve(nvar()); for (size_t v=0;v<nvar();v++) _order.push_back(v);
      return;
    }
    std::vector<std::string> strs = mex::split(opt,',');
    for (size_t i=0;i<strs.size();++i) {
      std::vector<std::string> asgn = mex::split(strs[i],'=');
      switch( Property(asgn[0].c_str()) ) {
        case Property::TempMin:  tempMin    = strtod(asgn[1].c_str(),NULL);  break;
        case Property::TempMax:  tempMax    = strtod(asgn[1].c_str(),NULL);  break;
        case Property::Best:     NoBest     = !atol(asgn[1].c_str()); break;
        case Property::Beliefs:  NoBeliefs  = !atol(asgn[1].c_str()); break;
        case Property::nIter:    nIter      = atol(asgn[1].c_str());  break;
        case Property::nSamples: nSamples   = atol(asgn[1].c_str());  break;
        default: break;
      }
    }
  }


	void init(const VarSet& vs) { init(); }

	void init() {
		_samples.clear();
		if (_initState.size()>0) _state=_initState;
		else {
			_state.resize(nvar());			// might want to search for a good initialization
			for (VarOrder::iterator i=_order.begin();i!=_order.end();++i) _state[var(*i)]=randi(var(*i).states());
		}
		if (!NoBeliefs) { 
			_beliefs.resize(nFactors()); 
			for (size_t f=0;f<nFactors();++f) _beliefs[f]=Factor(factor(f).vars(),0.0); 
		}
		if (!NoBest) {
			_best=_state;
			_lb=0.0; 
			for (size_t f=0;f<nFactors();++f) _lb+=std::log( factor(f)[sub2ind(factor(f).vars(),_state)] );
		}
	}


	void run() {

		double score=0.0; for (size_t f=0;f<nFactors();++f) score+=std::log( factor(f)[sub2ind(factor(f).vars(),_state)] );

		for (size_t j=0,i=0; i<nSamples; ++i) {													// keep nSamples evenly space samples
			size_t jNext = (size_t) ((1.0 + i)/nSamples*nIter);				//   among nIter steps
			for (;j<jNext;++j) {

				// Each iteration, go over all the variables:
	      for (size_t v=0; v<nvar();++v) {              
        	if (var(v).states()==0) continue;                    	//   (make sure they're non-empty)
        	const flist& factors = withVariable(var(v));      		// get the factors they are involved with
        	Factor F(var(v),1.0);
        	for (flist::const_iterator f=factors.begin(); f!=factors.end(); ++f) {
          	VarSet vs=factor(*f).vars();  vs/=var(v);       		// and condition on their neighbors
          	F *= factor(*f).slice(vs, sub2ind(vs,_state));
        	}
					if (!NoBest) score -= std::log(F[_state[v]]);					// remove current value
					if (temp != 1.0) _state[v] = (F^temp).sample();       	// then draw a new value
					else _state[v] = F.sample();                          	//   (annealed or not)
					if (!NoBest) { 
						score += std::log(F[_state[v]]);											// update score incrementally
						if (score>_lb) { _lb=score; _best=_state; }					//   and keep the best
					}
      	}																												///// end iterating over each variable

				// After each sweep, update our statistics
				if (!NoBest && isinf(score)) {												// if we're still looking for a valid config
					score=0.0; 																					//   we need to update the score completely
					for (size_t f=0;f<nFactors();++f) 
						score+=std::log( factor(f)[sub2ind(factor(f).vars(),_state)] ); 
				}
				if (!NoBeliefs) {																			// if we're keeping marginal estimates
					for (size_t f=0;f<nFactors();++f)										//   then run through the factors
						_beliefs[f][sub2ind(factor(f).vars(),_state)]+=1.0/nSamples;	//   and update their status
				}
				if (tempMin != tempMax) temp += (tempMax-tempMin)/nIter;	// update temperature if annealed

    	}
    	_samples.push_back(_state);
		}
	}

	double lb() 				 const 	{ return _lb; }
	double ub() 				 const 	{ throw std::runtime_error("Not available"); }
	vector<index> best() const 	{ return _best; }

	double logZ()   const { throw std::runtime_error("Not available"); }
	double logZub() const { throw std::runtime_error("Not available"); }
	double logZlb() const { throw std::runtime_error("Not available"); }

	const vector<vector<index> >& samples() { return _samples; }

	const Factor& belief(size_t i)   const { return _beliefs[i]; }
	const Factor& belief(Var v) 	  const { return belief(withVariable(v)[0]); }
	const Factor& belief(VarSet vs) const { throw std::runtime_error("Not implemented"); }
	const vector<Factor>& beliefs() const { return _beliefs; }

#ifdef MEX  
  // MEX Class Wrapper Functions //////////////////////////////////////////////////////////
  bool        mxCheckValid(const mxArray*);   // check if matlab object is compatible with this object
  void        mxSet(mxArray*);     // associate with A by reference to data
  mxArray*    mxGet();             // get a pointer to the matlab object wrapper (creating if required)
  void        mxRelease();         // disassociate with a matlab object wrapper, if we have one
  void        mxDestroy();         // disassociate and delete matlab object
  void        mxSwap(lbp& gm);     // exchange with another object and matlab identity
  /////////////////////////////////////////////////////////////////////////////////////////
#endif

private:
	bool NoBest, NoBeliefs;
	size_t nSamples, nIter;
	vector<index> _state, _initState;
	VarOrder _order;
	vector<index> _best;
	double _lb;
	vector<Factor> _beliefs;
	vector<vector<index> > _samples;
	double tempMin, tempMax, temp;

};


#ifdef MEX
//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
bool gibbs::mxCheckValid(const mxArray* GM) { 
	return factorGraph::mxCheckValid(GM);  										// we must be a factorGraph
}
void gibbs::mxSet(mxArray* GM) { throw std::runtime_error("NOT IMPLEMENTED"); }
mxArray* gibbs::mxGet() { throw std::runtime_error("NOT IMPLEMENTED"); }
void gibbs::mxRelease() { throw std::runtime_error("NOT IMPLEMENTED"); }
void gibbs::mxDestroy() { throw std::runtime_error("NOT IMPLEMENTED"); }
void gibbs::mxSwap(gibbs& gm) { throw std::runtime_error("NOT IMPLEMENTED"); }
#endif


//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
#endif  // re-include
