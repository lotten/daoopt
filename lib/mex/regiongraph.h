// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_REGIONGRAPH_H
#define __MEX_REGIONGRAPH_H

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <queue>
#include <stack>

#include "graphmodel.h"

/*
*/

namespace mex {

// Region Graph Base Class
// - list of factors (one for each region)
// - 
// 
// Use representation with larger / smaller sets?  need actual factors or just vars?
//
// Need (1) original factors (for updating?)   
//      (2) beliefs over maximal sets, maybe others also
//      (3) connectivity and counting numbers for subsets

class regionGraph : virtual public mxObject {
public:
  typedef graphModel::findex        findex;        // factor index
  typedef graphModel::vindex        vindex;        // variable index
  typedef graphModel::flist          flist;         // collection of factor indices

protected:

  // Simple ordering object for sorting by size (for containment partial order)
  struct regionCompare { 
    bool operator() (const VarSet& lhs, const VarSet& rhs) const {
      if (lhs.size() < rhs.size()) return true;
      else if (lhs.size() > rhs.size()) return false;
      else return lhs < rhs;
    }
  };

  // Simple ordering object for sorting region indices (for containment partial order)
  struct findexCompare { 
    const vector<VarSet>& _R;
    findexCompare(const vector<VarSet>& R) : _R(R) { } 
    bool operator()(findex i,findex j) { return regionCompare()(_R[i],_R[j]); } 
  };

  // Contained data:
  flist   _maximalRegions;                 // maximal regions (not heavily used ???)
  vector<flist>      _vAdj;                // variable-based region dependence
  std::map<VarSet,findex,regionCompare> _regionIndex; // region lookup (VarSet -> index)
  vector<VarSet>     _regions;             // list of regions  (index -> VarSet)
  vector<double>     _count;               // counting numbers
  vector<flist>      _children;            // not really used (???)
  vector<flist>      _parents;             //  "" ""  (???)

public:
  regionGraph() { } 
  regionGraph(const vector<Factor>& fs) {      // build region graph from list of factors
    for (size_t f=0;f<fs.size();++f) {
      if (fs[f].nvar() > 0) addRegion(fs[f].vars() , false);  // don't update counts yet
    }
    calculateCount();                    // update counting numbers for the regions
    //dump();
  }
  regionGraph(const vector<VarSet>& vs) {      // build region graph from list of regions
    for (size_t v=0;v<vs.size();++v) {
      if (vs[v].nvar() > 0) addRegion(vs[v] , false);   // don't update counts yet
    }
    calculateCount();                    // update counting numbers for the regions
  }
  
  // virtual regionGraph* clone() const { regionGraph* rg = new regionGraph(*this); return rg; }

  size_t        nvar()           const { return _vAdj.size(); }
  size_t        nregions()       const { return _regions.size(); }
  const flist&  maximalRegions() const { return _maximalRegions; }
  const VarSet& region(findex r) const { return _regions[r]; }

  // similar to graphModel dependency finding:
  //  contains(VarSet)   : all regions that are supersets (ancestors)
  //  intersects(VarSet) : all regions that contain overlap (potential nbrs or descendants)
  //  containedBy(VarSet): all regions that are subsets (descendants)
  const flist& contains(const Var& v) const { assert(v.label()<_vAdj.size()); return _vAdj[v.label()]; }
  flist contains(const VarSet& vs) const {
    flist fs = contains(vs[0]);
    for (size_t v=1;v<vs.size();v++) fs&=contains(vs[v]);
    return fs;
  }
  flist intersects(const VarSet& vs) const {
    flist fs;
    for (size_t v=0;v<vs.size();v++) if (vs[v].label()<nvar()) fs|=contains(vs[v]);
    return fs;
  }
  flist containedBy(const VarSet& vs) const {
    flist fs2, fs = intersects(vs);
    for (size_t f=0;f<fs.size();f++) if (_regions[fs[f]] << vs) fs2|=fs[f];
    return fs2;
  }

  // Sort a list of regions by a set-membership partial order (in practice, by size)
  vector<findex> sort(flist fs) {
    vector<findex> ret(fs.begin(),fs.end());
    std::sort(ret.begin(),ret.end(),findexCompare(_regions));
    return ret;
  }

  const flist& parents(findex r)  const { return _parents[r]; }
  const flist& children(findex r) const { return _children[r]; }

  // Recalculate counting numbers for all regions
  void calculateCount() {
    // walk through regions in order of decreasing size
    for (std::map<VarSet,findex>::reverse_iterator it=_regionIndex.rbegin();it!=_regionIndex.rend();++it) {
      findex r=it->second;
      _count[r]=1.0;
      flist ancestors = contains(_regions[r]); 
      for (flist::const_iterator p=ancestors.begin(); p!=ancestors.end(); ++p) _count[r]-=_count[*p];
    }
  }

