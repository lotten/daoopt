#ifndef __MEX_MPLP_H
#define __MEX_MPLP_H

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <cstdlib>
#include <cstring>

#include "factorgraph.h"
#include "alg.h"


namespace mex {

// Factor graph algorithm specialization to MPLP-like optimization
// 
// derived factorGraph contains original factors
// beliefs form a reparameterization of the same distribution

class mplp : public factorGraph, public gmAlg, virtual public mxObject {
public:
	typedef factorGraph::findex        findex;				// factor index
	typedef factorGraph::vindex        vindex;				// variable index
	typedef factorGraph::flist				 flist; 				// collection of factor indices
	//typedef factorGraph::Edge          Edge;

public:
	mplp() : factorGraph() { setProperties(); }
	mplp(const factorGraph& fg) : factorGraph(fg) { setProperties(); }
	mplp(vector<Factor> fs)     : factorGraph(fs) { setProperties(); }
	template <class InputIterator>
	mplp(InputIterator f, InputIterator l) : factorGraph(f,l) { setProperties(); }

	virtual mplp* clone() const { mplp* fg = new mplp(*this); return fg; }

	double lb() const { return 0; }	// !!!
	double ub() const { return _UB; }
	vector<index> best() const { return vector<index>(); } // !!!

	// Not a summation algorithm
	double logZ()   const { throw std::runtime_error("Not available"); }
	double logZub() const { throw std::runtime_error("Not available"); }
	double logZlb() const { throw std::runtime_error("Not available"); }


	Factor& belief(findex f) { return _beliefs[f]; }	//!!! const
	const Factor& belief(size_t f)   const { return _beliefs[f]; }	
	const Factor& belief(Var v)     const { return belief(localFactor(v)); }
	const Factor& belief(VarSet vs) const { throw std::runtime_error("Not implemented"); }
	const vector<Factor>& beliefs() const { return _beliefs; }


#ifdef MEX  
  // MEX Class Wrapper Functions //////////////////////////////////////////////////////////
  bool        mxCheckValid(const mxArray*);   // check if matlab object is compatible with this object
  void        mxSet(mxArray*);     // associate with A by reference to data
  mxArray*    mxGet();             // get a pointer to the matlab object wrapper (creating if required)
  void        mxRelease();         // disassociate with a matlab object wrapper, if we have one
  void        mxDestroy();         // disassociate and delete matlab object
  void        mxSwap(mplp& gm);     // exchange with another object and matlab identity
  /////////////////////////////////////////////////////////////////////////////////////////
#endif

  MEX_ENUM( Update   , Var,Factor,Edge,Tree);
	MEX_ENUM( Schedule , Fixed,Random,Flood,Priority); 

  MEX_ENUM( Property , Schedule,Update,StopIter,StopObj,StopMsg,StopTime);

  virtual void setProperties(std::string opt=std::string()) {
    if (opt.length()==0) {
      setProperties("Schedule=Fixed,Update=Var,StopIter=10,StopObj=-1,StopMsg=-1,StopTime=-1");
      return;
    }
    std::vector<std::string> strs = mex::split(opt,',');
    for (size_t i=0;i<strs.size();++i) {
      std::vector<std::string> asgn = mex::split(strs[i],'=');
      switch( Property(asgn[0].c_str()) ) {
				case Property::Schedule: _SchedMethod  = Schedule(asgn[1].c_str());    break;
				case Property::Update:   _UpdateMethod = Update(asgn[1].c_str());      break;
				case Property::StopIter: _stopIter     = strtod(asgn[1].c_str(),NULL); break;
				case Property::StopObj:  _stopObj      = strtod(asgn[1].c_str(),NULL); break;
				case Property::StopMsg:  _stopMsg      = strtod(asgn[1].c_str(),NULL); break;
				case Property::StopTime: _stopTime     = strtod(asgn[1].c_str(),NULL); break;
				default: break;
      }
    }
  }


	void init(const VarSet& vs) { init(); }	// !!! inefficient

	void init() { 
		_UB=0.0; 
		_beliefs=vector<Factor>(_factors); 
		for (vector<Factor>::const_iterator i=_beliefs.begin();i!=_beliefs.end();++i) _UB += std::log(i->max());
	}

