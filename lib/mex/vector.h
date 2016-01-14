// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_VECTOR_H
#define __MEX_VECTOR_H

#include <assert.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>



#include "mxObject.h"
#ifdef MEX
#include "matrix.h"
#else
#include <vector>
#endif

/*
 * mex::vector<T> : templated class for wrapping matlab vectors, akin to std::vector<T>
*/

namespace mex {


#ifndef MEX
template <class T>
class vector : public std::vector<T>, virtual public mxObject {
public:
  typedef typename std::vector<T>::iterator         iterator;
  typedef typename std::vector<T>::const_iterator   const_iterator;
  typedef typename std::vector<T>::reverse_iterator       reverse_iterator;
  typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;

	explicit vector() : std::vector<T>() { }
	explicit vector(size_t n, const T& t=T()) : std::vector<T>(n,t) { }
	vector(mex::vector<T> const& v) : std::vector<T>( (std::vector<T> const&)v ) { }
 	template<class inIter> vector(inIter first, inIter last) : std::vector<T>(first,last) { }
	vector<T>& operator=(const vector<T>& v) { std::vector<T>::operator=((std::vector<T>&)v); return *this; }

  // Tests for equality and lexicographical order ///
  bool operator==(const vector<T>& t) const { return (this->size()==t.size())&&std::equal(this->begin(),this->end(),t.begin()); }
  bool operator!=(const vector<T>& t) const { return !(*this==t); }
  bool operator< (const vector<T>& t) const { return std::lexicographical_compare(this->begin(),this->end(),t.begin(),t.end()); }
  bool operator<=(const vector<T>& t) const { return (*this==t || *this<t); }
	bool operator> (const vector<T>& t) const { return !(*this<=t); }
	bool operator>=(const vector<T>& t) const { return !(*this>t); }
  virtual void mxSwap(vector<T>& v) { std::vector<T>::swap(v); };
protected:
  size_t N_;
  virtual void mxUpdateN(void) { if (N_) this->resize(N_); else this->clear(); } 
};


#else

// Declare double-templated version, to be specialized to "built-in" and "mxObject" types
template <class T, bool IsMxObject=is_base_of<mex::mxObject,T >::value > class vector;


// Basic class definition (same for both); later functions are specialized to built-in or mxObject
template <class T>
class vector<T,false> : virtual public mxObject {
 public:
   // Type definitions //////////////////////////////
	 // add types _ref, _cref, _it, _rit, with templated defaults to T&, const T&, T*, const T*
	 //     types _base ; write (_base)(T) to memory, access memory r as T(_base r)
	 //     type should convert easily to something "reasonable"
	 // Mediated types provide differentiated access between "contained" memory and "observed" values
   typedef T*       iterator;
   typedef const T* const_iterator;
   typedef T*       reverse_iterator;
   typedef const T* const_reverse_iterator;
   typedef T*       bidirectional_iterator;
   typedef const T* const_bidirectional_iterator;

   // Basic checks ////////////////////////////////
   size_t size()     const { return N_; };
   size_t max_size() const { return std::numeric_limits<size_t>::max(); };
   size_t capacity() const { return C_; };
   bool empty()         const { return (size()==0); };

	 // Setting, changing sizes //////////////////////
   void reserve(size_t n);                                      // create memory for up to length n
   void resize(size_t n, const T& t=T());                       // resize (grow) vector to length n
   void clear() { erase(begin(),end()); };                      // empty vector contents

   // Iterators ////////////////////////////////////
   iterator begin()                       { return V_;   };
   const_iterator begin() const           { return V_;   };
   iterator end()                         { return V_+N_;};
   const_iterator end() const             { return V_+N_;};
   reverse_iterator rbegin()              { return V_+N_-1; };
   const_reverse_iterator rbegin() const  { return V_+N_-1; };
   reverse_iterator rend()                { return V_-1;  };
   const_reverse_iterator rend() const    { return V_-1;  };

