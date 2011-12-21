#include <stdlib.h>

#include <Globals.hxx>

#include "Sort.hxx"
#include "AVLtreeSimple.hxx"

#include "Problem.hxx"
#include "Graph.hxx"


extern "C" struct undirected_edge { int _u, _v ; } ;

int ARE::Graph::RemoveRedundantFillEdges(void)
{
	int i ;

	if (_nFillEdges < 1) 
		return 0 ;
	int nNewEdges = _nEdges + _nFillEdges ;
	if (0 != ReAllocateEdges(nNewEdges)) 
		return 1 ;

	// beginning of unused edges list
	AdjVar *AVstart = _StaticAdjVarTotalList + _nEdges ;
	struct undirected_edge *EdgesAdded = new struct undirected_edge[_nFillEdges] ;
	int *EdgesStartPtr = new int[_nNodes+1] ;
	if (NULL == EdgesStartPtr || NULL == EdgesAdded) {
		if (NULL == EdgesStartPtr) delete [] EdgesStartPtr ;
		if (NULL == EdgesAdded) delete [] EdgesAdded ;
		return 1 ;
		}
	EdgesStartPtr[_nNodes] = _nFillEdges ;

	// simulate node elimination; don't remove any edges, just add new ones.
	// note that _VarType[] for elimination variables should be 0 and _PosOfVarInList[] should be elimination index.
	AdjVar *AV = AVstart ;
	for (i = 0 ; i < _OrderLength ; i++) {
		int X = _VarElimOrder[i] ;
		int & ePTR = EdgesStartPtr[i] ;
		if (i > 0) 
			ePTR = EdgesStartPtr[i-1] ;
		else 
			ePTR = 0 ;
		int nAdded = 0 ;
		// connect neighbors of X that are still not eliminated.
		// NOTE 2010-11-15 KK : this code is slow in that its complexity is (m^2)*m; using the property that adj lists are sorted, this can be made faster (complexity m*(2*m)), 
		// where m is the degree of the node/variable.

		for (AdjVar *avX1 = _Nodes[X]._Neighbors ; NULL != avX1 ; avX1 = avX1->_NextAdjVar) {
			int u = avX1->_V ;
			if (_PosOfVarInList[u] < i) 
				// variable u is already eliminated, ignore it.
				continue ;
			for (AdjVar *avX2 = _Nodes[X]._Neighbors ; NULL != avX2 ; avX2 = avX2->_NextAdjVar) {
				int v = avX2->_V ;
				if (_PosOfVarInList[v] < i) 
					// variable v is already eliminated, ignore it.
					continue ;
				if (u == v) 
					continue ;
				AddEdge(u, v, *AV) ;
				if (AV->_V >= 0) {
					++AV ;
					if (u < v) {
						++nAdded ;
						EdgesAdded[ePTR]._u = u ;
						EdgesAdded[ePTR]._v = v ;
						++ePTR ;
						}
					}
				}
			}
		_nEdges += nAdded ;
		}

	// NOW EdgesStartPtr[i] is index in EdgesAdded array where edges added for elimination step i are.
	// number of edges added during step i is EdgesStartPtr[i+1] - EdgesStartPtr[i].

	// process elimination bakcwards
	for (i = _OrderLength - 1 ; i >= 0 ; i--) {
		int X = _VarElimOrder[i] ;
		int ePTR = EdgesStartPtr[i] ;
		int nAdded = EdgesStartPtr[i+1] - EdgesStartPtr[i] ;
		if (nAdded < 1) 
			continue ;
		}

	delete [] EdgesStartPtr ;
	delete [] EdgesAdded ;
	return 0 ;
}

