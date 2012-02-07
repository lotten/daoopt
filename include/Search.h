/*
 * Search.h
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
 *  Created on: Nov 3, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef SEARCH_H_
#define SEARCH_H_

#include "SearchSpace.h"
#include "SearchNode.h"
#include "Heuristic.h"
#include "Problem.h"
#include "ProgramOptions.h"
#include "Pseudotree.h"
#include "utils.h"

#ifdef PARALLEL_DYNAMIC
#include "SubproblemHandler.h"
#include "SubproblemCondor.h"
#endif

/* All search algorithms should inherit from this */
class Search {

protected:

//  count_t m_nodesOR;             // Keeps track of the number of OR nodes
//  count_t m_nodesAND;            // Keeps track of the number of AND nodes

  Problem* m_problem;           // The problem instance
  Pseudotree* m_pseudotree;     // Pseudo tree
  SearchSpace* m_space;         // Search space (incl. cache table)
  Heuristic* m_heuristic;       // Heuristic for search
#ifdef PARALLEL_DYNAMIC
  Subproblem* m_nextSubprob;    // Next subproblem for external solving
#endif

#ifdef PARALLEL_DYNAMIC
  bigint m_twbCache;            // caches the twb bound
  bigint m_hwbCache;            // caches the hwb bound
#endif

  vector<count_t> m_nodeProfile; // keeps track of AND node count per depth
  vector<count_t> m_leafProfile; // keeps track of leaf AND nodes per depth

  vector<val_t> m_assignment;   // The current (partial assignment)

  vector<SearchNode*> m_expand;  // Reusable vector for node expansions (to avoid repeated
                                 // (de)allocation of memory)

#ifdef PARALLEL_DYNAMIC
  /* keeps tracks up lower/upper bound on first OR node generated for
   * each depth level. used for initialization of cutoff scheme. */
//  vector<pair<double,double> > m_bounds;
#endif

public:
  /* returns the next leaf node, NULL if search done */
  SearchNode* nextLeaf() ;

  /* returns true iff the search is complete */
  virtual bool isDone() const = 0;

//  count_t getNoNodesOR() const { return m_nodesOR; }
//  count_t getNoNodesAND() const { return m_nodesAND; }
#ifdef PARALLEL_DYNAMIC
  count_t getSubCount() const { return m_space->getTrueRoot()->getSubCount(); }
#endif
//  pair<count_t,count_t> getNoNodes() const { return make_pair(m_nodesOR, m_nodesAND); }
  virtual count_t getSubproblemCount() const { assert(false); return NONE; }

  const vector<count_t>& getNodeProfile() const { return m_nodeProfile; }
  const vector<count_t>& getLeafProfile() const { return m_leafProfile; }
  const vector<val_t>& getAssignment() const { return m_assignment; }

  /* returns the current lower bound on the root problem solution
   * (mostly makes sense for conditioned subproblems) */
  double curLowerBound() const { return lowerBound(m_space->getTrueRoot()); }

  /* cur value of root OR node */
  double getCurOptValue() const;
#ifndef NO_ASSIGNMENT
  const vector<val_t>& getCurOptTuple() const;
#endif

//  void outputAndSaveSolution(const string& filename) const;

  /* restricts search to a subproblem as specified in file with path 's'
   * parses the file and then calls the next function.
   * returns true on success, false otherwise */
  bool restrictSubproblem(string s);

  /* restricts search to a subproblem rooted at 'rootVar'. Context instantiation
   * is extracted from 'assig', ancestral partial solution tree from 'pst' vector.
   * pst containst [OR value, AND label] top-down.
   * returns the (original) depth of the new root node */
  int restrictSubproblem(int rootVar, const vector<val_t>& assig, const vector<double>& pst);

  /* loads an initial lower bound from a file (in binary, for precision reasons).
   * returns true on success, false on error */
  bool loadInitialBound(string);

  bool updateSolution(double
#ifndef NO_ASSIGNMENT
    , const vector<val_t>&
#endif
  ) const;

  /* resets the queue/stack/etc. to the given node */
  virtual void reset(SearchNode* = NULL) = 0;

#ifdef PARALLEL_DYNAMIC
  /* returns the cached lower/upper bounds on the first node at each depth */
//  const vector<pair<double,double> >& getBounds() const { return m_bounds; }
#endif

protected:

  /* initializes the search space, returns the first node to process */
  SearchNode* initSearch();

  /* returns the next search node for processing (top of stack/queue/etc.) */
  virtual SearchNode* nextNode() = 0;

  /* expands the current node, returns true if no children were generated
   * (needs to be implemented in derived search classes) */
  virtual bool doExpand(SearchNode*) = 0;

  /* processes the current node (value instantiation etc.)
   * if trackHeur==true, caches lower/upper bounds for first node at each depth */
  bool doProcess(SearchNode*);

  /* performs a cache lookup; if successful, stores value into node and returns true */
  bool doCaching(SearchNode*);

  /* checks if node can be pruned, returns true if so */
  bool doPruning(SearchNode*);

  /* synchronizes the global assignment with the given node */
  void syncAssignment(const SearchNode*);

  /* generates the children of an AND node, writes them into argument vector,
   * returns true if no children */
  bool generateChildrenAND(SearchNode*, vector<SearchNode*>&);
  /* generates the children of an OR node, writes them into argument vector,
   * returns true if no children */
  bool generateChildrenOR(SearchNode*, vector<SearchNode*>&);

  /* checks if the node can be pruned (only meant for AND nodes) */
  bool canBePruned(SearchNode*) const;

  /* computes the heuristic of a new OR node, which requires precomputing
   * its child AND nodes' heuristic and label values, which are cached
   * for their explicit generation */
  double heuristicOR(SearchNode*);

  /* returns the current lower bound on the subproblem solution rooted at
   * n, taking into account solutions to parent problems (or the dummy partial
   * solution tree, in case of conditioned subproblems) */
  double lowerBound(const SearchNode*) const;

  /* the next two functions add context information to a search node. The difference
   * between the Cache and the Subprob version is that Cache might only be the partial
   * context (for adaptive caching) */
#ifndef NO_CACHING
  void addCacheContext(SearchNode*, const set<int>&) const;
#endif
#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  /* see comment above */
  void addSubprobContext(SearchNode*, const set<int>&) const;
#endif
#ifdef PARALLEL_DYNAMIC
  /* adds PST information for advanced pruning in (external) subproblem */
  void addPSTlist(SearchNode* node) const;
#endif

protected:
  virtual bool isMaster() const = 0;
  Search(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h) ;
public:
  virtual ~Search() {}

};

/* Inline definitions */

inline double Search::getCurOptValue() const {
  assert(m_space);
  return m_space->getTrueRoot()->getValue();
}

#ifndef NO_ASSIGNMENT
inline const vector<val_t>& Search::getCurOptTuple() const {
  assert(m_space);
  return m_space->getTrueRoot()->getOptAssig();
}
#endif



#endif /* SEARCH_H_ */
