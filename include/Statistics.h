/*
 * Statistics.h
 *
 *  Copyright (C) 2011 Lars Otten
 *  Licensed under the MIT License, see LICENSE.TXT
 *  
 *  Created on: Mar 1, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "DEFINES.h"

#ifdef PARALLEL_STATIC

#include "SearchNode.h"
#include "Pseudotree.h"

struct SubproblemStats {

  /* The following are all specific to the conditioned subproblem in question */
  int rootVar;
  int numVars;
  int depth;
  int height;
  int width;
  count_t subNodeCount;
  double upperBound;
  double lowerBound;
  double boundGap;

  void update(SearchNode*, PseudotreeNode*, count_t);

};

ostream& operator << (ostream& os, const SubproblemStats& s);

inline void SubproblemStats::update(SearchNode* n, PseudotreeNode* pt, count_t count) {
  assert(n && pt);
  assert(n->getType() == NODE_OR);
  assert(pt->getVar() == n->getVar());

  rootVar = n->getVar();

  numVars = pt->getSubprobSize();
  depth = pt->getDepth();
  height = pt->getSubHeight();
  width = pt->getSubWidth();

  upperBound = n->getHeur();
  lowerBound = n->getInitialBound();
  boundGap = upperBound - lowerBound;

  subNodeCount = count;
}

#endif

#ifdef PARALLEL_DYNAMIC

#include "Subproblem.h"

/**
 * keeps track of subproblem statistics, used for complexity estimates
 */
class Statistics {

protected:
  count_t _minN, _maxN;           // min and max number of nodes
  count_t _minE, _maxE;           // min and max estimates

  /* default values for branching, increment, and leaf node depth, and height */
  double defBra,defInc,defDep,defHei;

  double _alpha,_beta,_gamma;     // weights for the increment computation

public:
  vector<count_t> nodesAND;     // tracks the actual number of AND nodes
  vector<count_t> estimate;     // tracks the predicted number of nodes

  vector<int> height;       // tracks the height of the sub pseudo trees

  vector<double> branching;     // tracks the branching degree
  vector<double> increment;     // tracks the average increment
  vector<double> avgLeafDepth;  // tracks the average leaf node height

public:

  /* performs parameter initialization based on statistics of
   * a small subproblem
   * @depth: subproblem root depth
   * @height: height of subproblem pseudo tree
   * @N: number of nodes expanded in subproblem
   * @L: number of leaf nodes generated in subproblem
   * @D: cumulative depth of leaf nodes in subproblem
   * @lower: initial lower bound for subproblem
   * @upper: initial upper bound for subproblem (minibuckets)
   **/
  void init(int depth, int height, count_t N, count_t L, count_t D, double lower, double upper);

  /* records the subproblem information (after it has been solved) */
  void addSubprob(Subproblem*);

  /* returns the running average of the increments */
  double getAvgInc();
  /* returns the running average of branching degrees */
  double getAvgBra();
  /* returns the average sub pseudo tree height */
  double getAvgHei();
  /* performs normalization on a 'raw' estimate */
  double normalize(double) const;

  double getAlpha() const { return _alpha; }
  double getBeta()  const { return _beta;  }
  double getGamma() const { return _gamma; }

  Statistics();

};


inline Statistics::Statistics() :
    _minN( numeric_limits<count_t>::max() ), _maxN( numeric_limits<count_t>::min() ),
    _minE( numeric_limits<count_t>::max() ), _maxE( numeric_limits<count_t>::min() ),
    defBra(NONE), defInc(NONE), defDep(NONE), defHei(NONE),
    _alpha(1.0), _beta(0.5), _gamma(1.0)
{ /* intentionally empty */ }


#endif /* PARALLEL_DYNAMIC */

#endif /* STATISTICS_H_ */