	void run() {
		size_t stopIter = _stopIter * nFactors();											// easier to count updates than "iterations"
    double startTime = timeSystem();

		double dObj=_stopObj+1.0, dMsg=_stopMsg+1.0;									// initialize termination values
		size_t iter=0, print=1;
		Var nextVar; findex nextFactor; mex::vector<Edge> nextTree;		// temporary storage for updates

		for (; dMsg>=_stopMsg && iter<stopIter && dObj>=_stopObj; ) {
      if (_stopTime > 0 && _stopTime <= (timeSystem()-startTime)) break;       // time-out check

		// If we're using a random ordering scheme, generate a random (var,factor,tree)
		// If we're using a fixed scheme, point to the next (var,factor,tree)
		// If we're using a priority-based scheme, get the best (var,factor,tree)
		switch(_SchedMethod) {
			case Schedule::Fixed:
				switch(_UpdateMethod) {
					//case (BY_VAR):    nextVar=_orderVar[iter%_orderVar.size()]; break;
					//case (BY_FACTOR): nextFactor=_orderFac[iter%_orderFac.size()]; break;
					case (Update::Var):    nextVar=var(iter%nvar());	break;			// !!! better to use a meaningful order?
					case (Update::Factor): nextFactor=(iter%nFactors()); 	break; // ^^^ same;  vvv use small # of fixed trees?
					case (Update::Tree):   nextTree = spanTree(TreeType::WidthFirst, var(((nvar()+1)*iter)%nvar())); break;
					default: break;
				} break;
			case Schedule::Random:
				switch(_UpdateMethod) {
					case (Update::Var):    nextVar=var(randi(nvar()));			// Choose a random variable, factor, or spanning
					case (Update::Factor): nextFactor=randi(nFactors());		//   tree to reparameterize about at each iteration
					case (Update::Tree):   nextTree = spanTree(TreeType::WidthFirst, var(randi(nvar())));
					default: break;
				} break;
			case Schedule::Priority:
				throw std::runtime_error("Not implemented");
				break;
			default: break;
		}
		switch(_UpdateMethod) {
			case (Update::Var):    updateVar(nextVar);       iter++; break;
			case (Update::Factor): updateFactor(nextFactor); iter++; break;
			case (Update::Tree):   updateTree(nextTree);     iter+=nextTree.size() ? nextTree.size() : 1; break;
			default: break;
		}	

		if (iter>print*nFactors()) { print++; std::cout<<"UB: "<<_UB<<"\n"; }

		}
    printf("MPLP (%d iter, %0.1f sec): %f\n",(int)(iter/nFactors()),(timeSystem()-startTime),_UB);
	}

protected:	// Contained objects
	vector<Factor> _beliefs;
	double _UB;

	Update    _UpdateMethod;
	Schedule  _SchedMethod;
	double _stopIter, _stopObj, _stopMsg, _stopTime;




	// Update a single variable and all surrounding factors
	// Fixed point is all factors have equal max-marginals for all variables
	void updateVar(const Var& v) {
		findex vf = localFactor(v);											// collect factors: var node + 
		const mex::set<EdgeID>& nbrs = neighbors(vf);		//   its neighbors
		Factor fMatch = belief(vf); 										//   
		_UB -= std::log(fMatch.max());									// remove their bound contributions
		vector<Factor> fTmp; fTmp.resize(nbrs.size());	//   and compute their matched
		int ii=0;																				//   belief about v
		for (set<EdgeID>::const_iterator i=nbrs.begin();i!=nbrs.end();++i,++ii) {
			fTmp[ii] = belief(i->second).maxmarginal(v);
			fMatch *= fTmp[ii];
			_UB -= std::log(fTmp[ii].max());
		}
		_UB += std::log(fMatch.max());									// re-add total bound contribution
		fMatch ^= (1.0/(fTmp.size()+1));								//   and compute matched components
		ii=0; belief(vf)=fMatch;												// force var and neighbors to agree
		for (set<EdgeID>::const_iterator i=nbrs.begin();i!=nbrs.end();++i,++ii) {
			belief(i->second) *= fMatch/fTmp[ii];
		}
	}

	// Update a single factor and all surrounding variables
	// Fixed point is factor has max-value 1, product's MM agrees with variables
	void updateFactor(findex f) {
		if (isVarNode(f)) return;												// don't bother for variable nodes

		const VarSet vs = factor(f).vars();
		Factor F=belief(f);															// otherwise, collect the belief of this clique
		_UB -= std::log(F.max());												// removing its individual bound contributions
		for (VarSet::const_iterator v=vs.begin();v!=vs.end();++v) {
			F   *= belief(localFactor(*v));								// compute its collective belief
			_UB -= std::log(belief(localFactor(*v)).max());
		}

		belief(f)=F;																		// put singleton costs back to the variable nodes
		_UB += std::log(F.max());												// and re-add its "matched" bound contribution
		for (VarSet::const_iterator v=vs.begin();v!=vs.end();++v) {
			belief(localFactor(*v)) = F.maxmarginal(*v)^(1.0/vs.size());
			belief(f) /= belief(localFactor(*v));
		}
	}

