#include <stdlib.h>
#include <float.h>

#include "Globals.hxx"

#include "Sort.hxx"
#include "AVLtreeSimple.hxx"
#include "MersenneTwister.h"

#include "Problem.hxx"
#include "Graph.hxx"
using namespace std;

bool AdjustDomainSizes = false ;

ARE::Graph::Graph(ARP *Problem) 
	: 
	_Problem(Problem), 
	_nNodes(0), 
	_Nodes(NULL), 
	_nEdges(0), 
	_StaticAdjVarTotalList(NULL), 
	_VarElimOrderWidth(0), 
	_MaxVarElimComplexity(0.0), 
	_TotalVarElimComplexity(0.0), 
	_TotalNewFunctionStorageAsNumOfElements(0.0), 
	_nFillEdges(0), 
	_VarType(NULL), 
	_PosOfVarInList(NULL), 
	_OrderLength(0), 
	_VarElimOrder(NULL), 
	_nTrivialNodes(0), 
	_TrivialNodesList(NULL), 
	_nMinFillScore0Nodes(0), 
	_MinFill0ScoreList(NULL), 
	_nRemainingNodes(0), 
	_RemainingNodesList(NULL), 
	_IsValid(false),
	_RNG(4243)
{
	if (NULL != _Problem) 
		Create(*_Problem) ;
}


int ARE::Graph::Destroy(void) 
{
	if (NULL != _StaticAdjVarTotalList) {
		delete [] _StaticAdjVarTotalList ;
		_StaticAdjVarTotalList = NULL ;
		}
	if (NULL != _Nodes) {
		delete [] _Nodes ;
		_Nodes = NULL ;
		}
	if (NULL != _VarType) {
		delete [] _VarType ;
		_VarType = NULL ;
		}
	if (NULL != _PosOfVarInList) {
		delete [] _PosOfVarInList ;
		_PosOfVarInList = NULL ;
		}
	if (NULL != _VarElimOrder) {
		delete [] _VarElimOrder ;
		_VarElimOrder = NULL ;
		}
	if (NULL != _TrivialNodesList) {
		delete [] _TrivialNodesList ;
		_TrivialNodesList = NULL ;
		}
	if (NULL != _MinFill0ScoreList) {
		delete [] _MinFill0ScoreList ;
		_MinFill0ScoreList = NULL ;
		}
	if (NULL != _RemainingNodesList) {
		delete [] _RemainingNodesList ;
		_RemainingNodesList = NULL ;
		}
	_nTrivialNodes = _nMinFillScore0Nodes = _nRemainingNodes = _OrderLength = 0 ;
	_VarElimOrderWidth = 0 ;
	_MaxVarElimComplexity = 0.0 ;
	_TotalVarElimComplexity = 0.0 ;
	_TotalNewFunctionStorageAsNumOfElements = 0.0 ;
	_nFillEdges = 0 ;
	_Problem = NULL ;
	_nNodes = 0 ;
	_nEdges = 0 ;
	_IsValid = false ;
	return 0 ;
}


ARE::Graph::~Graph(void) 
{
	Destroy() ;
}


