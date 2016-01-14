// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_FACTORGRAPH_H
#define __MEX_FACTORGRAPH_H

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <queue>
#include <stack>

#include "graphmodel.h"
//#include "mSparse.h"

#ifdef MEX
#include "matrix.h"
#endif

/*
*/

namespace mex {

// Factor Graph Base Class
// - graphical model plus edges; syntax is bipartite between "variable nodes" and "factor nodes"
// 
// Some implementation notes:
//   Internally, factors and variables are mainly referenced by integer indices, with
//   0 <= f < nFactors() and 0 <= v < nvar().  However, many interface functions are called
//   using a variable object, Var(label,dim).  To convert, var(v) gives the vth Var object
//   and the internal function _vindex(V) gives the index corresponding to variable object V.
//
// Questions: 
// (1) idxset versus set issues? (caused by matlab 1-based indexing)
// (2) eventually need to change to Factor* to allow for polymorphism in factor class?

class factorGraph : public graphModel, virtual public mxObject {
public:
	typedef graphModel::findex        findex;				// factor index
	typedef graphModel::vindex        vindex;				// variable index
	typedef graphModel::flist				  flist; 				// collection of factor indices
protected:
	typedef uint32_t                  eindex;				// edge index

public:
	factorGraph() : graphModel(), _vIndex() { } 
	
	factorGraph(const factorGraph& fg) : graphModel((graphModel&)fg), _vIndex(fg._vIndex) { } 

	factorGraph(vector<Factor> fs) : graphModel(fs), _vIndex() {
		_vIndex.resize(nvar());
		_checkFactors();
	}
	
	template <class InputIterator>
	factorGraph(InputIterator first, InputIterator last) : graphModel(first,last), _vIndex() {
		//while (first!=last) { addFactor(*first); ++first; }
		_vIndex.resize(nvar());
    _checkFactors();
	}

	virtual factorGraph* clone() { factorGraph* fg = new factorGraph(*this); return fg; }

  void _checkFactors() {
    if (_vIndex.size() < nvar()) _vIndex.resize(nvar());
		std::vector<bool> found(nvar(),false);
    for (size_t i=0; i<_factors.size(); ++i) {
      if (_factors[i].nvar()==1) {
        size_t v=_vindex(*_factors[i].vars().begin());
        if (!found[v]) _vIndex[v]=i;
        found[v]=true;
      }
    }
    for (size_t v=0;v<found.size();++v) if (!found[v]) {
      // _vIndex[v]=
			addFactor( Factor(var(v),1.0) );
    }
		for (size_t i=0; i<_factors.size(); ++i) {
      if (!isVarNode(i))
				for (VarSet::const_iterator v=_factors[i].vars().begin();v!=_factors[i].vars().end();++v)
          addEdge(i,localFactor(*v));
		}
	}


	findex addFactor(const Factor& F) {
		findex use=graphModel::addFactor(F); 					// add the factor to the underlying collection
		_vIndex.resize(nvar(),vindex(-1));										// then update variable nodes and edges (!!!)
		if (F.nvar()==1 && _vIndex[*F.vars().begin()]==vindex(-1)) _vIndex[*F.vars().begin()]=use;
		else for(VarSet::const_iterator v=F.vars().begin();v!=F.vars().end();++v) {
			if (_vIndex[*v]==vindex(-1)) _vIndex[*v]=graphModel::addFactor(Factor(*v,1.0));
			addEdge(use,localFactor(*v));								// add edge to variable node
		}
		return use;
	}
	void removeFactor(findex f) {
		VarSet vs=_factors[f].vars();													// save the variables for clean-up
		graphModel::removeFactor(f);													// remove the factor from the collection
		for (VarSet::const_iterator v=vs.begin();v!=vs.end();++v) {
			removeEdge(f,localFactor(*v));								// remove the edge to the variable node
			if (localFactor(*v)==f) _vIndex[_vindex(*v)]=vindex(-1);	// removal of variable node => mark as missing (!!!)
		}
	}

	flist  _neighbors(findex i)    const { flist fl; for (mex::set<EdgeID>::const_iterator it=neighbors(i).begin();it!=neighbors(i).end();++it) fl+=it->second; return fl; }
	findex localFactor(vindex i)     const { return _vIndex[i]; }
	findex localFactor(Var v)        const { return _vIndex[_vindex(v)]; }
	bool   isVarNode(findex i)       const { return (factor(i).nvar()==1 && localFactor(*factor(i).vars().begin())==i); }
	flist  adjacentFactors(Var v)    const { return _neighbors(localFactor(v)); }
	flist  adjacentFactors(vindex v) const { return _neighbors(localFactor(v)); }
	VarSet adjacentVars(findex f)    const { return factor(f).vars(); }

	void swap(factorGraph& gm); 

#ifdef MEX  
  // MEX Class Wrapper Functions //////////////////////////////////////////////////////////
  bool        mxCheckValid(const mxArray*);   // check if matlab object is compatible with this object
  void        mxSet(mxArray*);     // associate with A by reference to data
  mxArray*    mxGet();             // get a pointer to the matlab object wrapper (creating if required)
  void        mxRelease();         // disassociate with a matlab object wrapper, if we have one
  void        mxDestroy();         // disassociate and delete matlab object
  void        mxSwap(factorGraph& gm);     // disassociate and delete matlab object
  /////////////////////////////////////////////////////////////////////////////////////////
#endif

protected:	// Functions
	eindex _eindex(findex i,findex j) { return edge(i,j).idx; }

