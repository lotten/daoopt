#ifndef __VARIABLES_H
#define __VARIABLES_H

#include <assert.h>
#include <iostream>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <algorithm>

#include "mxObject.h"
#include "mxUtil.h"
#include "vector.h"

#ifndef MEX
typedef size_t mwSize;
#endif 
/*
"VarSet" class -- compatible with uint32 arrays of variable ids used in the matlab version
  Mainly stores variable ID list (sorted uint32) and dimensions (mwSize)
  The main C++ interface allows for set-theoretic operations (intersect, union, etc) and some calculations of interest

Some problems: creating variables without dimensions can cause problems in later functions...
Need to clean these up in the Factor class...

*/

namespace mex {

class Var {
private:
  /// Label of the variable (its unique ID)
  size_t  _label;
  /// Number of possible values
  size_t  _states;
public:
  /// Default constructor (creates a variable with label 0 and 0 states)
  Var() : _label(0), _states(0) {}
  /// Constructs a variable with a given label and number of states
  Var( size_t label, size_t states ) : _label(label), _states(states) {}

  /// Returns the label
  size_t label() const { return _label; }
  /// Returns reference to label (unsafe !!)
  //size_t& label() { return _label; }

  /// Returns the number of states
  size_t states() const { return _states; }
  /// Returns reference to number of states (unsafe !!)
  //size_t& states() { return _states; }

	bool operator< (const Var& v) const {return (_label< v._label);}
	bool operator<=(const Var& v) const {return (_label<=v._label);}
	bool operator> (const Var& v) const {return (_label> v._label);}
	bool operator>=(const Var& v) const {return (_label>=v._label);}
	bool operator==(const Var& v) const {return (_label==v._label);}
	bool operator!=(const Var& v) const {return (_label!=v._label);}

	operator size_t() const { return _label; }	// !! added 
};	

typedef mex::vector<mex::midx<uint32_t> > VarOrder;

class VarSet : public virtual mxObject {
 public:
	typedef midx<uint32_t> vindex;   // variable ID #s
	typedef mwSize         vsize;    // dimension (cardinality) of variables

	class const_iterator : public std::iterator<std::bidirectional_iterator_tag,Var> {
		private:
			size_t i_;
			VarSet const* vs_;
			mutable Var tmp;
		public:
			// should use iterators to const key type !!!
			const_iterator(VarSet const* vs, size_t i) : tmp() { vs_=vs; i_=i; }
			const Var operator*() const { return Var(vs_->v_[i_],vs_->d_[i_]); }
			const Var* operator->() const { tmp=Var(vs_->v_[i_],vs_->d_[i_]); return &tmp; }  //!!! hacky
			const_iterator& operator++(void) { ++i_; return *this;};
			const_iterator operator++(int)  { ++(*this); return const_iterator(vs_,i_-1);};
			const_iterator& operator--(void) { --i_; return *this;};
			const_iterator operator--(int)  { --(*this); return const_iterator(vs_,i_+1);};
			bool operator==(const_iterator ci) { return (vs_==ci.vs_ && i_==ci.i_); };
			bool operator!=(const_iterator ci) { return !(*this==ci); };
	};
	typedef const_iterator const_reverse_iterator;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructors, Destructor 
/////////////////////////////////////////////////////////////////////////////////////////////////////////
	VarSet(void)                        : v_(), dLocal(), d_(NULL)                 { }  
	VarSet(const VarSet& vs)            : v_(vs.v_), dLocal(vs.d_,vs.d_+vs.size()) { d_=&dLocal[0]; }
	VarSet(const Var& v)                : v_(1,v.label()), dLocal(1,v.states())    { d_=&dLocal[0]; }  
	VarSet(const Var& v, const Var& v2) : v_(2), dLocal(2) { 
		if (v.label()<v2.label())      { v_[0]=v.label(); dLocal[0]=v.states(); v_[1]=v2.label(); dLocal[1]=v2.states(); }
		else if (v2.label()<v.label()) { v_[1]=v.label(); dLocal[1]=v.states(); v_[0]=v2.label(); dLocal[0]=v2.states(); }
		else                           { v_[0]=v.label(); dLocal[0]=v.states(); v_.resize(1); dLocal.resize(1); }
	}  
	template<typename iter>
	VarSet(iter B,iter E, size_t size_hint=0) : v_(), dLocal() {
		v_.reserve(size_hint); dLocal.reserve(size_hint); d_=&dLocal[0];
		for (;B!=E;++B) *this|=*B; 
	}
	// Destructor /////////////////////////
	~VarSet(void) { }