int ARE::Graph::Create(ARP & Problem) 
{
	Destroy() ;

	int n ;
	int adj_list[MAX_DEGREE_OF_GRAPH_NODE] ;

	_Problem = &Problem ;
	_nNodes = _Problem->N() ;
	if (_nNodes < 1) 
		return 0 ;

	_Nodes = new Node[_nNodes] ;
	_VarType = new char[_nNodes] ;
	_PosOfVarInList = new int[_nNodes] ;
	_VarElimOrder = new int[_nNodes] ;
	_TrivialNodesList = new int[_nNodes] ;
	_MinFill0ScoreList = new int[_nNodes] ;
	_RemainingNodesList = new int[_nNodes] ;
	if (NULL == _Nodes || NULL == _VarType || NULL == _PosOfVarInList || NULL == _VarElimOrder || NULL == _TrivialNodesList || NULL == _MinFill0ScoreList || NULL == _RemainingNodesList) { Destroy() ; return 1 ; }

	int i, j, k, l ;

	// *************************************************************************************************
	// calculate degree of each node/variable
	// *************************************************************************************************

	_nTrivialNodes = _nMinFillScore0Nodes = _nRemainingNodes = _OrderLength = 0 ;
// used with AdjustDomainSizes
static int iSum = 0, iN = 0, idone = 0 ; static MTRand rg(4244) ;
	for (i = 0 ; i < _nNodes ; i++) {
		int domain_size = _Problem->K(i) ;
		if (AdjustDomainSizes && domain_size > 1) {
			if (idone == 0) { idone = 1 ; rg.seed(97) ; }
			double x1 = rg.rand() ;
			if (x1 < 0.6667) {
				domain_size = 2 ;
			} else {
				domain_size = 30 ;
				}
			++iN ;
			iSum += domain_size ;
			}
		_Nodes[i]._Degree = 0 ;
		_Nodes[i]._LogK = domain_size > 1 ? log10((double) domain_size) : 0.0 ;
		_VarType[i] = -1 ;
		_PosOfVarInList[i] = -1 ;
		n = 0 ;
		for (j = 0 ; j < _Problem->nFunctions() ; j++) {
			ARE::Function *f = _Problem->getFunction(j) ;
			if (NULL == f) continue ;
			if (! f->ContainsVariable(i)) continue ;
			for (k = 0 ; k < f->N() ; k++) {
				int v = f->Argument(k) ;
				if (i == v) continue ;
				for (l = 0 ; l < n ; l++) 
					{ if (v == adj_list[l]) break ; }
				if (l < n) continue ;
				if (n >= MAX_DEGREE_OF_GRAPH_NODE) { Destroy() ; return 1 ; }
				adj_list[n++] = v ;
				}
			}
		_Nodes[i]._Degree = n ;
		_nEdges += _Nodes[i]._Degree ;
		}
	if (AdjustDomainSizes) {
		printf("\niSum = %d iN = %d", iSum, iN) ;
		}
	if (_nEdges < 1) 
		return 0 ;

	// *************************************************************************************************
	// create adjacent node list for each variable
	// *************************************************************************************************

	_StaticAdjVarTotalList = new AdjVar[_nEdges] ;
	if (NULL == _StaticAdjVarTotalList) { Destroy() ; return 1 ; }
	_nEdges >>= 1 ; // actual number of edges is half of what _nEdges is now, since we have double-counted them
	AdjVar *nextAdjVar = _StaticAdjVarTotalList ;
	// fill in adjacency list for each node/variable
	for (i = 0 ; i < _nNodes ; i++) {
		if (_Nodes[i]._Degree < 1) continue ;
		// generate adj variable list for i
		n = 0 ;
		for (j = 0 ; j < _Problem->nFunctions() ; j++) {
			ARE::Function *f = _Problem->getFunction(j) ;
			if (NULL == f) continue ;
			if (! f->ContainsVariable(i)) continue ;
			for (k = 0 ; k < f->N() ; k++) {
				int v = f->Argument(k) ;
				if (i == v) continue ;
				for (l = 0 ; l < n ; l++) 
					{ if (v == adj_list[l]) break ; }
				if (l < n) continue ;
				adj_list[n++] = v ;
				}
			}
		AdjVar *lastAdjVar = NULL ;
		for (j = 0 ; j < n ; j++) {
			AdjVar *av = nextAdjVar++ ;
			if (NULL == lastAdjVar) 
				_Nodes[i]._Neighbors = av ;
			else 
				lastAdjVar->_NextAdjVar = av ;
			lastAdjVar = av ;
			av->_V = adj_list[j] ;
			}
		// sort the list of neighbors
		if (_Nodes[i]._Degree > 1) {
			long keys[MAX_DEGREE_OF_GRAPH_NODE] ;
			AdjVar *data[MAX_DEGREE_OF_GRAPH_NODE] ;
			long left[32], right[32] ;
			for (lastAdjVar = _Nodes[i]._Neighbors, j = 0 ; NULL != lastAdjVar ; lastAdjVar = lastAdjVar->_NextAdjVar, j++) {
				keys[j] = lastAdjVar->_V ;
				data[j] = lastAdjVar ;
				}
#ifdef x64
			QuickSortLong_i64(keys, _Nodes[i]._Degree, (__int64 *) data, left, right) ;
#else
			QuickSortLong(keys, _Nodes[i]._Degree, (long *) data, left, right) ;
#endif // x64
			_Nodes[i]._Neighbors = lastAdjVar = data[0] ;
			for (j = 1 ; j < _Nodes[i]._Degree ; j++) {
				lastAdjVar->_NextAdjVar = data[j] ;
				lastAdjVar = lastAdjVar->_NextAdjVar ;
				}
			lastAdjVar->_NextAdjVar = NULL ;
			}
		}

	// *************************************************************************************************
	// compute min-fill score for all nodes
	// *************************************************************************************************

	for (i = 0 ; i < _nNodes ; i++) {
		_Nodes[i]._MinFillScore = 0 ;
		AdjVar *av ;
		for (av = _Nodes[i]._Neighbors ; NULL != av ; av = av->_NextAdjVar) {
			int u = av->_V ;
			// scan neighbor lists of i and av->_V; if a node is neighbor of i but not of av->_V, add 1 to minfillscore of i; we rely on the fact that _Nodes[]._Neighbors lists are sorted.
			AdjVar *av_u = _Nodes[u]._Neighbors ;
			AdjVar *av_i = _Nodes[i]._Neighbors ;
move_on :
			if (NULL == av_i) // all nodes left in av_u are adjacent to u but not to i; ignore them
				continue ;
			if (u == av_i->_V) {
				av_i = av_i->_NextAdjVar ;
				goto move_on ;
				}
			if (NULL == av_u) { // all nodes left in av_i are adjacent to i but not to u; add 1 for each
				for (; NULL != av_i ; av_i = av_i->_NextAdjVar) 
					{ if (u != av_i->_V) _Nodes[i]._MinFillScore++ ; }
				continue ;
				}
			if (i == av_u->_V) {
				av_u = av_u->_NextAdjVar ;
				goto move_on ;
				}
			if (av_u->_V < av_i->_V) { // av_u->_V is adjacent to u but not to i; ignore it
				av_u = av_u->_NextAdjVar ;
				goto move_on ;
				}
			if (av_u->_V > av_i->_V) { // av_i->_V is adjacent to i and not adjacent to u; add 1
				_Nodes[i]._MinFillScore++ ;
				av_i = av_i->_NextAdjVar ;
				goto move_on ;
				}
			// now it must be that av_u->_V == av_i->_V
			av_u = av_u->_NextAdjVar ;
			av_i = av_i->_NextAdjVar ;
			goto move_on ;
			}
		_Nodes[i]._MinFillScore >>= 1 ; // we have double-counted the min-fill-score, but counting each edge twice
		_Nodes[i]._EliminationScore = ComputeEliminationComplexity(i) ;

		}
// DEBUGGG

	// split nodes into : trivial, MinFillScore=0, RemainingNodes, mutually exclusive lists.
	for (i = 0 ; i < _nNodes ; i++) {
		if (_Nodes[i]._Degree <= 1) {
			_VarType[i] = 1 ;
			_PosOfVarInList[i] = _nTrivialNodes ;
			_TrivialNodesList[_nTrivialNodes++] = i ;
			continue ;
			}
		else if (0 == _Nodes[i]._MinFillScore) {
			_VarType[i] = 2 ;
			_PosOfVarInList[i] = _nMinFillScore0Nodes ;
			_MinFill0ScoreList[_nMinFillScore0Nodes++] = i ;
			}
		else {
			_VarType[i] = 3 ;
			_PosOfVarInList[i] = _nRemainingNodes ;
			_RemainingNodesList[_nRemainingNodes++] = i ;
			}
		}

	if (Test(1024) > 0) 
		printf("\nGRAPH CONSTRUST errors ...") ;

	_IsValid = true ;
	return 0 ;
}