   // Accessor functions ///////////////////////////
   T&       operator[](size_t n)       { return V_[n]; };
   const T& operator[](size_t n) const { return V_[n]; };
	 T&       at(size_t n)               { if (n<0 || n>=N_) throw std::out_of_range("N_"); return V_[n]; };
	 const T& at(size_t n)         const { if (n<0 || n>=N_) throw std::out_of_range("N_"); return V_[n]; };
   const T& back()                  const { return V_[N_]; };
   T&       back()                        { return V_[N_]; };
   const T& front()                 const { return V_[0];  };
   T&       front()                       { return V_[0];  };

   // Constructors  ////////////////////////////////
	 vector() { _init(0); }
   explicit vector(size_t n, const T& t=T()) { _init(n,t); };
   vector(const vector<T,false>& v) { _init(v.size()); std::copy(v.begin(),v.end(),begin()); };
	 template<class inIter>
	 vector(inIter first, inIter last) { _init(0); while (first!=last) { push_back(*first); ++first; } }
   //vector(std::initializer_list<T> L) { *this = vector(L.begin(),L.end()); }
   virtual ~vector();

/* Functions to have:
template <InputIt> assign(first,last);  assign(n, T&)  : replace vector with given inputs
get_allocator
*/

   // "Safe" insertion & removal ////////////////////
   void push_back(const T& t);                                  // Push operation (add at end)
   void pop_back();                                             // Pop operation (remove from end)

   void insert(iterator pos, const T& t);                       //
   void insert(iterator pos, size_t n, const T& t);          //
   template <class InputIterator> 
   void insert(iterator pos, InputIterator f, InputIterator l); //
   iterator erase(iterator pos);                                // remove a single entry from the vector
   iterator erase(iterator first, iterator last);               // remove a span of entries from the vector

   void swap(vector<T,false> &v);                                    // Exchange two vectors (completely)

   // Boolean comparison operators /////////////////////
   bool operator==(const vector<T,false>& t) const { return (this->size()==t.size())&&std::equal(begin(),end(),t.begin()); }
   bool operator!=(const vector<T,false>& t) const { return !(*this==t); }
   bool operator< (const vector<T,false>& t) const { return std::lexicographical_compare(begin(),end(),t.begin(),t.end()); }
   bool operator<=(const vector<T,false>& t) const { return (*this==t || *this<t); }
	 bool operator> (const vector<T,false>& t) const { return !(*this<=t); }
	 bool operator>=(const vector<T,false>& t) const { return !(*this>t); }

	 // Copy operator   //////////////////////////////////
	 vector<T,false>& operator=(const vector<T,false>& v);

   // MEX functionality ////////////////////////////////
   virtual bool     mxCheckValid(const mxArray* A);  // check that mxArray is valid for this object type
   virtual void     mxSet(mxArray* A);   // associate with A by reference to data
   virtual void     mxSetND(mxArray* A); //   "" but ignore dimensional checks
   virtual mxArray* mxGet();             // get a pointer to the matlab object wrapper (creating if required)
   virtual void     mxRelease();         // disassociate with a matlab object wrapper, if we have one
   virtual void     mxDestroy();         // disassociate and delete matlab object wrapper
   virtual void     mxSwap(vector<T,false>&);  // exchange matlab identities with another object

 protected:
	 void _init(size_t n=0, const T& t=T()) { C_=N_=0; V_=NULL; mxInit(); resize(n,t); };
	 virtual void mxUpdateN() { if (M_) mxSetN(M_,N_); }  // update matlab length value
   inline mxArray* mxCreateMatrix(size_t n);    // templated specialization for matlab matrix

