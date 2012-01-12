#ifndef __MEX_MINIBUCKET_H
#define __MEX_MINIBUCKET_H

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <cstdlib>
#include <cstring>

#include "factorgraph.h"
#include "alg.h"
#include "mplp.h"


namespace mex {

// Factor graph algorithm specialization to MPLP-like optimization
// 

class mbe : public graphModel, public gmAlg, virtual public mxObject {
public:
	typedef graphModel::findex        findex;				// factor index
	typedef graphModel::vindex        vindex;				// variable index
	typedef graphModel::flist		 		 	flist; 				// collection of factor indices

public:
	mbe() : graphModel() { setProperties(); }
	mbe(const graphModel& gm) : graphModel(gm), _gmo(gm)  { clearFactors(); setProperties(); }
	virtual mbe* clone() const { mbe* gm = new mbe(*this); return gm; }

	graphModel _gmo;

#ifdef MEX  
  // MEX Class Wrapper Functions //////////////////////////////////////////////////////////
  //bool        mxCheckValid(const mxArray*);   // check if matlab object is compatible with this object
  //void        mxSet(mxArray*);     // associate with A by reference to data
  //mxArray*    mxGet();             // get a pointer to the matlab object wrapper (creating if required)
  //void        mxRelease();         // disassociate with a matlab object wrapper, if we have one
  //void        mxDestroy();         // disassociate and delete matlab object
  //void        mxSwap(mbe& gm);     // exchange with another object and matlab identity
  /////////////////////////////////////////////////////////////////////////////////////////
	bool mxCheckValid(const mxArray* GM) { 
		return graphModel::mxCheckValid(GM);  										// we must be a factorGraph
		// !!! check that we have beliefs, or can make them
	}
	void mxSet(mxArray* GM) {
		if (!mxCheckValid(GM)) throw std::runtime_error("incompatible Matlab object type in mbe");
  	graphModel::mxSet(GM);														// initialize base components	
		// Check for algorithmic specialization???
	}
	mxArray* mxGet() { if (!mxAvail()) { graphModel::mxGet(); } return M_; }
	void mxRelease() { throw std::runtime_error("NOT IMPLEMENTED"); }
	void mxDestroy() { throw std::runtime_error("NOT IMPLEMENTED"); }
	void mxSwap(mbe& gm) { graphModel::mxSwap( (graphModel&)gm ); }
#endif

	// Can be an optimization algorithm or a summation algorithm....
	double ub() const { assert(elimOp==ElimOp::MaxUpper); return _logZ; }
	double lb() const { return _lb; }
	vector<index> best() const { throw std::runtime_error("Not implemented"); }

  double logZ()   const { return _logZ; }
  double logZub() const { assert(elimOp==ElimOp::SumUpper); return _logZ; }
  double logZlb() const { assert(elimOp==ElimOp::SumLower); return _logZ; }

	// No beliefs defined currently
  const Factor& belief(size_t f)  const { throw std::runtime_error("Not implemented"); }
  const Factor& belief(Var v)     const { throw std::runtime_error("Not implemented"); }
  const Factor& belief(VarSet vs) const { throw std::runtime_error("Not implemented"); }
  const vector<Factor>& beliefs() const { throw std::runtime_error("Not implemented"); }

	const graphModel& gmOrig() const { return _gmo; }

	void build(const graphModel& gmo, size_t iBound, const VarOrder& elimOrder);
	virtual void run() { }	// !!! init? or run?


	MEX_ENUM( ElimOp , MaxUpper,SumUpper,SumLower );

	MEX_ENUM( Property , ElimOp,iBound,sBound,Order,Distance,DoMatch,DoMplp,DoFill,DoJG );

  ElimOp elimOp;
	bool    _byScope;
	bool		_doMatch;
	int 		_doMplp;
	bool		_doFill;
	bool		_doJG;
  Factor::Distance distMethod;
  size_t   _iBound;
	size_t   _sBound;
	double   _logZ;
	double   _lb;
	VarOrder _order;
	vector<flist> atElim;
	vector<double> atElimNorm;

  /////////////////////////////////////////////////////////////////
  // Setting properties (directly or through property string)
  /////////////////////////////////////////////////////////////////