int ARE::Graph::Create(int nNodes, const std::vector<const std::vector<int>* > & fn_signatures)
{
	Destroy() ;

	if (nNodes < 0) nNodes = 0 ;

	_Problem = NULL ;
	_nNodes = nNodes ;
	if (_nNodes < 1) 
		return 0 ;

	_Nodes = new Node[_nNodes] ;
	_VarType = new char[_nNodes] ;
	_PosOfVarInList = new int[_nNodes] ;
	_VarElimOrder = new int[_nNodes] ;
	_TrivialNodesList = new int[_nNodes] ;
	_MinFill0ScoreList = new int[_nNodes] ;
	_RemainingNodesList = new int[_nNodes] ;
	if (NULL == _Nodes || NULL == _VarType || NULL == _PosOfVarInList || NULL == _VarElimOrder || NULL == _TrivialNodesList || NULL == _MinFill0ScoreList || NULL == _RemainingNodesList) { Destroy() ; return 1 ; }

	int i, j, n ;
	int u, v ;

	// *************************************************************************************************
	// initialize
	// *************************************************************************************************

	_nTrivialNodes = _nMinFillScore0Nodes = _nRemainingNodes = _OrderLength = 0 ;
	for (i = 0 ; i < _nNodes ; i++) {
		_Nodes[i]._Degree = 0 ;
		_VarType[i] = -1 ;
		_PosOfVarInList[i] = -1 ;
		}

	// *************************************************************************************************
	// calculate number of edges
	// *************************************************************************************************

	_nEdges = 0 ;
	for (size_t i = 0 ; i < fn_signatures.size() ; i++) {
		const std::vector<int> & fn_signature = * fn_signatures[i] ;
		n = (fn_signature.size() * (fn_signature.size() - 1)) ;
		if (n <= 0) 
			continue ;
		_nEdges += n ;
		}
	if (_nEdges <= 0) 
		return 0 ;
	// note : this will probably overestimate the number of edges, since the moral graphs of two functions may have an overlap

	// *************************************************************************************************
	// create adjacent node list for each variable
	// *************************************************************************************************

	_StaticAdjVarTotalList = new AdjVar[_nEdges] ;
	if (NULL == _StaticAdjVarTotalList) { Destroy() ; return 1 ; }
	_nEdges >>= 1 ; // actual number of edges is half of what _nEdges is now, since we have double-counted them
	AdjVar *nextAdjVar = _StaticAdjVarTotalList, *av ;

	n = 0 ;
	for (size_t i = 0 ; i < fn_signatures.size() ; i++) {
		const std::vector<int> & fn_signature = * fn_signatures[i] ;
		// enumerate all pairs of vars
		for (vector<int>::const_iterator it1 = fn_signature.begin() ; it1 != fn_signature.end() ; it1++) {
			u = *it1 ;
			for (vector<int>::const_iterator it2 = fn_signature.begin() ; it2 != fn_signature.end() ; it2++) {
				v = *it2 ;
				if (it1 == it2 || u == v) continue ;
				// check if u is already adjacent to v
				for (av = _Nodes[v]._Neighbors ; NULL != av ; av = av->_NextAdjVar) 
					{ if (u == av->_V) break ; }
				if (NULL != av) 
					continue ;
				// connect u to v
				++_Nodes[v]._Degree ;
				av = nextAdjVar++ ;
				av->_NextAdjVar = _Nodes[v]._Neighbors ;
				av->_V = u ;
				_Nodes[v]._Neighbors = av ;
				// count edges
				++n ;
				}
			}
		}
	_nEdges = n >> 1 ; // this is the actual number of edges

	// *************************************************************************************************
	// sort the list of neighbors for each node
	// *************************************************************************************************

	long keys[MAX_DEGREE_OF_GRAPH_NODE] ;
	AdjVar *data[MAX_DEGREE_OF_GRAPH_NODE] ;
	long left[32], right[32] ;
	for (i = 0 ; i < _nNodes ; i++) {
		if (_Nodes[i]._Degree <= 1) continue ;
		for (av = _Nodes[i]._Neighbors, j = 0 ; NULL != av ; av = av->_NextAdjVar, j++) {
			keys[j] = av->_V ;
			data[j] = av ;
			}
#ifdef x64
		QuickSortLong_i64(keys, _Nodes[i]._Degree, (__int64 *) data, left, right) ;
#else
		QuickSortLong(keys, _Nodes[i]._Degree, (long *) data, left, right) ;
#endif // x64
		_Nodes[i]._Neighbors = av = data[0] ;
		for (j = 1 ; j < _Nodes[i]._Degree ; j++) {
			av->_NextAdjVar = data[j] ;
			av = av->_NextAdjVar ;
			}
		av->_NextAdjVar = NULL ;
		// DEBUGGG test
		for (av = _Nodes[i]._Neighbors ; NULL != av ; av = av->_NextAdjVar) {
			u = av->_V ;
			if (NULL != av->_NextAdjVar) {
				v = av->_NextAdjVar->_V ;
				if (u >= v) {
					printf("\nBUGGGGGG") ;
					}
				}
			}
		}

	// *************************************************************************************************
	// compute min-fill score for all nodes
	// *************************************************************************************************

	for (i = 0 ; i < _nNodes ; i++) {
		_Nodes[i]._MinFillScore = 0 ;
		for (av = _Nodes[i]._Neighbors ; NULL != av ; av = av->_NextAdjVar) {
			int u = av->_V ;
			// scan neighbor lists of i and av->_V; if a node is neighbor of i but not of av->_V, add 1 to minfillscore of i; we rely on the fact that _Nodes[]._Neighbors lists are sorted.
			AdjVar *av_u = _Nodes[u]._Neighbors ;
			AdjVar *av_i = _Nodes[i]._Neighbors ;
move_on :
			if (NULL == av_i) // all nodes left in av_u are adjacent to u but not to i; ignore them
				continue ;
			if (u == av_i->_V) {
				av_i = av_i->_NextAdjVar ;
				goto move_on ;
				}
			if (NULL == av_u) { // all nodes left in av_i are adjacent to i but not to u; add 1 for each
				for (; NULL != av_i ; av_i = av_i->_NextAdjVar) 
					{ if (u != av_i->_V) _Nodes[i]._MinFillScore++ ; }
				continue ;
				}
			if (i == av_u->_V) {
				av_u = av_u->_NextAdjVar ;
				goto move_on ;
				}
			if (av_u->_V < av_i->_V) { // av_u->_V is adjacent to u but not to i; ignore it
				av_u = av_u->_NextAdjVar ;
				goto move_on ;
				}
			if (av_u->_V > av_i->_V) { // av_i->_V is adjacent to i and not adjacent to u; add 1
				_Nodes[i]._MinFillScore++ ;
				av_i = av_i->_NextAdjVar ;
				goto move_on ;
				}
			// now it must be that av_u->_V == av_i->_V
			av_u = av_u->_NextAdjVar ;
			av_i = av_i->_NextAdjVar ;
			goto move_on ;
			}
		_Nodes[i]._MinFillScore >>= 1 ; // we have double-counted the min-fill-score, but counting each edge twice

		}

	// *************************************************************************************************
	// compute min-complexity score for all nodes
	// *************************************************************************************************

	if (NULL != _Problem) {
		for (i = 0 ; i < _nNodes ; i++) {
			_Nodes[i]._EliminationScore = ComputeEliminationComplexity(i) ;
			}
		}

	// split nodes into : trivial, MinFillScore=0, RemainingNodes, mutually exclusive lists.
	for (i = 0 ; i < _nNodes ; i++) {
		if (_Nodes[i]._Degree <= 1) {
			_VarType[i] = 1 ;
			_PosOfVarInList[i] = _nTrivialNodes ;
			_TrivialNodesList[_nTrivialNodes++] = i ;
			continue ;
			}
		else if (0 == _Nodes[i]._MinFillScore) {
			_VarType[i] = 2 ;
			_PosOfVarInList[i] = _nMinFillScore0Nodes ;
			_MinFill0ScoreList[_nMinFillScore0Nodes++] = i ;
			}
		else {
			_VarType[i] = 3 ;
			_PosOfVarInList[i] = _nRemainingNodes ;
			_RemainingNodesList[_nRemainingNodes++] = i ;
			}
		}

	if (Test(1024) > 0) 
		printf("\nGRAPH CONSTRUCT errors ...") ;

	_IsValid = true ;
	return 0 ;
}


