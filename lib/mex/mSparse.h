#ifndef __MSPARSE_H
#define __MSPARSE_H

#include <assert.h>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <stdexcept>
#include <stdint.h>

#include "mxObject.h"

/*
 * mSparse<T> : templated class for wrapping matlab sparse matrices, akin to STL classes

Things to include:
(1) Access: M[i][j] = value,  M(i,j) = value
		Iterator, step through non-zero entries; need *it and also it.i() and it.j() ?
    M[i] -> subclass wrapping contiguous units (row?)
		  Iterator, step through non-zero entries of row only  (same iterator OK, different begin() and end())
(2) Resizing of non-zero lists if exceed capacity, a la mVector
(3) Operations?

Others?


mMatrix<T> ?  same as mVector but need 2D access & iterators?
              Operations? scalar operations? matrix operations?

mArray<T>  ?  as mVector but with dimension vector and "tuple" notion
              How to access?  linear index and with a vector of indices?
              Operations?  reshape, resize; arithmetic ops? 


mArraySparse<T> ? more complicated? list of clauses?


*/

namespace mex {


template <class T>
class mSparse : virtual public mxObject {
 public:
// Type definitions //////////////////////////////
#  ifndef MEX
	 typedef size_t   mwIndex;
#  endif
	 
	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 // Sparse Matrix Iterator class : move through non-empty entries of the matrix /////////////////////////////////
	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 class const_iterator : public std::iterator<std::forward_iterator_tag,T> {
		 public:
			 const_iterator(const mSparse<T>* s, size_t i, size_t k) {
					 s_=s;i_=i;k_=k; while(i<s_->m_ && s_->c_[i_+1]<=k) ++i_;
			 };
		   //T& operator*() {return s_->V_[k_]; };
		   const T& operator*() const {return s_->V_[k_]; };
			 size_t c() const { return i_; };
			 size_t r() const { return s_->r_[k_]; };
			 const_iterator operator++(void) { ++k_; while (i_<s_->m_ && s_->c_[i_+1]<=k_) ++i_; return *this; };            //prefix
			 const_iterator operator++(int)  { const_iterator tmp(s_,i_,k_); ++*this; return tmp; }; //postfix
			 const_iterator operator--(void) { --k_; while (s_->c_[i_]>k_) --i_; return *this; };
			 const_iterator operator--(int)  { const_iterator tmp=*this; --*this; return tmp; };
			 bool operator==(const const_iterator i) const { return (s_==i.s_ && k_==i.k_); };
			 bool operator!=(const const_iterator i) const { return !(*this==i); };
		 private:
			 const mSparse<T>* s_;
			 size_t i_,k_;
	 };
	 typedef       const_iterator iterator;
	 typedef       const_iterator reverse_iterator;
	 typedef       const_iterator const_reverse_iterator;

	 class TRef {
  	private:
    	mSparse<T>* s_;
			size_t i_,j_;
  	public:
    	TRef(mSparse<T>* s, size_t i, size_t j) : s_(s), i_(i), j_(j) { }
    	operator T() const { return s_->get(i_,j_); }
    	TRef& operator=(const T x) { s_->set(i_,j_,x); return *this; }
		};
	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // Sparse Matrix Column class : const reference to a single column of a sparse matrix   
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   class sparse_column { 
     public:
	     typedef mSparse<T>::const_iterator const_iterator;
	     typedef mSparse<T>::const_reverse_iterator const_reverse_iterator;
       sparse_column(const mSparse<T>* s, size_t col) { s_=s; i_=col; };
       size_t size() const { return s_->c_[i_+1] - s_->c_[i_]; };            // return non-zero column length
       const T& operator[](size_t j) const { return s_->get(i_,j); }       	// access entry
       //TRef operator[](size_t j) const { return TRef(s_,i_,j_); }       		// modify entry
			 const_iterator begin() const { return const_iterator(s_,i_,s_->c_[i_]); }
			 const_iterator end()   const { return const_iterator(s_,i_+1,s_->c_[i_+1]); }
     private:
       const mSparse<T>* s_;
       size_t i_;
   };
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////