	void   setIBound(size_t i) { _iBound=i ? i : std::numeric_limits<size_t>::max(); }
	size_t getIBound() const   { return _iBound; }
	void   setSBound(size_t s) { _sBound=s ? s : std::numeric_limits<size_t>::max(); }
	size_t getSBound() const   { return _sBound; }
	void            setOrder(const VarOrder& ord) { _order=ord; }
	const VarOrder& getOrder()                    { return _order; }


  virtual void setProperties(std::string opt=std::string()) {
    if (opt.length()==0) {
      setProperties("ElimOp=MaxUpper,iBound=4,sBound=inf,Order=MinWidth,Distance=LInf,DoMatch=1,DoMplp=0,DoFill=0,DoJG=0");
      _byScope = true;
      return;
    }
    std::vector<std::string> strs = mex::split(opt,',');
    for (size_t i=0;i<strs.size();++i) {
      std::vector<std::string> asgn = mex::split(strs[i],'=');
      switch( Property(asgn[0].c_str()) ) {
        case Property::ElimOp:  elimOp  = ElimOp(asgn[1].c_str()); break;
        case Property::iBound:  setIBound( atol(asgn[1].c_str()) ); break;
        case Property::sBound:  setSBound( atol(asgn[1].c_str()) ); break;
        case Property::Order:   _order=_gmo.order(graphModel::OrderMethod(asgn[1].c_str())); break;
				case Property::Distance: distMethod = Factor::Distance(asgn[1].c_str()); _byScope=false; break;
        case Property::DoMplp:  _doMplp  = atol(asgn[1].c_str()); break;
        case Property::DoMatch: _doMatch = atol(asgn[1].c_str()); break;
        case Property::DoFill:  _doFill = atol(asgn[1].c_str()); break;
        case Property::DoJG:    _doJG = atol(asgn[1].c_str()); break;
        default: break;
      }
    }
  }



	Factor elim(const Factor& F, const VarSet& vs) {
		switch (elimOp) {
			case ElimOp::SumUpper:
			case ElimOp::SumLower:  return F.sum(vs); break;
			case ElimOp::MaxUpper:  return F.max(vs); break;
		}
		throw std::runtime_error("Unknown elim op");
	}

	Factor marg(const Factor& F, const VarSet& vs) {
		switch (elimOp) {
			case ElimOp::SumUpper:
			case ElimOp::SumLower: return F.marginal(vs);    break;
			case ElimOp::MaxUpper: return F.maxmarginal(vs); break;
		}
		throw std::runtime_error("Unknown elim op");
	}

	Factor elimBound(const Factor& F, const VarSet& vs) {
		switch (elimOp) {
			case ElimOp::SumUpper:  return F.max(vs); break;
			case ElimOp::SumLower:  return F.min(vs); break;
			case ElimOp::MaxUpper:  return F.max(vs); break;
		}
		throw std::runtime_error("Unknown elim op");
	}

	// !!!! for Lars : lookup remaining cost given context
	template <class MapType>
	double logHeurToGo(Var v, MapType vals) const {
		double s=1.0;
		for (size_t i=0;i<atElim[_vindex(v)].size();++i) {
			findex ii=atElim[_vindex(v)][i];
			const VarSet& vs=factor(ii).vars();
			s *= factor(ii)[sub2ind(vs,vals)];
		}
		return std::log(s)+atElimNorm[_vindex(v)];
	}

	// Scoring function for bucket aggregation
	//   Unable to combine => -3; Scope-only => 1.0; otherwise a positive double score
	double score(const vector<Factor>& fin, const Var& VX, size_t i, size_t j, const vector<Factor>& tmp) {
		double err;
		const Factor& F1=fin[i], &F2=fin[j];													// (useful shorthand)
    size_t iBound = std::max(std::max(_iBound,F1.nvar()-1),F2.nvar()-1);			// always OK to keep same size
		size_t sBound = std::max(std::max(_sBound,F1.nrStates()),F2.nrStates());
		VarSet both = F1.vars()+F2.vars();
		if ( both.nvar()>iBound+1 || both.nrStates()>sBound ) err=-3;	// too large => -3
		//else if (_byScope) err = 1.0/std::log(both.nrStates()+1);					// greedy scope-based (check if useful???)
		else if (_byScope) err = 1.0/(F1.nvar()+F2.nvar());					    // greedy scope-based 2 (check if useful???)
		//else if (_byScope) err = 1;																		// scope-based => constant score
		else {
			Factor F12 = elim(F1*F2,VX), F1F2=tmp[i]*tmp[j];       // otherwise, compute score
			err = F12.distance(F1F2 , distMethod );
		}
		return err;
	}

