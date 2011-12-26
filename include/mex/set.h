#ifndef __MEX_SET_H
#define __MEX_SET_H

#include <assert.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <stdlib.h>
#include <stdint.h>

#include "mxObject.h"
#include "vector.h"

/*
 * mex::set<T> : templated class for wrapping sorted matlab vectors, usable in set operations
*/

namespace mex {

template <class T>
class set : public virtual mxObject, protected mex::vector<T> {
 public:
   // Type definitions //////////////////////////////
   typedef typename vector<T>::iterator               iterator;
   typedef typename vector<T>::const_iterator         const_iterator;
   typedef typename vector<T>::reverse_iterator       reverse_iterator;
   typedef typename vector<T>::const_reverse_iterator const_reverse_iterator;
	 typedef std::out_of_range  out_of_range;

   // Basic checks ////////////////////////////////
   size_t size()     const { return vector<T>::size(); }
   size_t max_size() const { return vector<T>::max_size(); }
   size_t capacity() const { return vector<T>::capacity(); }
   bool empty()         const { return vector<T>::empty(); }

	 // Setting, changing sizes //////////////////////
   void reserve(size_t n)                { vector<T>::reserve(n); }
   void clear()                          { vector<T>::clear(); }

   // Iterators ////////////////////////////////////
   const_iterator begin() const           { return vector<T>::begin(); }
   const_iterator end() const             { return vector<T>::end(); }
   const_reverse_iterator rbegin() const  { return vector<T>::rbegin(); }
   const_reverse_iterator rend() const    { return vector<T>::rend(); }

   // Accessor functions ///////////////////////////
   const T& operator[](size_t n) const { return vector<T>::operator[](n); }
   const T& at(size_t n)         const { return vector<T>::at(n); }
   const T& back()                  const { return vector<T>::back(); }
   const T& front()                 const { return vector<T>::front(); }
   const_iterator find(const T& x) const { return std::find(begin(),end(),x); }

   // Constructors  (missing a few from std::vector) ////////////////
   explicit set(size_t capacity=10) : vector<T>() { reserve(capacity); };
   //set(const set& v) : vector<T>() { resize(v.size()); std::copy(v.begin(),v.end(),_begin()); }
   set(const set& v) : vector<T>() { vector<T>::operator=((vector<T>&)v); } //resize(v.size()); std::copy(v.begin(),v.end(),_begin()); }
	 template<class inIter>
	 set(inIter first, inIter last) : vector<T>() { while (first!=last) { *this|=*first; ++first; } }
	 set<T>& operator=(const set<T>& s) { vector<T>::operator=( (vector<T>&)s ); return *this; }
   ~set()                        { } 

   // "Safe" insertion & removal ////////////////////
	 set<T>  operator| (const set<T>&) const;    // union
	 set<T>  operator+ (const set<T>&) const;    // union
	 set<T>  operator/ (const set<T>&) const;    // set-diff
	 set<T>  operator- (const set<T>&) const;    // set-diff
	 set<T>  operator& (const set<T>&) const;    // intersect
	 set<T>  operator^ (const set<T>&) const;    // symm set diff
	 set<T>  operator| (const T&) const;         // union
	 set<T>  operator+ (const T&) const;    // union
	 set<T>  operator/ (const T&) const;    // set-diff
	 set<T>  operator- (const T&) const;    // set-diff
	 set<T>  operator& (const T&) const;    // intersect
	 set<T>  operator^ (const T&) const;    // symm set diff

	 set<T>& operator|=(const set<T>&);    // union
	 set<T>& operator+=(const set<T>&);    // union
	 set<T>& operator/=(const set<T>&);    // set-diff
	 set<T>& operator-=(const set<T>&);    // set-diff
	 set<T>& operator&=(const set<T>&);    // intersect
	 set<T>& operator^=(const set<T>&);    // symm set diff

	 set<T>& operator|=(const T&);    // union
	 set<T>& operator+=(const T&);    // union
	 set<T>& operator/=(const T&);    // set-diff
	 set<T>& operator-=(const T&);    // set-diff
	 set<T>& operator&=(const T&);    // intersect
	 set<T>& operator^=(const T&);    // symm set diff

