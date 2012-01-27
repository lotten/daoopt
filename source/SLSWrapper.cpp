/*
 * SLSWrapper.cpp
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
 *      Author: lars
 */

#include "SLSWrapper.h"
#ifdef ENABLE_SLS

bool SLSWrapper::init(Problem* prob, int iter, int time) {

  assert(prob);
  m_problem = prob;

  sls4mpe::verbose = false;
  sls4mpe::start_timer();

  sls4mpe::assignmentManager = new sls4mpe::AssignmentManager();

  sls4mpe::assignmentManager->M_MPE = false;
  sls4mpe::assignmentManager->optimalLogMPEValue = DOUBLE_BIG;

  sls4mpe::network_filename[0] = '\0';
  sls4mpe::sls_filename[0] = '\0';
  //strncpy(sls4mpe::network_filename, filename.c_str(), filename.size());

  sls4mpe::maxRuns = iter;
  sls4mpe::maxTime = time;

  sls4mpe::preprocessingSizeBound = 0;

  sls4mpe::slsWrapper = this;

  // load network directly into SLS from Problem*
  sls4mpe::num_vars = prob->getN() - ((prob->hasDummy()) ? 1 : 0);  // -1 for dummy variable
  sls4mpe::num_pots = prob->getC() + 1; // for global constant dummy function
  sls4mpe::allocateVarsAndPTs(false);

  for (int i=0; i < sls4mpe::num_vars; ++i)
    sls4mpe::variables[i]->setDomainSize(prob->getDomainSize(i));

  for (int i=0; i < prob->getC(); ++i) {
    Function* fn = prob->getFunctions().at(i);

    sls4mpe::probTables[i]->init(fn->getArity());
    int j = 0;
    for (set<int>::const_iterator it = fn->getScope().begin();
         it != fn->getScope().end(); ++it, ++j)
      sls4mpe::probTables[i]->setVar(j, *it);

    sls4mpe::probTables[i]->setNumEntries(fn->getTableSize());
    for (size_t j = 0; j < fn->getTableSize(); ++j) {
#ifdef USE_LOG
      sls4mpe::probTables[i]->setLogEntry(j, fn->getTable()[j]);
#else
      sls4mpe::probTables[i]->setEntry(j, fn->getTable()[j]);
#endif
    }
  }

  // dummy function for global constant
  int i = sls4mpe::num_pots - 1;
  sls4mpe::probTables[i]->init(0);
  sls4mpe::probTables[i]->setNumEntries(1);
#ifdef USE_LOG
  sls4mpe::probTables[i]->setLogEntry(0, prob->getGlobalConstant());
#else
  sls4mpe::probTables[i]->setEntry(0, prob->getGlobalConstant());
#endif


  return true;
}


void SLSWrapper::reportSolution(double cost, int num_vars, int* assignment) {
  cost += sls4mpe::EPS;  // EPS needed to avoid floating point precision issues
#ifndef NO_ASSIGNMENT
  assert(assignment);
  vector<val_t> assigVec(num_vars);
  for (int i = 0; i < num_vars; ++i)
    assigVec[i] = assignment[i];
  m_problem->updateSolution(cost, assigVec, make_pair(0,0), true);
#else
  m_problem->updateSolution(cost, make_pair(0,0), true);
#endif
}


bool SLSWrapper::run() {
  m_assignment = new int[sls4mpe::num_vars];
  sls4mpe::start_timer();
  sls4mpe::runAlgorithm(&m_assignment, &m_likelihood);
  sls4mpe::deallocateVarsAndPTs(false);
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
  return m_likelihood + sls4mpe::EPS;  // EPS for floating point precision issues
}

#endif  /* ENABLE_SLS */

