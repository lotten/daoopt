// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_RBTREE_H
#define __MEX_RBTREE_H

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
 * mex::rbtree<Key,Value> : templated class for map/multimap (mappings) with matlab containers
*/

namespace mex {

#ifdef MEX
// Only need stand-alone implementation of red-black tree for matlab interfacing

struct rbtreeNode {
	uint32_t parent;
	uint32_t left, right;
	uint32_t red;
	explicit rbtreeNode() { parent=left=right=0; red=false; }
	explicit rbtreeNode(uint32_t p, uint32_t l, uint32_t r, bool rd) {
		parent=p; left=l; right=r; red=rd;
	}
};
template<>
struct mexable_traits<rbtreeNode> {
  typedef rbtreeNode base;
  typedef uint32_t   mxbase;
  static const int   mxsize=4;
};



template <class Key, class T=void*>
class rbtree : public virtual mxObject {
public:
	// Type definitions //////////////////////////////
  typedef uint32_t       nodeid;
	typedef uint8_t        logical;
	typedef Key            key_type;
	typedef T              mapped_type;
	typedef std::pair<const Key,T> value_type;			// map, multimap only
  typedef size_t         size_type;
  typedef ptrdiff_t      difference_type;

	// Iterator definitions //////////////////////////
	struct rbRef {					// helper class for accessing split data storage
		const Key& first;
		T& second;
		rbRef(const Key& f,T& s) : first(f), second(s) { }
	};
  class const_iterator {
		friend class rbtree<Key,T>;
    public:
		  typedef std::bidirectional_iterator_tag iterator_category;
			//typedef rbtree<Key,T>::value_type value_type;
			typedef rbRef value_type;
			typedef rbtree<Key,T>::difference_type difference_type;
			typedef rbRef* pointer;
			typedef rbRef  reference;
			const_iterator() : mp(NULL), n(0), data(NULL) { }
			const_iterator(const rbtree<Key,T>* m, nodeid N) : mp(m), n(N), data(NULL) { }
			~const_iterator() { if (data) delete data; }
      const_iterator& operator++(void) { return next(1); }
      const_iterator  operator++(int) { iterator r(mp,n); ++(*this); return r; }
      const_iterator& operator--(void) { return next(0); }
      const_iterator  operator--(int)  { iterator r(mp,n); --(*this); return r; }
			bool operator==(const const_iterator& i) const { return (mp==i.mp && n==i.n); }
			bool operator!=(const const_iterator& i) const { return !(*this == i); }
			//const value_type operator*() const { return std::make_pair(mp->KEY(n),mp->VAL(n)); } // "simpler" non-reference version
			const rbRef operator*() const { return rbRef(mp->KEY(n),mp->VAL(n)); }
			const rbRef* operator->() const { if (data) delete data; data = new rbRef(mp->KEY(n),mp->VAL(n)); return data; }
      rbtree<Key,T>::nodeid n;
    private:
			rbRef* data;
			const_iterator& next(logical dir) {		// move to the next (left or right) node
				if (mp->LINK(n,dir)!=NONE) {
				  n=mp->LINK(n,dir); while (n!=NONE && mp->LINK(n,!dir)!=NONE) n=mp->LINK(n,!dir);
				} else {
					nodeid prev=n; n=mp->PARENT(n);
					while (n!=NONE && mp->LINK(n,dir)==prev) { prev=n; n=mp->PARENT(n); }
				}
				return *this;
			}
      const rbtree<Key,T>* mp;
   };
   class iterator {
		 friend class rbtree<Key,T>;
     public:
		  typedef std::bidirectional_iterator_tag iterator_category;
			typedef rbRef value_type;
			typedef rbtree<Key,T>::difference_type difference_type;
			typedef rbRef* pointer;
			typedef rbRef  reference;
			iterator() : mp(NULL), n(0), data(NULL) { }
			iterator(rbtree<Key,T>* m, nodeid N) : mp(m), n(N), data(NULL) { }
			~iterator() { if (data) delete data; }
			operator const_iterator() { return const_iterator(mp,n); }
      iterator& operator++(void) { return next(1); }
      iterator  operator++(int) { iterator r(mp,n); ++(*this); return r; }
      iterator& operator--(void) { return next(0); }
      iterator  operator--(int)  { iterator r(mp,n); --(*this); return r; }
			bool operator==(const iterator& i) const { return (mp==i.mp && n==i.n); }
			bool operator!=(const iterator& i) const { return !(*this == i); }
			rbRef operator*()   { return rbRef(mp->KEY(n),mp->VAL(n)); }
			rbRef* operator->() { if (data) delete data; data = new rbRef(mp->KEY(n),mp->VAL(n)); return data; }
       rbtree<Key,T>::nodeid n;
     private:
			rbRef* data;
			 iterator& next(logical dir) {		// move to the next (left or right) node
				 if (mp->LINK(n,dir)!=NONE) {
				  n=mp->LINK(n,dir); while (n!=NONE && mp->LINK(n,!dir)!=NONE) n=mp->LINK(n,!dir);
				 } else {
					nodeid prev=n; n=mp->PARENT(n);
					while (n!=NONE && mp->LINK(n,dir)==prev) { prev=n; n=mp->PARENT(n); }
				 }
				 return *this;
			 }
       rbtree<Key,T>* mp;
	 };
   typedef iterator       reverse_iterator;
   typedef const_iterator const_reverse_iterator;

