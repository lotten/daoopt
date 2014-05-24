/*
 * LearningEngine.cpp
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
 *  
 *  Created on: Nov 3, 2011
 *      Author: lars
 */

#include "LearningEngine.h"

#ifdef PARALLEL_STATIC

namespace daoopt {

void LearningEngine::statsToFile(const string& fn) const {
  ofstream of(fn.c_str(), ios_base::out | ios_base::trunc);
  of << "#id\troot\tdepth\tvars\tlb\tub\theight\twidth\tor\tand" << endl;
  for (size_t i = 0; i < m_samples.size(); ++i) {
    of << i << '\t'
       << m_samples[i].rootVar << '\t'
       << m_samples[i].depth << '\t'
       << m_samples[i].numVars << '\t'
       << m_samples[i].lowerBound << '\t'
       << m_samples[i].upperBound << '\t'
       << m_samples[i].height << '\t'
       << m_samples[i].width << '\t'
       << UNKNOWN << '\t'  // # OR nodes
       << m_samples[i].subNodeCount << '\t'
       << endl;
  }
  of.close();
}

}  // namespace daoopt

#endif  /* PARALLEL_STATIC */