int ARE::Graph::ReAllocateEdges(void)
{
	int n = 0, i ;
	for (i = 0 ; i < _nNodes ; i++) 
		n += _Nodes[i]._Degree ;
	if (n < 1) 
		return 0 ;

	AdjVar *AV = new AdjVar[n] ;
	if (NULL == AV) 
		return 1 ;
	AdjVar *AVend = AV ;

	for (i = 0 ; i < _nNodes ; i++) {
		AdjVar *oldlist = _Nodes[i]._Neighbors ;
		_Nodes[i]._Neighbors = NULL ;
		AdjVar *newlistlast = NULL ;
		for (AdjVar *av = oldlist ; NULL != av ; av = av->_NextAdjVar) {
			AdjVar *newedge = AVend++ ;
			newedge->_V = av->_V ;
			newedge->_IterationEdgeAdded = av->_IterationEdgeAdded ;
			if (NULL == newlistlast) 
				_Nodes[i]._Neighbors = newedge ;
			else 
				newlistlast->_NextAdjVar = newedge ;
			newlistlast = newedge ;
			}
		}

	if (NULL != _StaticAdjVarTotalList) 
		delete [] _StaticAdjVarTotalList ;
	_StaticAdjVarTotalList = AV ;
	_nEdges = n >> 1 ;

	return 0 ;
}