   size_t N_;  // number of data stored
   size_t C_;  // current capacity
   T* V_;         // data storage
};


// Identical declaration of mxObject-derived specialization
template <class T>
class vector<T,true> : virtual public mxObject {
 public:
   typedef T*       iterator;
   typedef const T* const_iterator;
   typedef T*       reverse_iterator;
   typedef const T* const_reverse_iterator;
   typedef T*       bidirectional_iterator;
   typedef const T* const_bidirectional_iterator;
   size_t size()     const { return N_; };
   size_t max_size() const { return std::numeric_limits<size_t>::max(); };
   size_t capacity() const { return C_; };
   bool empty()      const { return (size()==0); };
   void reserve(size_t n);                                      // create memory for up to length n
   void resize(size_t n, const T& t=T());                       // resize (grow) vector to length n
   void clear() { erase(begin(),end()); };                      // empty vector contents
   iterator begin()                       { return V_;   };
   const_iterator begin() const           { return V_;   };
   iterator end()                         { return V_+N_;};
   const_iterator end() const             { return V_+N_;};
   reverse_iterator rbegin()              { return V_+N_-1; };
   const_reverse_iterator rbegin() const  { return V_+N_-1; };
   reverse_iterator rend()                { return V_-1;  };
   const_reverse_iterator rend()   const  { return V_-1;  };
   T&       operator[](size_t n)          { return V_[n]; };
   const T& operator[](size_t n)   const  { return V_[n]; };
	 T&       at(size_t n)                  { if (n<0 || n>=N_) throw std::out_of_range("N_"); return V_[n]; };
	 const T& at(size_t n)           const  { if (n<0 || n>=N_) throw std::out_of_range("N_"); return V_[n]; };
   const T& back()                 const  { return V_[N_]; };
   T&       back()                        { return V_[N_]; };
   const T& front()                const  { return V_[0];  };
   T&       front()                       { return V_[0];  };
   vector() { _init(0); };
   explicit vector(size_t n, const T& t=T()) { _init(n,t); };
   vector(const vector<T,true>& v) { _init(v.size()); std::copy(v.begin(),v.end(),begin()); };
	 template<class inIter>
	 vector(inIter first, inIter last) { _init(0); while (first!=last) { push_back(*first); ++first; } }
   //vector(std::initializer_list<T> L) { *this = vector(L.begin(),L.end()); }
   virtual ~vector();
   void push_back(const T& t);                                  // Push operation (add at end)
   void pop_back();                                             // Pop operation (remove from end)
   void insert(iterator pos, const T& t);                       //
   void insert(iterator pos, size_t n, const T& t);          		// Insert single element, multiple elements
   template <class InputIterator> 															//   or collection from iterators
   void insert(iterator pos, InputIterator f, InputIterator l); //
   iterator erase(iterator pos);                                // remove a single entry from the vector
   iterator erase(iterator first, iterator last);               // remove a span of entries from the vector
   void swap(vector<T,true> &v);                                // Exchange two vectors (completely)
   bool operator==(const vector<T,true>& t) const { return (this->size()==t.size())&&std::equal(begin(),end(),t.begin()); }
   bool operator!=(const vector<T,true>& t) const { return !(*this==t); }
   bool operator< (const vector<T,true>& t) const { return std::lexicographical_compare(begin(),end(),t.begin(),t.end()); }
   bool operator<=(const vector<T,true>& t) const { return (*this==t || *this<t); }
	 bool operator> (const vector<T,true>& t) const { return !(*this<=t); }
	 bool operator>=(const vector<T,true>& t) const { return !(*this>t); }
	 vector<T,true>& operator=(const vector<T,true>& v);
   virtual bool     mxCheckValid(const mxArray* A);  // check that mxArray is valid for this object type
   virtual void     mxSet(mxArray* A);   // associate with A by reference to data
   virtual mxArray* mxGet();             // get a pointer to the matlab object wrapper (creating if required)
   virtual void     mxRelease();         // disassociate with a matlab object wrapper, if we have one
   virtual void     mxDestroy();         // disassociate and delete matlab object wrapper
   virtual void     mxSwap(vector<T,true>&);  // exchange matlab identities with another object
 protected:
	 void _init(size_t n=0, const T& t=T()) { C_=N_=0; V_=NULL; mxInit(); resize(n,t); };
	 virtual void mxUpdateN() { if (M_) mxSetN(M_,N_); }  // update matlab length value
   inline mxArray* mxCreateMatrix(size_t n);            // templated specialization for matlab matrix

