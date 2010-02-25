/*
 * Graph.h
 *
 *  Created on: Oct 21, 2008
 *      Author: lars
 */

#ifndef GRAPH_H_
#define GRAPH_H_

/* uncomment to use separate edge list, uses more memory but
 * speeds up the hasEdge() queries */
//#define USE_EDGE_LIST
/* uncomment to use adjacency matrix, uses even more memory but
 * yields fastest hasEdge() queries */
#define USE_ADJ_MATRIX

/* use at most one of adj. matrix or edge list, prefer matrix */
#ifdef USE_ADJ_MATRIX
#undef USE_EDGE_LIST
#endif


/* class to be used for the neighbor sets, either hash_map or map
 * (in preliminary tests map was faster?)
 * UPDATE: Replaced by TR1 unordered map and set. */
//#define MAPCLASS unordered_map
#define MAPCLASS hash_map
//#define SETCLASS unordered_set
#define SETCLASS hash_set

#include "_base.h"

#ifdef USE_EDGE_LIST
typedef pair<int,int> Edge;
#endif

/*
 * Implements an undirected graph. Internal representation
 * is an adjacency list, which stores the neighbors of each
 * node. It could could thus represent a directed graph, but
 * in the implementation edges and their reverse edges are
 * kept in sync.
 * If USE_EDGE_LIST is defined, a global list of edges is
 * maintained as well, to speed up hasEdge() queries.
 * It USE_ADJ_MATRIX is defined, an adjacency matrix is
 * maintained, speeding up hasEdge() queries, but at increased
 * memory cost.
 */
class Graph {
protected:
  MAPCLASS<int, set<int> > m_neighbors;   // Adjacency lists
#ifdef USE_EDGE_LIST
  SETCLASS<Edge> m_edges;                 // Edge list
#endif
#ifdef USE_ADJ_MATRIX
  int m_n;
  vector<bool> m_matrix;                  // Adjacency matrix
#endif

  int m_statNumNodes;                      // No. of vertices
  int m_statNumEdges;                      // No. of edges

public:
  Graph(const int& n);

public:
  const set<int>& getNeighbors(const int& var);
  set<int>  getNodes();

  // Statistics
public:
  int getStatNodes() { return m_statNumNodes; }
  int getStatEdges() { return m_statNumEdges; }
  double getStatDensity();

public:
  void addNode(const int& i);
  void addEdge(const int& i, const int& j);

  void removeNode(const int& i);
  void removeEdge(const int& i, const int& j);

  bool hasNode(const int& i);
  bool hasEdge(const int& i, const int& j);

public:
  double scoreMinfill(const int& i);

protected:
  void addAdjacency(const int& i, const int& j);
  void removeAdjacency(const int& i, const int& j);

public:
  void addClique(const set<int>& s);

  // finds the connected components of the given set
  map<int,set<int> > connectedComponents(const set<int>&);
  size_t noComponents();

};

/****************************
 *  Inline implementations  *
 ****************************/

/* Constructor */
inline Graph::Graph(const int& n) : m_statNumNodes(0), m_statNumEdges(0) {
#if defined HASH_GOOGLE_DENSE | defined HASH_GOOGLE_SPARSE
  m_neighbors.set_deleted_key(UNKNOWN);
#endif
#ifdef HASH_GOOGLE_DENSE
  m_neighbors.set_empty_key(UNKNOWN-1);
#endif
#ifdef USE_ADJ_MATRIX
  m_n = n;
  m_matrix.resize(n*n);
#endif
}


/* Returns a node's neighbors */
inline const set<int>& Graph::getNeighbors(const int& var) {
  MAPCLASS<int,set<int> >::iterator it = m_neighbors.find(var);
  assert(it != m_neighbors.end());
  return it->second;
}


/* returns the set of graph nodes */
inline set<int> Graph::getNodes() {
  set<int> s;
  for (MAPCLASS<int, set<int> >::iterator it = m_neighbors.begin(); it != m_neighbors.end(); ++it) {
    s.insert(it->first);
  }
  return s;
}