   void remove(const T& t);                                   // remove a single element
   void add(const T& t);                                      // add a single element

   void swap(set<T> &v) { vector<T>::swap( (vector<T>&) v ); }

	 // Tests for equality and lexicographical order ///
	 bool operator==(const set<T>& t) const { return (this->size()==t.size())&&std::equal(this->begin(),this->end(),t.begin()); }
	 bool operator!=(const set<T>& t) const { return !(*this==t); }
	 bool operator< (const set<T>& t) const { return std::lexicographical_compare(this->begin(),this->end(),t.begin(),t.end()); }
	 bool operator<=(const set<T>& t) const { return (*this==t || *this<t); }
	 bool operator> (const set<T>& t) const { return !(*this<=t); }
	 bool operator>=(const set<T>& t) const { return !(*this>t); }
	
   // MEX functionality ////////////////////////////////
/*
   virtual void  mxInit();               // initialize any mex-related data structures, etc (private)
   virtual bool  mxCheckValid(const mxArray*); // check validity of matlab object for set<T>
   virtual void  mxSet(mxArray* A);      // associate with A by reference to data
   virtual mxArray*  mxGet();            // get a pointer to the matlab object wrapper (creating if required)
   virtual void  mxRelease();            // disassociate with a matlab object wrapper, if we have one
   virtual void  mxDestroy();            // disassociate & destroy matlab object
   virtual void  mxSwap(set<T>&);        // exchange matlab object identities with another set
*/
	 using vector<T>::mxInit;
	 using vector<T>::mxCheckValid;
	 using vector<T>::mxGet;
	 using vector<T>::mxSet;
	 using vector<T>::mxCopy;
	 using vector<T>::mxRelease;
	 using vector<T>::mxDestroy;
   virtual void  mxSwap(set<T>& v) { vector<T>::mxSwap( (vector<T>&) v ); }

