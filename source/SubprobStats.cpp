/*
 * SubprobStats.cpp
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

#include <SubprobStats.h>

#ifdef PARALLEL_STATIC

namespace daoopt {

const int SubprobStats::MIN = 0;
const int SubprobStats::MAX = 1;
const int SubprobStats::AVG = 2;
const int SubprobStats::SDV = 3;
const int SubprobStats::MED = 4;

/* should match the fields returned by getAll */
const string LEGEND[] =
  { "Vars", "Leafs", "Twb",
    "Wmin", "Wmax", "Wavg", "Wsdv", "Wmed",
    "WCmin", "WCmax", "WCavg", "WCsdv", "WCmed",
    "Kmin", "Kmax", "Kavg", "Ksdv", "Kmed",
    "Hmin", "Hmax", "Havg", "Hsdv", "Hmed" };

template<typename T, size_t N> T* end(T (&ra)[N]) {
  return ra + N;
}

const vector<string> SubprobStats::legend(LEGEND, end(LEGEND));

void SubprobStats::computeStats(const vector<int>& xs, double*& target) {
  double mini, maxi, avg, sdev, med;
  mini = *min_element(xs.begin(), xs.end());
  maxi = *max_element(xs.begin(), xs.end());
  vector<int> sorted(xs);
  std::sort(sorted.begin(), sorted.end());
  med = sorted.at(sorted.size()/2);

  if (xs.size() == 1) {
    avg = xs[0];
    sdev = 0.0;
  } else {
    avg = sdev = 0.0;
    BOOST_FOREACH( int x, sorted ) {
      avg += x;
      sdev += x*x;
    }
    size_t N = sorted.size();
    avg /= N;
    sdev /= N;
    sdev -= avg*avg;
    sdev = sqrt(sdev);
    sdev *= sqrt(N*1.0/(N-1));
  }

  if (target) delete[] target;
  target = new double[5];
  target[MIN] = mini;
  target[MAX] = maxi;
  target[AVG] = avg;
  target[SDV] = sdev;
  target[MED] = med;
}


void SubprobStats::getAll(vector<double>& out) const {
  out.push_back(m_varCount);
  out.push_back(m_leafCount);
  out.push_back(m_stateSpaceCond);
  double* ls[4] = { m_clusterSize, m_clusterSizeCond,
                    m_domainSize, m_leafDepth };
  BOOST_FOREACH( double* p, ls ) {
    if (p)
      for (size_t i=0; i<5; ++i)
        out.push_back(p[i]);
  }
}


ostream& operator << (ostream& os, const SubprobStats& stats) {
  vector<double> all = stats.getAll();
//  os << "STATS";
  BOOST_FOREACH( double d, all ) {
    os << "\t" << d;
  }
  return os;
}

}  // namespace daoopt

#endif /* PARALLEL_STATIC */