int ARE::Graph::ReAllocateEdges(int NewNumEdges)
{
	int n = 0, i ;
	for (i = 0 ; i < _nNodes ; i++) 
		n += _Nodes[i]._Degree ;
	if ((n >> 1) > NewNumEdges) 
		// not enough space for existing edges
		return 1 ;
	if (NewNumEdges < 1) 
		return 0 ;

	int m = NewNumEdges << 1 ;
	AdjVar *AV = new AdjVar[m] ;
	if (NULL == AV) 
		return 1 ;
	AdjVar *AVend = AV ;

	for (i = 0 ; i < _nNodes ; i++) {
		AdjVar *oldlist = _Nodes[i]._Neighbors ;
		_Nodes[i]._Neighbors = NULL ;
		AdjVar *newlistlast = NULL ;
		for (AdjVar *av = oldlist ; NULL != av ; av = av->_NextAdjVar) {
			AdjVar *newedge = AVend++ ;
			newedge->_V = av->_V ;
			newedge->_IterationEdgeAdded = av->_IterationEdgeAdded ;
			if (NULL == newlistlast) 
				_Nodes[i]._Neighbors = newedge ;
			else 
				newlistlast->_NextAdjVar = newedge ;
			newlistlast = newedge ;
			}
		}
	_nEdges = AVend - AV ;
	for (i = _nEdges ; i < m ; i++) {
		AdjVar & newedge = AV[i] ;
		newedge._IterationEdgeAdded = -1 ;
		newedge._V = -1 ;
		newedge._NextAdjVar = NULL ;
		}

	if (NULL != _StaticAdjVarTotalList) 
		delete [] _StaticAdjVarTotalList ;
	_StaticAdjVarTotalList = AV ;
	_nEdges >>= 1 ;

	return 0 ;
}


int ARE::Graph::AddEdge(int u, int v, AdjVar & uv, AdjVar & vu)
{
	AdjVar *avl, *avlPrevious ;

	uv._V = -1 ;
	avlPrevious = NULL ;
	bool vAlreadyonList = false ;
	for (avl = _Nodes[u]._Neighbors ; NULL != avl ; avl = avl->_NextAdjVar) {
		if (avl->_V == v) {
			vAlreadyonList = true ;
			break ;
			}
		if (avl->_V > v) 
			break ;
		avlPrevious = avl ;
		}
	if (! vAlreadyonList) {
		uv._V = v ;
		++(_Nodes[u]._Degree) ;
		if (NULL == avlPrevious) {
			uv._NextAdjVar = _Nodes[u]._Neighbors ;
			_Nodes[u]._Neighbors = &uv ;
			}
		else {
			uv._NextAdjVar = avlPrevious->_NextAdjVar ;
			avlPrevious->_NextAdjVar = &uv ;
			}
		}

	vu._V = -1 ;
	avlPrevious = NULL ;
	bool uAlreadyonList = false ;
	for (avl = _Nodes[v]._Neighbors ; NULL != avl ; avl = avl->_NextAdjVar) {
		if (avl->_V == u) {
			uAlreadyonList = true ;
			break ;
			}
		if (avl->_V > u) 
			break ;
		avlPrevious = avl ;
		}
	if (! uAlreadyonList) {
		vu._V = u ;
		++(_Nodes[v]._Degree) ;
		if (NULL == avlPrevious) {
			vu._NextAdjVar = _Nodes[v]._Neighbors ;
			_Nodes[v]._Neighbors = &vu ;
			}
		else {
			vu._NextAdjVar = avlPrevious->_NextAdjVar ;
			avlPrevious->_NextAdjVar = &vu ;
			}
		}

	return 0 ;
}


int ARE::Graph::AddEdge(int u, int v, AdjVar & uv)
{
	AdjVar *avl, *avlPrevious ;

	uv._V = -1 ;
	avlPrevious = NULL ;
	for (avl = _Nodes[u]._Neighbors ; NULL != avl ; avl = avl->_NextAdjVar) {
		if (avl->_V == v) 
			return 0 ;
		if (avl->_V > v) 
			break ;
		avlPrevious = avl ;
		}
	uv._V = v ;
	++(_Nodes[u]._Degree) ;
	if (NULL == avlPrevious) {
		uv._NextAdjVar = _Nodes[u]._Neighbors ;
		_Nodes[u]._Neighbors = &uv ;
		}
	else {
		uv._NextAdjVar = avlPrevious->_NextAdjVar ;
		avlPrevious->_NextAdjVar = &uv ;
		}

	return 0 ;
}


