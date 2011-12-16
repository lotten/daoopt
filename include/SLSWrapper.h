/*
 * SLSWrapper.h
 *
 *  Wrapper around the SLS4MPE code by Frank Hutter, so it can
 *  be used as a library.
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
 *  Created on: Nov 17, 2011
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef SLSWRAPPER_H_
#define SLSWRAPPER_H_

#include "Problem.h"

#ifdef ENABLE_SLS

#include "sls4mpe/main_algo.h"
#include "sls4mpe/timer.h"
#include "sls4mpe/global.h"
#include "sls4mpe/ProblemReader.h"

class SLSWrapper {
protected:
  double m_likelihood;
  int* m_assignment;
  Problem* m_problem;

public:
  bool init(Problem* prob, int iter, int time);
  bool run();
  double getSolution(vector<val_t>* tuple = NULL) const;
  void reportSolution(double cost, int num_vars, int* assignment);

public:
  SLSWrapper();
  virtual ~SLSWrapper();
};


/* Inline definitions */
inline SLSWrapper::SLSWrapper() :
    m_likelihood(0.0), m_assignment(NULL) {
  /* nothing here */
};

inline SLSWrapper::~SLSWrapper() {
  if (m_assignment)
    delete[] m_assignment;
}

#endif /* ENABLE_SLS */

#endif /* SLSWRAPPER_H_ */
