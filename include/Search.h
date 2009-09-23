/*
 * Search.h
 *
 *  Created on: Nov 3, 2008
 *      Author: lars
 */

#ifndef SEARCH_H_
#define SEARCH_H_

#include "SearchSpace.h"
#include "SearchNode.h"
#include "Heuristic.h"
#include "Problem.h"
#include "Pseudotree.h"
#include "utils.h"

#ifdef PARALLEL_MODE
#include "SubproblemHandler.h"
#include "SubproblemCondor.h"
#endif

// All search algorithms should inherit this
class Search {

protected:

  size_t m_nodesOR;             // Keeps track of the number of OR nodes
  size_t m_nodesAND;            // Keeps track of the number of AND nodes
  size_t m_nextThreadId;        // Next subproblem thread id

  Problem* m_problem;           // The problem instance
  Pseudotree* m_pseudotree;     // Pseudo tree
  SearchSpace* m_space;         // Search space (incl. cache table)
  Heuristic* m_heuristic;       // Heuristic for search
  SearchNode* m_nextLeaf;       // Next leaf node to hand on to processing (set by expandNext() )

  vector<val_t> m_assignment;   // The current (partial assignment)

public:
#ifdef PARALLEL_MODE
  // operator for thread execution
  void operator () () ;
#else
  // for single-threaded execution, returns the next leaf node
  SearchNode* nextLeaf() ;
#endif

public:
  virtual bool isDone() const = 0;

  size_t getNodesOR() const { return m_nodesOR; }
  size_t getNodesAND() const { return m_nodesAND; }
  size_t getThreadCount() const { return m_nextThreadId; }

  const vector<val_t>& getAssignment() const { return m_assignment; }

  double getCurOptValue() const;
#ifndef NO_ASSIGNMENT
  const vector<val_t>& getCurOptTuple() const;
#endif

  // restricts search to a subproblem as specified in file with path 's'
  void restrictSubproblem(const string& s);

protected:

  // expands the next node
  virtual void expandNext() = 0;

  // returns true iff search has more nodes to expand
  virtual bool hasMoreNodes() const = 0;

  // resets the queue/stack/etc. to the given node
  virtual void resetSearch(SearchNode*) = 0;

  // returns the first search node (top of stack/queue/etc.)
  virtual SearchNode* firstNode() const = 0;

  // checks if the node can be pruned (only meant for AND nodes)
  bool canBePruned(SearchNode*) const;

  // computes the heuristic of a new OR node, which requires precomputing
  // its child AND nodes' heuristic and label values, which are cached
  // for their explicit generation
  double heuristicOR(SearchNode*);

  // the next two functions add context information to a search node. The difference
  // between the Cache and the Subprob version is that Cache might only be the partial
  // context (for adaptive caching)
#ifndef NO_CACHING
  void addCacheContext(SearchNode*, const set<int>&) const;
#endif
#ifdef PARALLEL_MODE
  // see comment above
  void addSubprobContext(SearchNode*, const set<int>&) const;

  // adds PST information for advanced pruning in (external) subproblem
  void addPSTlist(SearchNode* node) const;
#endif

protected:
  Search(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h) ;

};


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