	// Simulated version for memory checking (scope only!)
	double scoreSim(const vector<VarSet>& fin, const Var& VX, size_t i, size_t j) {
		double err;
		const VarSet& F1=fin[i], &F2=fin[j];													// (useful shorthand)
    size_t iBound = std::max(std::max(_iBound,F1.nvar()-1),F2.nvar()-1);			// always OK to keep same size
		size_t sBound = std::max(std::max(_sBound,F1.nrStates()),F2.nrStates());
		VarSet both = F1+F2;
		if ( both.nvar()>iBound+1 || both.nrStates()>sBound ) err=-3;	// too large => -3
		//else if (_byScope) err = 1.0/std::log(both.nrStates()+1);			// greedy scope-based 1 (check if useful???)
		else if (_byScope) err = 1.0/(F1.nvar()+F2.nvar());					    // greedy scope-based 2 (check if useful???)
		//else if (_byScope) err = 1;																		// scope-based => constant score
		else {
			throw new std::runtime_error("Cannot simulate dynamic scoring procedure");
		}
		return err;
	}

  size_t SizeOf( const vector<Factor>& fs ) {
		size_t MemUsed=0; for (size_t f=0;f<fs.size();++f) MemUsed+=fs[f].nrStates(); return MemUsed;
  }
  size_t SizeOf( const vector<VarSet>& fs ) {
		size_t MemUsed=0; for (size_t f=0;f<fs.size();++f) MemUsed+=fs[f].nrStates(); return MemUsed;
  }


	// helper class for pairs of sorted indices
	struct sPair : public std::pair<size_t,size_t> {
		sPair(size_t ii, size_t jj) { if (ii<jj) {first=jj; second=ii; } else {first=ii; second=jj;} }
	};

	void init(const VarSet& vs) { init(); }						// !!! inefficient