 protected:
   //mxArray*    mxCreateMatrix(size_t n) { return vector<T>::mxCreateMatrix(n); }
   void     mxUpdateN(void)                  { vector<T>::mxUpdateN(); }
   void     resize(size_t n, const T& t=T()) { vector<T>::resize(n,t); }
   iterator _begin()                 { return vector<T>::begin(); }
   iterator _end()                   { return vector<T>::end(); }
   reverse_iterator _rbegin()        { return vector<T>::rbegin(); }
   reverse_iterator _rend()          { return vector<T>::rend(); }
};



template <class T>
set<T> set<T>::operator|(const set<T>& b) const { 
  set<T> d; d.resize(size()+b.size());                                            // reserve enough space
	set<T>::iterator dend;
  dend=std::set_union(begin(),end(),b.begin(),b.end(),d._begin());                  // use stl set function
	d.N_=dend-d.begin(); d.mxUpdateN();
	return d;
}
template <class T> set<T> set<T>::operator+(const set<T>& b) const { return *this|b; };
template <class T>
set<T> set<T>::operator/(const set<T>& b) const { 
  set<T> d; d.resize(size());                                                 // reserve enough space
	set<T>::iterator dend;
  dend=std::set_difference(begin(),end(),b.begin(),b.end(),d._begin());             // use stl set function
	d.N_=dend-d.begin(); d.mxUpdateN();
	return d;
}
template <class T> set<T> set<T>::operator-(const set<T>& b) const { return *this/b; };
template <class T>
set<T> set<T>::operator&(const set<T>& b) const { 
  set<T> d; d.resize(size());                                                 // reserve enough space
	set<T>::iterator dend;
  dend=std::set_intersection(begin(),end(),b.begin(),b.end(),d._begin());           // use stl set function
	d.N_=dend-d.begin(); d.mxUpdateN();
	return d;
}
template <class T>
set<T> set<T>::operator^(const set<T>& b) const { 
  set<T> d; d.resize(size()+b.size());                                              // reserve enough space
	set<T>::iterator dend;
  dend=std::set_symmetric_difference(begin(),end(),b.begin(),b.end(),d._begin());   // use stl set function
	d.N_=dend-d.begin(); d.mxUpdateN();
	return d;
}

template <class T> set<T>& set<T>::operator|=(const set<T>& b) { *this=*this|b; return *this; }
template <class T> set<T>& set<T>::operator+=(const set<T>& b) { *this=*this+b; return *this; }
template <class T> set<T>& set<T>::operator/=(const set<T>& b) { *this=*this/b; return *this; }
template <class T> set<T>& set<T>::operator-=(const set<T>& b) { *this=*this-b; return *this; }
template <class T> set<T>& set<T>::operator&=(const set<T>& b) { *this=*this&b; return *this; }
template <class T> set<T>& set<T>::operator^=(const set<T>& b) { *this=*this^b; return *this; }


template <class T>
set<T> set<T>::operator|(const T& b) const { 
  set<T> d; d.resize(size()+1);                                            // reserve enough space
	set<T>::iterator dend;
  dend=std::set_union(begin(),end(),&b,(&b)+1,d._begin());                  // use stl set function
	d.N_=dend-d.begin(); d.mxUpdateN();
	return d;
}
template <class T> set<T> set<T>::operator+(const T& b) const { return *this|b; };
template <class T>
set<T> set<T>::operator/(const T& b) const { 
  set<T> d; d.resize(size());                                                 // reserve enough space
	set<T>::iterator dend;
  dend=std::set_difference(begin(),end(),&b,(&b)+1,d._begin());             // use stl set function
	d.N_=dend-d.begin(); d.mxUpdateN();
	return d;
}
template <class T> set<T> set<T>::operator-(const T& b) const { return *this/b; };
template <class T>
set<T> set<T>::operator&(const T& b) const { 
  set<T> d; d.resize(size());                                                 // reserve enough space
	set<T>::iterator dend;
  dend=std::set_intersection(begin(),end(),&b,(&b)+1,d._begin());           // use stl set function
	d.N_=dend-d.begin(); d.mxUpdateN();
	return d;
}
template <class T>
set<T> set<T>::operator^(const T& b) const { 
  set<T> d; d.resize(size()+1);                                             // reserve enough space
	set<T>::iterator dend;
  dend=std::set_symmetric_difference(begin(),end(),&b,(&b)+1,d._begin());   // use stl set function
	d.N_=dend-d.begin(); d.mxUpdateN();
	return d;
}

template <class T> set<T>& set<T>::operator|=(const T& b) { *this=*this|b; return *this; }
template <class T> set<T>& set<T>::operator+=(const T& b) { *this=*this+b; return *this; }
template <class T> set<T>& set<T>::operator/=(const T& b) { *this=*this/b; return *this; }
template <class T> set<T>& set<T>::operator-=(const T& b) { *this=*this-b; return *this; }
template <class T> set<T>& set<T>::operator&=(const T& b) { *this=*this&b; return *this; }
template <class T> set<T>& set<T>::operator^=(const T& b) { *this=*this^b; return *this; }



template <class T> void set<T>::add(const T& t) { *this |= t; }
template <class T> void set<T>::remove(const T& t) { *this /= t; }
/*
template <class T>
void set<T>::add(const T& t) { 
	iterator i=std::lower_bound(_begin(),_end(),t);
	if (*i != t) vector<T>::insert(i,t);
};

template <class T>
void set<T>::remove(const T& t) { 
	iterator i=std::find(_begin(),_end(),t);
	//iterator i=std::binary_search(begin(),end(),t);
	if (i!=_end()) erase(i);
};
*/

/*
// ///////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
template <class T> void set<T>::mxInit() { vector<T>::mxInit(); }
template <class T> bool set<T>::mxCheckValid(const mxArray* A) { return vector<T>::mxCheckValid(A); }
template <class T> void set<T>::mxSet(mxArray* A) { vector<T>::mxSet(A); }
//template <class T> void set<T>::mxCopy(mxArray* A) { vector<T>::mxCopy(A); }
template <class T> mxArray* set<T>::mxGet() { return vector<T>::mxGet(); }
template <class T> void set<T>::mxRelease() { vector<T>::mxRelease(); }
template <class T> void set<T>::mxDestroy() { vector<T>::mxDestroy(); }
template <class T> void set<T>::mxSwap(set<T>& v) { vector<T>::mxSwap( (vector<T>&) v ); }
*/

}       // namespace mex
#endif  // re-include
