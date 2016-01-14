// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_LBP_H
#define __MEX_LBP_H

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <cstdarg>
#include <cstring>

#include "factorgraph.h"
#include "alg.h"
#include "indexedHeap.h"

/*
*/

namespace mex {

// Factor graph algorithm specialization for loopy belief propagation
// 

class lbp : public gmAlg, public factorGraph, virtual public mxObject {
public:
	typedef factorGraph::findex        findex;				// factor index
	typedef factorGraph::vindex        vindex;				// variable index
	typedef factorGraph::flist				 flist; 				// collection of factor indices

public:
	/// Constructors : from nothing, copy, list of factors, or input iterators
	lbp()                                 : factorGraph() { setProperties(); }
	lbp(const factorGraph& fg)            : factorGraph(fg) { setProperties(); }
	lbp(vector<Factor> fs)                : factorGraph(fs) { setProperties(); }
	template <class InputIterator>
	lbp(InputIterator f, InputIterator l) : factorGraph(f,l) { setProperties(); }

	virtual lbp* clone() const            { lbp* fg = new lbp(*this); return fg; }

#ifdef MEX  
  // MEX Class Wrapper Functions //////////////////////////////////////////////////////////
  //void        mxInit();            // initialize any mex-related data structures, etc (private)
  //inline bool mxAvail() const;     // check for available matlab object
  bool        mxCheckValid(const mxArray*);   // check if matlab object is compatible with this object
  void        mxSet(mxArray*);     // associate with A by reference to data
  mxArray*    mxGet();             // get a pointer to the matlab object wrapper (creating if required)
  void        mxRelease();         // disassociate with a matlab object wrapper, if we have one
  void        mxDestroy();         // disassociate and delete matlab object
  void        mxSwap(lbp& gm);     // exchange with another object and matlab identity
  /////////////////////////////////////////////////////////////////////////////////////////
#endif

	Factor& belief(size_t f) { return _beliefs[f]; }	//!!! const
	const Factor& belief(size_t f)  const { return _beliefs[f]; }
	const Factor& belief(Var v)     const { return belief(localFactor(v)); }
	const Factor& belief(VarSet vs) const { throw std::runtime_error("Not implemented"); }
	const vector<Factor>& beliefs() const { return _beliefs; }

	// Not a bound-producing algorithm but can try to produce a good config
 	double lb() const { throw std::runtime_error("Not available"); }
 	double ub() const { throw std::runtime_error("Not available"); }
  vector<index> best() const { throw std::runtime_error("Not available"); }

	// Gives an estimate of the partition function, but not a bound
	double logZ()   const { return _lnZ; }
	double logZub() const { throw std::runtime_error("Not available"); }
	double logZlb() const { throw std::runtime_error("Not available"); }
				


  MEX_ENUM( Schedule , Fixed,Random,Flood,Priority );

	MEX_ENUM( Property , Schedule,Distance,stopIter,stopObj,stopMsg);
  virtual void setProperties(std::string opt=std::string()) {
    if (opt.length()==0) {
      setProperties("Schedule=Priority,Distance=HPM,stopIter=10,stopObj=-1,stopMsg=-1");
      return;
    }
    std::vector<std::string> strs = mex::split(opt,',');
    for (size_t i=0;i<strs.size();++i) {
      std::vector<std::string> asgn = mex::split(strs[i],'=');
      switch( Property(asgn[0].c_str()) ) {
        case Property::Schedule:  _sched  = Schedule(asgn[1].c_str()); break;
        case Property::Distance:  _dist   = Factor::Distance(asgn[1].c_str()); break;
        case Property::stopIter:  setStopIter( strtod(asgn[1].c_str(),NULL) ); break;
        case Property::stopObj:   setStopObj( strtod(asgn[1].c_str(),NULL) ); break;
        case Property::stopMsg:   setStopMsg( strtod(asgn[1].c_str(),NULL) ); break;
        default: break;
      }
    }
  }

  void setStopIter(double d) { _stopIter = d; }                   // stop when d*(# factors) updates have been done
  void setStopObj(double d)  { _stopObj = d;  }                   // stop when objective change is less than d
  void setStopMsg(double d)  { _stopMsg = d;  }                   // stop when message updates sare less than d