	void init() {
    _logZ = 0.0;

		if (_doMplp) {
		  mplp _mplp(_gmo.factors()); 
      char niter[16]; sprintf(niter,"StopIter=%d",_doMplp);
		  _mplp.setProperties("Schedule=Fixed,Update=Var,StopObj=-1.0,StopMsg=-1.0");
      _mplp.setProperties(niter);
		  _mplp.init(); 
		  _mplp.run();
			//std::cout<<"After Mplp: "<<_mplp.ub()<<"\n";
		  _gmo = graphModel(_mplp.beliefs());
		}

		std::cout<<"Use scope only? "<<_byScope<<"\n";

		vector<Factor> fin(_gmo.factors());
		vector<double> Norm(_gmo.nFactors(),0.0);
		vector<flist>  vin;
		vector<vindex> parents = _gmo.pseudoTree(_order);
		//std::cout<<"PARENTS: "; for (size_t i=0;i<parents.size();++i) std::cout<<parents[i]<<" "; std::cout<<"\n";

		for (size_t i=0;i<_gmo.nvar();++i) vin.push_back(_gmo.withVariable(var(i)));
		atElim.clear(); atElim.resize(_gmo.nvar()); 
		atElimNorm.clear(); atElimNorm.resize(_gmo.nvar(),0.0); 
		vector<Factor> tmp; if (!_byScope) tmp.resize(_gmo.nFactors());	// temporary factors used in score heuristics
		vector<flist> Orig(_gmo.nFactors());							// origination info: which original factors are
		for (size_t i=0;i<Orig.size();++i) Orig[i]|=i;		//   included for the first time, and which newly
		vector<flist> New(_gmo.nFactors());								//   created clusters feed into this cluster


		//// Eliminate each variable in the sequence given: /////////////////////////////////
		for (VarOrder::const_iterator x=_order.begin();x!=_order.end();++x) {
			//std::cout<<"Eliminating "<<*x<<"\n";
			Var VX = var(*x);
			if (*x >= vin.size() || vin[*x].size()==0) continue;		// check that we have some factors over this variable

			flist ids = vin[*x]; 								// list of factor IDs contained in this bucket

	  	//// Select allocation into buckets ///////////////////////////////////////
			typedef flist::const_iterator flistIt;
			typedef std::pair<double,sPair> _INS;
			std::multimap<double,sPair > scores;
			std::map<sPair,std::multimap<double,sPair>::iterator> reverseScore;

			//std::cout<<"Initial table sizes: "; for (flistIt i=ids.begin();i!=ids.end();++i) std::cout<<fin[*i].numel()<<" "; std::cout<<"\n";

			//// Populate list of pairwise scores for aggregation //////////////////////
		  if (!_byScope) for (flistIt i=ids.begin();i!=ids.end();++i) tmp[*i] = elim(fin[*i],VX);
		  for (flistIt i=ids.begin();i!=ids.end();++i) {
				for (flistIt j=ids.begin(); j!=i; ++j) {
					double err = score(fin,VX,*i,*j,tmp); sPair sp(*i,*j);
				  reverseScore[sp]=scores.insert(_INS(err,sp));	 		// save score 
			  }
			  reverseScore[sPair(*i,*i)]=scores.insert(_INS(-1,sPair(*i,*i)));		 		// mark self index at -1
		  }
		
    	//// Run through until no more pairs can be aggregated: ////////////////////
			//   Find the best pair (ii,jj) according to the scoring heuristic and join
			//   them as jj; then remove ii and re-score all pairs with jj
    	for(;;) {
				std::multimap<double,sPair>::reverse_iterator top = scores.rbegin();
		 		//multimap<double,_IDX>::reverse_iterator	last=scores.lower_bound(top->first);	// break ties randomly !!!
				//std::advance(last, randi(std::distance(top,last)));
				//std::cout<<top->first<<" "<<top->second.first<<" "<<top->second.second<<"\n";
				
		  	if (top->first < 0) break;  													// if can't do any more, quit
		  	else {
			  	size_t ii=top->second.first, jj=top->second.second;
					//std::cout<<"Joining "<<ii<<","<<jj<<"; size "<<(fin[ii].vars()+fin[jj].vars()).nrStates()<<"\n";
			  	fin[jj] *= fin[ii]; 														// combine into j
					Norm[jj] += Norm[ii];
					erase(vin,ii,fin[ii].vars()); fin[ii]=Factor();	//   & remove i
	
					Orig[jj] |= Orig[ii]; Orig[ii].clear();  		// keep track of list of original factors in this cluster
					New[jj]  |= New[ii];  New[ii].clear();   		//   list of new "message" clusters incoming to this cluster
					
			  	if (!_byScope) tmp[jj] = elim(fin[jj],VX);     	// update partially eliminated entry for j
	
					for (flistIt k=ids.begin();k!=ids.end();++k) { 		// removing entry i => remove (i,k) for all k
						scores.erase(reverseScore[sPair(ii,*k)]); 
					}
					ids /= ii;

			  	for (flistIt k=ids.begin();k!=ids.end();++k) {  // updated j; rescore all pairs (j,k) 
						if (*k==jj) continue;
						double err = score(fin,VX,jj,*k,tmp); sPair sp(jj,*k);
				  	scores.erase(reverseScore[sp]);												// change score (i,j)
				  	reverseScore[sp]=scores.insert(_INS(err,sp));	//
			  	}
		  	}
	  	}


    	//// Perform any matching? /////////////////////////////////////////////////
			//    "Matching" here is: compute the largest overlap of all buckets, and ensure that the
			//    moments on that subset of variables are identical in all buckets.
			//    !!! also, add extra variables if we can afford them?
			//size_t beta=0;
	  	if (_doMatch && ids.size()>1) {

				// Possibly we should first extend the scopes of the buckets to be larger
				if (_doFill) {
					VarSet all=fin[ids[0]].vars(); for (size_t i=1;i<ids.size();i++) all|=fin[ids[i]].vars();
					//reorder all?
					for (size_t i=0;i<ids.size();i++) { 
						VarSet vsi = fin[ids[i]].vars(), orig=fin[ids[i]].vars();
						for (VarSet::const_iterator vj=all.begin();vj!=all.end();++vj) {
						  VarSet test = vsi + *vj;
							if (test.nvar() <= _iBound+1 && test.nrStates() <= _sBound) vsi=test;
						}
						if (vsi.nvar()>orig.nvar()) fin[ids[i]] += Factor(vsi-orig,0.0);
					}
				}

				Factor fmatch;
				vector<Factor> ftmp(ids.size());						// compute geometric mean
		  	VarSet var = fin[ids[0]].vars();						// on all mutual variables
				for (size_t i=1;i<ids.size();i++) var &= fin[ids[i]].vars();
				for (size_t i=0;i<ids.size();i++) {
          ftmp[i] = marg(fin[ids[i]],var);
          fmatch *= ftmp[i];
        }
				fmatch ^= (1.0/ids.size());									// and match each bucket to it
				for (size_t i=0;i<ids.size();i++) fin[ids[i]] *= fmatch/ftmp[i];
				
				//beta = addFactor( Factor(fmatch.vars(),1.0) );	// add node to new cluster graph
				//atElim[*x] |= beta;
	  	}


    	//// Weight heuristic? /////////
			//  Currently, we just take the first bucket; !!! add heuristics from matlab code
			size_t select = ids[0];
	  
    	//// Eliminate individually within buckets /////////////////////////////////
			//   currently does not use weights except 0/1; !!! add sumPower alternatives from matlab code
			vector<findex> alphas;
			//std::cout<<"Table sizes: ";
	  	for (flistIt i=ids.begin();i!=ids.end();++i) {
				// 
				// Create new cluster alpha over this set of variables; save function parameters also
				findex alpha = findex(-1), alpha2 = findex(-1);
				if (_doJG) { alpha=addFactor(fin[*i]); alphas.push_back(alpha); }

				//std::cout<<fin[*i].numel()<<" ";
		
				if (*i==select) fin[*i] = elim     (fin[*i],VX);
			  else            fin[*i] = elimBound(fin[*i],VX);	
        
				if (_doJG) _factors[alpha] /= fin[*i];
	
			  double maxf = fin[*i].max(); fin[*i]/=maxf;       // normalize for numerical stability	
				maxf=std::log(maxf); _logZ+=maxf; Norm[*i]+=maxf; // save normalization for overall bound

				if (!_doJG) alpha2 = addFactor(fin[*i]);          /// !!! change to adding a message not a factor?
				
				//if (_doJG && _doMatch && ids.size()>1) addEdge(alpha,beta);
				if (_doJG) for (size_t j=0;j<alphas.size()-1;++j) addEdge(alpha,alphas[j]);
				if (_doJG) for (flistIt j=New[*i].begin();j!=New[*i].end();++j) addEdge(*j,alpha);

				Orig[*i].clear(); New[*i].clear(); New[*i]|=alpha;  // now incoming nodes to *i is just alpha

				size_t k=parents[*x];                        //  mark next bucket and intermediates with msg for heuristic calc
				for (; k!=vindex(-1) && !fin[*i].vars().contains(var(k)); k=parents[k]) { atElim[k]|=alpha2; atElimNorm[k]+=Norm[*i]; }
				if (k!=vindex(-1)) { atElim[k] |= alpha2; atElimNorm[k]+=Norm[*i]; }  // need check?

				insert(vin,*i,fin[*i].vars());              // recompute and update adjacency
	  	}
			//std::cout<<"\n";

		}
		/// end for: variable elim order /////////////////////////////////////////////////////////

		Factor F;
		for (size_t i=0;i<fin.size();++i) F*=fin[i];
		assert( F.nvar() == 0 );
		_logZ += std::log(F.max());

		//std::cout<<"Bound "<<_logZ<<"\n";
	}

