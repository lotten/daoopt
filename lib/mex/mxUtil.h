// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MXUTIL__
#define __MXUTIL__

#include<cmath>
#include<ctime>
#include<sys/time.h>
#include<cassert>
#include<cstdlib>
#include<stdint.h>
#include<limits>

#include<string>
#include<sstream>
#include<vector>

#include "mxObject.h"

/*
#ifdef WINDOWS
    #include <windows.h>
//    #include <boost/math/special_functions/atanh.hpp>  // for atanh
//    #include <boost/math/special_functions/log1p.hpp>  // for log1p
    #include <float.h>  // for _isnan
#else
    // Assume POSIX compliant system. We need the following for querying the system time
    #include <sys/time.h>
#endif
*/


/* // from libDAI
#ifdef WINDOWS
double atanh( double x ) {
    return boost::math::atanh( x );
}
double log1p( double x ) {
    return boost::math::log1p( x );
}
#endif
*/

namespace mex {  

#ifdef MEX 
  inline bool isfinite(double v) { return mxIsFinite(v); }
  inline bool isnan(double v)    { return mxIsNaN(v); }
  inline double infty()          { return mxGetInf(); }
#else
  //static inline bool isfinite(value v) { return std::abs(v)!=std::numeric_limits<value>::infinity(); }
  inline bool isfinite(double v) { return (v <= std::numeric_limits<double>::max() && v >= -std::numeric_limits<double>::max()); }
  inline bool isnan(double v)    { return (v!=v); }
  inline double infty()          { return std::numeric_limits<double>::infinity(); }
#endif


// Returns system (wall clock) time in seconds
inline double timeSystem() {
#ifdef WINDOWS
    SYSTEMTIME tbuf;
    GetSystemTime(&tbuf);
    return( (double)(tbuf.wSecond + (double)tbuf.wMilliseconds / 1000.0) );
#else
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return( (double)(tv.tv_sec + (double)tv.tv_usec / 1000000.0) );
#endif
}

inline double timeProcess() {
#ifdef WINDOWS
    throw std::runtime_error("No process time implemented for Windows");
#else
    clock_t tv( clock() );
    return( (double)tv );
#endif
}


//////////////////////////////////////////////////////////////////////////////
// String splitting functions
//
inline std::vector<std::string>& split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}




//////////////////////////////////////////////////////////////////////////////
// Random number classes
//
// Call back to Matlab in MEX functions to preserve potential for reproducibility
//   and control of random seeds, etc.
//
inline void randSeed() { srand(time(0)); }
inline void randSeed(size_t s) { srand(s); }
#ifdef MEX
inline double randu() {		// Matlab version (consistent with matlab random #s & seeds)
	mxArray* M; mexCallMATLAB(1, &M, 0, NULL, "rand");
	double u=mxGetScalar(M); mxDestroyArray(M);
	return u;
}
inline double randn() {
	mxArray* M; mexCallMATLAB(1, &M, 0, NULL, "randn");
	double u=mxGetScalar(M); mxDestroyArray(M);
	return u;
}
inline int randi(int imax) {
	int guard = (randu()*imax);
	return (guard > imax) ? imax : guard;
}
#else
inline double randu()  { 	// non-Matlab versions
	return rand() / double(RAND_MAX); 
}
// randi returns a random integer in 0..imax-1
inline int randi(int imax) { 
 assert(imax>0);
 imax--;
 if (imax==0) return 0;
 int guard = (int) (randu()*imax)+1;
 return (guard > imax)? imax : guard;
}
inline double randn() {  // Marsaglia polar method
  double u,v,s; 
	u=2*randu()-1; v=2*randu()-1; s=u*u+v*v;
	return u*std::sqrt(-2*std::log(s)/s);
} 
#endif
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// midx class -- for interfacing with Matlab values used as indices
//
//   Matlab bases indices at 1; C++ at 0
// The midx class writes matlab indices into memory, but treats them as C++ values,
//   so that C++ code can write indices 0..N-1, and when passed back to Matlab,
//   Matlab will see 1..N instead.
//
#ifndef MEX
// Matlab index (if no matlab) is just a regular number
template <class T> class midx { 
	public:
		midx() { b=T()-1; }
		midx(T t) { b=t; }
		//midx(T t=T()) { b=t; }
		midx<T>& operator=(const T& t) { b=t; return *this; }
		operator T() const { return b; }
	private:
		T b;
};

#else
// If we're in Matlab, interface to add/subtract one
template <class T>
class midx {
  public:
	  typedef T                value_type;// what to think of it as
	  typedef struct { T t; }  base;			// our (type safe) base type
	  typedef T                mxbase;		// intrinsic matlab memory type
		static const int         mxsize=sizeof(base)/sizeof(mxbase);
		midx() { b.t=0; }
		//midx(T t=T())   { b.t=t+1; }
		midx(T t)   { b.t=t+1; }
		operator base() const { return b; }
		operator T()    const { return b.t-1; }
		midx<T>& operator=(const T& t) { b.t=t+1; return *this;}
	private:
		base b;
};
template <>
struct mexable_traits<mex::midx<double> > {
	typedef mex::midx<double>::base             base;
	typedef double           mxbase;
	static const int mxsize=1;
};
template <>
struct mexable_traits<mex::midx<uint32_t> > {
	typedef mex::midx<uint32_t>::base             base;
	typedef uint32_t	                            mxbase;
	static const int mxsize=1;
};
#endif
//////////////////////////////////////////////////////////////////////////////

// Enable containers of std::pairs of midx types
template <class T>
struct mexable_traits<std::pair<mex::midx<T>,mex::midx<T> > > {
  typedef T        base;
  typedef T        mxbase;
  static const int mxsize=2;
};



}		// end namespace mex

#endif