   // Basic checks ////////////////////////////////
   size_type size()     const { return __size(); }
   size_type max_size() const { return std::numeric_limits<uint32_t>::max()-2; }
   //size_type max_size() const { return _key.max_size(); }
   bool empty()         const { return (size()==0); }

  // Constructors  /////////////////////////////////////////////////
  explicit rbtree();
  template<class InputIterator> rbtree(InputIterator first, InputIterator last);
	~rbtree();
	rbtree<Key,T>& operator=(rbtree<Key,T> &mp);

	// Setting, changing sizes //////////////////////
  void clear() { __root()=NONE; if (size()>0) __avail()=2; for (nodeid n=2;n<_node.size()-1;n++) LINK(n,0)=n+1; LINK(_node.size()-1,0)=NONE; }
  void swap(rbtree<Key,T> &mp) { _key.swap(mp.key); _value.swap(mp.value); _node.swap(mp.node); }

  // Iterators ////////////////////////////////////
        iterator begin()                { return       iterator(this,__first()); }
  const_iterator begin() const          { return const_iterator(this,__first()); }
        iterator end()                  { return       iterator(this, NONE); }
  const_iterator end() const            { return const_iterator(this, NONE); }
        reverse_iterator rbegin()       { return       iterator(this,__last()); }
  const_reverse_iterator rbegin() const { return const_iterator(this,__last()); }
        reverse_iterator rend()         { return       iterator(this, NONE); }
  const_reverse_iterator rend() const   { return const_iterator(this, NONE); }

  // insertion & removal ////////////////////
	T& operator[](const key_type& x) { iterator i=_insertDown(std::make_pair(x,T()),false).first; return VAL(i.n); }

	iterator insertMulti(const value_type& p) { return _insertDown(p,true).first; }
  std::pair<iterator,bool> insertUnique(const value_type& p) { return _insertDown(p,false); }
	iterator insertMulti(iterator position, const value_type& p) { return insertMulti(p); } // !!! inefficient
	iterator insertUnique(iterator position, const value_type& p) { return insertUnique(p).first; } // !!! inefficient
	template<class InputIterator> void insertMulti(InputIterator first, InputIterator last) {
		iterator i=iterator(this,__root()); 
		while (first!=last) { i=insertMulti(i,*first); ++first; }
	}
	template<class InputIterator> void insertUnique(InputIterator first, InputIterator last) {
		iterator i=iterator(this,__root()); 
		while (first!=last) { i=insertUnique(i,*first); ++first; }
	}
	void erase(iterator position)               { _removeUp(position.n); }
	size_type eraseUnique(const key_type& x)    { return _removeDown(x); }
	size_type eraseMulti(const key_type& x)     { size_type d,n=0; while (d=_removeDown(x)) n+=d;  return n; } //!!! inefficient
	void erase(iterator first, iterator last)   { while (first!=last) { _removeUp(first.n); ++first; } }