	// Queue type wrappers for abstracting depth-1st, width-1st, etc. for tree search
	template<class T> class abstractQueue { public: virtual ~abstractQueue() { }; virtual void pop()=0; virtual void push(T)=0; virtual T first()=0; virtual bool empty()=0; };
	template<class T> class absQueueLIFO : public abstractQueue<T>, public std::stack<T> { public:
			void push(T t) { std::stack<T>::push(t); } void pop() { std::stack<T>::pop(); } T first() { return this->top(); } bool empty() { return std::stack<T>::empty(); } };
			//using std::stack<T>::push; using std::stack<T>::pop; T first() { return this->top(); } };
	template<class T> class absQueueFIFO : public abstractQueue<T>, public std::queue<T> { public:
			void push(T t) { std::queue<T>::push(t); } void pop() { std::queue<T>::pop(); } T first() { return this->front(); } bool empty() { return std::queue<T>::empty(); } };

public:

	// !!! re-write this better

	MEX_ENUM( TreeType , WidthFirst,DepthFirst,MaxWeight );

	mex::vector<Edge> spanTree(TreeType tt, Var root) {
		abstractQueue<findex> *Q; 
		switch (tt) {
			case TreeType::WidthFirst: Q=new absQueueFIFO<findex>(); break;
			case TreeType::DepthFirst: Q=new absQueueLIFO<findex>(); break;
			case TreeType::MaxWeight:  
			default:               throw std::runtime_error("Not implemented"); break;
		}
		Q->push(localFactor(root));
		vector<findex> used, fUsed; used.resize(nvar(),findex(-1)); fUsed.resize(nFactors(),0);
		mex::vector<Edge> tree; tree.reserve(2*nvar());
		while (!Q->empty()) {
			findex next = Q->first(); Q->pop();
			if (fUsed[next]!=0) continue;						// already taken this factor?
			uint32_t nFound=0, parent=0;						// if not look over variables and make sure
			const VarSet& vs=factor(next).vars();		//   at most one is already covered;
			for (VarSet::const_iterator v=vs.begin();v!=vs.end();++v) {
				if (used[_vindex(*v)] != vindex(-1)) { parent = used[_vindex(*v)]; nFound++; }
			}																				// if one is covered that's our parent; more
			if (nFound > 1) continue;								//   means this factor is unavailable so keep looking
			fUsed[next]=1;													// otherwise, mark factor, variables as covered
			if (isVarNode(next)) used[_vindex(*vs.begin())] = next;		// always cover by var node once visited
			else for (VarSet::const_iterator v=vs.begin();v!=vs.end();++v) {	
				if (used[_vindex(*v)]==vindex(-1)) used[_vindex(*v)]=next;			// otherwise remember which factor we are
			}
			// if we have a parent, add an edge to our trees
			if (nFound > 0) tree.push_back(Edge(parent,next)); //, _eindex(parent,next),_eindex(next,parent))); //!!!
			// if this is a root, do nothing (!!!); don't keep track of isolated nodes?

			const set<EdgeID> nbrs = neighbors(next);		// add all neighbors to the search queue
			for (set<EdgeID>::const_iterator n=nbrs.begin();n!=nbrs.end();++n) { Q->push(n->second); }
		}
		delete Q;
		return tree;
	}
	mex::vector<Edge> spanTree(TreeType tt=TreeType::WidthFirst) { return spanTree(tt, var(randi(nvar()))); }


protected:	// Contained objects
	//mex::mSparse<double>  _eIdx;			// Edge info: "edge" (i,j) -> matlab edge index [1..nEdges]
	//mex::mSparse<mex::midx<double> >  _eIdx;			// Edge info: "edge" (i,j) -> matlab edge index [1..nEdges]
	//mex::vector<mex::midx<uint32_t> > _eSrc, _eDst;	// edge index [0..nE-1] to src i, dst j
	//mex::stack<uint32_t,double> _eStack;// list of available edge slots

	//mex::vector<uint32_t> _vIndex;		// factor representing variable node in factorgraph
	mex::vector<mex::midx<uint32_t> > _vIndex;		// factor representing variable node in factorgraph

	// Algorithmic specialization data
};


#ifdef MEX
//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
bool factorGraph::mxCheckValid(const mxArray* GM) { 
	return graphModel::mxCheckValid(GM) && 										// we must be a graphModel
			   _vIndex.mxCheckValid( mxGetField(GM,0,"vIndex") );	// and have single-var indexing
	// !!! check that we have single-variable factors?
}

void factorGraph::mxSet(mxArray* M) {
	if (!mxCheckValid(M)) throw std::runtime_error("incompatible Matlab object type in factorGraph");

  graphModel::mxSet(M);															// initialize base components	
	_vIndex.mxSet(mxGetField(M,0,"vIndex"));

	// Check for algorithmic specialization???
}

mxArray* factorGraph::mxGet() {
	if (!mxAvail()) {
    mxArray* m;
		int retval = mexCallMATLAB(1,&m,0,NULL,"factorgraph");
		if (retval) mexErrMsgTxt("Error creating new factorgraph");
		factorGraph gm; gm.mxSet(m);
		gm = *this;
		gm.graphModel::mxGet();
    mxSwap(gm);
	}
	return M_;
}
void factorGraph::mxRelease() {							// shallow dissociation from matlab object (!!! ???)
	throw std::runtime_error("NOT IMPLEMENTED");
}
void factorGraph::mxDestroy() {
	throw std::runtime_error("NOT IMPLEMENTED");
}

void factorGraph::mxSwap(factorGraph& gm) {
	graphModel::mxSwap( (graphModel&)gm );
	_vIndex.mxSwap(gm._vIndex);
}
#endif		// ifdef MEX


//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
#endif  // re-include
