// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_GRAPH_H
#define __MEX_GRAPH_H

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdint.h>

#include "vector.h"
#include "set.h"
#include "stack.h"
#include "mxObject.h"
#include "mxUtil.h"

/*
*/

namespace mex {

// Graph Class : contain "nodes" (0..N-1) and "edges", with bidirectional indexing (0..2E-1)
//
// EdgeID represents edge (i->j), with index "idx"; "reverse" index ridx represents (j->i)
//   "blank" (missing) edges can sometimes appear, represented by EdgeID::NO_EDGE
//   (for example if edge (i,j) is requested and does not exist)

typedef std::pair<mex::midx<uint32_t>,mex::midx<uint32_t> > Edge;
struct EdgeID { 
	typedef mex::midx<uint32_t> index;
	index first, second;	// Edge e;
	index idx, ridx; 
	EdgeID() : first(), second(), idx(), ridx() { }
	EdgeID(index i, index j, index eij, index eji) : first(i), second(j), idx(eij), ridx(eji) { }
	operator Edge() const { return Edge(first,second); }
	static const EdgeID NO_EDGE; 
	bool operator==(const EdgeID& e) const { return e.first==first && e.second==second; }
	bool operator!=(const EdgeID& e) const { return !(*this==e); }
	bool operator< (const EdgeID& e) const { return (first < e.first || (first==e.first && second<e.second)); }
};
template <>
struct mexable_traits<mex::EdgeID > {
	typedef mex::midx<uint32_t>   base;
	typedef uint32_t              mxbase;
	static const int mxsize=4;
};

typedef mex::vector<Edge>           RootedTree;


//
class Graph : virtual public mxObject {
public:
	typedef EdgeID::index index;	 				// basic indexing type for graph
	mex::vector<mex::set<EdgeID> > _adj;			// look up EdgeID info by adj[i][jj]  (jj = position of j)
	mex::vector<EdgeID> _edges;								// look up EdgeID info by edges[eij]  (edge index)
	mex::stack<index,double> _vVacant;				// list of available vertex ids
	mex::stack<index,double> _eVacant;				// list of available edge ids

  explicit Graph(size_t n=0) : _adj(), _edges(), _vVacant(), _eVacant() { _adj.resize(n); }

	index addNode() {																// verify that the adjacency table is large enough
		index use;																		//   and get an index off the stack if available
		if (_vVacant.empty()) {
			use = nNodes();
			_adj.resize(nNodes()+1);
		} else {
			use = _vVacant.top(); _vVacant.pop();
		}
		return use;
	}

	void removeNode(index i) {		// pop_back, or add to vVacant
		_vVacant.push(i);						// can we keep track of "real" nNodes and iterate through them?
	}

	size_t nNodes() { return _adj.size(); }
	size_t nEdges() { return _edges.size()/2; }

	const EdgeID& addEdge(index i, index j) {				// add edges (i,j) and (j,i) to adj
		//std::cout<<"Add edge "<<i<<","<<j<<"\n";
		if (edge(i,j) != EdgeID::NO_EDGE) return edge(i,j);	// if exists already do nothing
		size_t eij, eji, emax=2*nEdges();								// otherwise get two edge indices
		if (_eVacant.empty()) { eij=emax++; _edges.resize(emax); } else { eij=_eVacant.top(); _eVacant.pop(); }
		if (_eVacant.empty()) { eji=emax++; _edges.resize(emax); } else { eji=_eVacant.top(); _eVacant.pop(); }
		_edges[eij] = EdgeID(i,j,eij,eji); _adj[i] |= _edges[eij];
		_edges[eji] = EdgeID(j,i,eji,eij); _adj[j] |= _edges[eji];
		return _edges[eij];
	}

	void removeEdge(index i, index j) {							// remove edges (i,j) and (j,i) from adj; add to eVacant
		if (edge(i,j) != EdgeID::NO_EDGE) {										// 
			EdgeID e = edge(i,j);
			index eij=e.idx, eji = e.ridx;
			_eVacant.push(eij); _eVacant.push(eji);// add eij, eji to edge stack
			_adj[i] /= _edges[eij]; _edges[eij]=EdgeID::NO_EDGE;
			_adj[j] /= _edges[eji]; _edges[eji]=EdgeID::NO_EDGE;
		}
	}

	void clear() { 
		clearEdges(); 
		while (!_vVacant.empty()) _vVacant.pop(); 
		_adj.resize(0);
	}

	void clearEdges() { 
		_edges.clear(); 
		size_t N=nNodes(); _adj.clear(); _adj.resize(N); 
		while (!_eVacant.empty()) _eVacant.pop(); 
	}

	const EdgeID& edge(index i, index j) const {
		if (i>=_adj.size())    return EdgeID::NO_EDGE;
		mex::set<EdgeID>::const_iterator it = _adj[i].find( EdgeID(i,j,0,0) );
		if (it==_adj[i].end()) return EdgeID::NO_EDGE;
		else                   return *it;
	}

	const mex::set<EdgeID>& neighbors(index i) const { return _adj[i]; } 
	const EdgeID& edge(index eij) const { return _edges[eij]; }

	const mex::vector<EdgeID>& edges() const { return _edges; }

	//void swap(Graph& G); 

#ifdef MEX  
  // MEX Class Wrapper Functions //////////////////////////////////////////////////////////
  bool mxCheckValid(const mxArray* M) {
			//std::cout<<_vVacant.mxCheckValid( mxGetField(M,0,"vacant"), mxGetField(M,0,"nVacant") )<<
      //        _eVacant.mxCheckValid( mxGetField(M,0,"eVacant"), mxGetField(M,0,"neVacant") )<<
      //        _edges.mxCheckValid( mxGetField(M,0,"edges") ) <<
      //        _adj.mxCheckValid( mxGetField(M,0,"adj") ) << "\n";

       return true &&  // No inheritance to check
              _vVacant.mxCheckValid( mxGetField(M,0,"vacant"), mxGetField(M,0,"nVacant") ) &&	
              _eVacant.mxCheckValid( mxGetField(M,0,"eVacant"), mxGetField(M,0,"neVacant") ) &&	
              _edges.mxCheckValid( mxGetField(M,0,"edges") ) &&
              _adj.mxCheckValid( mxGetField(M,0,"adj") );
	}
  void mxSet(mxArray* M) { 
			        // No inheritance to set
              _vVacant.mxSet( mxGetField(M,0,"vacant"), mxGetField(M,0,"nVacant") );
              _eVacant.mxSet( mxGetField(M,0,"eVacant"), mxGetField(M,0,"neVacant") );
              _edges.mxSet( mxGetField(M,0,"edges") );
              _adj.mxSet( mxGetField(M,0,"adj") );
	}
  mxArray*    mxGet()     { throw std::runtime_error("Not implemented"); }
  void        mxRelease() { throw std::runtime_error("Not implemented"); }
  void        mxDestroy() { throw std::runtime_error("Not implemented"); }
  void        mxSwap(Graph& g) { throw std::runtime_error("Not implemented"); }
  /////////////////////////////////////////////////////////////////////////////////////////
#endif

};




//////////////////////////////////////////////////////////////////////////////////////////////
}       // namespace mex

inline std::ostream& operator<<( std::ostream& out, const mex::Edge& e) { return out<<"("<<(int)e.first<<","<<(int)e.second<<")"; }
inline std::ostream& operator<<( std::ostream& out, const mex::EdgeID& e) { return out<<(mex::Edge)e; }

#endif  // re-include
