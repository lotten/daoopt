/////////////////////////////////////////////////////////////////////////////////////
// Factor.h  --  class definition for matlab-compatible factor class
//
// A few functions are defined only for MEX calls (construction & load from matlab)
// Most others can be used more generally.
// 
//////////////////////////////////////////////////////////////////////////////////////

//#include <assert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <algorithm>
#include <numeric>
#include <float.h>
#include <vector>

#include "mxUtil.h"
#include "VarSet.h"
#include "subindex.h"
#include "index.h"
#include "vector.h"

#include "Factor.h"

namespace mex {


/************************************************************************************
 ************************************************************************************
 IMPLEMENTATION
 ************************************************************************************
************************************************************************************/

//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions
//////////////////////////////////////////////////////////////////////////////////////////////
#ifdef MEX
bool Factor::mxCheckValid(const mxArray* M) {
	return mxIsStruct(M) && t_.mxCheckValid(mxGetField(M,0,"t")) && v_.mxCheckValid(mxGetField(M,0,"v"));
}

void Factor::mxSet(mxArray* M) {
  if (mxAvail()) {
		//mxDestroyArray(M_);            //   destroy old matlab object if it existed
	} // !!! what to do about v_ and t_?
	M_=M;
	if (M) {														// if we got a matlab object, set to it
		t_.mxSetND(mxGetField(M_,0,"t"));
		v_.mxSet(mxGetField(M_,0,"v"), mxGetDimensions(t_.mxGet()));
	} else {														// null pointer => clear 
		*this = Factor();
	}
}

mxArray* Factor::mxGet() {
	if (!mxAvail()) {
		mxArray* m; 
		int retval = mexCallMATLAB(1,&m,0,NULL,"factor");           // create new empty factor object
		if (retval) mexErrMsgTxt("Error creating new factor");			//   associated with a matlab mxArray
		Factor f; f.mxSet(m);																				// use the copy constructor to put our data
		f = *this;  																								//   there (in matlab-allocated memory)
		mxSwap(f);																									// then swap everything for the new object
	}
  return M_;
};

void Factor::mxRelease() {                                 // Disassociate with a given matlab object
  if (mxAvail()) {                                         //   without deallocating / destroying it
		t_.mxRelease(); v_.mxRelease(); 
  }
}
void Factor::mxDestroy() {																	//  Disassociate with a given matlab object
	mxArray* m=M_;																						//    and also destroy it
	mxRelease();
	if (m) mxDestroyArray(m);
}
void Factor::mxSwap(Factor& f) {
  v_.mxSwap(f.v_);
	t_.mxSwap(f.t_);
	std::swap(M_,f.M_);
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////


vector<Factor> Factor::readUai10(std::istream& is) {
	size_t nvar, ncliques, csize, v, nval;
	char* st; st = new char[20];
	is >> st;
	if ( strcasecmp(st,"MARKOV") ) 
		throw std::runtime_error("Only UAI-2010 Markov-format files are supported currently");

  is >> nvar;
  vector<size_t> dims(nvar);
  for (size_t i=0;i<nvar;i++) is>>dims[i];
  
  is >> ncliques;
  //vector<vector<uint32_t> > cliques(ncliques);
	std::vector<std::vector<Var> > cliques(ncliques);
	std::vector<VarSet > sets(ncliques);
  for (size_t i=0;i<ncliques;i++) {
    is >> csize;
    cliques[i].reserve(csize);
    for (size_t j=0;j<csize;j++) { 
      is>>v; Var V(v,dims[v]);
      cliques[i].push_back(V); 
      sets[i] |= V;
    }   
  }
  
  //vector<vector<double> > tables(ncliques);
  vector<Factor> tables(ncliques);
  for (size_t i=0;i<ncliques;i++) {
    is >> nval;
    assert(nval == sets[i].nrStates());
    tables[i] = Factor(sets[i],0.0);
    Permute pi( cliques[i] , true);  // !! use DAI permutation routine, reverse order
    for (size_t j=0;j<nval;j++) is>>tables[i][pi.convertLinearIndex(j)];
  }

	delete[] st;
  return tables;
}



} // end namespace mex

