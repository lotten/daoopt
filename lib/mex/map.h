#ifndef __MEX_MAP_H
#define __MEX_MAP_H

#include <assert.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <stdlib.h>
#include <stdint.h>

#include <map>
#include "mxObject.h"
#include "rbtree.h"

/*
 * mex::map<Key,Value>      : templated class for map (unique mappings) with matlab containers
 * mex::multimap<Key,Value> : templated class for multimap (non-unique mappings) with matlab containers
*/

namespace mex {

#ifndef MEX
template <class Key, class T>
class map : public virtual mxObject, public std::map<Key,T> { 
public:
	bool checkValid() { return true; };
};

template <class Key, class T>
class multimap : public virtual mxObject, public std::multimap<Key,T> { 
public:
	bool checkValid() { return true; };
};


#else
template <class Key, class T>
class map : public virtual mxObject, protected rbtree<Key,T> {
public:
	// Type definitions //////////////////////////////
	typedef Key            key_type;
	typedef T              mapped_type;
	typedef typename mex::rbtree<Key,T>::value_type value_type;			// map, multimap only
  typedef size_t         size_type;
  typedef ptrdiff_t      difference_type;

  typedef typename mex::rbtree<Key,T>::iterator                     iterator;
  typedef typename mex::rbtree<Key,T>::const_iterator         const_iterator;
  typedef typename mex::rbtree<Key,T>::iterator             reverse_iterator;
  typedef typename mex::rbtree<Key,T>::const_iterator const_reverse_iterator;

  // Basic checks ////////////////////////////////
	using rbtree<Key,T>::size;
	using rbtree<Key,T>::max_size;
	using rbtree<Key,T>::empty;

  // Constructors  /////////////////////////////////////////////////
  explicit map() : rbtree<Key,T>() { }
  template<class InputIterator> map(InputIterator first, InputIterator last) : rbtree<Key,T>(first,last) { }
	map<Key,T>& operator=(map<Key,T> &mp) { rbtree<Key,T>::operator=((rbtree<Key,T>&)mp); return *this;} 

	// Setting, changing sizes //////////////////////
	using rbtree<Key,T>::clear;
  void swap(map<Key,T> &mp) { rbtree<Key,T>::swap((rbtree<Key,T>&) mp); }

  // Iterators ////////////////////////////////////
	using rbtree<Key,T>::begin;
	using rbtree<Key,T>::end;
	using rbtree<Key,T>::rbegin;
	using rbtree<Key,T>::rend;
	
  // insertion & removal ////////////////////
	using rbtree<Key,T>::operator[];
	std::pair<iterator,bool> insert(const value_type& p) { return rbtree<Key,T>::insertUnique(p); }
	iterator insert(iterator position, const value_type& p) { return rbtree<Key,T>::insertUnique(position, p); }
	template<class InputIterator> void insert(InputIterator first, InputIterator last) {
		iterator i=iterator(this,this->__root()); 
		while (first!=last) { i=insert(i,*first); ++first; }
	}
	using rbtree<Key,T>::erase;
	size_type erase(const key_type& x) { return eraseUnique(x); }

	// !!! missing key_comp, value_comp

	// Searching and intervals ////////////////////////////////////
	using rbtree<Key,T>::find;
	size_type count ( const key_type& x ) const { return (find(x)==end()) ? 0 : 1; }

	using rbtree<Key,T>::lower_bound;
	using rbtree<Key,T>::upper_bound;
	using rbtree<Key,T>::equal_range;

  // MEX functionality ////////////////////////////////
  using rbtree<Key,T>::mxInit;	
  using rbtree<Key,T>::mxCheckValid;
  using rbtree<Key,T>::mxSet;
  using rbtree<Key,T>::mxGet;
  using rbtree<Key,T>::mxRelease;
  using rbtree<Key,T>::mxDestroy;
	virtual void mxSwap(map<Key,T>& mp) { rbtree<Key,T>::mxSwap((rbtree<Key,T>&) mp); }

	using rbtree<Key,T>::checkValid;	
};



template <class Key, class T>
class multimap : public virtual mxObject, protected rbtree<Key,T> {
public:
	// Type definitions //////////////////////////////
	typedef Key            key_type;
	typedef T              mapped_type;
	typedef typename mex::rbtree<Key,T>::value_type value_type;			// map, multimap only
  typedef size_t         size_type;
  typedef ptrdiff_t      difference_type;

  typedef typename mex::rbtree<Key,T>::iterator                     iterator;
  typedef typename mex::rbtree<Key,T>::const_iterator         const_iterator;
  typedef typename mex::rbtree<Key,T>::iterator             reverse_iterator;
  typedef typename mex::rbtree<Key,T>::const_iterator const_reverse_iterator;

  // Basic checks ////////////////////////////////
	using rbtree<Key,T>::size;
	using rbtree<Key,T>::max_size;
	using rbtree<Key,T>::empty;

  // Constructors  /////////////////////////////////////////////////
  explicit multimap() : rbtree<Key,T>() { }
  template<class InputIterator> multimap(InputIterator first, InputIterator last) : rbtree<Key,T>(first,last) { }
	multimap<Key,T>& operator=(multimap<Key,T> &mp) { rbtree<Key,T>::operator=((rbtree<Key,T>&)mp); return *this;} 

	// Setting, changing sizes //////////////////////
	using rbtree<Key,T>::clear;
  void swap(multimap<Key,T> &mp) { rbtree<Key,T>::swap((rbtree<Key,T>&) mp); }

  // Iterators ////////////////////////////////////
	using rbtree<Key,T>::begin;
	using rbtree<Key,T>::end;
	using rbtree<Key,T>::rbegin;
	using rbtree<Key,T>::rend;
	
  // insertion & removal ////////////////////
	using rbtree<Key,T>::operator[];
	iterator insert(const value_type& p) { return rbtree<Key,T>::insertMulti(p); }
	iterator insert(iterator position, const value_type& p) { return rbtree<Key,T>::insertMulti(position, p); }
	template<class InputIterator> void insert(InputIterator first, InputIterator last) {
		iterator i=iterator(this,this->__root()); 
		while (first!=last) { i=insert(i,*first); ++first; }
	}
	using rbtree<Key,T>::erase;
	size_type erase(const key_type& x) { return eraseMulti(x); }

	// !!! missing key_comp, value_comp

	// Searching and intervals ////////////////////////////////////
	using rbtree<Key,T>::find;
	using rbtree<Key,T>::count;

	using rbtree<Key,T>::lower_bound;
	using rbtree<Key,T>::upper_bound;
	using rbtree<Key,T>::equal_range;

  // MEX functionality ////////////////////////////////
  using rbtree<Key,T>::mxInit;	
  using rbtree<Key,T>::mxCheckValid;
  using rbtree<Key,T>::mxSet;
  using rbtree<Key,T>::mxGet;
  using rbtree<Key,T>::mxRelease;
  using rbtree<Key,T>::mxDestroy;
	virtual void mxSwap(multimap<Key,T>& mp) { rbtree<Key,T>::mxSwap((rbtree<Key,T>&) mp); }

	using rbtree<Key,T>::checkValid;	
};
#endif // ifdef MEX


}       // namespace mex
#endif  // re-include