	/// Initialize the data structures
	virtual void init(const VarSet& vs) { init(); } // !!! inefficient
	virtual void init() { 
		_beliefs=vector<Factor>(_factors);                            // copy initial beliefs from factors
		_msg=vector<Factor>(); _msg.resize(2*nEdges());               // initialize messages to the identity
		for (size_t e=0;e<2*nEdges();++e) if (edge(e)!=EdgeID::NO_EDGE) {  // f'n of the right variables
			_msg[e]=Factor( factor(edge(e).first).vars() & factor(edge(e).second).vars(), 1.0 );
		}
		_msgNew=vector<Factor>(_msg);                                 // copy that as "updated" message list

		_lnZ = 0.0;                                                   // compute initial partition f'n estimate
		for (size_t f=0;f<nFactors();++f) {
			belief(f) /= belief(f).sum();                               // normalize the beliefs
			_lnZ += (belief(f)*log(factor(f))).sum() + objEntropy(f);   // and compute the free energy estimate
		}

		if (_sched==Schedule::Priority) {                             // for priority scheduling
			for (size_t e=0;e<2*nEdges();++e)                           //  initialize all edges to infinity
       if (edge(e)!=EdgeID::NO_EDGE) priority.insert( std::numeric_limits<double>::infinity() , e);
		 } else {
			for (size_t f=0;f<nFactors();++f) forder.push_back(f);      // for fixed scheduling, get a default order
     }
	}



  /// Run the algorithm until one of the stopping criteria is reached
	virtual void run() {
		size_t stopIter = _stopIter * nFactors();											// it's easier to count updates than "iterations"

		double dObj=_stopObj+1.0, dMsg=_stopMsg+1.0;									// initialize termination values
		size_t iter=0, print=1;                                       // count updates and "iterations" for printing
		size_t f,n=0;                                                 // 

		for (; dMsg>=_stopMsg && iter<stopIter && dObj>=_stopObj; ) {

			if (_sched==Schedule::Priority) {                           // priority schedule =>
				f=edge(priority.top().second).second;											//   get next factor for update from queue
				priority.pop();  
			} else {                                                    // fixed schedule =>
				f=forder[n];                                              //   get next factor from list
				if (++n == forder.size()) n=0; 
			}

			if (_sched!=Schedule::Flood) {                              // For non-"flood" schedules,
			  Factor logF = log(factor(f)); dObj=0.0;                   // compute new belief and update objective:
			  dObj -= (belief(f)*logF).sum() + objEntropy(f);           //   remove old contribution
			  acceptIncoming(f);                                        //   accept all messages into factor f
			  _lnZ += dObj += (belief(f)*logF).sum() + objEntropy(f);   //   re-add new contribution
			}
			updateOutgoing(f);																					//   update outgoing messages from factor f

			if (_sched==Schedule::Priority) dMsg=priority.top().first;  // priority schedule => easy to check msg stop
			else if (_stopMsg>0 && n==0) {                              // else check once each time through all factors
				dMsg=0.0;
				for (size_t e=0;e<2*nEdges();++e) dMsg=std::max(dMsg, _msgNew[e].distance(_msg[e], _dist));
			}

			if (_sched==Schedule::Flood && n==0) {                      // for flooding schedules, recalculate all
				dObj = _lnZ; _lnZ = 0.0;                                  //   the beliefs and objective now
        for (size_t f=0;f<nFactors();++f) {
					acceptIncoming(f);
					_lnZ += (belief(f)*log(factor(f))).sum() + objEntropy(f);
				}
				dObj -= _lnZ;
			}

			if (iter>print*nFactors()) { print++; std::cout<<"lnZ: "<<_lnZ<<"\n"; }
			iter++;
		}

	}

protected:	// Contained objects
	vector<Factor>   _beliefs;                       // store calculated messages and beliefs
	vector<Factor>   _msg;
	vector<Factor>   _msgNew;

	indexedHeap      priority;                       // store priority schedule of edges
	vector<findex>   forder;	                       // or fixed order of factors

