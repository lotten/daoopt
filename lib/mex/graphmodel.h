#ifndef __MEX_GRAPHMODEL_H
#define __MEX_GRAPHMODEL_H

#include <assert.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <cstring>
#include <map>

#include "mxObject.h"
#include "VarSet.h"
#include "Factor.h"
#include "graph.h"
#include "vector.h"
#include "set.h"
#include "map.h"
#include "stack.h"

/*
*/

namespace mex {

//template <class T> class idxset : public mex::set<mex::midx<T> > { };

// Graphical Model Base Class
// - Simplest form of a "graphical model" -- just a collection of variables and factors
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


// ToDo:
// Container aspect: contains 
//    vector<Factor> : insert(F), erase(i), compress (maximal sets only)
//    map<Var,set<fidx>> : find factors with var / varset
//    edges between factor "nodes" : add, remove, clear, neighborhood lists

// Functional operations
//    Orderings: variable (elimination orders) and node (spanning & covering trees, etc)
//    MRF adj:  map<Var, VarSet> , markovBlanket(v), cliques? 
//    VarDepUpdates:  change Var:FIdx map:  erase(&map,i,vs), insert(&map,i,vs)
//    Edge/adjacency:  edge(i,j), neighbors(i), 
//	  


class graphModel : public Graph, virtual public mxObject {
public:
#ifdef MEX
	friend void ::mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
#endif

	/// Useful typedefs
	typedef mex::midx<uint32_t>    					findex;		// factor index
	typedef mex::midx<uint32_t>    					vindex;		// variable index
	typedef mex::set<mex::midx<uint32_t> >  flist; 		// collection of factor indices

	/// Constructors, copy and assignment
	graphModel()                     : _factors(), _vAdj(), _dims() { };
	graphModel(const graphModel& gm) : Graph((Graph&)gm), _factors(gm._factors), _vAdj(gm._vAdj), _dims(gm._dims) { }
  graphModel& operator=(const graphModel& Obj);
	virtual graphModel* clone()        { graphModel* gm=new graphModel(*this); return gm; }
  graphModel(vector<Factor> fs)    : Graph(), _factors(fs), _vAdj(), _dims() { _fixup(); }
	template <class InputIterator>
	graphModel(InputIterator first, InputIterator last) : _factors(first,last), _vAdj(), _dims() { _fixup(); }


	/// Basic accessors	
	size_t        nvar()                      const { return _vAdj.size(); }      // # of variables 
	Var           var(vindex i)               const { return Var(i,_dims[i]); } 	// convert index to Variable object
	size_t        nFactors()         const { return _factors.size(); }   					// # of factors in the model
	const Factor& factor(findex idx) const { return _factors[idx];   }    				// accessor for factor number idx
	const vector<Factor>& factors()  const { return _factors;        }						//   and for factor container

	/// Basic variable-based manipulations
	const flist&  withVariable(const Var&  v) const { return _vAdj[_vindex(v)]; }	// factors depending on variable v
	flist         withVarSet(const VarSet& vs) const;															//   or on all of a set of variables
  flist         intersects(const VarSet& vs) const;
  flist         contains(const Var& v)       const { return withVariable(v); }
  flist         contains(const VarSet& vs)   const { return withVarSet(vs); }
  flist         containedBy(const VarSet& vs) const;
	VarSet        markovBlanket(const Var& v)  const;                              // variables that v may depend on
  VarSet        markovBlanket(const VarSet& vs) const;
	vector<VarSet> mrf() const;																										//   full variable-to-var adjacency

	/// Factor ("node") manipulation operations
	findex addFactor(const Factor& F);                              // add a factor to our collection
	void removeFactor(findex idx);                                  // remove a factor from the collection
	void clearFactors() { _factors.clear(); _vAdj.clear(); _vAdj.resize(nvar()); Graph::clear(); }	// remove all factors

	// check graphical model properties
	bool isbinary()   const	{ for (size_t i=0;i<_dims.size();++i) { if (_dims[i]>2) return false; } return true; }
	bool ispairwise() const { for (size_t i=0;i<nFactors();++i) { if (_factors[i].nvar()>2) return false; } return true; }

	/// Distribution-based operators
	Factor joint(size_t maxsize=0) const;                           // directly calculate joint distribution function
	void   consolidate(VarOrder ord=VarOrder());										// merge factors into maximal sets

	/// Ordering: variable (elimination) orders and factor orders
	size_t inducedWidth(const VarOrder&) const;          	// find induced width (complexity) of elimination order
  std::pair<size_t,size_t> pseudoTreeSize(const VarOrder&) const;	// find induced width & height of elim order
	vector<vindex> pseudoTree(const VarOrder&) const;     // find parents for given elimination order