	// !!! missing key_comp, value_comp

	// Searching and intervals ////////////////////////////////////
	      iterator find ( const key_type& x )       { return       iterator(this, _find(x)); }
	const_iterator find ( const key_type& x ) const { return const_iterator(this, _find(x)); }

	size_type count ( const key_type& x ) const { 
	 	std::pair<const_iterator,const_iterator> eq = equal_range(x);
	 	return distance(eq.first,eq.second);
	}

	      iterator lower_bound ( const key_type& x )       { return       iterator(this,_lower_bound(x,__root())); }
	const_iterator lower_bound ( const key_type& x ) const { return const_iterator(this,_lower_bound(x,__root())); }
        iterator upper_bound ( const key_type& x )       { return       iterator(this,_upper_bound(x,__root())); }
  const_iterator upper_bound ( const key_type& x ) const { return const_iterator(this,_upper_bound(x,__root())); }

  std::pair<iterator,iterator> equal_range ( const key_type& x ) { 
		std::pair<nodeid,nodeid> eq=_equal_range(x);
		return std::make_pair(iterator(this,eq.first),iterator(this,eq.second));
	}
  std::pair<const_iterator,const_iterator> equal_range ( const key_type& x ) const {
		std::pair<nodeid,nodeid> eq=_equal_range(x);
		return std::make_pair(const_iterator(this,eq.first),const_iterator(this,eq.second));
	}

   // MEX functionality ////////////////////////////////
  //virtual void  mxInit();               // initialize any mex-related data structures, etc (private)
  virtual bool  mxCheckValid(const mxArray*); // check validity of matlab object for set<T>
  virtual void  mxSet(mxArray* A);      // associate with A by reference to data
  virtual mxArray*  mxGet();            // get a pointer to the matlab object wrapper (creating if required)
  virtual void  mxRelease();            // disassociate with a matlab object wrapper, if we have one
  virtual void  mxDestroy();            // disassociate & destroy matlab object
  virtual void  mxSwap(rbtree<Key,T>&);    // exchange matlab object identities with another set

  void checkValid(bool unique=false) { checkRBConditions(__root(),unique); }

private:

	// !!! Note: accessor definitions depend on rend() and end() being iterators with node "NONE"

  nodeid _find(const key_type& x) const {
  	nodeid q=__root();
  	while (q!=NONE && KEY(q)!=x) q=LINK(q, (KEY(q)<x));
  	return q;
	}

  nodeid _lower_bound(const key_type& x, nodeid start) const {
  	nodeid q=start, p=NONE;	// save elements >= and descend from "start", prefering left
  	while (q!=NONE) { if (KEY(q)>=x) p=q; q=LINK(q, KEY(q)<x); }
  	return p;
	}

  nodeid _upper_bound(const key_type& x, nodeid start) const {
  	nodeid q=start, p=NONE;	// save elements > and descend from "start", prefering right
  	while (q!=NONE) { if (KEY(q)>x) p=q; q=LINK(q, KEY(q)<=x); }
  	return p;
	}

	std::pair<nodeid,nodeid> _equal_range(const key_type& x) const {
		return std::make_pair( _lower_bound(x,__root()),_upper_bound(x,__root()) );
		/* // Trying to be smarter...
		nodeid first, last, r; first = last = r = _find(x);
		if (r!=NONE) { 
			if (r!=__root()) r=PARENT(r); 
			first=_lower_bound(x,r); 
			last=_upper_bound(x,r); 
			if (last==NONE && KEY(__last())!=x) last=(++const_iterator(this,first)).n;
		}
		return std::make_pair(first,last);
		*/
	}

