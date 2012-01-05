#ifndef __MEX_STACK_H
#define __MEX_STACK_H

#include <assert.h>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <stdint.h>

#include "mxObject.h"
#include "vector.h"
#ifdef MEX
#include "matrix.h"
#else 
#include <stack>
#endif

/*
 * mex::stack<T,Tn> : templated class for wrapping matlab vectors
 * The matlab "object" is a vector of type T, and a scalar of type Tn, typically modified by reference
 * (If you don't want by reference, there is no reason not to use mex::vector<T>)
*/

namespace mex {

#ifndef MEX
template <class T, class Tn>
class stack : public std::stack<T>, public virtual mxObject { 
	virtual bool mxCheckValid(const mxArray* A,const mxArray* N=NULL) { return false; }
	virtual void mxSet(mxArray* A, mxArray* N=NULL) { }
	virtual void mxCopy(const mxArray* A, const mxArray* N=NULL) { }
	void mxSwap(stack<T,Tn>& S) { this->swap(S); }
};

#else
template <class T,class Tn>
class stack : private vector<T>, public virtual mxObject {
 public:
   // Constructors  (missing a few from std::vector) ////////////////
   stack(size_t n=0) : NN_(NULL), vector<T>(n) { this->N_=0; vector<T>::mxUpdateN(); };
   //stack(size_t n, const T& t) : vector<T>(n,t) { this->N_=0; mxUpdateN(); };
   stack(const stack<T,Tn>& v) : NN_(NULL), vector<T>(v) { };

	 bool     empty()      { return vector<T>::empty(); }
	 size_t   size()       { return vector<T>::size();  }
	 const T& top() const  { return vector<T>::back();  }
	 T&       top()        { return vector<T>::back();  }
	 void push(const T& t) { vector<T>::push_back(t);   }
	 void pop()            { vector<T>::pop_back();     }

   // MEX functionality ////////////////////////////////
   virtual void mxInit() { vector<T>::mxInit(); NN_=NULL; }
   virtual bool mxCheckValid(const mxArray* M,const mxArray* N=NULL) {
			mex::vector<Tn> tmp;
			return (vector<T>::mxCheckValid(M) && (N==NULL || tmp.mxCheckValid(N)));
	 }
	 using vector<T>::mxGet;
   virtual void mxSet(mxArray* M, mxArray* N=NULL) {
			if (!mxCheckValid(M,N)) throw std::runtime_error("incompatible Matlab object type(s) in mex::stack");
			vector<T>::mxSet(M);
			if (N) { NN_ = (Tn*) mxGetData(N); this->N_=*NN_; } else NN_=NULL;
	 }
   virtual void mxCopy(const mxArray* M, const mxArray* N=NULL) { vector<T>::mxCopy(M); NN_=NULL; }
   virtual void mxRelease() { vector<T>::mxRelease(); NN_=NULL; }
   virtual void mxDestroy() { vector<T>::mxDestroy(); NN_=NULL; }
	 void mxSwap(stack<T,Tn>& S) { vector<T>::mxSwap((vector<T>&)S); std::swap(NN_,S.NN_); }

 protected:
   virtual void mxUpdateN() {
		if ((NN_==NULL) || (this->N_ > mxGetN(this->M_)))   // if it increases, or we have no 2nd length variable
			vector<T>::mxUpdateN();                           //   we should register the new length
		if (NN_) *NN_ = (Tn) this->N_;
	 }

   Tn*  NN_;      // matlab "mirrored" size N_

};
#endif // def MEX

}      // namespace mex
#endif // re-include
