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

/* All search algorithms should inherit from this */
class Search {

protected:

  count_t m_nodesOR;             // Keeps track of the number of OR nodes
  count_t m_nodesAND;            // Keeps track of the number of AND nodes
  count_t m_nextThreadId;        // Next subproblem thread id

  Problem* m_problem;           // The problem instance
  Pseudotree* m_pseudotree;     // Pseudo tree
  SearchSpace* m_space;         // Search space (incl. cache table)
  Heuristic* m_heuristic;       // Heuristic for search
#ifdef PARALLEL_MODE
  Subproblem* m_nextSubprob;    // Next subproblem for external solving (set by expandNext() )
#endif

#ifdef PARALLEL_MODE
  bigint m_twbCache;            // caches the twb bound
  bigint m_hwbCache;            // caches the hwb bound
#endif

  vector<count_t> m_nodeProfile; // keeps track of AND node count per depth
  vector<count_t> m_leafProfile; // keeps track of leaf AND nodes per depth

  vector<val_t> m_assignment;   // The current (partial assignment)

#ifdef PARALLEL_MODE
  /* keeps tracks up lower/upper bound on first OR node generated for
   * each depth level. used for initialization of cutoff scheme. */
//  vector<pair<double,double> > m_bounds;
#endif

public:
  /* for single-threaded execution, returns the next leaf node */
  SearchNode* nextLeaf() ;

public:
  virtual bool isDone() const = 0;

  count_t getNoNodesOR() const { return m_nodesOR; }
  count_t getNoNodesAND() const { return m_nodesAND; }
#ifdef PARALLEL_MODE
  count_t getSubCount() const { return m_space->getTrueRoot()->getSubCount(); }
#endif
  pair<count_t,count_t> getNoNodes() const { return make_pair(m_nodesOR, m_nodesAND); }
  count_t getThreadCount() const { return m_nextThreadId; }

  const vector<count_t>& getNodeProfile() const { return m_nodeProfile; }
  const vector<count_t>& getLeafProfile() const { return m_leafProfile; }
  const vector<val_t>& getAssignment() const { return m_assignment; }

  // returns the current lower bound on the root problem solution
  // (mostly makes sense for conditioned subproblems)
  double curLowerBound() const { return lowerBound(m_space->getTrueRoot()); }

  // cur value of root OR node
  double getCurOptValue() const;
#ifndef NO_ASSIGNMENT
  const vector<val_t>& getCurOptTuple() const;
#endif

//  void outputAndSaveSolution(const string& filename) const;

  // restricts search to a subproblem as specified in file with path 's'
  // parses the file and then calls the next function.
  // returns true on success, false otherwise
  bool restrictSubproblem(const string& s);

  // restricts search to a subproblem rooted at 'rootVar'. Context instantiation
  // is extracted from 'assig', ancestral partial solution tree from 'pst' vector.
  // pst containst [OR value, AND label] top-down.
  // returns the (original) depth of the new root node
  int restrictSubproblem(int rootVar, const vector<val_t>& assig, const vector<double>& pst);

  // loads an initial lower bound from a file (in binary, for precision reasons).
  // returns true on success, false on error
  bool loadInitialBound(string);

  // sets the initial lower bound
  virtual void setInitialBound(double) const = 0;

#ifndef NO_ASSIGNMENT
  virtual void setInitialSolution(const vector<val_t>&) const = 0;
#endif

#ifdef PARALLEL_MODE
  // returns the cached lower/upper bounds on the first node at each depth
//  const vector<pair<double,double> >& getBounds() const { return m_bounds; }
#endif

protected:

  // returns true iff search has more nodes to expand
  virtual bool hasMoreNodes() const = 0;

  // resets the queue/stack/etc. to the given node
  virtual void resetSearch(SearchNode*) = 0;

  // returns the next search node for processing (top of stack/queue/etc.)
  virtual SearchNode* nextNode() = 0;

  // expands the current node, returns true if no children were generated
  // (needs to be implemented in derived search classes)
  virtual bool doExpand(SearchNode*) = 0;

protected:

  // processes the current node (value instantiation etc.)
  // if trackHeur==true, caches lower/upper bounds for first node at each depth
  bool doProcess(SearchNode*);

  // performs a cache lookup; if successful, stores value into node and returns true
  bool doCaching(SearchNode*);

  // checks if node can be pruned, returns true if so
  bool doPruning(SearchNode*);

  // checks if the node can be pruned (only meant for AND nodes)
  bool canBePruned(SearchNode*) const;

  // computes the heuristic of a new OR node, which requires precomputing
  // its child AND nodes' heuristic and label values, which are cached
  // for their explicit generation
  double heuristicOR(SearchNode*);

  // returns the current lower bound on the subproblem solution rooted at
  // n, taking into account solutions to parent problems (or the dummy partial
  // solution tree, in case of conditioned subproblems)
  double lowerBound(SearchNode*) const;

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
  virtual bool isMaster() const = 0;
  Search(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h) ;

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
