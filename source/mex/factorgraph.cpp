#if 0

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <queue>
#include <stack>

#include "factorgraph.h"

namespace mex {

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
  NOTIFY("factorGraph::mxSet\n");
	if (!mxCheckValid(M)) throw std::runtime_error("incompatible Matlab object type in factorGraph");

  graphModel::mxSet(M);															// initialize base components	
	_vIndex.mxSet(mxGetField(M,0,"vIndex"));

	// Check for algorithmic specialization???
}

mxArray* factorGraph::mxGet() {
  NOTIFY("factorGraph::mxGet\n");
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
  NOTIFY("graphModel::mxSwap\n");
	graphModel::mxSwap( (graphModel&)gm );
	_vIndex.mxSwap(gm._vIndex);
}
#endif		// ifdef MEX


//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex
#endif