/* Computes the graph density*/
inline double Graph::getStatDensity() {
  if (m_statNumNodes)
    return 2.0*m_statNumEdges / (m_statNumNodes) / (m_statNumNodes-1);
  else
    return 0.0;
}

/* Adds a node to the graph */
inline void Graph::addNode(const int& i) {
  if (m_neighbors.find(i) == m_neighbors.end()) {
    m_neighbors.insert(make_pair(i, set<int>() ));
    ++m_statNumNodes;
  }
}


/* Adds the edge (i,j) to the graph, also adds the reversed edge. */
inline void Graph::addEdge(const int& i, const int& j) {
  addAdjacency(i,j);
  addAdjacency(j,i);
#ifdef USE_EDGE_LIST
  if (i<j) m_edges.insert(make_pair(i,j));
  else m_edges.insert(make_pair(j,i));
#endif
  ++m_statNumEdges;
}


/* removes the node and all related edges from the graph */
inline void Graph::removeNode(const int& i) {
  MAPCLASS<int,set<int> >::iterator iti = m_neighbors.find(i);
  if (iti != m_neighbors.end()) {
    set<int>& S = iti->second;
    for (set<int>::iterator it = S.begin(); it != S.end(); ++it) {
      removeAdjacency(*it, i);
      --m_statNumEdges;
    }
    m_neighbors.erase(iti);
    --m_statNumNodes;
  }
}


/* removes a single (undirected) edge from the graph */
inline void Graph::removeEdge(const int& i, const int& j) {
  removeAdjacency(i,j);
  removeAdjacency(j,i);
#ifdef USE_EDGE_LIST
  if (i<j) m_edges.erase(make_pair(i,j));
  else m_edges.erase(make_pair(j,i));
#endif
  --m_statNumEdges;
}


/* returns TRUE iff node i is in the graph */
inline bool Graph::hasNode(const int& i) {
  MAPCLASS<int,set<int> >::iterator iti = m_neighbors.find(i);
  return (iti != m_neighbors.end());
}


/* returns TRUE iff edge between nodes i and j exists */
inline bool Graph::hasEdge(const int& i, const int& j) {
#ifdef USE_ADJ_MATRIX
  return m_matrix[i*m_n+j];
#endif
#ifdef USE_EDGE_LIST
  if (i<j) return m_edges.find(make_pair(i,j)) != m_edges.end();
  else return m_edges.find(make_pair(j,i)) != m_edges.end();
#else
  MAPCLASS<int,set<int> >::iterator iti = m_neighbors.find(i);
  if (iti != m_neighbors.end()) {
    set<int>::iterator itj = iti->second.find(j);
    return itj != iti->second.end();
  } else
    return false;
#endif
}


/* Adds the edge (i,j) to the adjacency list. Does not add the reversed edge. */
inline void Graph::addAdjacency(const int& i, const int& j) {
  // Make sure node exists
  addNode(i);
  // Add edge (i,j) to the graph.
  MAPCLASS<int,set<int> >::iterator it = m_neighbors.find(i); // guaranteed to find node
  set<int>& s = it->second;
  //if (s.find(j) == s.end())  // TODO check not required?
    s.insert(j);
#ifdef USE_ADJ_MATRIX
  m_matrix[i*m_n+j] = 1;
#endif
}


/* Removes the adjacency entry (i,j) but NOT the reverse (j,i) */
inline void Graph::removeAdjacency(const int& i, const int& j) {
  MAPCLASS<int,set<int> >::iterator iti = m_neighbors.find(i);
  if (iti != m_neighbors.end()) {
    iti->second.erase(j);
  }
#ifdef USE_ADJ_MATRIX
  m_matrix[i*m_n+j] = 0;
#endif
}


/* Adds the nodes in s to the graph and fully connects them */
inline void Graph::addClique(const set<int>& s) {
  for (set<int>::const_iterator it = s.begin(); it != s.end(); ++it) {
    addNode(*it); // insert the node
    set<int>::const_iterator it2 = it;
    while (++it2 != s.end()) {
      addEdge(*it,*it2);
    }
  }
}



#endif /* GRAPH_H_ */
