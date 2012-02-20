/*
 * SubprobStats.h
 *
 *  Copyright (C) 2008-2012 Lars Otten
 *  This file is part of DAOOPT.
 *
 *  DAOOPT is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DAOOPT is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DAOOPT.  If not, see <http://www.gnu.org/licenses/>.

 *  Created on: Jan 14, 2012
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef SUBPROBSTATS_H_
#define SUBPROBSTATS_H_

#include "_base.h"

#ifdef PARALLEL_STATIC

/* stores dynamic subproblem features (associated with specific search node) */
struct SubprobFeatures {
  double ratioPruned;  // ratio of heuristically pruned nodes
  double ratioDead;    // ratio of dead-end nodes (0-valued)
  double ratioLeaf;    // ratio of leaf nodes (pseudotree leaf)
  double avgNodeDepth; // average node depth in subproblem search space
  double avgLeafDepth; // average leaf depth in subproblem search space
  double avgBranchDeg; // average branching factor in subproblem search space
  SubprobFeatures();
};

/* stores static information about a subproblem's complexity
 * (associated with pseudo tree node) */
class SubprobStats {
  friend ostream& operator << (ostream& os, const SubprobStats& stats);

public:
  static const int MIN;  // TODO: should be an enum?
  static const int MAX;
  static const int AVG;
  static const int SDV;
  static const int MED;

  static const vector<string> legend;

protected:
  double m_varCount;          // Number of variables in subproblem.
  double m_leafCount;         // Number of pseudotree leaves below this node.
  double* m_clusterSize;      // Statistics regarding unconditioned clusters.
  double* m_clusterSizeCond;  // Statistics regarding clusters below
                              // conditioned on this node's context.
  double* m_leafDepth;        // Statistics regarding the depth of the
                              // different pseudotree leaves.
  double* m_domainSize;       // Statistics regarding the domain sizes of
                              // variables in the pseudo tree below.

  /* generic helper functions */
  void computeStats(const vector<int>&, double*&);
  double getStats(double*, int) const;

public:
  SubprobStats();
  ~SubprobStats();

  void setClusterStats(const vector<int>& sizes);
  void setClusterCondStats(const vector<int>& sizes);
  void setDepthStats(const vector<int>& depths);
  void setDomainStats(const vector<int>& domains);

  double getClusterStats(int idx) const;
  double getClusterCondStats(int idx) const;
  double getDepthStats(int idx) const;
  double getDomainStats(int idx) const;

  double getVarCount() const { return m_varCount; }
  double getLeafCount() const { return m_leafCount; }

  /* appends all available features to the given vector */
  void getAll(vector<double>&) const;
  vector<double> getAll() const;
};

ostream& operator << (ostream& os, const SubprobStats& stats);

/* Inline functions */

inline SubprobFeatures::SubprobFeatures() :
    ratioPruned(UNKNOWN), ratioDead(UNKNOWN), ratioLeaf(UNKNOWN),
    avgNodeDepth(UNKNOWN), avgLeafDepth(UNKNOWN), avgBranchDeg(UNKNOWN)
{ /* empty */ }

inline double SubprobStats::getStats(double* xs, int idx) const {
  assert (xs  && idx >= 0 && idx < 4);
  return xs[idx];
}

inline void SubprobStats::setClusterStats(const vector<int>& sizes) {
  this->computeStats(sizes, m_clusterSize);
}
inline double SubprobStats::getClusterStats(int idx) const {
  return this->getStats(m_clusterSize, idx);
}

inline void SubprobStats::setClusterCondStats(const vector<int>& sizes) {
  this->computeStats(sizes, m_clusterSizeCond);
}
inline double SubprobStats::getClusterCondStats(int idx) const {
  return this->getStats(m_clusterSizeCond, idx);
}

inline void SubprobStats::setDepthStats(const vector<int>& depths) {
  this->computeStats(depths, m_leafDepth);
  m_leafCount = depths.size();
}
inline double SubprobStats::getDepthStats(int idx) const {
  return this->getStats(m_leafDepth, idx);
}

inline void SubprobStats::setDomainStats(const vector<int>& domains) {
  this->computeStats(domains, m_domainSize);
  m_varCount = domains.size();
}
inline double SubprobStats::getDomainStats(int idx) const {
  return this->getStats(m_domainSize, idx);
}

inline vector<double> SubprobStats::getAll() const {
  vector<double> v;
  this->getAll(v);
  return v;
}

inline SubprobStats::SubprobStats() :
    m_varCount(UNKNOWN), m_leafCount(UNKNOWN), m_clusterSize(NULL),
    m_clusterSizeCond(NULL), m_leafDepth(NULL), m_domainSize(NULL)
{ /* nothing */ }

inline SubprobStats::~SubprobStats() {
  if (m_clusterSize) delete[] m_clusterSize;
  if (m_clusterSizeCond) delete[] m_clusterSizeCond;
  if (m_leafDepth) delete[] m_leafDepth;
  if (m_domainSize) delete[] m_domainSize;
}

#endif /* PARALLEL_STATIC */
#endif /* SUBPROBSTATS_H_ */
