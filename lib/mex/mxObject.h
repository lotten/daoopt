// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_MXOBJECT_H
#define __MEX_MXOBJECT_H
/* How to use the mex class interface functions
 *
 *     Association with a Matlab object is treated like the existence of a pointer to the C++ object, but 
 *   managed internally.  In other words, whatever changes are undergone by the object, the object is required
 *   to make sure that the Matlab variable(s) it represents also undergo the same changes.  The object is thus
 *   assumed to "own" the Matlab variable and is free to change its values, reallocate its memory, etc.
 *
 *   Creation of a Matlab/C++ hybrid object can take place in one of three ways:  
 *     First, we can create the object in a stand-alone way, in which case it may or may not create anything 
 *   in Matlab (it may use standard memory allocation, etc.)  When a Matlab object is desired, for example
 *   to return from a MEX file, we call mxGet() and a mxArray will be created for us.
 *     Second, we can create an object by copying an existing mxArray object (for example, a value passed
 *   into a MEX file).  We call mxCopy(-); this will create a new matlab object for us to write data into
 *   in-place.
 *     Third, we can create an object from an existing mxArray, to be modified in-place.  It is against
 *   Matlab rules to do this with passed arguments, but if you're feeling confident you can do it anyway.
 *   This can potentially modify unintended variables back in the Matlab namespace, if the data are linked,
 *   but can also be useful if you are careful.
 *
 *   A note on equality operators:
 *     We will always override "operator=" to write the right-hand-side data into our own data structures,
 *   which may be associated with a mxArray; we do *not* copy the mxArray pointer or otherwise modify the
 *   "matlab identity" of the variable.
 *
*/

/* Notes to self
 * Getting passed objects:
 *   read-only by reference:   const vector<int> v(M);        (prevents non-const accesses)
 *   deep copy                 vector<int> v; v.mxCopy(M);    (all data owned by object; can delete)
 *   copy-on-write???:         vector<int> v; v.mxCopyOnWrite(M);  (data not owned; copy before any operation)
 *   possibly shared data:     vector<int> v; v.mxShared(M);  (data pointers may be shared; copy before resize)
 *   personal copy, by ref:    vector<int> v; v.mxSet(M);     (all bets are off)
 *
 * Returning objects:
 *   destructor does not delete memory? leaves for matlab?
 *   OR, does delete memory unless "shared" flag : be sure to "set shared" before returning data
*/


#include <stdint.h>
#include <stdexcept>
#include <assert.h>

#ifdef MEX
#include <mex.h>
#include <matrix.h>
extern "C" {
mxArray* mxCreateReference(mxArray* pr);
mxArray* mxCreateSharedDataCopy(mxArray* pr);
}
#endif

#ifndef MEX
	typedef int mxArray;
#endif

namespace mex {


#define mxPAUSE() mexCallMATLAB(0, NULL, 0, NULL, "pause");



////////////////////////////////////////////////////////////////
// Simplified inheritance checking in case we lack boost::
//#include <boost/type_traits.hpp>
typedef char (&yes)[1];
typedef char (&no)[2];
template <typename B, typename D>
struct Host
{
  operator B*() const;
  operator D*();
};
template <typename B, typename D>
struct is_base_of
{
  template <typename T>
  static yes check(D*, T);
  static no check(B*, int);
  static const bool value = sizeof(check(Host<B,D>(), int())) == sizeof(yes);
};
////////////////////////////////////////////////////////////////
template<class T, class B> struct Derived_from {
	static void constraints(T* p) { B* pb = p; }
	Derived_from() { void(*p)(T*) = constraints; }
};
////////////////////////////////////////////////////////////////

#ifdef MEX
// templated method to obtain matlab typing index; no default to force compiler error for non-builtin
template <typename T> struct mxClassT; // { static const mxClassID value = mxUNKNOWN_CLASS; };
template <> struct mxClassT<double>   { static const mxClassID value = mxDOUBLE_CLASS; };
template <> struct mxClassT<uint64_t> { static const mxClassID value = mxUINT64_CLASS; };
template <> struct mxClassT<int64_t>  { static const mxClassID value = mxINT64_CLASS; };
template <> struct mxClassT<uint32_t> { static const mxClassID value = mxUINT32_CLASS; };
template <> struct mxClassT<int32_t>  { static const mxClassID value = mxINT32_CLASS; };
template <> struct mxClassT<uint16_t> { static const mxClassID value = mxUINT16_CLASS; };
template <> struct mxClassT<int16_t>  { static const mxClassID value = mxINT16_CLASS; };
template <> struct mxClassT<uint8_t>  { static const mxClassID value = mxUINT8_CLASS; };
template <> struct mxClassT<int8_t>   { static const mxClassID value = mxINT8_CLASS; };
#endif


class mxObject;
class mxAny;

//////////////////////////////////////////////////////////////////
// mxObject base class of all matlab-memory based objects
//   a matlab object is one that corresponds to a single variable (mxArray) in matlab
//   inheriting classes must implement a number of accessor and manipulation functions
//   for a non-MEX environment, we define blank versions so inheriting classes need not redefine them.
//
class mxObject {
public:
	mxObject() { mxInit(); }
	virtual ~mxObject() { };

// All derived classes *should* implement "operator=" and "mxSwap" for their own class type
//  (no way to force this in C++)
// virtual mexable& operator=(const mexable&) = 0;
// virtual mexable& mxSwap(mexable&) = 0;

# ifdef MEX  // MEX versions: all methods are pure virtual and must be overridden

