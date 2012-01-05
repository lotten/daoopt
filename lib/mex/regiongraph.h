#ifndef __MEX_REGIONGRAPH_H
#define __MEX_REGIONGRAPH_H

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
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

class regionGraph : public graphModel, virtual public mxObject {
public:
	typedef graphModel::findex        findex;				// factor index
	typedef graphModel::vindex        vindex;				// variable index
	typedef graphModel::flist				  flist; 				// collection of factor indices

public:
	regionGraph() : graphModel() { } 
	
	regionGraph(const regionGraph& rg) : graphModel((graphModel&)rg), { } 

	regionGraph(vector<Factor> fs);
	
	template <class InputIterator>
	factorGraph(InputIterator first, InputIterator last);

	virtual regionGraph* clone() const { regionGraph* rg = new regionGraph(*this); return rg; }

	// add maximal set (fixing up connectivity and counting numbers?)
	// remove maximal set (if it can be done???)
	//
	// add set (full control?)
	// add edge between sets
	//
	// check if counting numbers are valid for all variables (?)
	// check properties: valid, non-singular, maxent-consistent, etc?


	void swap(regionGraph& gm); 	// !!!

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

protected:	// Contained objects
	// connectivity information (parents, children, etc?)
	// list of maximal subsets?  list of descendents of each maximal subset?  list of parents of each set?

	// Algorithmic specialization data
};


#ifdef MEX
//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
bool factorGraph::mxCheckValid(const mxArray* GM) { 
	return graphModel::mxCheckValid(GM);			// !!!
}
void factorGraph::mxSet(mxArray* M) { throw std::runtime_error("NOT IMPLEMENTED"); }
mxArray* regionGraph::mxGet()       { throw std::runtime_error("NOT IMPLEMENTED"); }
void regionGraph::mxRelease()       { throw std::runtime_error("NOT IMPLEMENTED"); }
void regionGraph::mxDestroy()       { throw std::runtime_error("NOT IMPLEMENTED"); }
void regionGraph::mxSwap(regionGraph& gm) { throw std::runtime_error("NOT IMPLEMENTED"); }
#endif		// ifdef MEX


//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
#endif  // re-include