  // Recalculate counting numbers at region r and below
  void calculateCount(findex r) {
    _count[r]=1.0;
    flist ancestors = contains(_regions[r]);      // Compute r's counting # from its ancestors
    for (flist::const_iterator p=ancestors.begin(); p!=ancestors.end(); ++p) _count[r]-=_count[*p];
    flist descend = containedBy(_regions[r]);     // Now update all of r's descendants:
    for (flist::const_iterator c=descend.begin(); c!=descend.end(); ++c) {
      flist ancestors = contains(_regions[*c]);   //   find their ancestors and recompute
      for (flist::const_iterator p=ancestors.begin(); p!=ancestors.end(); ++p) _count[*c]-=_count[*p];
    }
  }

  // Insert a new region (but not any extra regions besides that one)
  void insertRegion(const VarSet& vs) {
    if (_regionIndex.count(vs)) return;          // already in our region graph?
    findex idx = _regions.size();               // otherwise add it to our lists
    _regionIndex[vs] = idx;
    _regions.push_back(vs); _children.push_back(flist()); _parents.push_back(flist()); 
    _count.push_back(0.0); 
    graphModel gm; gm.insert(_vAdj, idx, vs);   // update variable adjacency
    //graphModel::insert(_vAdj, idx, vs);         // update variable adjacency   !!! better

    flist ms = _maximalRegions & contains(vs);
    if (ms.size()==0) {                        // if it's a new maximal region
      _maximalRegions += idx;                  //   find out if it subsumes any others
      ms = _maximalRegions & containedBy(vs);  //   and if so push them down
      for (size_t r=0; r<ms.size(); ++r) pushDown(idx,ms[r]);
    } else {                                   // if it's non-maximal, push it down from the top
      for (size_t r=0; r<ms.size(); ++r) pushDown(ms[r],idx);
    }
  }

  // Find where region r goes, somewhere below region p   (not useful???)
  void pushDown(findex p, findex r) {          // push down region r from parent region p
    if (p==r) return;
    bool done = false;
    for (flist::const_iterator c=_children[p].begin();c!=_children[p].end();++c) {
      if (_regions[*c] >> _regions[r]) {        // if it goes below c, push down further
        pushDown(*c,r); done=true;
      } else if (_regions[*c] << _regions[r]) { // if it goes between p and c, insert it
        _children[r] += *c; _parents[r] += p; _children[p] -= *c; _children[p] += r; done=true;
      }
    }
    if (!done) {                               // if we didn't find a home yet, it's a new child of p
      _children[p]+=r; _parents[r]+=p;
    }
  }

  std::ostream& flistOut(std::ostream& os, flist const& L) {  os<<"{"; for (size_t i=0;i<L.size();++i) os<<L[i]<<","; os<<"}"; return os; }

  void dump() {
    // walk through regions in order, or in order of decreasing size
    //for (size_t r=0;r<_regions.size();++r) {
    for (std::map<VarSet,findex>::reverse_iterator it=_regionIndex.rbegin();it!=_regionIndex.rend();++it) {
      findex r = it->second;
      std::cout<<"Region "<<r<<"="<<_regions[r]; //<<": P="<<_parents[r]<<", C="<<_children[r]<<", n="<<_count[r]<<"\n";
      std::cout<<": P="; flistOut(std::cout,_parents[r]); std::cout<<", C="; flistOut(std::cout,_children[r]);
      std::cout<<", n="<<_count[r]<<"\n";
    }
  }

  // Insert a new region, and any regions formed by intersections with it
  void addRegion(const VarSet& vs, bool updateCount=true) {
    flist nbrs = intersects(vs);      // find all the regions we could intersect with
    insertRegion(vs);                 // insert vs itself, then any intersections with existing regions
    for (flist::const_iterator it=nbrs.begin();it!=nbrs.end();++it) insertRegion( vs & _regions[*it] );
    if (updateCount) calculateCount(_regionIndex[vs]); // recalculate counting numbers from vs downward
  }

  // check if counting numbers are valid for all variables (?)
  // check properties: valid, non-singular, maxent-consistent, etc?


  void swap(regionGraph& gm);   // !!!

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

protected:  // Contained objects
  // connectivity information (parents, children, etc?)
  // list of maximal subsets?  list of descendents of each maximal subset?  list of parents of each set?

  // Algorithmic specialization data
};


#ifdef MEX
//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
bool factorGraph::mxCheckValid(const mxArray* GM) { 
  return graphModel::mxCheckValid(GM);      // !!!
}
void factorGraph::mxSet(mxArray* M) { throw std::runtime_error("NOT IMPLEMENTED"); }
mxArray* regionGraph::mxGet()       { throw std::runtime_error("NOT IMPLEMENTED"); }
void regionGraph::mxRelease()       { throw std::runtime_error("NOT IMPLEMENTED"); }
void regionGraph::mxDestroy()       { throw std::runtime_error("NOT IMPLEMENTED"); }
void regionGraph::mxSwap(regionGraph& gm) { throw std::runtime_error("NOT IMPLEMENTED"); }
#endif    // ifdef MEX


//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
#endif  // re-include
