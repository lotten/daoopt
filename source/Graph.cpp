/*
 * Graph.cpp
 *
 *  Created on: Oct 21, 2008
 *      Author: lars
 */

#include "Graph.h"


double Graph::scoreMinfill(const int& i) {
  MAPCLASS<int,set<int> >::iterator iti = m_neighbors.find(i);
  assert(iti != m_neighbors.end());

  set<int>& S = iti->second;
  double c = 0.0;
  set<int>::iterator it1,it2;
  for (it1 = S.begin(); it1 != S.end(); ++it1) {
    it2 = it1;
    while (++it2!=S.end()) {
      if (!hasEdge(*it1,*it2)) ++c;
    }
  }
  return c;
}


map<int,set<int> > Graph::connectedComponents(const set<int>& s) {

  map<int,set<int> > comps; // will hold the components

  // return zero components for empty argument set
  if (s.size()==0) return comps;

  // keeps track of which node has been visited
  SETCLASS<int> nodes(s.begin(), s.end());
#if defined HASH_GOOGLE_SPARSE | defined HASH_GOOGLE_DENSE
  nodes.set_deleted_key(UNKNOWN);
#endif
#ifdef HASH_GOOGLE_DENSE
  nodes.set_empty_key(UNKNOWN-1);
#endif

  // stack for DFS search
  stack<int> dfs;

  set<int> comp; int c=0; // current component and index
  for ( ; !nodes.empty(); ++c) {

    comp.clear();

    dfs.push(* nodes.begin());
    nodes.erase(dfs.top());

    // depth first search
    while (!dfs.empty()) {
      set<int>& neighbors = m_neighbors[dfs.top()];
      comp.insert(dfs.top()); dfs.pop();
      for (set<int>::const_iterator it = neighbors.begin(); it != neighbors.end(); ++it) {
        if (nodes.find(*it) != nodes.end()) {
          dfs.push(*it);
          nodes.erase(*it);
        }
      }
    }
    // record component
    comps[c] = comp;
  }
  return comps;
}


size_t Graph::noComponents() {

  set<int> nodes;
  for (int i=0; i<m_n; ++i) {
    nodes.insert(i);
  }

  return connectedComponents(nodes).size();

}