  void tighten(int nIter, double stopTime=-1, double stopObj=-1) {
    const mex::vector<EdgeID>& elist = edges();
    double startTime=timeSystem(), dObj=infty();
    size_t iter;
		for (iter=0; iter<nIter; ++iter) {
      if (dObj < stopObj) break; else dObj=0.0;
			for (size_t i=0;i<elist.size();++i) {
        if (stopTime > 0 && stopTime <= (timeSystem()-startTime)) break;
     	 	findex a,b; a=elist[i].first; b=elist[i].second; 
				if (a>b) continue;
				VarSet both = _factors[a].vars() & _factors[b].vars();
				//std::cout<<_factors[a].vars()<<"; "<<_factors[b].vars()<<"= "<<both<<"\n";
				Factor fratio = (_factors[a].maxmarginal(both) / _factors[b].maxmarginal(both))^(0.5);
				_factors[b] *= fratio; _factors[a] /= fratio;
			}
			for (size_t i=0;i<nFactors();++i) {
      	double maxf = _factors[i].max(); _factors[i]/=maxf; 
        double lnmaxf=std::log(maxf); _logZ+=lnmaxf; dObj+=lnmaxf;
			}
		std::cout<<"Tightening "<<_logZ<<"; d="<<dObj<<"\n";
		}
		double Zdist=std::exp(_logZ/nFactors());
		for (size_t f=0;f<nFactors();++f) _factors[f]*=Zdist;  // !!!! WEIRD; FOR GLOBAL CONSTANT TRANFER
    printf("JGLP (%d iter, %0.1f sec): %f\n",(int)iter,(timeSystem()-startTime),_logZ);
	}


