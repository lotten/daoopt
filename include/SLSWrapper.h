/*
 * SLSWrapper.h
 *
 *  Wrapper around the SLS4MPE code by Frank Hutter, so it can
 *  be used as a library.
 *
 *  Copyright (C) 2011 Lars Otten
 *  Licensed under the MIT License, see LICENSE.TXT
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

public:
  bool init(string filename, int iter, int time);
  bool run();
  double getSolution(vector<val_t>* tuple = NULL) const;

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