	std::pair<iterator,bool>  _insertDown(const value_type& p, bool unique);
	size_type _removeDown(const key_type& );
	void _removeUp(nodeid);

	//iterator  _insertUp(iterator hint, const value_type& p, bool& create);
	//size_type _removeUp(nodeid n);



protected:
  vector<T>          _value;		// rbtree, multirbtree only
  vector<key_type>   _key;
  //vector<nodeid>     parent,left,right;
  //vector<logical>    red;
	vector<rbtreeNode> _node;
	//nodeid             _root;
	static const nodeid  NONE=0;

	//nodeid             _avail;
	//nodeid             _size;
	//nodeid _first, _last;	// point to begin & rend; update with ++ or -- when adding/removing elements

/*TODO 
 * Create node[0] (NONE) with pointers to root and avail
 *        node[1] (?)    with pointers to first and last
 * size can be stored in node[1] as parent
 * key, value are length N, node is length N+2
*/
/*
	 void init() {
		__root()=NONE; avail=NONE;
		_first = _last = iterator(this,root);
		//if (parent.size()) {
		//	LINK(0,1)=NONE; 
		//}
	 }
*/
 private:
	 // Simplifying accessor functions:
	 nodeid&     PARENT(nodeid n) { return _node[n].parent; }
	 nodeid&     LINK(nodeid n, bool dir) { return dir ? _node[n].left : _node[n].right; }
	 uint32_t&   RED(nodeid n) { return _node[n].red; }
	 key_type&   KEY(nodeid n) { return _key[n-2]; }
	 T&          VAL(nodeid n) { return _value[n-2]; }
	 bool isRed(nodeid n)   { return (n!=NONE) && (_node[n].red==1); }

	 const nodeid&     PARENT(nodeid n) const { return _node[n].parent; }
	 const nodeid&     LINK(nodeid n, bool dir) const { return dir ? _node[n].left : _node[n].right; }
	 const uint32_t&   RED(nodeid n) const { return _node[n].red; }
	 const key_type&   KEY(nodeid n) const { return _key[n-2]; }
	 const T&          VAL(nodeid n) const { return _value[n-2]; }

	      nodeid& __size() { return _node[1].parent; }
	const nodeid& __size() const { return _node[1].parent; }
	//nodeid& __root() { return _root; }
	nodeid& __root() { return LINK(NONE,1); }
	const nodeid& __root() const { return LINK(NONE,1); }
	nodeid& __avail() { return LINK(NONE,0); }
	      nodeid& __first()       { return _node[1].left; }
	const nodeid& __first() const { return _node[1].left; }
	      nodeid& __last()        { return _node[1].right; }
	const nodeid& __last()  const { return _node[1].right; }

   // Basic red-black operations:
	 nodeid rotate1(nodeid, bool);
	 nodeid rotate2(nodeid, bool);
	 bool checkRBConditions(nodeid,bool);