	// Simulate for memory size info.  Cannot use dynamic decision-making. //////////////////////////////////////////
	// Also return largest function table size???  Might re-enable dynamic decisions...
	size_t simulateMemory( vector<VarSet>* cliques = NULL ) {

		vector<VarSet> fin; for (size_t f=0;f<_gmo.nFactors();++f) fin.push_back(_gmo.factor(f).vars());
		vector<flist>  vin; for (size_t i=0;i<_gmo.nvar();++i)     vin.push_back(_gmo.withVariable(var(i)));
    vector<VarSet> _factors;
		size_t MemUsed=0;   for (size_t f=0;f<fin.size();++f)      MemUsed += fin[f].nrStates(); 
		size_t MemMax = MemUsed;

		//// Eliminate each variable in the sequence given: /////////////////////////////////
		for (VarOrder::const_iterator x=_order.begin();x!=_order.end();++x) {
			Var VX = var(*x);
			if (*x >= vin.size() || vin[*x].size()==0) continue;		// check that we have some factors over this variable

			flist ids = vin[*x]; 								// list of factor IDs contained in this bucket

	  	//// Select allocation into buckets ///////////////////////////////////////
			typedef flist::const_iterator flistIt;
			typedef std::pair<double,sPair> _INS;
			std::multimap<double,sPair > scores;
			std::map<sPair,std::multimap<double,sPair>::iterator> reverseScore;

			//// Populate list of pairwise scores for aggregation //////////////////////
		  for (flistIt i=ids.begin();i!=ids.end();++i) {
				for (flistIt j=ids.begin(); j!=i; ++j) {
					double err = scoreSim(fin,VX,*i,*j); sPair sp(*i,*j);
				  reverseScore[sp]=scores.insert(_INS(err,sp));	 		// save score 
			  }
			  reverseScore[sPair(*i,*i)]=scores.insert(_INS(-1,sPair(*i,*i)));		 		// mark self index at -1
		  }
		
    	//// Run through until no more pairs can be aggregated: ////////////////////
    	for(;;) {
				std::multimap<double,sPair>::reverse_iterator top = scores.rbegin();
		  	if (top->first < 0) break;  													// if can't do any more, quit
		  	else {
			  	size_t ii=top->second.first, jj=top->second.second;
					double tmp  = fin[jj].nrStates() + fin[ii].nrStates();  // going to remove original ii, jj
			  	fin[jj] += fin[ii]; 														// combine into jj
					MemUsed += fin[jj].nrStates();                  // and add new jj
					MemMax   = std::max(MemMax,MemUsed);
					MemUsed -= tmp;
					erase(vin,ii,fin[ii]); fin[ii]=VarSet();	//   & remove i
	
					for (flistIt k=ids.begin();k!=ids.end();++k) { 		// removing entry i => remove (i,k) for all k
						scores.erase(reverseScore[sPair(ii,*k)]); 
					}
					ids /= ii;

			  	for (flistIt k=ids.begin();k!=ids.end();++k) {  // updated j; rescore all pairs (j,k) 
						if (*k==jj) continue;
						double err = scoreSim(fin,VX,jj,*k); sPair sp(jj,*k);
				  	scores.erase(reverseScore[sp]);												// change score (i,j)
				  	reverseScore[sp]=scores.insert(_INS(err,sp));	//
			  	}
		  	}
	  	}

			//// If matching, we usually create another clique in the graph
			if (_doMatch && ids.size()>1) {
				if (_doFill) {  // if filling, we increase the size of the buckets
					VarSet all=fin[ids[0]]; for (size_t i=1;i<ids.size();i++) all|=fin[ids[i]];
					for (size_t i=0;i<ids.size();i++) { 
						VarSet vsi = fin[ids[i]], orig=fin[ids[i]];
						for (VarSet::const_iterator vj=all.begin();vj!=all.end();++vj) {
						  VarSet test = vsi + *vj;
							if (test.nvar() <= _iBound+1 && test.nrStates() <= _sBound) vsi=test;
						}
						if (vsi.nvar()>orig.nvar()) {
				      double tmp = fin[ids[i]].nrStates();    // remove the old factor from memory
							fin[ids[i]] += vsi;                   //   increase it's size and
				      MemUsed += fin[ids[i]].nrStates();		//   put back into list of factors
					    MemMax = std::max(MemMax,MemUsed);
							MemUsed -= tmp;
						}
					}
				}
				// matching step:
				VarSet var = fin[ids[0]]; 
        for (size_t i=1;i<ids.size();i++) var &= fin[ids[i]];
				MemUsed += var.nrStates() * (ids.size()+2);           // temporary storage (+1 for possible temp var)
				MemMax   = std::max(MemMax,MemUsed);
				MemUsed -= var.nrStates() * (ids.size()+2);           // !!! don't keep cluster?
			}

    	//// Eliminate individually within buckets /////////////////////////////////
	  	for (flistIt i=ids.begin();i!=ids.end();++i) {

        if (cliques!=NULL) cliques->push_back(fin[*i]);  // save clique if desired

				//MemUsed += fin[*i].nrStates();        // create cluster alpha
				double tmp = fin[*i].nrStates();        // Remove the old incoming function,
        if (_doJG) { _factors.push_back( fin[*i] ); MemUsed += fin[*i].nrStates(); }
				fin[*i] -= VX;												//   eliminate the variable
				MemUsed += fin[*i].nrStates();				//   and put back into list of factors
				MemMax   = std::max(MemMax,MemUsed);
				MemUsed -= tmp;
        if (!_doJG) { _factors.push_back( fin[*i] );
				MemUsed += fin[*i].nrStates();        // create cluster alpha2
        }
				MemMax   = std::max(MemMax,MemUsed);

				insert(vin,*i,fin[*i]);              	// recompute and update adjacency
	  	}
		
		}
		/// end for: variable elim order /////////////////////////////////////////////////////////

		return MemMax;
		//return MemUsed;
	}


/*
 *
MBE: (1) forward pass MBE
     (2) constructing cluster graph and connectivity

MPLP (1) "by variable" for any graphical model
     (2) "by cluster set" as a generic function
		     - add "reverse schedule" for MBE tightening
     (2) "by cluster" subgradient descent
		     - for each cluster, choose random maximizer and convert to vector form
				 - save by cluster id and push solution by variable id into "map" of vectors
				 - compute averages by variable id
				 - for each cluster, update parameters
		 (3) "by edge" in a factor graph, scheduled
		 (4) by tree block
		 (5) by monotone chains


 *
*/

};


//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
#endif  // re-include