   size_t N_;  // number of data stored
   size_t C_;  // current capacity
   T* V_;         // data storage
};


/// !!! TODO !!!
// (1) in builtin implementation, specialize to non-initialized memory
// (2) in mxObject implementation, ensure proper deletion operation and related non-init issues


/*** IMPLEMENTATIONS *******/

//////// Containing built-in objects as regular vectors //////////////////////////////////////////////////////////////
//
template<class T> mxArray* vector<T,false>::mxCreateMatrix(size_t n) { 
	return mxCreateNumericMatrix( mexable_traits<T>::mxsize, n, mxClassT<typename mexable_traits<T>::mxbase>::value,mxREAL); 
}
template<class T> bool vector<T,false>::mxCheckValid(const mxArray* m) { 
	if (m==NULL) return false;
	//std::cout<<mxGetClassID(m)<<" , "<<mxClassT<typename mexable_traits<T>::mxbase>::value<<"\n";
	//std::cout<<mxGetN(m)<<","<<mxGetM(m)<<" = "<<mexable_traits<T>::mxsize<<"\n";
	if (mxGetClassID(m)!=mxClassT<typename mexable_traits<T>::mxbase>::value) return false;
	if (mxGetN(m)!=0 && mxGetM(m)!=mexable_traits<T>::mxsize) return false;
	return true;
}

//////// Containing mxObjects as cell-array vectors //////////////////////////////////////////////////////////////////
template<class T> mxArray* vector<T,true>::mxCreateMatrix(size_t n) { return mxCreateCellMatrix(1,n); }
// Test to see if this object is a valid vector of mxObjects
// Currently doesn't check entries (may result in mxSet() throwing an exception)
template<class T> bool vector<T,true>::mxCheckValid(const mxArray* m) { return mxGetClassID(m)==mxCELL_CLASS; }

/*
// From James Tursa -- use undocumented matlab function to create unintialized matrix
mxArray *mxCreateUninitNumericMatrix(mwSize m, mwSize n, mxClassID classid, mxComplexity ComplexFlag);

// From Minka's lightspeed -- uninitialized memory allocation
template<T>
mxObject::mxArray* mxCreateNumericMatrixE(size_t m,size_t n,mxClassID cl,mxComplexity cplx) {
  mxObject::mxArray* M=mxCreateNumericMatrix(1,1,cl,cplx);
	size_t sz = m*n*sizeof(T); // used to always be sizeof(double), but not any more...
  mxSetM(M,m); mxSetN(M,n); mxSetPr(M,mxRealloc(mxGetPr(M),sz));
	return M;
}
*/



/*** BUILT-IN TYPES *******/
template <class T>
vector<T,false>::~vector() { 															// Destructor: leave matlab objects for garbage
	if (V_ && !mxAvail()) mxFree(V_); 											//  collection (in case shared); free any new memory
}; 

template <class T>
vector<T,true>::~vector() { 															// Destructor: (!!!) should delete V_ anyway?
	if (V_ && !mxAvail()) delete[] V_; 											//   question is whether to delete elements of V_?
}; 



template <class T>
void vector<T,false>::reserve(size_t n) {                 // Reserve more memory when we're a matlab object
	if (n<=C_) return;
	//n--; n|=n>>1; n|=n>>2; n|=n>>4; n|=n>>8; n|=n>>16; n|=n>>32; n++; // round to next power-of-two?
	T* mem = (T*) mxCalloc(n, sizeof(T));                   // get memory from matlab system
	assert(mem);
	std::copy(begin(),end(),mem);                           // copy data over  (assumes native type => no constructor)
	std::swap(mem,V_);                                      // exchange old for new data pointer
	C_=n;                                                   // set new capacity
  if (mxAvail()) mxSetData(M_,(void*)V_);                 // let matlab know about new memory location
  if (mem) mxFree(mem);                                   // and delete the old memory (native type => no destructor)
}

template <class T>
void vector<T,true>::reserve(size_t n) {                  // Reserve more memory when we're a matlab object  (!!!???)
  if (n<=C_) return;
  T* mem = new T[n];                                        //   allocate some new memory
  if (mxAvail()) {                                          // If we're a matlab object,
    mxArray* m = mxCreateCellMatrix(1,n); assert(m);        // create a new cell array (to have proper length)
    void** a1 = (void**) mxGetData(m);    assert(a1);
    void** a2 = (void**) mxGetData(M_);
    mxSetData(m,(void*)a2); mxSetData(M_,(void*)a1);        // swap data entries of new & old cell arrays
    for (size_t i=0;i<N_;i++) std::swap(*a1++,*a2++);    // swap elements to new memory in old cell array
    mxSetN(m,C_);
    C_=n;
    mxDestroyArray(m);                                      // destroy temporary cell array  
    for (size_t i=0; i<N_; i++) mem[i].mxSet(V_[i].mxGet()); // take over old mex objects
  } else {
    for (size_t i=0; i<N_; i++) mem[i].swap(V_[i]);         // if non-mex, just copy objects over
    C_=n;
  }
  if (V_) delete[] V_;                                      //   delete old memory
  V_=mem;                                                   //   and save pointer
};



template <class T>
void vector<T,false>::resize(size_t n, const T& t) {            // Resize vector : may call reserve, also changes length
	if (n<=N_) { N_=n; }
	else {
		if (n>C_) reserve(n);
		iterator it=end(); N_=n; while (it!=end()) *it++=t;
	}
  mxUpdateN();                                            // update Matlab's knowledge of our length
}

template <class T>
void vector<T,true>::resize(size_t n, const T& t) {
  if (n<=N_) { for (iterator i=V_+n;i!=end();++i) i->mxDestroy(); N_=n; }
  else {
    if (n>C_) reserve(n);
    size_t i=N_;
    N_=n; mxUpdateN();                                      // update Matlab's knowledge of our length
    for (;i<n;++i) {
      V_[i]=t;                                              // assumes pre-initialized V_
      if (mxAvail()) mxSetCell(M_,i,V_[i].mxGet());
    }
  }
}





template <class T>
void vector<T,false>::push_back(const T& t) {       // "Push" operation, extending size if required
  if (N_==C_) reserve( (C_>3)?2*C_:4 );      	// get more memory if required (4 entries or 2x current size)
  V_[N_++]=t; 																// native type => no constructor to worry about 
  mxUpdateN(); 
};

template <class T>
void vector<T,true>::push_back(const T& t) {       // "Push" operation, extending size if required
  if (N_==C_) reserve( (C_>3)?2*C_:4 );           // get more memory if required (10 entries or 2x current size)
  if (C_ > N_) {
    V_[N_++]=t;
    mxUpdateN();
    if (mxAvail()) mxSetCell(M_,N_-1,V_[N_-1].mxGet());
  }
}



template <class T>
void vector<T,false>::pop_back() {                  // "Pop" operation
  --N_;                                       // should call destructor; assumes native type => none
	mxUpdateN();
}

template <class T>
void vector<T,true>::pop_back() {             // "Pop" operation
	end()->mxDestroy();
  --N_;                                  
	mxUpdateN();
}




/// Inserting entries from a vector ///////////////////////
template <class T>
void vector<T,false>::insert(iterator pos, size_t n, const T& t) {
	size_t N2 = end() - pos;
	resize(size()+n,t);
	iterator e2 = end()-1, e1=e2-n;
	for (size_t i=0;i<N2;i++) std::swap(*e1--,*e2--); 		// ok because T is a native type
}
template <class T>
void vector<T,false>::insert(iterator pos, const T& t) { insert(pos,(size_t)1,t); }
template <class T>
template <class InputIterator>
void vector<T,false>::insert(iterator pos, InputIterator first, InputIterator last) {
	ptrdiff_t N=distance(begin(),pos);		// save in case we reallocate
	insert(pos,distance(first,last));			// figure out how much space we need
	std::copy(first,last,begin()+N);			// then write into the block
}


template <class T>
void vector<T,true>::insert(iterator pos, size_t n, const T& t) {
  size_t N2 = end() - pos;
  resize(size()+n,t);
  iterator e2 = end()-1, e1=e2-n;
  for (size_t i=0;i<N2;i++,--e1,--e2) swap(*e1,*e2);    // assumes swap() implemented
}
template <class T> void vector<T,true>::insert(iterator pos, const T& t) { insert(pos,(size_t)1,t); }
template <class T> template <class InputIterator>
void vector<T,true>::insert(iterator pos, InputIterator first, InputIterator last) {
  ptrdiff_t N=distance(begin(),pos);    // save in case we reallocate
  insert(pos,distance(first,last));     // figure out how much space we need
  std::copy(first,last,begin()+N);      // then write into the block
}




/// Removing entries from a vector ///////////////////////
template <class T>
typename vector<T,false>::iterator vector<T,false>::erase(iterator pos) { erase(pos,pos+1); };

template <class T>
typename vector<T,false>::iterator vector<T,false>::erase(iterator first, iterator last) { // remove a span of entries 
  ptrdiff_t d=(last-first);
	std::copy(last,end(),first);
	N_-=d; mxUpdateN(); 
};

template <class T>
typename vector<T,true>::iterator vector<T,true>::erase(iterator pos) {   // remove a single entry from the vector
  iterator last=pos+1;
  while(last!=end()) (*(pos++)).mxSwap(*(last++-1));
  N_--; mxUpdateN();
  V_[N_].mxDestroy();
}
template <class T>
typename vector<T,true>::iterator vector<T,true>::erase(iterator first, iterator last) { // remove a span of entries 
  ptrdiff_t d=(last-first);
  while (last!=end()) { first->mxSwap(*last); ++first; ++last; }
  N_-=d; mxUpdateN();
  for (size_t i=N_;i<N_+d;i++) V_[i].mxDestroy();
}




/// Exchange two vectors ////////////////////////////
template <class T>
void vector<T,false>::swap(vector<T,false> &v)  {      // exchange data between two vectors (keeps mxObject identities)
  if (mxAvail())   mxSetData(M_,(void*)v.V_);          // point each Matlab object at the other's data
  if (v.mxAvail()) mxSetData(v.M_,(void*)V_);                 
	std::swap(V_,v.V_);                                  // now swap the pointers in the C++ object
	std::swap(C_,v.C_);
	std::swap(N_,v.N_);
}

template <class T>
void vector<T,true>::swap(vector<T,true> &v)  {
  if (mxAvail()||v.mxAvail()) {                         // if either is a matlab object,
    mxGet(); v.mxGet();                                 // force both to be to avoid problems
    if (mxAvail())   mxSetData(M_,(void*)v.V_);         // point each Matlab object at the other's data
    if (v.mxAvail()) mxSetData(v.M_,(void*)V_);
  }
  std::swap(V_,v.V_);                                   // now swap the pointers in the C++ object
  std::swap(C_,v.C_);
  std::swap(N_,v.N_);
}



/// Copy operator //////////////////////////////////
template <class T>
vector<T,false>& vector<T,false>::operator=(const vector<T,false>& v) {       // Copy operator
	if (this != &v) {
    if (v.N_ > C_) { N_=0; reserve(v.N_); };    // empty container before reallocating
		std::copy(v.begin(),v.end(),V_);            // copy over elements (assumes native type => no constructor)
		N_=v.N_; mxUpdateN();                       // set new size
	}
	return *this;
}

template <class T>
vector<T,true>& vector<T,true>::operator=(const vector<T,true>& v) {       // Copy operator
  if (this != &v) {
    clear();                                    // empty container 
    if (v.N_ > C_) { reserve(v.N_); };          // reserve space for copy
    std::copy(v.begin(),v.end(),V_);            // copy over elements (assumes reserve called constructor)
    N_=v.N_; mxUpdateN();                       // set new size
  }
  return *this;
}




//////////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
void vector<T,false>::mxSet(mxArray* m) {
	if (!mxCheckValid(m)) throw std::runtime_error("incompatible Matlab object type in mex::vector");
  if (mxAvail()) mxDestroyArray(M_);                        // destroy any old data we were storing
  else if (V_) mxFree(V_);                                  //  ""
  M_=m;                                                     // save our new structure
  if (m) {
    C_=N_=mxGetN(m);                                        // get the new data we will store, size
		if (mxGetM(m)==0) mxSetM(m,mexable_traits<T>::mxsize);
    V_ = (T*) mxGetData(m);                                 //   and values
		if (N_==0) V_=NULL;
  } else {C_=N_=0; V_=NULL; };                              // nullify if requested
}

template <class T>
void vector<T,false>::mxSetND(mxArray* m) {									// FOR MIXED SIZE ARRAYS AS 1xN
	if (mxGetClassID(m)!=mxClassT<typename mexable_traits<T>::mxbase>::value)
		throw std::runtime_error("incompatible Matlab object type in mex::vector");
  if (mxAvail()) mxDestroyArray(M_);                        // destroy any old data we were storing
  else if (V_) mxFree(V_);                                  //  ""
  M_=m;                                                     // save our new structure
  if (m) {
    C_=N_=mxGetNumberOfElements(m);                         // get the new data we will store, size
    V_ = (T*) mxGetData(m);                                 //   and values
		if (N_==0) V_=NULL;
  } else {C_=N_=0; V_=NULL; };                              // nullify if requested
}

template <class T>
void vector<T,true>::mxSet(mxArray* m) {
  if (!mxCheckValid(m)) throw std::runtime_error("incompatible Matlab object type in mex::cellvector");
  if (mxAvail()) mxDestroyArray(M_);                        // destroy any old data we were storing
  if (V_) { delete[] V_; V_=NULL; }                         //  ""
  M_=m;                                                     // save our new structure
  if (M_) {
    C_=N_=mxGetN(M_);
		if (mxGetM(M_)==0) mxSetM(M_,1);
		if (V_) delete[] V_;
    V_ = new T[C_];                                         // need object holders for the elements
    for (size_t i=0;i<N_;i++) {                          // for each element in cell,
      mxArray* ci = mxGetCell(M_,i);                        //   point C++ object to capture it
      if (ci) V_[i].mxSet(ci);                              // if it's not blank save it
      else    mxSetCell(M_,i,V_[i].mxGet());                //  (!!!) blank => write a pointer to a T() object there
    }
  }
  if (N_==0) V_=NULL;
}



template <class T>
mxArray* vector<T,false>::mxGet() {
	if (!mxAvail()) {
    mxArray* m = mxCreateMatrix(N_);                        // create new memory (a matrix)
		vector<T> v; v.mxSet(m);
		if (V_==NULL) std::swap(M_,v.M_);												// if we've no data at all, just flip matlab object
		else { swap(v); mxSwap(v); }                   					// swap data & then bring the whole thing back
	} else { }
	return M_;
};

template <class T>
mxArray* vector<T,true>::mxGet() {
  if (!mxAvail()) {
    M_ = mxCreateCellMatrix(1,N_); assert(M_);              // create new cell array
    for (size_t i=0;i<N_;i++) {
      mxSetCell(M_, i, V_[i].mxGet());
    }
  } else { }
  return M_;
};


template <class T>
void vector<T,false>::mxRelease() {                               // Disassociate with a given matlab object
	if (mxAvail()) {                                          //   leaving it for matlab's garbage collection
		T* v = (T*) mxCalloc(N_, sizeof(T));
		std::copy(begin(),end(),v);
    V_=v;
	}
}

template <class T>
void vector<T,true>::mxRelease() {                           // Disassociate with a given matlab object
  if (mxAvail()) {                                          //   leaving it in memory for garbage collection
    T* v=new T[C_];
    for (size_t i=0;i<N_;i++) v[i]=V_[i];                // copy constructor will lose matlab object-ness
		delete[] V_;
    V_=v;                                                   //  in elements
    M_=NULL;
  }
}


template <class T>
void vector<T,false>::mxDestroy() {                           // Disassociate with a given matlab object
	mxArray* m = M_;                                      //   deallocating / destroying it
	mxRelease();
	if (m) mxDestroyArray(m);
}

template <class T>
void vector<T,true>::mxDestroy() {                           // Disassociate with a given matlab object
  mxArray* m = M_;                                          //   explicitly destroying it
  mxRelease();
  if (m) mxDestroyArray(m);
}


template <class T>
void vector<T,false>::mxSwap(vector<T,false>& v) {                  // Exchange matlab identities with another object
	std::swap(M_,v.M_);
	std::swap(V_,v.V_);
	std::swap(N_,v.N_); std::swap(C_,v.C_);	
}

template <class T>
void vector<T,true>::mxSwap(vector<T,true>& v) {              // Exchange matlab identities between two objects
  std::swap(M_,v.M_);                                       //  swap matlab IDs (pointers)
  std::swap(V_,v.V_);                                       //  swap c++ object lists
  std::swap(N_,v.N_); std::swap(C_,v.C_);                   //  swap sizes
}



//////////////////////////////////////////////////////////////////////////////////////////////

#endif  // ndef MEX
}       // namespace mex
#endif  // re-include
