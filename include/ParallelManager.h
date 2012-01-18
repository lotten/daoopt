/*
 * ParallelManager.h
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
 *  Created on: Apr 11, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef PARALLELMANAGER_H_
#define PARALLELMANAGER_H_

#include "_base.h"

#ifdef PARALLEL_STATIC

#include "ProgramOptions.h"
#include "Search.h"
#include "LimitedDiscrepancy.h"
#include "BranchAndBoundSampler.h"
#include "BoundPropagator.h"
#include "LearningEngine.h"
#include "utils.h"

class ParallelManager : virtual public Search {

protected:

  /* counter for subproblem IDs */
  size_t m_subprobCount;
  ProgramOptions* m_options;

  /* for LDS */
//  SearchSpace* m_ldsSpace;  // plain pointer to avoid destructor call
  scoped_ptr<LimitedDiscrepancy> m_ldsSearch;
  scoped_ptr<BoundPropagator> m_ldsProp;

  /* for complexity prediction */
  scoped_ptr<LearningEngine> m_learner;

  /* for sampling subproblems */
  scoped_ptr<SearchSpace> m_sampleSpace;
  scoped_ptr<Search> m_sampleSearch;

  /* stack for local solving */
  stack<SearchNode*> m_stack;

  /* subproblems that are easy enough for local solving */
  vector<SearchNode*> m_local;

  /* subproblems that have been submitted to the grid */
  vector<SearchNode*> m_external;

  /* for propagating leaf nodes */
  BoundPropagator m_prop;

protected:
  /* implemented from Search class */
  bool isDone() const;
  bool doExpand(SearchNode* n);
  void reset(SearchNode* p);
  SearchNode* nextNode();
  bool isMaster() const { return true; }

/*
public:
  void setInitialSolution(double
#ifndef NO_ASSIGNMENT
   ,const vector<val_t>&
#endif
  ) const;
*/

protected:
  /* moves the frontier one step deeper by splitting the given node */
  bool deepenFrontier(SearchNode*, vector<SearchNode*>& out);
  /* evaluates a node (wrt. order in the queue) */
  double evaluate(const SearchNode*) const;
  /* filters out easy subproblems */
  bool isEasy(const SearchNode*) const;
  /* applies LDS to the subproblem, i.e. mini bucket forward pass;
   * returns true if subproblem was solved fully */
  bool applyLDS(SearchNode*);

  /* compiles the name of a temporary file */
  string filename(const char* pre, const char* ext, int count = NONE) const;
  /* submits jobs to the grid system (Condor) */
  bool submitToGrid() const;
  /* waits for all external jobs to finish */
  bool waitForGrid() const;

  /* creates the encoding of subproblems for the condor submission */
  string encodeJobs(const vector<SearchNode*>&) const;
  /* solves a subproblem locally through AOBB */
  void solveLocal(SearchNode*);


public:
  /* stores the lower bound to file for subsequent retrieval */
  bool storeLowerBound() const;
  /* loads lower bound from file (post mode) */
  bool loadLowerBound();
  /* performs subproblem sampling and learns a model to predict complexities */
  bool doLearning();
  /* computes the parallel frontier and , returns true iff successful */
  bool findFrontier();
  /* writes subproblem information to disk */
  bool writeJobs() const;
  /* initiates parallel subproblem computation through Condor */
  bool runCondor() const;
  /* parses the results from external subproblems */
  bool readExtResults();
  /* recreates the frontier given a previously written subproblem file */
  bool restoreFrontier();

  /* implemented from Search */
  count_t getSubproblemCount() const { return m_subprobCount; }

public:
  ParallelManager(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h);
  virtual ~ParallelManager() {}

};


/* Inline definitions */

inline bool ParallelManager::isDone() const {
  assert(false); // should never be called
  return false;
}

inline void ParallelManager::reset(SearchNode* n) {
  m_external.clear();
  m_local.clear();
  m_external.push_back(n);
}

#endif /* PARALLEL_STATIC */

#endif /* PARALLELMANAGER_H_ */