	// Copy, Swap /////////////////////////
	VarSet& operator= (const VarSet& B) {
		v_=B.v_; dLocal=vector<vsize>(B.d_,B.d_+B.size()); d_=&dLocal[0];
  	return *this;
	}

	void swap(VarSet& v) { v_.swap(v.v_); dLocal.swap(v.dLocal); std::swap(d_,v.d_); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors
/////////////////////////////////////////////////////////////////////////////////////////////////////////
  const size_t   size()         const { return v_.size();  };   // # of variables
  const size_t   nvar()         const { return v_.size();  };   // # of variables
  const vsize*   dims()         const { return d_;  };   				// dimensions of variables
	size_t         nrStates()     const { size_t ns=1; for (size_t i=0;i<size();++i) ns*=d_[i]; return (ns==0)?1:ns; }
  const Var operator[] (size_t c) const { return Var(v_[c],d_[c]); }; // index into variables
  //void setDimsFromGlobal(const vsize* dGlobal);         // set dims from a global vector (!!!)
  //void setDimsFromGlobal(const VarSet& Global);       // set dims from a global object

   // Tests for equality and lexicographical order ///
	bool operator==(const VarSet& B) const { return (size()==B.size())&&std::equal(v_.begin(),v_.end(),B.v_.begin()); }
  bool operator!=(const VarSet& B) const { return !(*this==B); }
  bool operator< (const VarSet& t) const { return std::lexicographical_compare(this->begin(),this->end(),t.begin(),t.end()); }
  bool operator<=(const VarSet& t) const { return (*this==t || *this<t); }
  bool operator> (const VarSet& t) const { return !(*this<=t); }
  bool operator>=(const VarSet& t) const { return !(*this>t); }
	

	const_iterator         begin()  const { return const_iterator(this,0); };
	const_iterator         end()    const { return const_iterator(this,size()); };
	const_reverse_iterator rbegin() const { return const_iterator(this,size()-1); };
	const_reverse_iterator rend()   const { return const_iterator(this,size_t(-1)); };
	

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operators:  union (+,|), setdiff (-,/), intersection (&), xor (^)
/////////////////////////////////////////////////////////////////////////////////////////////////////////
  //VarSet  operator+  ( const VarSet& B ) const;                      // union
  VarSet& operator+= ( const VarSet& B ) { return (*this=*this+B); } // 
  VarSet  operator|  ( const VarSet& B ) const	{ return *this+B; }  // union (also)
  VarSet& operator|= ( const VarSet& B ) { return (*this=*this|B); } // 

  //VarSet  operator- (const VarSet& B) const;                         // set-diff
  //VarSet& operator-= ( const VarSet& B ) { return (*this=*this-B); } // 
  VarSet  operator/ ( const VarSet& B ) const	{ return *this-B; }    // set-diff (also)
  //VarSet& operator/= ( const VarSet& B ) { return (*this=*this/B); } // 
  VarSet& operator/= ( const VarSet& B ) { return (*this-=B); } // 

  //VarSet  operator& (const VarSet& B) const;                         // intersection
  VarSet& operator&= ( const VarSet& B ) { return (*this =(*this & B)); }	// 

  //VarSet  operator^ ( const VarSet& B ) const;                       // set-symmetric diff (xor)
  VarSet& operator^= ( const VarSet& B ) { return (*this=*this^B); } // 

// Operators on single-element arguments
  VarSet  operator+  ( const Var& B ) const { return *this+ VarSet(B); }
  VarSet& operator+= ( const Var& B )       { return *this+=VarSet(B); }
  VarSet  operator|  ( const Var& B ) const { return *this| VarSet(B); }
  VarSet& operator|= ( const Var& B )       { return *this|=VarSet(B); }
  VarSet  operator-  ( const Var& B ) const { return *this- VarSet(B); }
  VarSet& operator-= ( const Var& B )       { return *this-=VarSet(B); }
  VarSet  operator/  ( const Var& B ) const { return *this/ VarSet(B); }
  VarSet& operator/= ( const Var& B )       { return *this/=VarSet(B); }
  VarSet  operator&  ( const Var& B ) const { return *this& VarSet(B); }
  VarSet& operator&= ( const Var& B )       { return *this&=VarSet(B); }
  VarSet  operator^  ( const Var& B ) const { return *this^ VarSet(B); }
  VarSet& operator^= ( const Var& B )       { return *this^=VarSet(B); }

	/*

	VarSet operator+ ( const VarSet& B ) const {
  	VarSet dest(size()+B.size());
    vector<vindex>::const_iterator vi=v_.begin(), vj=B.v_.begin(); vector<vindex>::iterator vk=dest.v_.begin();
		vector<vsize>::const_iterator  di=d_.begin(), dj=B.d_.begin(); vector<vsize>::iterator  dk=dest.dLocal.begin();
		for (;vi!v_.end() && vj!=B.v_.end();) {
			if      (*vi<*vj) { *dk++=*di++; *vk++=*vi++; }
			else if (*vj<*vi) { *dk++=*dj++; *vk++=*vj++; }
			else              { *dk++=(*di ? *di : *dj); di++; dj++; *vk++=*vi++; vj++; }
		}
		while (vi!=v_.end())   { *dk++=*di++; *vk++=*vi++; }
		while (vj!=B.v_.end()) { *dk++=*dj++; *vk++=*vj++; }
		size_t n=std::distance(dest.v_.begin(),vk); dest.v_.resize(n); dest.dLocal.resize(n);
  	return dest;
	}
	VarSet& operator-=( const VarSet& B ) {
		if (!isLocal) dLocal.reserve(size());
    vector<vindex>::const_iterator vi=v_.begin(), vj=B.v_.begin(); vector<vindex>::iterator vk=v_.begin();
		vector<vsize>::const_iterator  di=d_.begin(), dj=B.d_.begin(); vector<vsize>::iterator  dk=dLocal.begin();
		for (;vi!v_.end() && vj!=B.v_.end();) {
			if      (*vi<*vj) { *dk++=*di++; *vk++=*vi++; }
			else if (*vj<*vi) { dj++; vj++; }
			else              { di++; dj++; vi++; vj++; }
		}
		while (vi!=v_.end())   { *dk++=*di++; *vk++=*vi++; }
		size_t n=std::distance(dest.v_.begin(),vk); dest.v_.resize(n); dest.dLocal.resize(n); d_=&dLocal[0];
  	return *this;
	}
	VarSet& operator-=( const Var& B ) 
	  vector<vindex>::iterator vi=
	*/

	VarSet operator+ ( const VarSet& B ) const {
  	VarSet dest(size()+B.size());
  	size_t i,j,k;
  	for (i=0,j=0,k=0;i<size()&&j<B.size();) {
    	if (v_[i]==B.v_[j])     { dest.dLocal[k]=(d_[i]==0 ? B.d_[j] : d_[i]); dest.v_[k++]=  v_[i++]; j++; }
	  	else if (v_[i]>B.v_[j]) { dest.dLocal[k]=B.d_[j];                      dest.v_[k++]=B.v_[j++];      }
    	else                    { dest.dLocal[k]=  d_[i];                      dest.v_[k++]=  v_[i++];      }
  	}
  	while (i<size())   { dest.dLocal[k]=  d_[i]; dest.v_[k++]=  v_[i++]; }
  	while (j<B.size()) { dest.dLocal[k]=B.d_[j]; dest.v_[k++]=B.v_[j++]; }
  	dest.v_.resize(k); dest.dLocal.resize(k);
  	return dest;
	}

	VarSet operator- (const VarSet& B) const {
  	VarSet dest(size());
  	size_t i,j,k;
  	for (i=0,j=0,k=0;i<size()&&j<B.size();) {
    	if      (v_[i]< B.v_[j])  { dest.dLocal[k]=d_[i]; dest.v_[k++]=v_[i++]; }
    	else if (v_[i]==B.v_[j])  { i++; j++; }
    	else                      { j++;      }
  	}
  	while (i<size()) { dest.dLocal[k]=d_[i]; dest.v_[k++]=v_[i++]; }
  	dest.v_.resize(k); dest.dLocal.resize(k);
  	return dest;
	}

	VarSet& operator-=(const VarSet& B) {
  	VarSet& dest = *this;
  	size_t i,j,k;
  	for (i=0,j=0,k=0;i<size()&&j<B.size();) {
    	if      (v_[i]< B.v_[j])  { dest.dLocal[k]=d_[i]; dest.v_[k++]=v_[i++]; }
    	else if (v_[i]==B.v_[j])  { i++; j++; }
    	else                      { j++;      }
  	}
  	while (i<size()) { dest.dLocal[k]=d_[i]; dest.v_[k++]=v_[i++]; }
  	dest.v_.resize(k); dest.dLocal.resize(k);
  	return dest;
	}
	
	VarSet operator& (const VarSet& B) const {
  	VarSet dest( size()>B.size() ? B.size() : size() );
  	size_t i,j,k;
  	for (i=0,j=0,k=0;i<size()&&j<B.size();) {
    	if      (v_[i]< B.v_[j]) { i++; }
    	else if (v_[i]==B.v_[j]) { dest.dLocal[k]=(d_[i]==0 ? B.d_[j] : d_[i]); dest.v_[k++]=v_[i++]; j++; }
    	else                     { j++; }
  	}
  	dest.v_.resize(k); dest.dLocal.resize(k);
  	return dest;
	}
	
	VarSet operator^ ( const VarSet& B ) const {
  	VarSet dest(size()+B.size());
  	size_t i,j,k;
  	for (i=0,j=0,k=0;i<size()&&j<B.size();) {
    	if (v_[i]==B.v_[j])     { i++; j++; }
    	else if (v_[i]>B.v_[j]) { dest.dLocal[k]=B.d_[j]; dest.v_[k++] = B.v_[j++]; }
    	else                    { dest.dLocal[k]=  d_[i]; dest.v_[k++] =   v_[i++]; }
  	}
  	while (i <   size())      { dest.dLocal[k]=  d_[i]; dest.v_[k++] =   v_[i++]; }
  	while (j < B.size())      { dest.dLocal[k]=B.d_[j]; dest.v_[k++] = B.v_[j++]; }
  	dest.v_.resize(k); dest.dLocal.resize(k);
  	return dest;
	}

  VarSet& insert(const Var& v) { return *this+=v; }
	VarSet& erase(const Var& v)  { return *this-=v; }
	bool operator<< (const VarSet& S) const { return std::includes( S.v_.begin(),S.v_.end(), v_.begin(),v_.end() ); }
	bool operator>> (const VarSet& S) const { return std::includes( v_.begin(),v_.end(), S.v_.begin(),S.v_.end() ); }
	bool intersects(const VarSet& S) const { return (*this & S).size() > 0; }
	bool contains(const Var& v) const { return std::binary_search( v_.begin(),v_.end(), v.label() ); }


  friend std::ostream& operator<< (std::ostream &os, mex::VarSet const& v) { 
    os<<"{";
	  if (v.nvar()!=0) { os << v.v_[0]; for (size_t i=1;i<v.nvar();i++) os <<","<< v.v_[i]; }
    os<<"}";
		return os;
  }

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Matlab interface functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef MEX  
bool mxCheckValid(const mxArray* M) { return v_.mxCheckValid(M); }
void mxSet(mxArray* M) { mxSet(M,NULL); }
void mxSet(mxArray* M, const vsize* dnew) { 
	v_.mxSet(M); 
	if (dnew) d_=dnew; else { dLocal.resize(size()); d_=&dLocal[0]; std::fill(dLocal.begin(),dLocal.end(),0); }
}
mxArray* mxGet() { return v_.mxGet(); }
void mxRelease() { v_.mxRelease(); if (!isLocal()) { dLocal=vector<vsize>(d_,d_+size()); d_=&dLocal[0]; } }
void mxDestroy() { v_.mxDestroy(); if (!isLocal()) { dLocal=vector<vsize>(d_,d_+size()); d_=&dLocal[0]; } }
VarSet& mxSwap(VarSet& v) { v_.mxSwap(v.v_); dLocal.swap(v.dLocal); std::swap(d_,v.d_); std::swap(M_,v.M_); }
#endif

protected:
	vector<vindex> v_;	// variable IDs
	vector<vsize> dLocal;	// non-const version (equals d_) if we allocated the dimensions ourselves
  const vsize* d_;			// dimensions of the variables (potentially from Matlab)

	bool isLocal() { return (d_==&dLocal[0]); }		// check for local storage of dimensions
	explicit VarSet(const size_t Nv) : v_(Nv), dLocal(Nv) { d_=&dLocal[0]; }	// "reserved memory" constructor

};



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
void VarSet::setDimsFromGlobal(const VarSet& Global) { 
  assert(isLocal());
  VarSet tmp = Global & *this;
	assert(tmp.size()==size());
	std::copy(tmp.d_,tmp.d_+tmp.size(),dLocal.begin());
}

void VarSet::setDimsFromGlobal(const vsize* Global) { 
  assert(isLocal());
  for (size_t i=0;i<size();++i) dLocal[i]=Global[v_[i]];
}
*/

template <class MapType>
inline size_t sub2ind(const VarSet& vs, const MapType& val) {
  if (vs.nrStates()==0) throw std::runtime_error("sub2ind with uninitialized dimensions");
  size_t i=0,m=1;
  for (size_t v=0;v<vs.size();++v) {
    i += m*(size_t)val[vs[v]]; m*=vs[v].states();
  } 
  return i;
}
template <class MapType>
inline size_t sub2ind(const VarSet& vs, MapType& val) {
  if (vs.nrStates()==0) throw std::runtime_error("sub2ind with uninitialized dimensions");
  size_t i=0,m=1;
  for (size_t v=0;v<vs.size();++v) {
    i += m*(size_t)val[vs[v]]; m*=vs[v].states();
  } 
  return i;
}
template <class MapType>
inline void ind2sub(const VarSet& vs, size_t i, MapType& val) {
  if (vs.nrStates()==0) throw std::runtime_error("ind2sub with uninitialized dimensions");
  for (size_t v=0; v<vs.size(); ++v) {
    size_t rem = i%vs[v].states();
    val[vs[v]] = rem; i-=rem; i/=vs[v].states();
  }
  return val;
}

inline vector<uint32_t> ind2sub(const VarSet& vs, size_t i) {
  if (vs.nrStates()==0) throw std::runtime_error("ind2sub with uninitialized dimensions");
  vector<uint32_t> val; val.resize(vs.size());
  for (size_t v=0; v<vs.size(); ++v) {
    val[v] = i%vs[v].states(); i-=val[v]; i/=vs[v].states();
  }
  return val;
}

}       // namespace mex





/*
std::ostream& operator<< (std::ostream &os, mex::VarSet const& v) {
//  os<<"{";
//  for (mex::VarSet::size_t i=0;i<v.nvar();i++) os << ( (i) ? "," : "" ) << v[i];
//  os<<"}";
  v.display(os);
	return os;
}
*/

#endif  // re-include