int ARE::Graph::RemoveEdge(int u, int v, AdjVar * & uv, AdjVar * & vu)
{
	AdjVar *avl, *avlPrevious ;

	uv = NULL ;
	avlPrevious = NULL ;
	for (avl = _Nodes[u]._Neighbors ; NULL != avl ; avl = avl->_NextAdjVar) {
		if (avl->_V == v) {
			if (NULL == avlPrevious) 
				_Nodes[u]._Neighbors = avl->_NextAdjVar ;
			else 
				avlPrevious->_NextAdjVar = avl->_NextAdjVar ;
			uv = avl ;
			uv->_NextAdjVar = NULL ;
			--(_Nodes[u]._Degree) ;
			break ;
			}
		avlPrevious = avl ;
		}

	vu = NULL ;
	avlPrevious = NULL ;
	for (avl = _Nodes[v]._Neighbors ; NULL != avl ; avl = avl->_NextAdjVar) {
		if (avl->_V == u) {
			if (NULL == avlPrevious) 
				_Nodes[v]._Neighbors = avl->_NextAdjVar ;
			else 
				avlPrevious->_NextAdjVar = avl->_NextAdjVar ;
			vu = avl ;
			vu->_NextAdjVar = NULL ;
			--(_Nodes[v]._Degree) ;
			break ;
			}
		avlPrevious = avl ;
		}

	return 0 ;
}


int ARE::Graph::operator=(const Graph & G)
{
	int i ;

	if (! _IsValid || _Problem != G._Problem || _nNodes != G._nNodes || _nEdges != G._nEdges) {
		Destroy() ;
		_Problem = G._Problem ;
		_nNodes = G._nNodes ;
		_nEdges = G._nEdges ;
		if (_nNodes > 0) {
			_Nodes = new Node[_nNodes] ;
			_VarType = new char[_nNodes] ;
			_PosOfVarInList = new int[_nNodes] ;
			_VarElimOrder = new int[_nNodes] ;
			_TrivialNodesList = new int[_nNodes] ;
			_MinFill0ScoreList = new int[_nNodes] ;
			_RemainingNodesList = new int[_nNodes] ;
			if (NULL == _Nodes || NULL == _VarType || NULL == _PosOfVarInList || NULL == _VarElimOrder || NULL == _TrivialNodesList || NULL == _MinFill0ScoreList || NULL == _RemainingNodesList) { Destroy() ; return 1 ; }
			}
		if (_nEdges > 0) {
			_StaticAdjVarTotalList = new AdjVar[_nEdges << 1] ;
			if (NULL == _StaticAdjVarTotalList) { Destroy() ; return 1 ; }
			}
		}

	// assume that everything is the same in this/G, but the adj lists of variables are different.
	for (i = 0 ; i < _nNodes ; i++) {
		_VarType[i] = G._VarType[i] ;
		_PosOfVarInList[i] = G._PosOfVarInList[i] ;
		_Nodes[i]._Degree = G._Nodes[i]._Degree ;
		_Nodes[i]._LogK = G._Nodes[i]._LogK ;
		_Nodes[i]._MinFillScore = G._Nodes[i]._MinFillScore ;
		_Nodes[i]._EliminationScore = G._Nodes[i]._EliminationScore ;
		if (_Nodes[i]._Degree < 1) G._Nodes[i]._Neighbors = NULL ;
		else { int offset = G._Nodes[i]._Neighbors - G._StaticAdjVarTotalList ; _Nodes[i]._Neighbors = _StaticAdjVarTotalList + offset ; }
		}

	_OrderLength = G._OrderLength ;
	for (i = 0 ; i < _OrderLength ; i++) 
		_VarElimOrder[i] = G._VarElimOrder[i] ;
	_nTrivialNodes = G._nTrivialNodes ;
	for (i = 0 ; i < _nTrivialNodes ; i++) 
		_TrivialNodesList[i] = G._TrivialNodesList[i] ;
	_nMinFillScore0Nodes = G._nMinFillScore0Nodes ;
	for (i = 0 ; i < _nMinFillScore0Nodes ; i++) 
		_MinFill0ScoreList[i] = G._MinFill0ScoreList[i] ;
	_nRemainingNodes = G._nRemainingNodes ;
	for (i = 0 ; i < _nRemainingNodes ; i++) 
		_RemainingNodesList[i] = G._RemainingNodesList[i] ;

	// fix up edges
	int e = _nEdges << 1 ;
	for (i = 0 ; i < e ; i++) {
		AdjVar & AV = G._StaticAdjVarTotalList[i] ;
		AdjVar & av =   _StaticAdjVarTotalList[i] ;
		av._V = AV._V ;
		av._IterationEdgeAdded = -1 ;
		if (NULL == AV._NextAdjVar) 
			av._NextAdjVar = NULL ;
		else {
			int offset = AV._NextAdjVar - G._StaticAdjVarTotalList ;
			av._NextAdjVar = _StaticAdjVarTotalList + offset ;
			}
		}

	_VarElimOrderWidth = G._VarElimOrderWidth ;
	_MaxVarElimComplexity = G._MaxVarElimComplexity ;
	_TotalVarElimComplexity = G._TotalVarElimComplexity ;
	_TotalNewFunctionStorageAsNumOfElements = G._TotalNewFunctionStorageAsNumOfElements ;
	_nFillEdges = G._nFillEdges ;

	_IsValid = true ;
	return 0 ;
}