   // Basic checks ////////////////////////////////
	 size_t nnz()      const { return c_[m_]-c_[0]; };
	 size_t max_size() const { return std::numeric_limits<size_t>::max(); };
	 size_t nzmax()    const { return C_; };
	 bool empty()         const { return (nnz()==0); };

	 // Setting, changing sizes //////////////////////
	 void reserve(size_t nnz);															// reserve more memory for data
	 void resize(size_t n, size_t m);										// resize matrix 
	 void clear() { for (size_t i=0;i<=m_;++i) c_[i]=0; }	// empty matrix contents

   // Iterators ////////////////////////////////////
	 const_iterator begin() const           { return const_iterator(this,0,0);   }
	 const_iterator end() const             { return const_iterator(this,m_,c_[m_]-c_[0]);} 
	 //const_iterator end() const             { return const_iterator(this,m_,c_[m_]);}  // !!! c[m_]-c[0]?
	 //const_reverse_iterator rbegin() const  { return const_iterator(I_+N_-1,J_+N_-1,V_+N_-1); };
   //const_reverse_iterator rend() const    { return const_iterator(I_-1,J_-1,V_-1);  };

	 // Accessor functions ///////////////////////////
	 const T& operator()(size_t i, size_t j) const { return get(i,j); };
	 TRef operator()(size_t i, size_t j) { return TRef(this,i,j); }
	 const T& blank() { return T0_; }

	 // Constructors  ////////////////////////////////
	 explicit mSparse(size_t n=0, size_t m=0, size_t nnz=0) {
		 mxInit();
		 n_=n; m_=m; C_=nnz; r_=NULL; V_=NULL;
		 c_=new mwIndex[m_+1]; for (size_t i=0;i<m_+1;i++) c_[i]=0;
		 if (C_) {
		   r_=new mwIndex[C_];   
		   V_=new T[C_];
		 }
		 T0_ = T();
	 };
   ~mSparse() { if (!mxAvail()) { if (c_) delete[] c_; if (r_) delete[] r_; if (V_) delete[] V_; } };

	 // Get/Set elements ////////////////////////////
   void set(size_t i, size_t j, const T& val);
   const T& get(size_t i, size_t j) const;
   sparse_column operator[](size_t i) const { return sparse_column(this,i); };
   //const sparse_column operator[](size_t i) const { return sparse_column(this,i); };

	 // Insert/Erase elements ////////////////////////////
   void insert(size_t i, size_t j, const T& val);
   void erase(size_t i, size_t j);

	 void Print() {
	  std::cout<<m_<<","<<n_<<","<<C_<<"\n";
		for (iterator i=begin();i!=end();++i) std::cout<<"("<<i.r()<<","<<i.c()<<")="<<*i<<"\n";
   }

	 // Exchange two vectors ////////////////////////////
	 void swap(mSparse<T> &s);             // !!! not implemented

	 // Binary comparison tests /////////////////////////
	 bool operator==(const mSparse<T>& s);    // test for equality
	 bool operator<(const mSparse<T>& s);     // test for lexicographical order

  // MEX Class Wrapper Functions //////////////////////////////////////////////////////////
# ifdef MEX
  //void        mxInit();            // initialize any mex-related data structures, etc (private)
  //inline bool mxAvail() const;     // check for available matlab object
  //void        mxCopy(const mxArray*)// associate with deep copy of matlab object
  bool        mxCheckValid(const mxArray*);   // check if matlab object is compatible with this object
  void        mxSet(mxArray*);     // associate with A by reference to data
  mxArray*    mxGet();             // get a pointer to the matlab object wrapper (creating if required)
  void        mxRelease();         // disassociate with a matlab object wrapper, if we have one
  void        mxDestroy();         // disassociate and delete matlab object
  void        mxSwap(mSparse<T>&); // disassociate and delete matlab object
# endif
  /////////////////////////////////////////////////////////////////////////////////////////
	 mSparse<T>& mxNew();             // create a new matlab object wrapper for our data
	 mSparse<T>& mxClear();           // disassociate with a matlab object wrapper, if we have one