	MEX_ENUM( OrderMethod , MinFill,WtMinFill,MinWidth,WtMinWidth,Random );

	VarOrder order(OrderMethod) const;											// find variable elimination order
	// orderings: return score; shortcut exit if worse than score !!!

	/// Helpful manipulation functions for other data
	void insert(vector<flist>& adj, findex i, const VarSet& vs);	// add node i, with vars vs, from adj list
	void erase(vector<flist>& adj, findex i, const VarSet& vs);		// erase node i, with vars vs, from adj list

protected:
	size_t  _vindex(const Var& v) const { return v.label();          } // look up variable's index
	flist&  _withVariable(const Var& v) { return _vAdj[_vindex(v)];  } // mutable accessor of adjacency
	void    _fixup();

	// Internal helper functions
	double orderScore(const vector<VarSet>&,size_t i,OrderMethod) const;
	VarOrder orderRandom() const;

public:
#ifdef MEX  
  // MEX Class Wrapper Functions //////////////////////////////////////////////////////////
  //void        mxInit();            // initialize any mex-related data structures, etc (private)
  //inline bool mxAvail() const;     // check for available matlab object
  bool        mxCheckValid(const mxArray*);   // check if matlab object is compatible with this object
  void        mxSet(mxArray*);     // associate with A by reference to data
  mxArray*    mxGet();             // get a pointer to the matlab object wrapper (creating if required)
  void        mxRelease();         // disassociate with a matlab object wrapper, if we have one
  void        mxDestroy();         // disassociate and delete matlab object
  void        mxSwap(graphModel& gm);     // disassociate and delete matlab object
  /////////////////////////////////////////////////////////////////////////////////////////
#endif

protected:
	vector<Factor>     _factors;  // collection of all factors in the model
private:
	//stack<uint32_t,double> vacant_;		// "blank" factors, used for inserting / deleting
	vector<flist>      _vAdj;			// variable adjacency lists (variables to factors)
	mex::vector<double> _dims;             // dimensions of variables as stored in graphModel object