	double           _lnZ;                           // current objective function value

	Schedule         _sched;                         // schedule type
	Factor::Distance _dist;	                         // message distance measure for priority
	double           _stopIter, _stopObj, _stopMsg;	 // and stopping criteria

	/*
	void updateMsg(Edge e) {
		_msgNew[e.idx] = (belief(e.src)/_msg[e.rev]).marginal( belief(e.dst).vars() );
	}
	void acceptMsg(Edge e) {
		belief(e.dst) *= _msgNew[e.idx]/_msg[e.idx];		// update belief to reflect new message
		_msg[e.idx]=_msgNew[e.idx];											// move message into accepted set
	}
	*/

	/// Calculate the entropy contribution to the free energy from node n
	double objEntropy(size_t n) {
		double obj = belief(n).entropy();
		if (!isVarNode(n)) {
			VarSet vs=adjacentVars(n);
			for (VarSet::const_iterator i=vs.begin();i!=vs.end();++i)
				obj -= belief(n).marginal(*i).entropy();
		}
		return obj;
	}

	/// Re-calculate the belief at node n from the current incoming messages
  void calcBelief(size_t n) {
		const set<EdgeID>& nbrs = neighbors(n);				// get all incoming edges
		belief(n)=factor(n);                          // calculate local factor times messages
		for (set<EdgeID>::const_iterator i=nbrs.begin();i!=nbrs.end();++i) belief(n) *= _msg[i->ridx];
		belief(n) /= belief(n).sum();                 // and normalize
	}

	/// Accept all the incoming messages into node n, and recompute its belief
	void acceptIncoming(size_t	n) {								//
		const set<EdgeID>& nbrs = neighbors(n);				// get the list of neighbors
		belief(n)=factor(n);                          //   and start with just the local factor
		for (set<EdgeID>::const_iterator i=nbrs.begin();i!=nbrs.end();++i) {
			_msg[i->ridx] = _msgNew[i->ridx];           // accept each new incoming message
			belief(n) *= _msg[i->ridx];                 //   and include it in the belief
			if (_sched==Schedule::Priority) 
				priority.erase(i->ridx);                  // accepted => remove from priority queue
		}
		belief(n) /= belief(n).sum();                 // normalize belief
	}

	/// Recompute new messages from node n to its neighbors
	void updateOutgoing(size_t n) {									//
		const set<EdgeID>& nbrs = neighbors(n);				// get the list of neighbors
		for (set<EdgeID>::const_iterator i=nbrs.begin();i!=nbrs.end();++i) {
			_msgNew[i->idx] = (belief(n)/_msg[i->ridx]).marginal( belief(i->second).vars() );
			_msgNew[i->idx] /= _msgNew[i->idx].sum();   // normalize message
			if (_sched==Schedule::Priority)             // and update priority in schedule
				priority.insert( _msgNew[i->idx].distance(_msg[i->idx],_dist) , i->idx );
		}
	}


};


#ifdef MEX
//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
bool lbp::mxCheckValid(const mxArray* GM) { 
	//if (!strcasecmp(mxGetClassName(GM),"graphModel")) return false;
	// hard to check if we are derived from a graphmodel without just checking elements:
	return factorGraph::mxCheckValid(GM);  										// we must be a factorGraph
	// !!! check that we have beliefs, or can make them
}

void lbp::mxSet(mxArray* GM) {
	if (!mxCheckValid(GM)) throw std::runtime_error("incompatible Matlab object type in factorGraph");
  factorGraph::mxSet(GM);														// initialize base components	

	// Check for algorithmic specialization???
}

mxArray* lbp::mxGet() {
	if (!mxAvail()) {
		factorGraph::mxGet();
	}
	return M_;
}
void lbp::mxRelease() {	throw std::runtime_error("NOT IMPLEMENTED"); }
void lbp::mxDestroy() { throw std::runtime_error("NOT IMPLEMENTED"); }

void lbp::mxSwap(lbp& gm) {
	factorGraph::mxSwap( (factorGraph&)gm );
}
#endif


//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
#endif  // re-include