  virtual inline void  mxInit()  { M_=NULL; }     // initialize any matlab-specific implementation
  inline bool  mxAvail() const { return M_!=NULL; }     // true if we already represent a matlab object
	virtual bool mxCheckValid(const mxArray*) = 0;   // check validity of mxArray* for this c++ object type

	virtual void mxCopy(const mxArray* m) {    // create a *copy* (deep copy) of the matlab data & wrap it
	  mxSet(mxDuplicateArray(m)); }      //   for a shallow copy use: mxSet(); mxRelease(); mxGet();   (!!! broken?)
	virtual void mxSet(mxArray*) = 0;    // wrap passed matlab data; assumes irrevocable, exclusive ownership 
	virtual mxArray* mxGet() = 0;        // Return a pointer to the wrapped object (created if required)
	virtual void mxRelease() = 0;        // Disassociate from matlab object, leaving memory to matlab garbage collection
	virtual void mxDestroy() = 0;        // Delete matlab object and revert *this to a clean T()
	                                     //   to keep a copy use e.g. Obj2=Obj; Obj.mxDestroy(); Obj.mxSwap(Obj2);
	//virtual void mxSwap() = 0;         // Exchange *matlab* identities of two vars (useful for putting mxObj in containers)
# else
	virtual inline  void mxInit()             { M_=NULL; };
  inline bool  mxAvail()  const             { return false; }
	virtual bool mxCheckValid(const mxArray*)  { return false; }
	virtual void mxCopy(const mxArray*)        { return; }
	virtual void mxSet(mxArray*)              { return; }
	virtual mxArray* mxGet()                  { return NULL; }
	virtual void mxRelease()                  { return; }
	virtual void mxDestroy()                  { return; }
	//virtual void mxSwap()                   { return; }
# endif

