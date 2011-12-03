/*
 * SLSWrapper.cpp
 *
 *  Created on: Nov 17, 2011
 *      Author: lars
 */

#include "SLSWrapper.h"

bool SLSWrapper::init(string filename, int iter, int time/*Problem* prob*/) {

  sls4mpe::verbose = false;
  sls4mpe::start_timer();

  sls4mpe::assignmentManager.M_MPE = false;
  sls4mpe::assignmentManager.optimalLogMPEValue = DOUBLE_BIG;

  sls4mpe::network_filename[0] = '\0';
  sls4mpe::sls_filename[0] = '\0';
  strncpy(sls4mpe::network_filename, filename.c_str(), filename.size());

  sls4mpe::maxRuns = iter;
  sls4mpe::maxTime = time;

  sls4mpe::preprocessingSizeBound = 0;

  sls4mpe::ProblemReader pReader;
  pReader.readNetwork();

#if false
  /* the following emulates the readUAI() method */
  sls4mpe::num_vars = prob->getN();
  sls4mpe::num_pots = prob->getC();
  sls4mpe::allocateVarsAndPTs(false);

  for (int i=0; i < prob->getN(); ++i)
    sls4mpe::variables[i]->setDomainSize(prob->getDomainSize(i));

  for (int i=0; i < prob->getC(); ++i) {
    Function* fun = prob->getFunctions()[i];

    sls4mpe::probTables[i]->init(fun->getArity());
    sls4mpe::probTables[i]->setNumEntries(fun->getTableSize());
    int j=0;
    for (set<int>::const_iterator it = fun->getScope().begin();
         it != fun->getScope().end(); ++it, ++j) {
      sls4mpe::probTables[i]->setVar(j, *it);
    }

    double* table = fun->getTable();
    for (size_t j = 0; j < fun->getTableSize(); ++j)
      sls4mpe::probTables[i]->setEntry(j,table[j]);

  }
#endif
  return true;
}

bool SLSWrapper::run() {
  m_assignment = new int[sls4mpe::num_vars];
  sls4mpe::start_timer();
  sls4mpe::runAlgorithm(&m_assignment, &m_likelihood);
  return true;
}

double SLSWrapper::getSolution(vector<val_t>* tuple) const {
  if (tuple) {
    tuple->clear();
    tuple->resize(sls4mpe::num_vars);
    for (int i=0; i<sls4mpe::num_vars; ++i) {
      tuple->at(i) = m_assignment[i];
    }
  }
  return m_likelihood;
}