	 nodeid makeNode(const key_type& key, const T& val);
	 void removeNode(nodeid);
	 void swapNodes(nodeid, nodeid);

};


template<class Key,class T>
rbtree<Key,T>::rbtree() : _value(),_key(),_node() { 
	mxInit();
	_node.push_back( rbtreeNode() );	// create stub pointer to root and vacant list
	_node.push_back( rbtreeNode() );	// create stub pointer to first and last
	__root() = NONE; 
	__avail()=NONE;
	__first()=NONE; __last()=NONE;
	__size()=0;
}

template<class Key,class T>
rbtree<Key,T>::~rbtree() {
}

// ///////////////////////////////////////////////////////////////////////////////////////////
// Internal Red/Black functions
//////////////////////////////////////////////////////////////////////////////////////////////


template<class Key,class T>
typename rbtree<Key,T>::nodeid rbtree<Key,T>::makeNode(const key_type& x, const T& t) { 
	nodeid ret;
	if (__avail() != NONE) {						// if we have some empty slots,
		ret = __avail();									//   take one of them and
		__avail() = LINK(__avail(),0);		//   move "available" pointer down the linked list
		LINK(ret,0)=NONE;
		KEY(ret)=x; VAL(ret)=t; RED(ret)=1;		// color new nodes red
	} else { // avail = NONE;
	  _value.push_back( t );				// otherwise, we need to add an position
	  _key.push_back( x );	  			//   for this node
		_node.push_back( rbtreeNode(NONE,NONE,NONE,1) );	// color new nodes red
		ret = _node.size()-1;	// !!!
	}
	__size()++;
	return ret;
}

template<class Key,class T>
void rbtree<Key,T>::removeNode(nodeid n) { 
  VAL(n)=T();							// empty the data element to default
	KEY(n)=Key();							// and same for the key element
	_node[n] = rbtreeNode(NONE,NONE,NONE,false);
	LINK(n,0)=__avail();					// and add the node to the "available"
	__avail() = n;										//   linked list
	__size()--;
}

template<class Key,class T>
void rbtree<Key,T>::swapNodes(nodeid a, nodeid b) { 	// assumes b is "lower than" a
	rbtreeNode tmp=_node[a]; _node[a]=_node[b]; _node[b]=tmp;
	if (LINK(b,0)==b) LINK(b,0)=a; 								// make sure if a,b were neighbors we fix pointers
	if (LINK(b,1)==b) LINK(b,1)=a; 
	nodeid p=PARENT(b); LINK(p,LINK(p,1)==a)=b;   // fix up parentage pointers for b
	if (LINK(b,0)!=NONE) PARENT(LINK(b,0))=b;
	if (LINK(b,1)!=NONE) PARENT(LINK(b,1))=b;
	p=PARENT(a); LINK(p,LINK(p,1)==b)=a;   				// do the same for a
	if (LINK(a,0)!=NONE) PARENT(LINK(a,0))=a;
	if (LINK(a,1)!=NONE) PARENT(LINK(a,1))=a;
}
////////////////////////////////////////////////////////////////////////////////
// Rotate1 (dir=right)            Rotate2 (dir=right)
//     R          S						         R               R                 D     
//   S   X  =>  Y   R               A     B  =>     D      B   =>     A     R  
//  Y Z            Z X            C   D           A   F              C E   F B
//                                   E F         C E                        
//
// note: does not fix links to/from parents of root R; returns id of new root (S or D)
//
template<class Key,class T>
typename rbtree<Key,T>::nodeid rbtree<Key,T>::rotate1(nodeid root, bool dir) {
  nodeid save = LINK(root,!dir);	// (for "dir"="right") save our "left" child
	LINK(root,!dir) = LINK(save,dir);	// grab its right child as our new left child
	PARENT(LINK(save,dir))=root;
	LINK(save,dir) = root;						// and move ourselves into its old slot
	PARENT(save)=PARENT(root); PARENT(root)=save;
	RED(root)=1;
	RED(save)=0;
	return save;
}

template<class Key,class T>
typename rbtree<Key,T>::nodeid rbtree<Key,T>::rotate2(nodeid root, bool dir) {
	LINK(root,!dir) = rotate1(LINK(root,!dir), !dir);
	return rotate1(root,dir);
}

template<class Key,class T>
bool rbtree<Key,T>::checkRBConditions(nodeid root, bool unique=false) {
  int lh, rh;
	if (root == NONE) return 1;
	else {
    //std::cout<<root<<" {"<<KEY(root)<<":"<<VAL(root)<<"} ("<<PARENT(root)<<";"<<LINK(root,0)<<","<<LINK(root,1)<<") ["<<isRed(root)<<"]\n";
		nodeid ln=LINK(root,0), rn=LINK(root,1);

		if ((ln!=NONE && PARENT(ln)!=root)||(rn!=NONE && PARENT(rn)!=root)) { 
			std::cout<<"Parent error\n";
		 	return false; 
		}
		// Check for consecutive red links
		if ( isRed(root) ) {
			if (isRed(ln) || isRed(rn)) {
				std::cout<<"Red violation\n";
				return false;
			}
		}
		lh = checkRBConditions(ln,unique);  // recurse subtrees
		rh = checkRBConditions(rn,unique);
    // Check for invalid ordering conditions
		if ( (ln!=NONE && KEY(ln) > KEY(root)) || (rn!=NONE && KEY(rn) < KEY(root))) {
			std::cout<<"Ordering violation\n";
			return false;
		}
		if ( unique && ((ln!=NONE && KEY(ln)==KEY(root)) || (rn!=NONE && KEY(rn)==KEY(root)))) {
			std::cout<<"Uniqueness violation\n";
			return false;
		}
		// Check for black height mismatch
		if (lh!=NONE && rh!=NONE && lh!=rh) {
			std::cout<<"Height violation\n";
			return false;
		}
		// Return count of black links
		if (ln!=NONE && rh!=NONE) return isRed(root) ? lh : lh+1;
		else return false;
	}
}

template<class Key,class T>		// !!! shouldn't overwrite value if found (?)
typename std::pair<typename rbtree<Key,T>::iterator,bool> rbtree<Key,T>::_insertDown(const value_type& p , bool unique) {
	const key_type& x=p.first; 
  const mapped_type& val=p.second;			// convert from std::pair form for now...
	bool created=false;
	nodeid q;
  if (__root() == NONE) { // empty tree
		q = makeNode(x,val);					// !!! note to be careful: makeNode can invalidate __root();
		__root() = q;									//  so separate assignment like so
		created = true;
		assert(__root()!=NONE);       // check for failure
	} else {
	  nodeid g,t;  	// grandparent & great-grandparent
		nodeid p;  		// parent
		bool dir=false, last;
		// set up helpers
		t=0; 				// point t at dummy root; LINK(0,1) = root node of rbtree
		g=p=NONE;
		q=__root();	// point q at root of rbtree
    // search down the tree
		for (;;) {
      if (q==NONE) { // insert new node at bottom
				q=makeNode(x,val);		
				LINK(p,dir)=q;  PARENT(q)=p;
				//if (q==0) return 0;  // check for failure
				created=true;
			}
			else if (isRed(LINK(q,0)) && isRed(LINK(q,1))) {
				// color flip
				RED(q)=1;
				RED(LINK(q,0))=0;
				RED(LINK(q,1))=0;
			}

			// Fix red violation
			if (isRed(q) && isRed(p)) {
				bool dir2 = (LINK(t,1)==g);
				if (q==LINK(p,last)) LINK(t,dir2)=rotate1(g,!last);
				else LINK(t,dir2)=rotate2(g,!last);
			}

			// Stop if found (or inserted if no uniqueness)
			if (created) break;
			if (!unique && KEY(q)==x) { VAL(q)=val; break; }

			last = dir;
			dir = (KEY(q)<x);

      // update helpers
			if (g!=NONE) t=g;
			g=p; p=q;
			q=LINK(q,dir);
		}
	}
  // make root black
	RED(__root())=0;

	if (__first()==NONE || x<=KEY(__first())) __first()=q;
	if (__last()==NONE || x>KEY(__last()))   __last() =q;
	return std::make_pair(iterator(this,q), created);
}

/*
 * Find the predecessor and successor of the node set
 * Remove node n, 
 *
*/

template<class Key,class T>
void rbtree<Key,T>::_removeUp( nodeid n ) {
	for (;;) {
		if (LINK(n,0)==NONE || LINK(n,1)==NONE) {
			nodeid save = LINK(n,LINK(n,0)==NONE);	// save non-null link
			LINK(PARENT(n), LINK(PARENT(n),1)==n) = save;
			if (save!=NONE) PARENT(save)=PARENT(n);
			if (isRed(n)) return;										// if somebody's red we can just change it and
			else if (isRed(save)) { RED(save)=0; return; }	// be done
			// else need to recurse upward and fix black violation
			n=save;
			break;
		} else {
			nodeid q = (--iterator(this,n)).n;// find predecessor of n and swap them; then
			swapNodes(n,q); 									// keep going downward from predecessor's position (now n)
		}
	}
	bool done=false;											// if we couldn't resolve a black violation earlier
	while (n!=NONE) { 										//   recurse upward and rotate to resolve it
		nodeid s,p=PARENT(n);
		bool dir=(LINK(p,1)==n);
		s=LINK(p,!dir);
		if (s!=NONE && !isRed(s)) {											// if our sibling is black
			if (!isRed(LINK(s,0)) && !isRed(LINK(s,1))) {	// and has two black children
				if (isRed(p)) done=true;										// we can make the sibling red w/o violation
				RED(p)=0; RED(s)=1;							// if the parent is also red we can change it and be done
			} else {
				bool wasRed = isRed(n);					// if one sibling is red we can fix this by
				if (isRed(LINK(s,!dir))) p=rotate1(p,dir);	// rotating the red into position
				else p=rotate2(p,dir);					// and then merging to enforce invariant property
				RED(p)=wasRed;
				RED(LINK(p,0))=RED(LINK(p,1))=0;// fix up colors and note that we're done
				done=true;
			}
		} else if (LINK(s,dir)!=NONE) {			// otherwise we have a red sibling; if it has a nearer
			nodeid r=LINK(s,dir);							//   child, call it r (otherwise keep going up...)
			if (!isRed(LINK(r,0)) && !isRed(LINK(r,1))) {
				p=rotate1(p,dir);								// if r's children are black we can rotate around p
				RED(LINK(LINK(p,dir),!dir))=1;	// moving one to our side; make it red & p black
			} else {													// if one r-child is red, 
				if (isRed(LINK(r,dir))) LINK(s,dir)=rotate1(r,!dir);	// might have to rotate around r
				p=rotate2(p,dir);								// now rotate to bring (black) p down on our side
				RED(LINK(s,dir))=0;							// and fix up the colorings
				RED(LINK(p,!dir))=1;
			}
			RED(p)=0; RED(LINK(p,dir))=0;			// fix up colors; if we did this we're done too
			done=true;
		}
		if (done) return;										// if we couldn't rotate a black our way,
		n=p;																//  we'll have to keep going upward and trying
	}
}


template<class Key,class T>
typename rbtree<Key,T>::size_type rbtree<Key,T>::_removeDown( const key_type& x ) {
	nodeid f=NONE;   // found item
  if (__root() != NONE) {
		nodeid q,p,g; // helpers
    bool dir=true;
		// set up helpers
		q=0; // point to dummy node;  LINK(0,true)=root of rbtree
		g=p=NONE;
		//Search for key and push a red down
		while (LINK(q,dir)!=NONE) {
			bool last=dir; g=p; p=q; q=LINK(q,dir);	// update helpers: g=p.parent, p.last=q,
			dir= (KEY(q)<x);												//   q moves closer to x, q.dir is closer still
			if (KEY(q)==x) f=q; 										// save found node for removal and continue to successor

			// push a red node down until we arrive at a leaf
			if (!isRed(q) && !isRed(LINK(q,dir))) {	// if q or its desired child are red, we're done
				if (isRed(LINK(q,!dir)))							//   if the other child is red, just rotate around q
					p = LINK(p,last) = rotate1(q,dir);
				else { //if (!isRed(LINK(q,!dir))) {		// redundant check???
					nodeid s=LINK(p,!last);					// if they're all black, check q's sibling
					if (s!=NONE && p!=0) {					// if its children are also black we can just color flip
						if (!isRed(LINK(s,0)) && !isRed(LINK(s,1))) {
							RED(p)=0; RED(s)=1; RED(q)=1;  // color flip
						} else {											// otherwise there's a red at our sibling's child
							bool dir2=(LINK(g,1)==p);		// figure out p's direction from g and rotate around p
							if (isRed(LINK(s,last))) LINK(g,dir2)=rotate2(p,last);	//   to move red into place
							else LINK(g,dir2)=rotate1(p,last);	
							
							RED(q)=RED(LINK(g,dir2))=1;		// ensure correct coloring: now q and g's
							RED(LINK(LINK(g,dir2),0))=0;	//   child (p's old position) are red and
							RED(LINK(LINK(g,dir2),1))=0;	//   the children of p's old position are black
						}
					}
				}
			}
		}
	  // replace and remove if found	
		if (f!=NONE) {
			if (f==__first()) __first()=(++iterator(this,__first())).n;	// update top and bottom if required
			if (f==__last())  __last() =(--iterator(this,__last())).n;
			// now move q to f's place
			LINK(p, (LINK(p,1)==q)) = LINK(q, (LINK(q,0)==NONE));		// parent p now points to q's child (if any)
			if (q!=f) {
				_node[q]=_node[f];													// splice q in where f was (half of "swapNodes")
				if (LINK(q,0)!=NONE) PARENT(LINK(q,0))=q;		// fix up parentage links as well
				if (LINK(q,1)!=NONE) PARENT(LINK(q,1))=q;
				LINK(PARENT(q), LINK(PARENT(q),1)==f)=q;
			}
			removeNode(f);
		}
		// update root and make it black
		if (__root()!=NONE) RED(__root())=0;
	}
	return (f==NONE) ? 0 : 1;
}




// ///////////////////////////////////////////////////////////////////////////////////////////
// MEX specific functions, and non-mex stubs for compatibility
//////////////////////////////////////////////////////////////////////////////////////////////
//template<class Key, class T> void rbtree<Key,T>::mxInit() { M_=NULL; }
template<class Key, class T> bool rbtree<Key,T>::mxCheckValid(const mxArray* M) { 
	return mxIsStruct(M) &&
			    _key.mxCheckValid(mxGetField(M,0,"key")) &&
					_value.mxCheckValid(mxGetField(M,0,"value")) &&
					_node.mxCheckValid(mxGetField(M,0,"node")) &&
					(mxGetN(mxGetField(M,0,"node"))==mxGetN(mxGetField(M,0,"key"))+2);
	// also check that node is 4x(n+2) and key,value are 1xn?
}
template<class Key, class T> void rbtree<Key,T>::mxSet(mxArray* M) { 
	if (!mxCheckValid(M)) throw std::runtime_error("incompatible Matlab object type in rbtree");
	M_ = M;
	_key.mxSet(mxGetField(M,0,"key"));   
	_value.mxSet(mxGetField(M,0,"value"));
	_node.mxSet(mxGetField(M,0,"node")); 
}

template<class Key, class T> mxArray* rbtree<Key,T>::mxGet() { 
	if (!mxAvail()) {
		const char* strNames[] = {"key","value","node"};
		mxArray* m=mxCreateStructMatrix(1,1,3,strNames);
		mxSetField(m,0,"key",  _key.mxGet());
		mxSetField(m,0,"value",_value.mxGet());
		mxSetField(m,0,"node", _node.mxGet());
		M_=m;
		// !!! need to create matrices of proper type for elements
		//mxArray* m; int retval = mexCallMATLAB(1,&m,0,NULL,"rbtree");	// !!! very dangerous if function name does not exist
		//if (retval) mexErrMsgTxt("Error creating new rbtree");
  	//rbtree<Key,T> rb; rb.mxSet(m);
		//rb._key.mxGet(); rb._value.mxGet(); rb._node.mxGet();
		//mxSwap(rb);
	}
	return M_;
}
template<class Key, class T> void rbtree<Key,T>::mxRelease() { }
template<class Key, class T> void rbtree<Key,T>::mxDestroy() { }
template<class Key, class T> void rbtree<Key,T>::mxSwap(rbtree<Key,T>& rb) { 
  _key.swap(rb._key); _value.swap(rb._value); _node.swap(rb._node);
	std::swap(M_,rb.M_);
}

#endif // def MEX

}       // namespace mex
#endif  // re-include
