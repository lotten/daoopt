// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#include <assert.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <cstring>
#include <map>

#include "mex/graphmodel.h"

/*
*/

namespace mex {

const mex::EdgeID mex::EdgeID::NO_EDGE(-1,-1,0,0);


graphModel& graphModel::operator=(const graphModel& Obj) {
	Graph::operator=((Graph&)Obj);			// copy graph elements over
	_factors = Obj._factors;						// copy list of factors
	_vAdj    = Obj._vAdj;								// variable dependencies
	_dims    = Obj._dims;								// and variable dimensions over
	//vacant_  = Obj.vacant_;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
//MEXFUNCTIONS_STRUCT( graphModel, factors, vAdj, vacant, vAdj, dims )

#ifdef MEX
bool graphModel::mxCheckValid(const mxArray* M) { 
	//if (!strcasecmp(mxGetClassName(M),"graphModel")) return false;
	// hard to check if we are derived from a graphmodel without just checking elements:
	return Graph::mxCheckValid(M) && 
			   _factors.mxCheckValid( mxGetField(M,0,"factors") ) &&
				 _vAdj.mxCheckValid( mxGetField(M,0,"vNbrs") ) &&
				 _dims.mxCheckValid( mxGetField(M,0,"dims") );
}

void graphModel::mxSet(mxArray* M) {
	if (!mxCheckValid(M)) throw std::runtime_error("incompatible Matlab object type in graphModel");
	Graph::mxSet(M);
  M_ = M;
  _factors.mxSet(mxGetField(M,0,"factors"));         										// get factor list structure
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
/*
graphModel::graphModel() : _factors(), _vAdj(), _dims() { };

graphModel::graphModel(const graphModel& gm) : _factors(gm._factors), //vacant_(gm.vacant_), 
		_vAdj(gm._vAdj), _dims(gm._dims) { }

graphModel::graphModel(vector<Factor> fs) : _factors(fs), _vAdj(), _dims() { _fixup() }

template <class InputIterator>
graphModel::graphModel(InputIterator first, InputIterator last) : _factors(first,last), _vAdj(), _dims() {
	_fixup();
	//for (;first!=last;++first) addFactor(*first);						// do something more efficient but less general???
}
*/

void graphModel::_fixup() {
	size_t nVar=0;
  for (vector<Factor>::iterator f=_factors.begin();f!=_factors.end();++f) {
		if (f->nvar()) nVar = std::max(nVar,f->vars().rbegin()->label()+1);
	}
	
	_vAdj.resize(nVar); _dims.resize(nVar);								// make space for variable inclusion mapping
  for (size_t f=0;f<_factors.size();++f) {								// for each factor,
		findex use = addNode();
		assert( use == f );
		const VarSet& v = _factors[f].vars();									//   save the variables' dimensions and 
		for (VarSet::const_iterator i=v.begin();i!=v.end();++i) {		//   index this factor as including them
			_dims[_vindex(*i)] = i->states();										// check against current values???
			_withVariable(*i) |= f;
		}
	}
}


void graphModel::insert(vector<flist>& adj, findex idx, const VarSet& vs) {
  if (vs.nvar()>0 && adj.size() <= vs.rbegin()->label()) 	// if we need to, expand our set of variables
		adj.resize(vs.rbegin()->label()+1); 									//   to be large enough to be indexed by label
	for (size_t i=0;i<vs.nvar();++i) adj[vs[i]]|=idx;			//   and add factor to adj list
}

void graphModel::erase(vector<flist>& adj, findex idx, const VarSet& vs) {
	for (size_t i=0;i<vs.nvar();++i) adj[vs[i]]/=idx; 		//   remove a factor from each var's adj list
}

graphModel::findex graphModel::addFactor(const Factor& F) {
	const VarSet& v=F.vars();
	findex use = addNode();
  //if (use>=nFactors()) _factors.push_back(F); else _factors[use]=F;
  if (use>=nFactors()) {
    if (_factors.capacity()>nFactors()) _factors.push_back(F);
    else {                                               // if we'd need to copy, do it manually
      vector<Factor> tmp; tmp.reserve(2*nFactors()); tmp.resize(nFactors()+1);
      for (size_t i=0;i<_factors.size();++i) tmp[i].swap(_factors[i]); tmp[nFactors()]=F;
      _factors.swap(tmp);
    }
  } else _factors[use]=F;

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

graphModel::findex graphModel::smallest(const flist& fl) {
  assert(fl.size() > 0);
  findex ret = *fl.begin();
  for (flist::const_iterator f=fl.begin(); f!=fl.end(); ++f) 
    if (factor(ret).nvar() > factor(*f).nvar()) ret=*f;
  return ret;
}

graphModel::findex graphModel::largest(const flist& fl) {
  assert(fl.size() > 0);
  findex ret = *fl.begin();
  for (flist::const_iterator f=fl.begin(); f!=fl.end(); ++f) 
    if (factor(ret).nvar() < factor(*f).nvar()) ret=*f;
  return ret;
}

vector<uint32_t> graphModel::maxSimple( ) {
  vector<uint32_t> vals( nvar() );
  for (size_t v=0;v<nvar();++v) if (withVariable(var(v)).size()) {
    vals[v] = factor( smallest(withVariable(var(v))) ).argmax();
  }
  return vals;
}

vector<uint32_t> graphModel::maxSequential( const VarOrder& order ) {
  vector<uint32_t> vals( nvar() );
  VarSet done;
  for (VarOrder::const_reverse_iterator v=order.rbegin();v!=order.rend();++v) {
    Var V=var(*v);
    const flist& use = withVariable(V);
    Factor mm(V, 1.0);
    for (flist::const_iterator f=use.begin();f!=use.end();++f) {
      size_t condi = 0;
      VarSet condv = factor(*f).vars() & done;
      if (condv.size()) condi = sub2ind(condv,vals);
      mm*=factor(*f).condition(condv,condi).maxmarginal(V);
    }
    vals[*v] = mm.argmax();
    done += V;
  }
  return vals;
} 
    


// joint(maxSize) : return a brute force joint probability table (throw exception if > maxSize)
Factor graphModel::joint(size_t maxsize) const {
	if (maxsize) {
	  size_t D=1; 
    for (size_t i=0;i<nvar();i++) {      // check for joint being "small" 
      D *= (_dims[i] ? _dims[i] : 1);	
		  if (D>maxsize) throw std::runtime_error("graphModel::joint too large");
    }
  }
	Factor F;																								// brute force construction of joint table
	for (size_t i=0;i<nFactors();i++) F *= _factors[i];
	return F;
}

//
// markovBlanket(Var), markovBlanket(VarSet): find variables to render Var(Set) conditionally independent
//
VarSet graphModel::markovBlanket(const Var& v) const {
	VarSet vs;
	const flist& nbrs = withVariable(v);
	for (flist::const_iterator f=nbrs.begin(); f!=nbrs.end(); ++f) vs |= factor(*f).vars();
	vs/=v;
	return vs;
}
VarSet graphModel::markovBlanket(const VarSet& vs) const {
	VarSet ret = markovBlanket(vs[0]); 
	for (size_t v=1;v<vs.size();v++) ret|=markovBlanket(vs[v]);
	return ret;
}

// mrf() : obtain a Markov random field representation (Var->Var adjacency) of the collection of factors
vector<VarSet> graphModel::mrf() const { 
	vector<VarSet> vvs; 
	for (size_t v=0;v<nvar();++v) vvs.push_back(markovBlanket(var(v))); 
	return vvs;
}

// contains(VarSet), intersects(VarSet), containedBy(VarSet) : 
//   find all factors that contain, are contained by, or intersect with a set of variables
graphModel::flist graphModel::withVarSet(const VarSet& vs) const {
	flist fs = withVariable(vs[0]); 
	for (size_t v=1;v<vs.size();v++) fs&=withVariable(vs[v]);
	return fs;
}
graphModel::flist graphModel::intersects(const VarSet& vs) const {
	flist fs = withVariable(vs[0]); 
	for (size_t v=1;v<vs.size();v++) fs|=withVariable(vs[v]);
	return fs;
}
graphModel::flist graphModel::containedBy(const VarSet& vs) const {
	flist fs2, fs = intersects(vs);
  for (size_t f=0;f<fs.size();f++) if (_factors[fs[f]].vars() << vs) fs2|=fs[f];
	return fs2;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Elimination orders 
//////////////////////////////////////////////////////////////////////////////////////////////

// 
// inducedWidth: Compute the induced width of some elimination order
//
size_t graphModel::inducedWidth(const VarOrder& order) const { 
	return pseudoTreeSize(order).first;
/*
	// Could replace with return pseudoTreeSize(order).first;
	size_t width=0;
  vector<VarSet> adj = mrf();

	// Eliminate in the given order of labels, tracking adjacency
	for (VarOrder::const_iterator i=order.begin(); i!=order.end(); ++i) {
		if (*i<0 || *i>=nvar()) continue;
		//std::cout<<"Var "<<*i<< " : "<<adj[_vindex(var(*i))]<<"\n";
		VarSet vi = adj[_vindex(var(*i))];
    for (VarSet::const_iterator j=vi.begin(); j!=vi.end(); ++j) {
			adj[ _vindex(*j) ] |= vi;
			adj[ _vindex(*j) ] -= VarSet(var(*i),*j); 
			width = std::max(width,adj[_vindex(*j)].nvar());
		}
	}
	return width;
*/
}

std::pair<size_t,size_t> graphModel::pseudoTreeSize(const VarOrder& order) const {
	size_t width=0, maxHeight=0;
	vector<size_t> heights; heights.resize(nvar(),0);
  vector<VarSet> adj = mrf();

	// Eliminate in the given order of labels, tracking adjacency
	for (VarOrder::const_iterator i=order.begin(); i!=order.end(); ++i) {
		if (*i<0 || *i>=nvar()) continue;
		VarSet vi = adj[_vindex(var(*i))];
    for (VarSet::const_iterator j=vi.begin(); j!=vi.end(); ++j) {
			adj[ _vindex(*j) ] |= vi;
			adj[ _vindex(*j) ] -= VarSet(var(*i),*j); 
			heights[ _vindex(*j) ] = std::max( heights[_vindex(*j)], heights[_vindex(var(*i))]+1 );
			maxHeight = std::max(maxHeight,heights[_vindex(*j)]);
			width = std::max(width,adj[_vindex(*j)].nvar());
		}
	}
	return std::make_pair(width, maxHeight);
}


vector<graphModel::vindex> graphModel::pseudoTree(const VarOrder& order) const {
	vector<vindex> parents(nvar(),-1);
  vector<VarSet> adj = mrf();

	// Eliminate in the given order of labels, tracking adjacency
	for (VarOrder::const_iterator i=order.begin(); i!=order.end(); ++i) {
		if (*i<0 || *i>=nvar()) continue;
		VarSet vi = adj[_vindex(var(*i))];
    for (VarSet::const_iterator j=vi.begin(); j!=vi.end(); ++j) {
			adj[ _vindex(*j)] |= vi;
			adj[ _vindex(*j) ] -= VarSet(var(*i),*j); 
		}

		for (VarOrder::const_iterator k=i; k!=order.end();++k) {
      if (vi.contains(var(*k))) { parents[*i]=*k; break; }
		}
	}
	return parents;
}


// 
// orderRandom() : Return a randomly selected elimination ordering
//
VarOrder graphModel::orderRandom() const {
  VarOrder order; order.resize(nvar());
	for (size_t i=0;i<nvar();i++) order[i]=var(i).label();		// build a list of all the variables
	std::random_shuffle( order.begin(),order.end() );					// and randomly permute them
  return order;
}

double graphModel::orderScore(const vector<VarSet>& adj, size_t i, OrderMethod kOType) const {
	double s=0.0;
  switch (kOType) {
	case OrderMethod::MinFill:
		for (VarSet::const_iterator j=adj[i].begin();j!=adj[i].end();++j)
			s += (adj[i] - adj[_vindex(*j)]).size();
		break;
	case OrderMethod::WtMinFill:
		for (VarSet::const_iterator j=adj[i].begin();j!=adj[i].end();++j)
			s += (adj[i] - adj[*j]).nrStates();
		break;
	case OrderMethod::MinWidth:
		s = adj[i].size();
		break;
	case OrderMethod::WtMinWidth:
		s = adj[i].nrStates();
		break;
	default: throw std::runtime_error("Unknown elimination ordering type"); break;
	}
	return s;
}

// order : variable elimination orders, mostly greedy score-based
VarOrder graphModel::order(OrderMethod kOType) const {
  VarOrder order; order.resize(nvar());

	if (kOType==OrderMethod::Random) {											// random orders are treated here
		for (size_t i=0;i<nvar();i++) order[i]=var(i).label();//   build a list of all the variables
		std::random_shuffle( order.begin(),order.end() );			//   and randomly permute them
  	return order;																					//   then return
	}

	vector<VarSet> adj = mrf();
  //vector<set<Var> > adj(adj1.size());
  //for (size_t i=0;i<adj1.size();++i) adj[i] = set<Var>(adj1[i].begin(),adj1[i].end());

  typedef std::pair<double,size_t> NN;
	typedef std::multimap<double,size_t> sMap;  
  sMap scores;
  std::vector<sMap::iterator > reverse(nvar());

	for (size_t v=0;v<nvar();v++) 								// get initial scores
		reverse[v]=scores.insert( NN( orderScore(adj,v,kOType),v) );

  for (size_t ii=0;ii<nvar();++ii) {								// Iterate through, selecting variables
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
      if (fix.size()<scores.size()) {
			  if (kOType==OrderMethod::MinWidth || kOType==OrderMethod::WtMinWidth) 
				   fix |= adj[v]; //var(v);				// (width methods only need v, not nbrs)
			  else fix |= adj[v];				// come back and recalculate their scores
      }
		}
		for (VarSet::const_iterator j=fix.begin();j!=fix.end();++j) {
			size_t jj = j->label();
			scores.erase(reverse[jj]);	// remove and update (score,index) pairs
    	reverse[jj] = scores.insert(NN(orderScore(adj,jj,kOType),jj)); 
		}
  }
  return order;
}

// order : variable elimination orders, mostly greedy score-based
void graphModel::improveOrder(OrderMethod kOType, double& oldScore, VarOrder& oldOrder) const {
  VarOrder order; order.resize(nvar());

	if (kOType==OrderMethod::Random) {											// random orders are treated here
		for (size_t i=0;i<nvar();i++) order[i]=var(i).label();//   build a list of all the variables
		std::random_shuffle( order.begin(),order.end() );			//   and randomly permute them
    // !!! what scoring mechanism to use?
    //std::pair<size_t,size_t> newsize = pseudoTreeSize(order);
    //if (newsize.first < width || (newsize.first == width && newsize.second < height)) {
    //  width=newsize.first; height=newsize.second; oldOrder=order;
    //}
    oldOrder = order;
  	return;
	}

	vector<VarSet> adj = mrf();
  typedef std::pair<double,size_t> NN;
	typedef std::multimap<double,size_t> sMap;  
  sMap scores;
  std::vector<sMap::iterator > reverse(nvar());
  double newScore;

	for (size_t v=0;v<nvar();v++) 								// get initial scores
		reverse[v]=scores.insert( NN( orderScore(adj,v,kOType),v) );

  for (size_t ii=0;ii<nvar();++ii) {						// Iterate through, selecting variables
    sMap::iterator first = scores.begin();			// Choose a random entry from among the smallest
    newScore = std::max(first->first, newScore);// keep track of largest score for this ordering
    if (newScore > oldScore) return;            //   & if worse than cutoff value, just quit
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
			if (kOType==OrderMethod::MinWidth || kOType==OrderMethod::WtMinWidth) 
				   fix |= adj[v]; //var(v);				// (width methods only need v, not nbrs)
			else fix |= adj[v];				// come back and recalculate their scores
		}
		for (VarSet::const_iterator j=fix.begin();j!=fix.end();++j) {
			size_t jj = j->label();
			scores.erase(reverse[jj]);	// remove and update (score,index) pairs
    	reverse[jj] = scores.insert(NN(orderScore(adj,jj,kOType),jj)); 
		}
  }
  // !!! check for tiebreaker?
  oldOrder = order;
  return;
}

//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