 protected:
	 mxArray*    mxCreateMatrix(size_t n);    // templated specialization for matlab matrix

	 size_t m_,n_; // # rows and columns
	 //size_t N_;    // number of data stored (nnz)
	 size_t C_;    // current capacity (nnzmax)
	 mwIndex* c_;   // location of 1st entry associated with each column
	 mwIndex* r_;   // row indices of nonzero entries
	 T* V_;           // value storage
	 //mxArray*  M_;    // matlab wrapper pointer

	 T T0_;           // default "blank" value
};



template <class T>
bool mSparse<T>::operator==(const mSparse<T>& s) { 
  if (nnz()!=s.nnz()) return false;
  for (iterator a=begin(),b=s.begin(); a!=end() && b!=s.end(); ++a,++b) 
    if (a.i()!=b.i() || a.j()!=b.j() || *a!=*b) return false;
  return true;
}
template <class T>
bool mSparse<T>::operator<(const mSparse<T>& s) {
  for (iterator a=begin(),b=s.begin(); a!=end() && b!=s.end(); ++a,++b) if (*a<*b) return true;
  if (nnz()<s.nnz()) return true;
  return false;
}


template <class T>
const T& mSparse<T>::get(size_t r,size_t c) const {
	if (c>=m_) return T0_;
  for (size_t ii=c_[c];ii<c_[c+1];ii++)
		if (r_[ii]==r) return V_[ii];
  return T0_;	
}

template <class T>
void mSparse<T>::set(size_t i,size_t j, const T& val) {
  if (val==T0_) erase(i,j);         // are we setting to the blank value?
	else          insert(i,j,val);    //  or a non-blank value?
}



template <class T>
void mSparse<T>::reserve(size_t nnz) {
  if (C_>=nnz) return;   // don't make matrix smaller 
  if (mxAvail()) {
#ifdef MEX
    if (C_<nnz) { V_=(T*)mxRealloc(V_, (nnz)*sizeof(T)); r_=(mwIndex*)mxRealloc(r_,(nnz)*sizeof(mwIndex)); }
    if (V_==NULL || r_==NULL) throw std::runtime_error("Memory reallocation failed");
    C_=nnz;
    mxSetIr(M_,r_); mxSetData(M_,V_);
#endif
  } else {
    mwIndex *r=new mwIndex[nnz];
    T *V=new T[nnz];
    std::copy(r_,r_+C_,r);
    std::copy(V_,V_+C_,V);
    if (V_) delete[] V_; if (r_) delete[] r_;
    V_=V; r_=r; C_=nnz;
  }
}

template <class T>
void mSparse<T>::resize(size_t n,size_t m) {
  if (n_<n) n_=n;
	if (m_>=m) return;		// don't make matrix smaller 
  if (mxAvail()) {
#ifdef MEX
  	if (m_<m)   c_=(mwIndex*)mxRealloc(c_, (m+1)*sizeof(mwIndex));
  	if (c_==NULL) throw std::runtime_error("Memory reallocation failed");
		for (size_t i=m_+1;i<=m;++i) c_[i]=c_[m_];
    m_=m;
  	mxSetJc(M_,c_);
#endif
  } else {
		mwIndex *c=new mwIndex[m+1]; 
		std::copy(c_,c_+m_+1,c); for (size_t i=m_+1;i<m+1;++i) c[i]=c_[m_];
		if (c_) delete[] c_;
		c_=c; m_=m;
  }
}


template <class T>
void mSparse<T>::insert(size_t r,size_t c, const T& val) {
	size_t k;
  for (k=c_[c];k<c_[c+1] && r_[k]<=r;k++) {
		if (r_[k]==r) { V_[k]=val;  return; }    // if already in the matrix
	}
	if (C_==c_[m_]) reserve( C_>3 ? 2*C_ : 4); // if not enough memory, reserve more
	assert(c_[m_]<C_);

	for (size_t b=c_[m_];b>k;b--) {           // shift existing data down one position
		V_[b]=V_[b-1]; r_[b]=r_[b-1];
	}
	V_[k]=val; r_[k]=r;
	for (size_t ii=c+1;ii<=m_;ii++) ++c_[ii];   // add one to positions of later columns
}

template <class T>
void mSparse<T>::erase(size_t r,size_t c) {
	size_t k;
  for (k=c_[c];k<c_[c+1] && r_[k]<=r;k++) {
		if (r_[k]==r) {                                   // if in the matrix	
			for (;k<c_[m_];++k) {
				V_[k]=V_[k+1]; r_[k]=r_[k+1];                 // shift existing data back, removing that point
			}
			for (size_t ii=c+1;ii<=m_;ii++) --c_[ii];    // subtract one from positions of later columns
			return; 
		}
	} // otherwise, entry is not in matrix; do nothing
}



#ifdef MEX
// Implementations for use with MEX

template <class T>
bool mSparse<T>::mxCheckValid(const mxArray* m) {
	return mxIsSparse(m);
}

template <class T>
void mSparse<T>::mxSet(mxArray* m) {
  if (mxAvail()) mxDestroyArray(M_);                        // destroy any old data we were storing
  else {
		if (V_) delete[] V_;                                 //  ""
		if (c_) delete[] c_;
		if (r_) delete[] r_;
	}
  M_=m;                                                     // save our new structure
  if (m) {
    m_=mxGetM(M_); n_=mxGetN(M_);                           // get the new data we will store, size
		C_=mxGetNzmax(M_); 
		c_=mxGetJc(M_);
		r_=mxGetIr(M_);
    V_ = (T*) mxGetData(M_);                                //   and values
		if (!C_) { r_=NULL; V_=NULL; }
  } else {C_=m_=n_=0; r_=NULL; V_=NULL; };                  // nullify if requested
}

template <class T>
mxArray* mSparse<T>::mxGet() {                  // return a handle to an associated matlab wrapper
  if (!mxAvail()) {
    mxArray* m = mxCreateMatrix(nnz());                                // create new memory (a matrix) !!!
    T*  v = (T*) mxGetData(m);                                      //  and get its data pointer
    mwIndex* c = mxGetJc(m);                                      //  and index pointers
    mwIndex* r = mxGetIr(m);                                      //  
    for (size_t i=0;i<nnz();i++) {r[i]=r_[i]; v[i]=V_[i]; }      // copy off our current data
	  for (size_t i=0;i<m_+1;i++)   c[i]=c_[i];
    V_=v; c_=c; r_=r; M_=m;                                         // save our new structure
  }
  return M_;
};

template <class T>
void mSparse<T>::mxRelease() {
	throw std::runtime_error("Not Implemented");
}
template <class T>
void mSparse<T>::mxDestroy() {
	throw std::runtime_error("Not Implemented");
}

template <class T>
void mSparse<T>::mxSwap(mSparse<T>& v) {
	std::swap(r_,v.r_);
  std::swap(c_,v.c_);
  std::swap(V_,v.V_);
	std::swap(m_,v.m_); std::swap(n_,v.n_); std::swap(C_,v.C_); 
  std::swap(T0_,v.T0_);
  std::swap(M_,v.M_);
}



template<> mxArray* mSparse<mex::midx<double> >::mxCreateMatrix(size_t n) { return mxCreateSparse(m_,n_,C_,mxREAL); }
template<> mxArray* mSparse<double>::mxCreateMatrix(size_t n) { return mxCreateSparse(m_,n_,C_,mxREAL); }
template<> mxArray* mSparse<bool>::mxCreateMatrix(size_t n)   { return mxCreateSparseLogicalMatrix(m_,n_,C_); }

#endif // ifdef MEX
//////////////////////////////////////////////////////////////////////////////////////////////
} // end namespace mex

#endif  // end single-inclusion
