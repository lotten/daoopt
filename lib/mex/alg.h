// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_ALG_H
#define __MEX_ALG_H

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>
#include <cstdarg>
#include <cstring>

namespace mex {

typedef mex::midx<uint32_t> index;

// Pure virtual algorithm class for interface functions
class gmAlg {
public:
	// Basics
	gmAlg() { }
	virtual gmAlg* clone() const = 0;

	//virtual void setProperties(const char* opt=NULL,...) = 0;	// set parameters
	// Parameter settings including # of iterations, cpu time, delta objective or elementals?

	virtual void init() = 0;										// initialize algorithm
	virtual void init(const VarSet& vs) = 0;		//   or a sub-structure
	virtual void run() = 0;											// runs the algorithm

	// For partition function tasks:
  virtual double logZ() const = 0;						// partition function estimate or bound
  virtual double logZub() const = 0;					//   upper bound and
  virtual double logZlb() const = 0;					//   lower bound if available

	// For optimization tasks:
	virtual double ub() const = 0;							// optimal configuration upper bound
	virtual double lb() const = 0;							//   and lower bound if available
	virtual vector<index> best() const = 0;			// best configuration found so far

	// For variational methods.  (the concept of "belief" may vary)
	virtual const Factor& belief(size_t i) const = 0;			// belief associated with a node of the graph
	virtual const Factor& belief(Var v) const = 0;				//   or with a particular variable
	virtual const Factor& belief(VarSet vs) const = 0;		//   or with a set of variables
	virtual const vector<Factor>& beliefs() const = 0;		// or all of them


	// get graphical model?  original?
	//                       new (from reparameterization property)? 

	// "backup" and "restore"
	// "clamp" or "cavity"

};

// calcMarginal and calcPairBeliefs

// findMaximum


} // end namespace


#endif  // end inclusion
