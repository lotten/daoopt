/*
 * MiniBucketElimMplp.h
 * by Alex Ihler, December 2011
 *
 * Interface for Alex Ihler's minibucket implementation
 */

#ifndef MINIBUCKETELIMNEW_H_
#define MINIBUCKETELIMNEW_H_

#include "Heuristic.h"
#include "Function.h"
#include "Problem.h"
#include "ProgramOptions.h"
#include "Pseudotree.h"
#include "utils.h"

#include "MiniBucket.h"

#include "mex/mbe.h"


/* The overall minibucket elimination */
class MiniBucketElimMplp : public Heuristic {
public:

  // checks if the given i-bound would exceed the memlimit, in that
  // case compute highest possible i-bound that 'fits'
  size_t limitSize(size_t memlimit, const vector<val_t> *assignment);

  // Model pre-processing step, if any  (here, run mplp)
  bool preprocess(const vector<val_t>* assignment);

  // builds the heuristic, limited to the relevant subproblem, if applicable.
  // if computeTables=false, only returns size estimate (no tables computed)
  size_t build(const vector<val_t>* assignment = NULL, bool computeTables = true);

  // returns the global upper bound
  double getGlobalUB() const { return _mbe.ub() + _p->getGlobalConstant(); }

  // computes the heuristic for variable var given a (partial) assignment
  double getHeur(int var, const vector<val_t>& assignment) const {
	  double res=_mbe.logHeurToGo(_mbe.var(var),assignment); 
		return res;
	}

  // reset the i-bound
  void setIbound(int ibound) { _mbe.setIBound(ibound); }
  // gets the i-bound
  int getIbound() const { return _mbe.getIBound(); }

  // gets sum of tables sizes
  size_t getSize() const {  
		size_t sz=0;
	  const mex::vector<mex::Factor>& flist = _mbe.factors();
		for (size_t i=0;i<flist.size();++i) sz += flist[i].numel();
		return sz;
	}

  bool writeToFile(string fn) const { return true; }
  bool readFromFile(string fn)			{ return false; }


  // added functionality
  bool doJGLP();
  bool doMPLP();

public:
  MiniBucketElimMplp(Problem* p, Pseudotree* pt, ProgramOptions* po, int ib);
  ~MiniBucketElimMplp() { }

protected:
	Problem* _p;
	Pseudotree* _pt;
	mex::mbe  _mbe;
	size_t _memlimit;
	ProgramOptions* _options;

  // Helper functions to move between DAO & mex factor formats
  mex::vector<mex::Factor> copyFactors( void );
  void rewriteFactors( const vector<mex::Factor>& factors);

};



#endif /* MINIBUCKETELIM_H_ */