	// Algorithmic specialization data
};

/************************************************************************************
 *
 *

graphModel& graphModel::operator=(const graphModel& Obj) {
	Graph::operator=((Graph&)Obj);			// copy graph elements over
	_factors = Obj._factors;						// copy list of factors
	_vAdj    = Obj._vAdj;								// variable dependencies
	_dims    = Obj._dims;								// and variable dimensions over
	//vacant_  = Obj.vacant_;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
//MEXFUNCTIONS_STRUCT( graphModel, factors, vAdj, vacant, vAdj, dims )

#ifdef MEX
bool graphModel::mxCheckValid(const mxArray* M) { 
	//if (!strcasecmp(mxGetClassName(M),"graphModel")) return false;
	// hard to check if we are derived from a graphmodel without just checking elements:
	return _factors.mxCheckValid( mxGetField(M,0,"factors") ) &&
			   //vacant_.mxCheckValid( mxGetField(M,0,"vacant"), mxGetField(M,0,"nVacant") ) &&
				 _vAdj.mxCheckValid( mxGetField(M,0,"vNbrs") ) &&
				 _dims.mxCheckValid( mxGetField(M,0,"dims") );
}

void graphModel::mxSet(mxArray* M) {
	if (!mxCheckValid(M)) throw std::runtime_error("incompatible Matlab object type in graphModel");
  M_ = M;
  _factors.mxSet(mxGetField(M,0,"factors"));         										// get factor list structure
	//vacant_.mxSet(mxGetField(M,0,"vacant") , mxGetField(M,0,"nVacant"));	// stack of vacant indices
  _vAdj.mxSet(mxGetField(M,0,"vNbrs"));              										// get adjacency structure
	_dims.mxSet(mxGetField(M,0,"dims"));																		// get vector of var dimensions

	// Check for algorithmic specialization???
}

mxArray* graphModel::mxGet() {
	if (!mxAvail()) {
    mxArray* m; int retval = mexCallMATLAB(1,&m,0,NULL,"graphmodel");
		if (retval) mexErrMsgTxt("Error creating new graphModel");
		graphModel Obj; Obj.mxSet(m);
		Obj = *this;
		Obj._factors.mxGet(); //Obj.vacant_.mxGet(); 
		Obj._vAdj.mxGet(); Obj._dims.mxGet();
    mxSwap(Obj);
	}
	return M_;
}
void graphModel::mxRelease() { throw std::runtime_error("Not implemented"); }
void graphModel::mxDestroy() { throw std::runtime_error("Not implemented"); }

void graphModel::mxSwap(graphModel& S) {
	_factors.mxSwap(S._factors);
	//vacant_.mxSwap(S.vacant_);
	_vAdj.mxSwap(S._vAdj);
	_dims.mxSwap(S._dims);
	std::swap(M_,S.M_);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// Accessors and mutators
//////////////////////////////////////////////////////////////////////////////////////////////
graphModel::graphModel() : _factors(), _vAdj(), _dims() { };

graphModel::graphModel(const graphModel& gm) : _factors(gm._factors), //vacant_(gm.vacant_), 
		_vAdj(gm._vAdj), _dims(gm._dims) { }

graphModel::graphModel(vector<Factor> fs) : _factors(fs), _vAdj(), _dims() {
	size_t nVar=0;
  for (vector<Factor>::iterator f=_factors.begin();f!=_factors.end();++f)
		nVar = std::max(nVar,f->vars().rend()->label());
	
	_vAdj.resize(nVar); _dims.resize(nVar);								// make space for variable inclusion mapping
  for (size_t f=0;f<_factors.size();++f) {								// for each factor,
		const VarSet& v = _factors[f].vars();									//   save the variables' dimensions and 
		for (VarSet::const_iterator i=v.begin();i!=v.end();++i) {		//   index this factor as including them
			_dims[_vindex(*i)] = i->states();										// check against current values???
			_withVariable(*i) |= f;
		}
	}
}

template <class InputIterator>
graphModel::graphModel(InputIterator first, InputIterator last) : _factors(), _vAdj(), _dims() {
	for (;first!=last;++first) addFactor(*first);						// do something more efficient but less general???
}

void graphModel::insert(vector<flist>& adj, findex idx, const VarSet& vs) {
  if (vs.nvar()>0 && adj.size() <= vs.rend()->label()) 	// if we need to, expand our set of variables
		adj.resize(vs.rend()->label()+1); 									//   to be large enough to be indexed by label
	for (size_t i=0;i<vs.nvar();++i) adj[vs[i]]|=idx;			//   and add factor to adj list
}

void graphModel::erase(vector<flist>& adj, findex idx, const VarSet& vs) {
	for (size_t i=0;i<vs.nvar();++i) adj[vs[i]]/=idx; 		//   remove a factor from each var's adj list
}

graphModel::findex graphModel::addFactor(const Factor& F) {
	const VarSet& v=F.vars();
	findex use = addNode();
	if (use>=nFactors()) _factors.push_back(F); else _factors[use]=F;
	insert(_vAdj,use,v);
	if (_dims.size()<_vAdj.size()) _dims.resize(_vAdj.size(),0);
	for (VarSet::const_iterator i=v.begin();i!=v.end();++i) { 		// look up dimensions if required
		if (_dims[_vindex(*i)]==0) _dims[_vindex(*i)]=i->states();		// add if we haven't seen this var
		else if (_dims[_vindex(*i)]!=i->states())										//   or check it against our current states
			throw std::runtime_error("Incompatible state dimension in added factor");
	}
	return use;                                             // return the factor index used
}

void graphModel::removeFactor(findex idx) {
	erase(_vAdj,idx,factor(idx).vars());													// remove from variable lists
	_factors[idx] = Factor();                                   		// empty its position
	removeNode(idx);																							// and remove the node
}


Factor graphModel::joint(size_t maxsize) const {
	if (maxsize) {
	  size_t D=1; for (size_t i=0;i<nvar();i++) D*=_dims[i];	// check for joint being "small" 
		if (D>maxsize) throw std::runtime_error("graphModel::joint too large");
  }
	Factor F;																								// brute force construction of joint table
	for (size_t i=0;i<nFactors();i++) F *= _factors[i];
	return F;
}

VarSet graphModel::markovBlanket(const Var& v) const {
	VarSet vs;
	const flist& nbrs = withVariable(v);
	for (flist::const_iterator f=nbrs.begin(); f!=nbrs.end(); ++f) vs |= factor(*f).vars();
	vs/=v;
	return vs;
}
vector<VarSet> graphModel::mrf() const { 
	vector<VarSet> vvs; 
	for (size_t v=0;v<nvar();++v) vvs.push_back(markovBlanket(var(v))); 
	return vvs;
}
graphModel::flist graphModel::withVarSet(const VarSet& vs) const {
	flist fs = withVariable(vs[0]); 
	for (size_t v=1;v<vs.size();v++) fs&=withVariable(vs[v]);
	return fs;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Elimination orders 
//////////////////////////////////////////////////////////////////////////////////////////////

// 
// inducedWidth: Compute the induced width of some elimination order
//
size_t graphModel::inducedWidth(const VarOrder& order) const { 
	// Could replace with return pseudoTreeSize(order).first;
	size_t width=0;
  vector<VarSet> adj = mrf();

	// Eliminate in the given order of labels, tracking adjacency
	for (VarOrder::const_iterator i=order.begin(); i!=order.end(); ++i) {
		//std::cout<<"Var "<<*i<< " : "<<adj[_vindex(var(*i))]<<"\n";
		VarSet vi = adj[_vindex(var(*i))];
    for (VarSet::const_iterator j=vi.begin(); j!=vi.end(); ++j) {
			adj[ _vindex(*j) ] |= vi;
			adj[ _vindex(*j) ] -= VarSet(var(*i),*j); 
			width = std::max(width,adj[_vindex(*j)].nvar());
		}
	}
	return width;
}


std::pair<size_t,size_t> graphModel::pseudoTreeSize(const VarOrder& order) const {
	size_t width=0;
	vector<size_t> heights; heights.resize(nvar(),0);
  vector<VarSet> adj = mrf();

	// Eliminate in the given order of labels, tracking adjacency
	for (VarOrder::const_iterator i=order.begin(); i!=order.end(); ++i) {
		//std::cout<<"Var "<<*i<< " : "<<adj[_vindex(var(*i))]<<"\n";
		VarSet vi = adj[_vindex(var(*i))];
    for (VarSet::const_iterator j=vi.begin(); j!=vi.end(); ++j) {
			adj[ _vindex(*j) ] |= vi;
			adj[ _vindex(*j) ] -= VarSet(var(*i),*j); 
			heights[ _vindex(*j) ] = std::max( heights[_vindex(*j)], heights[_vindex(var(*i))]+1 );
			width = std::max(width,adj[_vindex(*j)].nvar());
		}
	}
	return std::make_pair(width,heights[_vindex(var(*order.rend()))]);
}


// 
// orderRandom() : Return a randomly selected elimination ordering
//
VarOrder graphModel::orderRandom() const {
  VarOrder order; order.resize(nvar());
	for (vindex i=0;i<nvar();i++) order[i]=var(i).label();		// build a list of all the variables
	std::random_shuffle( order.begin(),order.end() );					// and randomly permute them
  return order;
}

double graphModel::orderScore(const vector<VarSet>& adj, size_t i, OrderType kOType) const {
	double s=0.0;
  switch (kOType) {
	case MIN_FILL:
		for (VarSet::const_iterator j=adj[i].begin();j!=adj[i].end();++j)
			s += (adj[i] - adj[_vindex(*j)]).size();
		break;
	case MIN_WFILL:
		for (VarSet::const_iterator j=adj[i].begin();j!=adj[i].end();++j)
			s += (adj[i] - adj[*j]).nrStates();
		break;
	case MIN_WIDTH:
		s = adj[i].size();
		break;
	case MIN_WWIDTH:
		s = adj[i].nrStates();
		break;
	default: throw std::runtime_error("Unknown elimination ordering type"); break;
	}
	return s;
}

// order : variable elimination orders, mostly greedy score-based
VarOrder graphModel::order(OrderType kOType) const {
	if (kOType==RANDOM) return orderRandom();

  VarOrder order; order.resize(nvar());
	vector<VarSet> adj = mrf();
  typedef std::pair<double,size_t> NN;
	typedef std::multimap<double,size_t> sMap;  
  sMap scores;
  std::vector<sMap::iterator > reverse(nvar());

	for (size_t v=0;v<nvar();v++) 								// get initial scores
		reverse[v]=scores.insert( NN( orderScore(adj,v,kOType),v) );

  for (int ii=0;ii<nvar();++ii) {								// Iterate through, selecting variables
    sMap::iterator first = scores.begin();			// Choose a random entry from among the smallest
		sMap::iterator last = scores.upper_bound(first->first);	
		std::advance(first, randi(std::distance(first,last)));
		size_t i = first->second;

    order[ii] = var(i).label();  	              // save its label in the ordering
		scores.erase(reverse[i]);									  // remove it from our list
    VarSet vi = adj[i];   					      			// go through adjacent variables (copy: adj may change)
		VarSet fix;																	//   and keep track of which need updating
		for (VarSet::const_iterator j=vi.begin(); j!=vi.end(); ++j) {
			size_t v = _vindex(*j);
      adj[v] |= vi;             // and update their adjacency structures
      adj[v] /= var(i);
			//fix |= v;								// width methods only need v, not nbrs !!
			fix |= adj[v];						// come back and recalculate their scores
		}
		for (VarSet::const_iterator j=fix.begin();j!=fix.end();++j) {
			size_t jj = j->label();
			scores.erase(reverse[jj]);	// remove and update (score,index) pairs
    	reverse[jj] = scores.insert(NN(orderScore(adj,jj,kOType),jj)); 
		}
  }
  return order;
}


************************************************************************************/

//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
#endif  // re-include