int ARE::Graph::Test(int MaxWidthAcceptableForSingleVariableElimination)
{
	int i, j, k, m, n, u, v, ret = 0 ;
	AdjVar *av ;

	if (MaxWidthAcceptableForSingleVariableElimination < 1) 
		MaxWidthAcceptableForSingleVariableElimination = 1 ;
	else if (MaxWidthAcceptableForSingleVariableElimination > MAX_DEGREE_OF_GRAPH_NODE) 
		MaxWidthAcceptableForSingleVariableElimination = MAX_DEGREE_OF_GRAPH_NODE ;

	// TEST that degree is correct
	for (i = 0 ; i < _nNodes ; i++) {
		int last_v = -1 ;
		for (j = 0, av = _Nodes[i]._Neighbors ; NULL != av ; av = av->_NextAdjVar, j++) {
			if (av->_V < 0 || av->_V >= _nNodes) 
				{ printf("\nMinFill Test FAIL : var=%d, has a bad neighbor %d ...", (int) i, (int) av->_V) ; ret++ ; }
			if (last_v >= av->_V) 
				{ printf("\nMinFill Test FAIL : var=%d, has unsorted neighbor list %d, %d ...", (int) i, (int) last_v, (int) av->_V) ; ret++ ; }
			if (av->_V == i) 
				{ printf("\nMinFill Test FAIL : var=%d, has an edge that goed to itself ...", (int) i) ; ret++ ; }
			last_v = av->_V ;
			}
		if (j != _Nodes[i]._Degree) 
			{ printf("\nMinFill Test FAIL : var=%d, degree=%d, but actual=%d ...", (int) i, (int) _Nodes[i]._Degree, (int) j) ; ret++ ; }
		}

	// TEST that edges are mutual (u,v) -> (v,u)
	AdjVar *av_u ;
	for (i = 0 ; i < _nNodes ; i++) {
		for (av = _Nodes[i]._Neighbors ; NULL != av ; av = av->_NextAdjVar) {
			u = av->_V ;
			// check that u has an edge pointing to i
			for (av_u = _Nodes[u]._Neighbors ; NULL != av_u ; av_u = av_u->_NextAdjVar) {
				if (i == av_u->_V) 
					break ;
				}
			if (NULL == av_u) 
				{ printf("\nMinFill Test FAIL : var=%d, is adj to %d, but %d is not adj to %d ...", (int) i, (int) u, (int) u, (int) i) ; ret++ ; }
			}
		}

	// Test trivial nodes, MinFillScore0, etc.
	j = k = m = n = 0 ;
	for (i = 0 ; i < _nNodes ; i++) {
		if (_PosOfVarInList[i] < 0 || _PosOfVarInList[i] >= _nNodes) 
			{ printf("\nMinFill Test FAIL : var=%d, _PosOfVarInList = %d...", (int) i, (int) _PosOfVarInList[i]) ; ret++ ; }
		if (0 == _VarType[i]) {
			if (_VarElimOrder[_PosOfVarInList[i]] != i) 
				{ printf("\nMinFill Test FAIL : var=%d, in Ordered list (%d) but list[%d] is %d, not %d...", (int) i, (int) _PosOfVarInList[i], (int) _PosOfVarInList[i], (int) _VarElimOrder[_PosOfVarInList[i]], (int) i) ; ret++ ; }
			++n ;
			}
		else if (1 == _VarType[i]) {
			if (_TrivialNodesList[_PosOfVarInList[i]] != i) 
				{ printf("\nMinFill Test FAIL : var=%d, in Trivial list (%d) but list[%d] is %d, not %d...", (int) i, (int) _PosOfVarInList[i], (int) _PosOfVarInList[i], (int) _TrivialNodesList[_PosOfVarInList[i]], (int) i) ; ret++ ; }
			++m ;
			}
		else if (2 == _VarType[i]) {
			++j ;
			if (_MinFill0ScoreList[_PosOfVarInList[i]] != i) 
				{ printf("\nMinFill Test FAIL : var=%d, in MinFill0 list (%d) but list[%d] is %d, not %d...", (int) i, (int) _PosOfVarInList[i], (int) _PosOfVarInList[i], (int) _MinFill0ScoreList[_PosOfVarInList[i]], (int) i) ; ret++ ; }
			if (_Nodes[i]._MinFillScore > 0) 
				{ printf("\nMinFill Test FAIL : var=%d, in MinFill0 list (%d) but MinFillScore>0(%d)...", (int) i, (int) _PosOfVarInList[i], (int) _Nodes[i]._MinFillScore) ; ret++ ; }
			}
		else if (3 == _VarType[i]) {
			++k ;
			if (_RemainingNodesList[_PosOfVarInList[i]] != i) 
				{ printf("\nMinFill Test FAIL : var=%d, in General list (%d) but list[%d] is %d, not %d...", (int) i, (int) _PosOfVarInList[i], (int) _PosOfVarInList[i], (int) _RemainingNodesList[_PosOfVarInList[i]], (int) i) ; ret++ ; }
			}
		else 
			{ printf("\nMinFill Test FAIL : var=%d type=%d unknown ...", (int) i, (int) _VarType[i]) ; ret++ ; }
		}
	if (m != _nTrivialNodes) {
		{ printf("\nMinFill Test FAIL : _nTrivialNodes=%d but actual is %d", (int) _nTrivialNodes, (int) j) ; ret++ ; }
		}
	if (n != _OrderLength) {
		{ printf("\nMinFill Test FAIL : _OrderLength=%d but actual is %d", (int) _OrderLength, (int) j) ; ret++ ; }
		}
	if (j != _nMinFillScore0Nodes) {
		{ printf("\nMinFill Test FAIL : _nMinFillScore0Nodes=%d but actual is %d", (int) _nMinFillScore0Nodes, (int) j) ; ret++ ; }
		}
	if (k != _nRemainingNodes) {
		{ printf("\nMinFill Test FAIL : _nRemainingNodes=%d but actual is %d", (int) _nRemainingNodes, (int) k) ; ret++ ; }
		}
	if (_nTrivialNodes + _nRemainingNodes + _nMinFillScore0Nodes + _OrderLength != _nNodes) {
		{ printf("\nMinFill Test FAIL : n Ordered,Trivial,MinFill0,General not equal to nNodes") ; ret++ ; }
		}
	for (i = 0 ; i < _nMinFillScore0Nodes ; i++) {
		if (_MinFill0ScoreList[i] < 0 || _MinFill0ScoreList[i] >= _nNodes) 
			{ printf("\nMinFill Test FAIL : _nMinFillScore0List[%d]=%d", i, _MinFill0ScoreList[i]) ; ret++ ; }
		}
	for (i = 0 ; i < _nRemainingNodes ; i++) {
		if (_RemainingNodesList[i] < 0 || _RemainingNodesList[i] >= _nNodes) 
			{ printf("\nMinFill Test FAIL : _RemainingNodesList[%d]=%d", i, _RemainingNodesList[i]) ; ret++ ; }
		}
	for (i = 0 ; i < _nTrivialNodes ; i++) {
		if (_TrivialNodesList[i] < 0 || _TrivialNodesList[i] >= _nNodes) 
			{ printf("\nMinFill Test FAIL : _TrivialNodesList[%d]=%d", i, _TrivialNodesList[i]) ; ret++ ; }
		}
	for (i = 0 ; i < _OrderLength ; i++) {
		if (_VarElimOrder[i] < 0 || _VarElimOrder[i] >= _nNodes) 
			{ printf("\nMinFill Test FAIL : _VarElimOrder[%d]=%d", i, _VarElimOrder[i]) ; ret++ ; }
		}

	// TEST that graph ElimComplexity of each node is accurate
	if (NULL != _Problem) {
		for (i = 0 ; i < _nNodes ; i++) {
			// this here assumes that width may not be up-to-date for nodes whose width is larger than the limit, on the 
			// assumption that we won't pick these nodes for elimination anyway.
			if (_Nodes[i]._Degree > MaxWidthAcceptableForSingleVariableElimination) 
				continue ;
			double score = ComputeEliminationComplexity(i) ;
			if (fabs(score - _Nodes[i]._EliminationScore) > 0.001) {
				printf("\nMinFill Test FAIL : var=%d degree=%d, is considered, but ElimScore=%g is not accurate; real score=%g ...", (int) i, (int) _Nodes[i]._Degree, _Nodes[i]._EliminationScore, score) ;
				// don't return error code; for nodes of large degree, we may not update the ElimScore. 
				// e.g. there may be nodes, with a degree 1 or 2 higher than BestKnownWidth, whose real ElimScore is non-infinite, 
				// but we have infinite, since we will not update these scores of these variable, since they will not be picked by the elim-computation-algorithm, 
				// so that fact that we don't know exact value is not important.
				ret++ ;
				}
			}
		}

	// TEST that graph MinFill scores are accurate
	for (i = 0 ; i < _nNodes ; i++) {
		int score = 0 ;
		for (AdjVar *av_u = _Nodes[i]._Neighbors ; NULL != av_u ; av_u = av_u->_NextAdjVar) {
			u = av_u->_V ;
			for (AdjVar *av_v = _Nodes[i]._Neighbors ; NULL != av_v ; av_v = av_v->_NextAdjVar) {
				v = av_v->_V ;
				if (u == v) continue ;
				// check if u and v are adjacent
				for (av = _Nodes[u]._Neighbors ; NULL != av ; av = av->_NextAdjVar) {
					if (av->_V == v) 
						break ;
					}
				if (NULL == av) 
					score++ ;
				}
			}
		score >>= 1 ;
		if (score != _Nodes[i]._MinFillScore) 
			{ printf("\nMinFill Test FAIL : var=%d, is considered, but MinFillScore=%d is not accurate; real score=%d ...", (int) i, (int) _Nodes[i]._MinFillScore, (int) score) ; ret++ ; }
		}

	return ret ;
}


