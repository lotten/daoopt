/*
 * BranchAndBound.h
 *
 *  Created on: Nov 3, 2008
 *      Author: lars
 */

#ifndef BRANCHANDBOUND_H_
#define BRANCHANDBOUND_H_

#include "Search.h"
#include "SearchSpace.h"
#include "ProgramOptions.h"

#ifdef PARALLEL_MODE
#include "SubproblemHandler.h"
#include "SubproblemCondor.h"
#endif

#include "debug.h"

class BranchAndBound : public Search {

protected:
  size_t m_nodesOR;           // Keeps track of the number of OR nodes
  size_t m_nodesAND;          // Keeps track of the number of AND nodes
  size_t m_nextThreadId;      // Next subproblem thread id
  SearchNode* m_nextLeaf;     // Next leaf node to hand on to processing (set by expandNext() )
  stack<SearchNode*> m_stack; // The DFS stack of nodes
  vector<val_t> m_assignment;   // The current (partial assignment)

public:

  // operator for thread execution
#ifdef PARALLEL_MODE
  // operator for calling as a separate thread
  void operator() ();
#else
  // for single-threaded execution, returns the next leaf node
  SearchNode* nextLeaf();
#endif

protected:
  // expands the next node on the stack
  void expandNext();

  // the next two functions add context information to a search node. The difference
  // between the Cache and the Subprob version is that Cache might only be the partial
  // context (for adaptive caching)
#ifndef NO_CACHING
  void addCacheContext(SearchNode*, const set<int>&) const;
#endif

#ifdef PARALLEL_MODE
  void addSubprobContext(SearchNode*, const set<int>&) const;
#endif

  // checks if the node can be pruned (only meant for AND nodes)
  bool canBePruned(SearchNode*) const;
#ifdef PARALLEL_MODE
  // adds PST information for advanced pruning in (external) subproblem
  void addPSTlist(SearchNode* node) const;
#endif
  // computes the heuristic of a new OR node, which requires precomputing
  // its child AND nodes' heuristic and label values, which are cached
  // for their explicit generation
  double heuristicOR(SearchNode*);

public:
  // returns true if the search space has been explored fully
  bool isDone() const;
  double getCurOptValue() const;
#ifndef NO_ASSIGNMENT
  const vector<val_t>& getCurOptTuple() const;
#endif

  size_t getNodesOR() const { return m_nodesOR; }
  size_t getNodesAND() const { return m_nodesAND; }
  size_t getThreadCount() const { return m_nextThreadId; }

  const vector<val_t>& getAssignment() const { return m_assignment; }

public:
  void restrictSubproblem(const string& s);

public:
  BranchAndBound(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur);
};


// Inline definitions

inline bool BranchAndBound::isDone() const { return m_stack.empty(); }

inline double BranchAndBound::getCurOptValue() const {
  assert(m_space);
  return m_space->getTrueRoot()->getValue();
}

#ifndef NO_ASSIGNMENT
inline const vector<val_t>& BranchAndBound::getCurOptTuple() const {
  assert(m_space);
  return m_space->getTrueRoot()->getOptAssig();
}
#endif

#endif /* BRANCHANDBOUND_H_ */
