/*
 * Pseudotree.h
 *
 *  Created on: Oct 24, 2008
 *      Author: lars
 */

#ifndef PSEUDOTREE_H_
#define PSEUDOTREE_H_

#include "float.h"

#include "_base.h"
#include "Graph.h"
#include "Function.h"
#include "Problem.h"


// forward declaration
class PseudotreeNode;


class Pseudotree {
protected:
  PseudotreeNode* m_root;

  int m_depth;
  int m_width;
  int m_components;

  vector<PseudotreeNode*> m_nodes;
  vector<list<PseudotreeNode*> > m_levels;
  vector<int> m_elimOrder;

public:
  // computes an elimination order into 'elim' and returns
  // its induced width
  static int eliminate(Graph G, vector<int>& elim);

  // builds the pseudo tree according to order 'elim'
  void build(Graph G, const vector<int>& elim, const int cachelimit = NONE);

  // returns the width of the pseudo tree
  int getWidth() const { return m_width; }

  int getDepth() const { return m_depth; }

  int getComponents() const { return m_components; }

  // returns the root node
  PseudotreeNode* getRoot() const { return m_root; }

  // returns the pseudotree node for variable i
  PseudotreeNode* getNode (int i) const;

  const vector<int>& getElimOrder() const { return m_elimOrder; }

  // augments the pseudo tree with function information
  void addFunctionInfo(const vector<Function*>& fns);

#ifdef PARALLEL_MODE
  // computes the subproblem complexity parameters for all subtrees
  void computeComplexities(const Problem& problem);
#endif

  const list<Function*>& getFunctions(int i) const;

  void restrictSubproblem(int i);

protected:
  // creates a new node in the PT for variable i, with context N. Also makes sure
  // existing roots are checked and connected appropriately.
  void insertNewNode(const int& i, const set<int>& N, list<PseudotreeNode*>& roots);

public:
  Pseudotree(const int& n);
  ~Pseudotree();
};



class PseudotreeNode {

#ifdef PARALLEL_MODE
protected:
  // internal container class for subproblem complexity information
  class Complexity {
  public:
    int subwidth; // Induced width of subproblem if conditioned on context
    bigint subsize; // Upper bound on subproblem size if conditioned on context
    bigint ownsize; // Own cluster size, i.e. product of cluster variable domain sizes
    bigint numContexts; // Number of possible context instantiations
  public:
    Complexity() : subwidth(UNKNOWN), subsize(UNKNOWN), ownsize(UNKNOWN), numContexts(UNKNOWN) {}
    Complexity(int i, bigint b, bigint o, bigint n) : subwidth(i), subsize(b), ownsize(o), numContexts(n) {}
    Complexity(const Complexity& c) : subwidth(c.subwidth), subsize(c.subsize), ownsize(c.ownsize), numContexts(c.numContexts) {}
  };
#endif

protected:
  int m_var; // The node variable
  int m_depth; // The node's depth in the tree
  PseudotreeNode* m_parent; // The parent node
#ifdef PARALLEL_MODE
  Complexity* m_complexity; // Contains information about subproblem complexity
#endif
  set<int> m_subproblemVars; // The variables in the subproblem (including self)
  set<int> m_context; // The node's full OR context
  set<int> m_cacheContext; // The (possibly smaller) context for (adaptive) caching
  list<int> m_cacheResetList; // List of var's whose cache tables need to be reset when this
                              // var's search node is expanded (for adaptive caching)
  list<Function*> m_functions; // The functions that will be fully instantiated at this point
  vector<PseudotreeNode*> m_children; // The node's children

public:
  void addToContext(int v) { m_context.insert(v); }
  void setContext(const set<int>& c) { m_context = c; }
  void setCacheContext(const set<int>& c) { m_cacheContext = c; }
  void addCacheContext(int i) { m_cacheContext.insert(i); }
  void addCacheReset(int i) { m_cacheResetList.push_back(i); }
  void addChild(PseudotreeNode* p) { m_children.push_back(p); }
  void setChild(PseudotreeNode* p) { m_children.clear(); m_children.push_back(p); }
  void setParent(PseudotreeNode* p) { m_parent = p; }
  void addFunction(Function* f) { m_functions.push_back(f); }

  PseudotreeNode* getParent() const { return m_parent; }
  const vector<PseudotreeNode*>& getChildren() const { return m_children; }
  const set<int>& getFullContext() const { return m_context; }
  const set<int>& getCacheContext() const { return m_cacheContext; }
  const list<int>& getCacheResetList() const { return m_cacheResetList; }

  const list<Function*>& getFunctions() const { return m_functions; }

  int getVar() const { return m_var; }
  int getDepth() const { return m_depth; }
  size_t getSubprobSize() const { return m_subproblemVars.size(); }
  const set<int>& getSubprobVars() const { return m_subproblemVars; }
#ifdef PARALLEL_MODE
  int getSubwidth() const { assert(m_complexity); return m_complexity->subwidth; }
  bigint getSubsize() const { assert(m_complexity); return m_complexity->subsize; }
  bigint getOwnsize() const { assert(m_complexity); return m_complexity->ownsize; }
  bigint getNumContexts() const { assert(m_complexity); return m_complexity->numContexts; }
#endif

#ifdef PARALLEL_MODE
  void initSubproblemComplexity(const vector<val_t>& domains);
#endif
  const set<int>& updateSubprobVars();
  int updateDepth(int d);

//protected:
//  Complexity computeSubCompRec(const set<int>& A) const;

public:
  PseudotreeNode(int v, const set<int>& s);
  ~PseudotreeNode() {
#ifdef PARALLEL_MODE
    if (m_complexity) delete m_complexity;
#endif
    }
};


// Inline definitions

inline PseudotreeNode* Pseudotree::getNode(int i) const {
  assert (i<(int)m_nodes.size());
  return m_nodes[i];
}

inline const list<Function*>& Pseudotree::getFunctions(int i) const {
  assert (i < (int) m_nodes.size());
  return m_nodes[i]->getFunctions();
}

inline Pseudotree::Pseudotree(const int& n) :
  m_root(NULL), m_depth(UNKNOWN), m_width(0), m_components(0), m_nodes(n) {}

inline Pseudotree::~Pseudotree() {
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
    if (*it) delete *it;
  }
}

// PseudotreeNode

//inline void PseudotreeNode::initSubproblemComplexity() {
//  m_complexity = new Complexity(this->computeSubCompRec(m_context));
//}

// Constructor
inline PseudotreeNode::PseudotreeNode(int v, const set<int>& s) :
#ifdef PARALLEL_MODE
  m_var(v), m_depth(UNKNOWN), m_parent(NULL), m_complexity(NULL), m_context(s) {}
#else
  m_var(v), m_depth(UNKNOWN), m_parent(NULL), m_context(s) {}
#endif

#endif /* PSEUDOTREE_H_ */
