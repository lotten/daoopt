#ifndef ARE_Graph_HXX_INCLUDED
#define ARE_Graph_HXX_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <set>

//#include "Heap.hxx"
//#include "AVLtreeSimple.hxx"
//#include "AVLtree.hxx"
#include "MersenneTwister.h"

//#include "Problem.hxx"

class CMauiAVLTreeSimple ;
class CMauiAVLTree ;

namespace ARE
{

class ARP ;

class AdjVar
{
public :
	int _V ;
	int _IterationEdgeAdded ;
	AdjVar *_NextAdjVar ;
public :
	AdjVar(void) : _V(-1), _IterationEdgeAdded(-1), _NextAdjVar(NULL) { }
} ;

class AdjVarListHelper
{
protected :
	AdjVar *_av ;
	long _ListSize ;
	long _ListUsed ;
public :
	int Reset(void) 
	{
		_ListUsed = 0 ;
		return 0 ;
	}
	AdjVar *GetNextElement(void)
	{
		if (_ListUsed >= _ListSize) 
			return NULL ;
		return &(_av[_ListUsed++]) ;
	}
public :
	AdjVarListHelper(long ListSize = 1)
		:
		_av(NULL), 
		_ListSize(0), 
		_ListUsed(0)
	{
		if (ListSize < 1) 
			ListSize = 0 ;
		else if (ListSize > 1048576) 
			ListSize = 1048576 ;
		_av = new AdjVar[ListSize] ;
		if (NULL != _av) 
			_ListSize = ListSize ;
	}
	~AdjVarListHelper(void)
	{
		if (NULL != _av) delete [] _av ;
	}
} ;

class AdjVarMemoryDynamicManager
{
protected :
	AdjVarListHelper *_SpaceList[1024] ;
	long _nSpaceList, _nSpaceListCurrentUseIdx ;
	long _ReAllocationSize ;
public :
	inline long nSpaceList(void) const { return _nSpaceList ; }
public :
	int Reset(void)
	{
		if (_nSpaceList <= 0) 
			return 0 ;
		for (int i = 0 ; i < _nSpaceList ; i++) 
			_SpaceList[i]->Reset() ;
		_nSpaceListCurrentUseIdx = 0 ;
		return 0 ;
	}
	inline AdjVar *GetNextElement(void)
	{
		// for speed, this fn is inline; also we assume that _nSpaceList is always > 0 (i.e. _nSpaceList_minus_1 >= 0).
		AdjVar *av = _SpaceList[_nSpaceListCurrentUseIdx]->GetNextElement() ;
		if (NULL != av) 
			return av ;
		if (_nSpaceListCurrentUseIdx < _nSpaceList-1) 
			return _SpaceList[++_nSpaceListCurrentUseIdx]->GetNextElement() ;
		if (_nSpaceList >= 1024) 
			return NULL ;
		_SpaceList[_nSpaceList] = new AdjVarListHelper(_ReAllocationSize) ;
		if (NULL == _SpaceList[_nSpaceList]) 
			return NULL ;
		_nSpaceListCurrentUseIdx = _nSpaceList ;
		return _SpaceList[_nSpaceList++]->GetNextElement() ;
	}
public :
	AdjVarMemoryDynamicManager(long ReAllocationSize = 65536)
		:
		_nSpaceList(0), 
		_nSpaceListCurrentUseIdx(-1), 
		_ReAllocationSize(65536)
	{
		if (ReAllocationSize < 1) 
			ReAllocationSize = 1 ;
		else if (ReAllocationSize > 1048576) 
			ReAllocationSize = 1048576 ;
		_ReAllocationSize = ReAllocationSize ;
		_SpaceList[0] = new AdjVarListHelper(_ReAllocationSize) ;
		if (NULL != _SpaceList[0]) 
			{ _nSpaceList = 1 ; _nSpaceListCurrentUseIdx = 0 ; }
	}
	~AdjVarMemoryDynamicManager(void)
	{
		for (int i = 0 ; i < _nSpaceList ; i++) {
			delete _SpaceList[i] ;
			}
	}
} ;

class Node
{
public :
	int _Degree ;
	double _LogK ; // log of the domain size of the variable
	int _MinFillScore ;
	double _EliminationScore ; // log of the product of domain sizes of variables in N_G[v]
	AdjVar *_Neighbors ;
public :
	Node(int Degree = 0) : _Degree(Degree), _LogK(0.0), _MinFillScore(-1), _EliminationScore(0.0), _Neighbors(NULL) { }
} ;

class Graph
{
public :
	ARP *_Problem ;
public :
	int _nNodes ;
	Node *_Nodes ; // we assume that the adj-node-list for each node is sorted in increasing order of node indeces
public :
	int _nEdges ; // this is the actual number of edges in the graph; size of _StaticAdjVarTotalList is 2*_nEdges.
	AdjVar *_StaticAdjVarTotalList ;
public :
	// this function is used when the initial edge set has changed (e.g. because some variables have been eliminated), 
	// and we want to consolidate/cleanup the edge set.
	int ReAllocateEdges(void) ;
	// this function is used when the initial edge set has changed and number of edges has changed, 
	// and we want to consolidate/cleanup the edge set.
	// note that NewNumEdges may be larger than current number of edges.
	int ReAllocateEdges(int NewNumEdges) ;
	// this function adds an edge (both ways) between two variables, using the provided AdjVar objects; if uv/vu data member _V is -1, then this obj was not used (probably because already adjacent before).
	int AddEdge(int u, int v, AdjVar & uv, AdjVar & vu) ;
	// this function adds an edge u->v, using the provided AdjVar object
	int AddEdge(int u, int v, AdjVar & uv) ;
	// this function removes an edge between two variables, returning the AdjVar objects that were used for the edges
	int RemoveEdge(int u, int v, AdjVar * & uv, AdjVar * & vu) ;
public :
	int _VarElimOrderWidth ;
	double _MaxVarElimComplexity ; // log og
	double _TotalVarElimComplexity ; // log of
	double _TotalNewFunctionStorageAsNumOfElements ; // log of
	int _nFillEdges ;
public :
	inline double ComputeEliminationComplexity(int v) 
	{
		double score = _Nodes[v]._LogK ;
		for (AdjVar *av = _Nodes[v]._Neighbors ; NULL != av ; av = av->_NextAdjVar) 
			score += _Nodes[av->_V]._LogK ;
		return score ;
/*
		__int64 score = _Problem->K(v) ;
		for (AdjVar *av = _Nodes[v]._Neighbors ; NULL != av ; av = av->_NextAdjVar) {
			score *= _Problem->K(av->_V) ;
			if (score >= InfiniteSingleVarElimComplexity) 
				{ score = InfiniteSingleVarElimComplexity ; return score ; }
			}
		return score ;
*/
	}
	inline void AdjustScoresForArcAddition(int u, int v, int CurrentIteration, CMauiAVLTreeSimple & avlVars2CheckScore)
	{
		/*
			we will move nodes whose score is decremented to 0, into avlVars2CheckScore list.
			scores whose score is incremented are in general list and stay there, so there is no need to add them to the avlVars2CheckScore list.
		*/

		// do a comparative scan of neighbor lists of u and v
		AdjVar *av_u = _Nodes[u]._Neighbors ;
		AdjVar *av_v = _Nodes[v]._Neighbors ;
move_on2 :
		if (NULL == av_v) { // all nodes left in av_u are adjacent to u but not to v; add 1 (to u) for each neighbor.
			for (; NULL != av_u ; av_u = av_u->_NextAdjVar) {
				if (v != av_u->_V) 
					_Nodes[u]._MinFillScore++ ;
				}
			return ;
			}
		if (u == av_v->_V) {
			av_v = av_v->_NextAdjVar ;
			goto move_on2 ;
			}
		if (NULL == av_u) { // all nodes left in av_v are adjacent to v but not to u; add 1 (to u) for each neighbor.
			for (; NULL != av_v ; av_v = av_v->_NextAdjVar) {
				if (u != av_v->_V) 
					_Nodes[v]._MinFillScore++ ;
				}
			return ;
			}
		if (v == av_u->_V) {
			av_u = av_u->_NextAdjVar ;
			goto move_on2 ;
			}
		if (av_u->_V < av_v->_V) { // av_u->_V is adjacent to u but not to v.
			if (v != av_u->_V) 
				_Nodes[u]._MinFillScore++ ;
			av_u = av_u->_NextAdjVar ;
			goto move_on2 ;
			}
		if (av_u->_V > av_v->_V) { // av_v->_V is adjacent to v but not to u.
			if (u != av_v->_V) 
				_Nodes[v]._MinFillScore++ ;
			av_v = av_v->_NextAdjVar ;
			goto move_on2 ;
			}
		// now it must be that av_u->_V == av_v->_V; if this common-adjacent-node existing before this iteration, subtract 1.
		// Note that here w = av_u->_V == av_v->_V may on adjacent to X ("on the circle") or not ("outside the circle").
		// in case that is it on the circle, we need to make sure that edges (u,w) and (v,w) existed before.
		if (u < v && av_u->_IterationEdgeAdded < CurrentIteration && av_v->_IterationEdgeAdded < CurrentIteration) {
			if (0 == --(_Nodes[av_u->_V]._MinFillScore)) 
				avlVars2CheckScore.Insert(av_u->_V) ;
			}
		av_u = av_u->_NextAdjVar ;
		av_v = av_v->_NextAdjVar ;
		goto move_on2 ;
	}
public :
	// 0=ordered, 1=Trivial, 2=MinFillScore0, 3=General
	char *_VarType ;
	int *_PosOfVarInList ;
	// list of ordered variables; in elimination order.
	int _OrderLength ;
	int *_VarElimOrder ;
	// temporary space for holding trivial nodes (degree(X) <= 1)
	int _nTrivialNodes ;
	int *_TrivialNodesList ;
	// temporary holding space for MinFillScore=0 nodes. During OrderingCreation, we rely on the fact that once a node's MinFill score becomes 0, it stays zero.
	// note that if node is trivial, it should not be in MinFillScore=0 list.
	int _nMinFillScore0Nodes ;
	int *_MinFill0ScoreList ;
	// temporary holding space for remining nodes.
	int _nRemainingNodes ;
	int *_RemainingNodesList ;
public :
	// temporary space for storing edges to add during each variable ordering computation iteration
	short _EdgeU[100000] ;
	short _EdgeV[100000] ;
public :
	inline void RemoveVarFromList(int X)
	{
		int i = _PosOfVarInList[X] ;
		// it must be that i>=0
		_PosOfVarInList[X] = -1 ;
		if (3 == _VarType[X]) {
			if (--_nRemainingNodes != i) {
				_RemainingNodesList[i] = _RemainingNodesList[_nRemainingNodes] ;
				_PosOfVarInList[_RemainingNodesList[i]] = i ;
				}
			}
		else if (2 == _VarType[X]) {
			if (--_nMinFillScore0Nodes != i) {
				_MinFill0ScoreList[i] = _MinFill0ScoreList[_nMinFillScore0Nodes] ;
				_PosOfVarInList[_MinFill0ScoreList[i]] = i ;
				}
			}
		else if (1 == _VarType[X]) {
			if (--_nTrivialNodes != i) {
				_TrivialNodesList[i] = _TrivialNodesList[_nTrivialNodes] ;
				_PosOfVarInList[_TrivialNodesList[i]] = i ;
				}
			}
		// else X is ordered; we will not remove these variables.
	}
	inline void ProcessPostEliminationNodeListLocation(int u) 
	{
		// check if u has become special; this function is called, after X is eliminated, for all nodes u whose MinFill score has changed

		if (2 == _VarType[u]) { // u is in MilFillScore0 list; check if it can be moved up to trivial list
			// check if u has become trivial
			if (_Nodes[u]._Degree <= 1) {
// DEBUGGG
//printf("\nMinFill : round %d picked var=%d; making u=%d trivial, VarToMinFill0ScoreListMap[u]=%d ...", (int) nOrdered, (int) X, (int) u, (int) _VarToMinFill0ScoreListMap[u]) ;
				RemoveVarFromList(u) ;
				_PosOfVarInList[u] = _nTrivialNodes ;
				_TrivialNodesList[_nTrivialNodes++] = u ;
				_VarType[u] = 1 ;
				}
			}
		else if (3 == _VarType[u]) { // u is in general list; check if it can be moved up to MinFillScore=0 or trivial list
			// check if u has become trivial
			if (_Nodes[u]._Degree <= 1) {
				RemoveVarFromList(u) ;
// DEBUGGG
//printf("\nMinFill : round %d picked var=%d; making u=%d trivial, VarToMinFill0ScoreListMap[u]=%d ...", (int) nOrdered, (int) X, (int) u, (int) _VarToMinFill0ScoreListMap[u]) ;
				_PosOfVarInList[u] = _nTrivialNodes ;
				_TrivialNodesList[_nTrivialNodes++] = u ;
				_VarType[u] = 1 ;
				}
			// check if u has become MinFillScore=0
			else if (_Nodes[u]._MinFillScore <= 0) {
				RemoveVarFromList(u) ;
				_PosOfVarInList[u] = _nMinFillScore0Nodes ;
				_MinFill0ScoreList[_nMinFillScore0Nodes++] = u ;
				_VarType[u] = 2 ;
				}
			}
	}
public :
	bool _IsValid ;
protected :
	MTRand _RNG ;
public :
	int ComputeVariableEliminationOrder_Simple(
		char CostFunction, // 0=MinFill, 1=MinDegree, 2=MinComplexity
		// width/complexity of the best know order; used to cut off search when the elimination order we found is not very good
		int WidthLimit, 
		bool EarlyTermination_W, 
		double TotalComplexityLimit, // log of the complexity limit
		bool EarlyTermination_C, 
		// if true, we will quit after all Trivial/MinFillScore0 nodes have been used up. This is typically used to generate a starting point for large-scale randomized searches.
		bool QuitAfterEasyIsDone, 
		// width that is considered trivial; whenever a variale with this width is found, it can be eliminated right away, even if it is not the variable with the smallest width
		int EasyWidth, 
		// when no easy variables (e.g. degree <=1, MinFillScore=0) are to pick, we pick randomly among a few best nodes.
		int nRandomPick, 
		double eRandomPick, 
		// aux avl tree to keep track of vars whose scores may change during each iteration; passed in here so that it does not have to be created each time.
		CMauiAVLTreeSimple & avlVars2CheckScore,
		// an array of AdjVar objects; usually statically/globally allocated; used to extra supply of AdjVar objects used/needed when edges are added to the graph.
		AdjVar *TempAdjVarSpace, int nTempAdjVarSpace) ;
	int ComputeVariableEliminationOrder_Simple_wMinFillOnly(
		// width/complexity of the best know order; used to cut off search when the elimination order we found is not very good
		int WidthLimit, 
		bool EarlyTermination_W, 
		// if true, we will quit after all Trivial/MinFillScore0 nodes have been used up. This is typically used to generate a starting point for large-scale randomized searches.
		bool QuitAfterEasyIsDone, 
		// width that is considered trivial; whenever a variale with this width is found, it can be eliminated right away, even if it is not the variable with the smallest width
		int EasyWidth, 
		// when no easy variables (e.g. degree <=1, MinFillScore=0) are to pick, we pick randomly among a few best nodes.
		int nRandomPick, 
		double eRandomPick, 
		// aux avl tree to keep track of vars whose scores may change during each iteration; passed in here so that it does not have to be created each time.
		CMauiAVLTreeSimple & avlVars2CheckScore,
		// an array of AdjVar objects; usually statically/globally allocated; used to extra supply of AdjVar objects used/needed when edges are added to the graph.
		AdjVarMemoryDynamicManager & TempAdjVarSpace) ;
public :
	int RemoveRedundantFillEdges(void) ;
public :
	int operator=(const Graph & G) ;
	int Test(int MaxWidthAcceptableForSingleVariableElimination) ;
public :
	Graph(ARP *Problem = NULL) ;
	~Graph(void) ;
	int Destroy(void) ;

	// create a graph from a problem
	int Create(ARP & Problem) ;

	// create a graph from the given set of fn signatures. 
	// we assume each signature is correct : var indeces range [0, nNodes), there are no repetitions.
	int Create(int nNodes, const std::vector<const std::vector<int>* > & fn_signatures) ;
} ;

} // namespace ARE

#endif // ARE_Graph_HXX_INCLUDED