	// Update (up to) a spanning tree and its variables
	// Fixed point is factors have max-value 1, product's MM agrees with variables
	// rootedTree is a list of edges (findex->findex) oriented away from the root(s)
	// !!! assumes single root (position zero)
	void updateTree(const mex::vector<Edge>& rootedTree ) {
		vector<double> N; N.resize(nFactors(),0.0);											// store sub-tree sizes
		if (rootedTree.size()==0) return;											

		if (isVarNode(rootedTree[0].first)) N[rootedTree[0].first]=1.0;	// initialize sub-tree sizes
		for (mex::vector<Edge>::const_iterator e=rootedTree.begin();e!=rootedTree.end();++e) {
			if (isVarNode(e->second)) N[e->second]=1.0; else N[e->second]=0.0;
		}
		vector<Factor> msgUp; msgUp.resize( nFactors() );			// make storage for upward messages
		vector<Factor> msgProd( _beliefs );										//  and their products at each node
		// only need to copy beliefs for nodes in tree (!!!)
		
		for (int e=rootedTree.size()-1;e>=0;--e) {
			findex p=rootedTree[e].first,i=rootedTree[e].second; 		//i=findex of node, p=findex of parent
			N[p]+=N[i];																					// count nodes in rooted subtree
			msgUp[i] = msgProd[i].maxmarginal(factor(p).vars());// compute upward msg and multipy it
			msgProd[p] *= msgUp[i];															//  into the message product at parent
			_UB -= std::log(belief(i).max());										// remove contribution of each child node
		}
		_UB -= std::log(belief(rootedTree[0].first).max());			//    and finally the root

		for (size_t e=0;e!=rootedTree.size();++e) {
			findex p=rootedTree[e].first,i=rootedTree[e].second; //i=findex of node, p=findex of parent
			if (isVarNode(i)) continue;													// if this is not a factor node, skip
			const VarSet& nbrs = adjacentVars(i);								// else get list of adjacent variables
			uint32_t N0 = N[p]; N[p]-=N[i];											// change Np to exclude this subtree
			Factor B=msgProd[i] * msgProd[p]/msgUp[i];
			for (VarSet::const_iterator j=nbrs.begin();j!=nbrs.end();++j) {
				findex jj=localFactor(*j);												// do weighted matching based on tree sizes
				msgProd[jj]=B.maxmarginal(factor(jj).vars())^(N[jj]/N0);
			}
			for (VarSet::const_iterator j=nbrs.begin();j!=nbrs.end();++j) {
				findex jj=localFactor(*j);												// then update factor & variable beliefs
				B/=msgProd[jj]; belief(jj)=msgProd[jj];						// !!! only needed if a leaf?
			}
			belief(i) = B;																			// this factor gets what's left over
		}

		for (size_t e=0;e!=rootedTree.size();++e) {							// now all the bound has been placed in the
			findex i = rootedTree[e].second;												//   variable nodes; run through them and
			if (!isVarNode(i)) continue;												//   add up the new upper bound contribution
			_UB += std::log(belief(i).max());
		} _UB += std::log(belief(rootedTree[0].first).max());
	}

};


#ifdef MEX
//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
bool mplp::mxCheckValid(const mxArray* GM) { 
	return factorGraph::mxCheckValid(GM);  										// we must be a factorGraph
	// !!! check that we have beliefs, or can make them
}

void mplp::mxSet(mxArray* GM) {
	if (!mxCheckValid(GM)) throw std::runtime_error("incompatible Matlab object type in mplp");
  factorGraph::mxSet(GM);														// initialize base components	

	// Check for algorithmic specialization???
}

mxArray* mplp::mxGet() {
	if (!mxAvail()) {
		factorGraph::mxGet();
	}
	return M_;
}
void mplp::mxRelease() {							// shallow dissociation from matlab object (!!! ???)
	throw std::runtime_error("NOT IMPLEMENTED");
}
void mplp::mxDestroy() {
	throw std::runtime_error("NOT IMPLEMENTED");
}

void mplp::mxSwap(mplp& gm) {
	factorGraph::mxSwap( (factorGraph&)gm );
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
#endif  // re-include
