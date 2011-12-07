/*
 * LearningEngine.cpp
 *
 *  Copyright (C) 2011 Lars Otten
 *  Licensed under the MIT License, see LICENSE.TXT
 *  
 *  Created on: Nov 3, 2011
 *      Author: lars
 */

#include "LearningEngine.h"

#ifdef PARALLEL_STATIC

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

#endif  /* PARALLEL_STATIC */