	friend class mex::mxAny;
protected:
	mxArray* M_;
};


class mxAny : virtual public mxObject {
public: 
	mxAny() { mxInit(); }
	mxAny(mxArray* m) { mxSet(m); }
	void swap(mxAny& x) { throw std::runtime_error("No mxAny swap except mxSwap"); }
	bool mxCheckValid(const mxArray* m) { return (m!=NULL); }
	void mxSet(mxArray* m) { M_=m; }
	mxArray* mxGet() { if (!M_) throw std::runtime_error("Can't mxGet a null"); return M_; }
	mxAny& operator=(const mxObject& c) { assert(c.mxAvail()); mxCopy(c.M_); }
	void mxRelease() { mxInit(); } //!!!
	void mxDestroy() { } //!!!
};


/*
 * mexable_traits<T> : helper class used in mex:: containers of non mxObject-derived elements
 *   to potentially mediate access between matlab vs. c++ values and memory storage.  Override
 *   this to specify a particular matlab memory allocation and conversion process.  Includes:
 * "Default" class for built-in types
 * std::pair class for structures of two elements of equal type
 * midx<double>, <uint32_t> for matlab indices (1-based values) interpreted as 0-based offsets (in mxUtil)
*/


// Default: enable containers of basic built-in types
template <class T>
struct mexable_traits {
  typedef T        base;
  typedef T        mxbase;
  static const int mxsize=1;
};

// Enable containers of std::pairs of matched type
template <class T>
struct mexable_traits<std::pair<T,T> > {
  typedef T        base;
  typedef T        mxbase;
  static const int mxsize=2;
};




// This horrible macro stuff will hopefully replace most instances of declaring 
// the mxCheckValid() ... mxSwap()  functions for structure-based representations
// (i.e. those which are simply a collection of mxObjects, with nothing extra)

#define STRINGIZE(arg)  STRINGIZE1(arg)
#define STRINGIZE1(arg) STRINGIZE2(arg)
#define STRINGIZE2(arg) #arg

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

#define FOR_EACH_1(what, x, ...) what(x)
#define FOR_EACH_2(what, x, ...)\
          what(x) FOR_EACH_1(what,  __VA_ARGS__) 
#define FOR_EACH_3(what, x, ...)\
          what(x) FOR_EACH_2(what, __VA_ARGS__)
#define FOR_EACH_4(what, x, ...)\
          what(x) FOR_EACH_3(what,  __VA_ARGS__) 
#define FOR_EACH_5(what, x, ...)\
          what(x) FOR_EACH_4(what,  __VA_ARGS__) 
#define FOR_EACH_6(what, x, ...)\
          what(x) FOR_EACH_5(what,  __VA_ARGS__)
#define FOR_EACH_7(what, x, ...)\
          what(x) FOR_EACH_6(what,  __VA_ARGS__)
#define FOR_EACH_8(what, x, ...)\
          what(x) FOR_EACH_7(what,  __VA_ARGS__)

#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__) 
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N 
#define FOR_EACH_RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0

#define FOR_EACH_(N, what, x, ...) CONCATENATE(FOR_EACH_, N)(what, x, __VA_ARGS__)
#define FOR_EACH(what, x, ...) FOR_EACH_(FOR_EACH_NARG(x, __VA_ARGS__), what, x, __VA_ARGS__)

// Go from a field tag name to its c++ variable name
#define PRIVATE(x) CONCATENATE(x,_)

// Process single elements of "for-each" contained field
#define MXCHECKVALID(field) PRIVATE(field).mxCheckValid(mxGetField(M,0,STRINGIZE(field))) &&
#define MXSETSINGLE(field)  PRIVATE(field).mxSet(mxGetField(M,0,STRINGIZE(field)));
#define MXGETSINGLE(field)  PRIVATE(field).mxGet();
#define MXSWAPSINGLE(field) PRIVATE(field).mxSwap(S.PRIVATE(field));

// Construct required mex functions
#define MEXFUNCTIONS_STRUCT(MyObject, ...) \
  bool MyObject::mxCheckValid(const mxArray* M) { \
    return FOR_EACH(MXCHECKVALID, __VA_ARGS__) true; \
  } \
  \
  void MyObject::mxSet(mxArray* M) { \
		M_=M; \
    FOR_EACH(MXSETSINGLE, __VA_ARGS__) \
  } \
  \
  mxArray* MyObject::mxGet() { \
    if (!mxAvail()) { \
      mxArray* m; int retval=mexCallMATLAB(1,&m,0,NULL,STRINGIZE(MyObject)); \
      if (retval) mexErrMsgTxt("Error creating new "STRINGIZE(MyObject)); \
      MyObject Obj; Obj.mxSet(m); Obj=*this; \
      FOR_EACH(MXGETSINGLE, __VA_ARGS__) \
      mxSwap(Obj); \
    } \
    return M_; \
  } \
  void MyObject::mxRelease() { throw std::runtime_error("Not implemented"); } \
  void MyObject::mxDestroy() { throw std::runtime_error("Not implemented"); } \
  void MyObject::mxSwap(MyObject& S) { \
    FOR_EACH(MXSWAPSINGLE, __VA_ARGS__) \
    std::swap(M_,S.M_); \
  }



} // end namespace mex

#endif
