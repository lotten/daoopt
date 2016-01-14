#include <stdlib.h>

#include "Globals.hxx"

#include "Sort.hxx"
#include "AVLtreeSimple.hxx"

#include "Problem.hxx"
#include "Graph.hxx"

int ARE::Graph::ComputeVariableEliminationOrder_Simple(char CostFunction, int WidthLimit, bool EarlyTermination_W, double TotalComplexityLimit, bool EarlyTermination_C, bool QuitAfterEasyIsDone, int EasyWidth, int n4RandomPick, double eRandomPick, CMauiAVLTreeSimple & avlVars2CheckScore, AdjVar *TempAdjVarSpace, int nTempAdjVarSpace)
{
	_nFillEdges = 0 ;
	if (NULL == _Problem || _nNodes < 1) 
		return 0 ;

	if (WidthLimit > MAX_DEGREE_OF_GRAPH_NODE) 
		WidthLimit = MAX_DEGREE_OF_GRAPH_NODE ;
	if (EasyWidth < 1) 
		EasyWidth = 1 ;

	int i, j, k, X, u, v ;

	int nRemaining = _nNodes - _OrderLength ;
	int IterationIdx = _OrderLength ;

	// this is a list of keeping edges that were part of the graph and were removed and that can be reused
	AdjVar *AdjVarFreeList = NULL ;
	AdjVar *AVFLend = NULL ;

#define maxNumVarsForPicking 256 // this is built-in hard limit of the number of variables we consider for picking; make it large enough.
	int nVarsToPickFrom ;
	int VarsToPickFrom[maxNumVarsForPicking] ; // make sure this array has at least maxNumVarsForPicking elements
	double VarsToPickFrom_ProbFactor[maxNumVarsForPicking] ; // make sure this array has at least maxNumVarsForPicking elements
	double VarsToPickFrom_Scores[maxNumVarsForPicking] ; // make sure this array has at least maxNumVarsForPicking elements; keep sorted in inc order.
	if (n4RandomPick < 1) 
		n4RandomPick = 1 ;
	else if (n4RandomPick > maxNumVarsForPicking) 
		n4RandomPick = maxNumVarsForPicking ;

	int nTotalEdgesAdded = 0 ;
	int nEdgesAdded = 0 ;

pick_next_var :

	if (1+nRemaining <= EasyWidth) {
		for (i = 0 ; i < _nTrivialNodes ; i++) {
			X = _TrivialNodesList[i] ;
			_VarType[X] = 0 ;
			_PosOfVarInList[X] = _OrderLength ;
			_VarElimOrder[_OrderLength++] = X ;
			if (_Nodes[X]._Degree > _VarElimOrderWidth) 
				_VarElimOrderWidth = _Nodes[X]._Degree ;
			}
		for (i = 0 ; i < _nMinFillScore0Nodes ; i++) {
			X = _MinFill0ScoreList[i] ;
			_VarType[X] = 0 ;
			_PosOfVarInList[X] = _OrderLength ;
			_VarElimOrder[_OrderLength++] = X ;
			if (_Nodes[X]._Degree > _VarElimOrderWidth) 
				_VarElimOrderWidth = _Nodes[X]._Degree ;
			}
		for (i = 0 ; i < _nRemainingNodes ; i++) {
			X = _RemainingNodesList[i] ;
			_VarType[X] = 0 ;
			_PosOfVarInList[X] = _OrderLength ;
			_VarElimOrder[_OrderLength++] = X ;
			if (_Nodes[X]._Degree > _VarElimOrderWidth) 
				_VarElimOrderWidth = _Nodes[X]._Degree ;
			}
		_nTrivialNodes = _nMinFillScore0Nodes = _nRemainingNodes = 0 ;
		return 0 ;
		}

	if (_nTrivialNodes > 0) {
		X = _TrivialNodesList[0] ;
		goto eliminate_picked_variable ;
		}
	else if (_nMinFillScore0Nodes > 0) {
		X = _MinFill0ScoreList[0] ;
		goto eliminate_picked_variable ;
		}

	if (QuitAfterEasyIsDone) 
		return 0 ;

	// **********************************************************************
	// Pool size is 1. collect nodes with the best score, and pick on randomly out of these.
	// **********************************************************************

	if (n4RandomPick < 2) {
		nVarsToPickFrom = 0 ;
		__int64 best_score = _I64_MAX ;
		for (i = 0 ; i < _nRemainingNodes ; i++) {
			u = _RemainingNodesList[i] ;
			if (EarlyTermination_W) {
				if (_Nodes[u]._Degree > WidthLimit) 
					// don't consider variables with larger width that known best width.
					// this may not be optimal, since we really minimize total elimination complexity, and space.
					// however, we want to quickly give up on an ordering computation, if it does not lead to a new better ordering that the previously known best.
					// we use the width as the key.
					continue ;
				}
			if (EarlyTermination_C) {
				if (_Nodes[u]._EliminationScore >= InfiniteSingleVarElimComplexity_log) 
					// don't consider whose elimination complexity if too large.
					continue ;
				}

			// There should be no variables with MinFillScore=0, although it is not important for the code here to work correctly.
			// We will pick n4RandomPick variables with the smallest EliminationComplexity.
			__int64 score ;
			if (1 == CostFunction) 
				score = _Nodes[u]._Degree ;
			else if (2 == CostFunction) 
				score = _Nodes[u]._EliminationScore ;
			else 
				score = _Nodes[u]._MinFillScore ;
			if (score > best_score) 
				continue ;
			if (score < best_score) {
				nVarsToPickFrom = 1 ;
				VarsToPickFrom[0] = u ;
				best_score = score ;
				}
			else if (nVarsToPickFrom < maxNumVarsForPicking) {
				VarsToPickFrom[nVarsToPickFrom++] = u ;
				}
			}
		if (0 == nVarsToPickFrom) 
			return ERRORCODE_NoVariablesLeftToPickFrom ;
		else if (1 == nVarsToPickFrom) 
			i = 0 ;
		else 
			i = _RNG.randInt(nVarsToPickFrom-1) ;
		X = VarsToPickFrom[i] ;
		goto eliminate_picked_variable ;
		}

	// **********************************************************************
	// Pool picking
	// **********************************************************************

	nVarsToPickFrom = 0 ;
	for (i = 0 ; i < _nRemainingNodes ; i++) {
		u = _RemainingNodesList[i] ;
#ifdef TEST_COMPL_CORRECT
{
double c = ComputeEliminationComplexity(u) ;
if (fabs(c - _Nodes[u]._EliminationScore) > 0.001) 
printf("\nERROR : _Nodes[u]._EliminationScore wrong") ;
}
#endif // TEST_COMPL_CORRECT
		if (EarlyTermination_W) {
			if (_Nodes[u]._Degree > WidthLimit) 
				// don't consider variables with larger width that known best width.
				// this may not be optimal, since we really minimize total elimination complexity, and space.
				// however, we want to quickly give up on an ordering computation, if it does not lead to a new better ordering that the previously known best.
				// we use the width as the key.
				continue ;
			}
		if (EarlyTermination_C) {
			if (_Nodes[u]._EliminationScore >= InfiniteSingleVarElimComplexity_log) 
				// don't consider whose elimination complexity if too large.
				continue ;
			}

		// There should be no variables with MinFillScore=0, although it is not important for the code here to work correctly.
		// We will pick n4RandomPick variables with the smallest EliminationComplexity.
		double score ;
		if (1 == CostFunction) 
			score = _Nodes[u]._Degree ;
		else if (2 == CostFunction) {
			score = _Nodes[u]._EliminationScore ;
			}
		else 
			score = _Nodes[u]._MinFillScore ;
		if (nVarsToPickFrom < n4RandomPick) {
			VarsToPickFrom_Scores[nVarsToPickFrom] = score ;
			VarsToPickFrom[nVarsToPickFrom] = u ;
			++nVarsToPickFrom ;
			}
		else {
			// find max
			k = 0 ;
			for (j = 1 ; j < nVarsToPickFrom ; j++) {
				if (VarsToPickFrom_Scores[j] > VarsToPickFrom_Scores[k]) 
					k = j ;
				}
			// if better than max, replace it
			if (score < VarsToPickFrom_Scores[k]) {
				VarsToPickFrom_Scores[k] = score ;
				VarsToPickFrom[k] = u ;
				}
			}
		}
	if (0 == nVarsToPickFrom) 
		return ERRORCODE_NoVariablesLeftToPickFrom ;

	// pick variable to eliminate
	if (nVarsToPickFrom > 1) {
		// we assume that no VarsToPickFrom_ProbFactor[] is 0.
		double nTotal = 0.0 ;
		for (i = 0 ; i < nVarsToPickFrom ; i++) {
			VarsToPickFrom_Scores[i] += 1.0 ;
			if (VarsToPickFrom_Scores[i] > 1000000.0) 
				// this check is necessary so that pow() won't get too large and throw exception.
				// also, note that score should not be 0!!! since pow() will throw exception on that too.
				VarsToPickFrom_Scores[i] = 1000000.0 ;
			// NOTE 2011-02-06 KK : MinComplexity case (2) right now score here is log. we may want to exp() it.
			VarsToPickFrom_ProbFactor[i] = pow(VarsToPickFrom_Scores[i], eRandomPick) ;
			nTotal += VarsToPickFrom_ProbFactor[i] ;
			}
		double r = _RNG.randExc(nTotal) ;
		for (i = 0 ; i < nVarsToPickFrom ; i++) {
			r -= VarsToPickFrom_ProbFactor[i] ;
			if (r < 0.0) 
				break ;
			}
		}
	else 
		i = 0 ;
	X = VarsToPickFrom[i] ;

eliminate_picked_variable :

	nTotalEdgesAdded += nEdgesAdded ;
	nEdgesAdded = 0 ;

	avlVars2CheckScore.EmptyQuick() ;

	/*
		DEF = width is number of variables in a cluster - 1.
	*/

	RemoveVarFromList(X) ;
	_VarType[X] = 0 ;
	_PosOfVarInList[X] = _OrderLength ;
	_VarElimOrder[_OrderLength++] = X ;
	--nRemaining ;

	if (_Nodes[X]._Degree > _VarElimOrderWidth) 
		_VarElimOrderWidth = _Nodes[X]._Degree ;
	if (_Nodes[X]._EliminationScore > _MaxVarElimComplexity) 
		_MaxVarElimComplexity = _Nodes[X]._EliminationScore ;
	_TotalVarElimComplexity += log10(1.0 + pow(10.0, _Nodes[X]._EliminationScore - _TotalVarElimComplexity)) ;
	double space = _Nodes[X]._EliminationScore - _Nodes[X]._LogK ;
	_TotalNewFunctionStorageAsNumOfElements += log10(1.0 + pow(10.0, space - _TotalNewFunctionStorageAsNumOfElements)) ;

	if (EarlyTermination_C) {
		// check if the complexity is blown
		if (_TotalVarElimComplexity >= TotalComplexityLimit) 
			return ERRORCODE_EliminationComplexityTooLarge ;
		// check if InfiniteSingleVarElimComplexity is blown
		if (_MaxVarElimComplexity >= InfiniteSingleVarElimComplexity_log) 
			return ERRORCODE_EliminationComplexityTooLarge ;
		}
	if (EarlyTermination_W) {
		// check if the width is blown
		if (_VarElimOrderWidth >= WidthLimit) 
			return ERRORCODE_EliminationWidthTooLarge ;
		}

	AdjVar *av_X_end = NULL, *av ;

	if (0 == _Nodes[X]._Degree) {
		goto pick_next_var ;
		}

	IterationIdx = _OrderLength ;

	// reduce the graph : remove edges adj to X + add edges between variables adj to X that don't have an edge.
	// when removing an edge, there are two cost updates : 
	//		1) common neighbors of the two endpoints of the edge have +1 (in the future, when the common endpoint is eliminated, one more edge needs to be added)
	//		2) for each non-common neighbor, for both endpoints, the endpoint gets -1, since in the future the endpoint does not have to add an edge
	//		*) note that for us case here, common endpoints are also adj to X, and since they would get both +1 and -1, resulting in 0, and therefore, 
	//			we only have to add -1 for each adj node, not adj to X, for each node adj to X.
	// when adding an edge (u,v), the cost updates are :
	//		1) all common neighbors of u,v get -1 (they no longer have to add an edge in the future, if they were to be eliminated)
	//		2) u/v gets, for each non-common neighbors, +1 since a new edge between v/u and that non-common neighbor needs to be added

//	// TODO/NOTE : we use long to store edges, which leaves 16 bits for variable, limiting nNodes to 16.
//	edges2add.Empty() ;

	AdjVar *avX ;
	for (avX = _Nodes[X]._Neighbors ; NULL != avX ; avX = avX->_NextAdjVar) {
		av_X_end = avX ;
		u = avX->_V ;
		avlVars2CheckScore.Insert(u) ;
		_Nodes[u]._Degree-- ;
		_Nodes[u]._EliminationScore -= _Nodes[X]._LogK ;

		// do a comparative scan of neighbor lists of X and u
		AdjVar *av_u = _Nodes[u]._Neighbors ;
		AdjVar *av_X = _Nodes[X]._Neighbors ;
		AdjVar *av_u_last = NULL, *av_u_next ;
move_on :
		if (NULL == av_X) { // all nodes left in av_u are adjacent to u but not to X
			for (; NULL != av_u ; av_u = av_u_next) {
				av_u_next = av_u->_NextAdjVar ;
				if (X == av_u->_V) { // take av_u out of the adjacency list of u
					if (NULL == av_u_last) 
						{ _Nodes[u]._Neighbors = av_u_next ; }
					else 
						{ av_u_last->_NextAdjVar = av_u_next ; }
					// add av_u to the list of empty edges
					if (NULL == AVFLend) AdjVarFreeList = AVFLend = av_u ; else AVFLend->_NextAdjVar = av_u ; AVFLend = av_u ; av_u->_NextAdjVar = NULL ;
					}
				else { // (u,X) edge will be gone; u no longer has to connect av_u->_V and X; subtract 1.
					_Nodes[u]._MinFillScore-- ;
					av_u_last = av_u ;
					}
				}
			continue ;
			}
		if (u == av_X->_V) {
			av_X = av_X->_NextAdjVar ;
			goto move_on ;
			}
		if (NULL == av_u) { // all nodes left in av_X are adjacent to X but not to u; add an edge for each, between (av_X->_V, u)
			for (; NULL != av_X ; av_X = av_X->_NextAdjVar) {
				v = av_X->_V ;
				if (u == v) continue ;

				if (u < v) {
					// edges2add.Insert((u << 16) | v) ; // else edges2add.Insert((v << 16) | u) ; 2010-10-08 KK : add iff u<v, otherwise we would add twice
					_EdgeU[nEdgesAdded] = u ;
					_EdgeV[nEdgesAdded] = v ;
					nEdgesAdded++ ;
					}


				// fetch empty structure
				if (NULL != AdjVarFreeList) { av = AdjVarFreeList ; AdjVarFreeList = AdjVarFreeList->_NextAdjVar ; if (NULL == AdjVarFreeList) AVFLend = NULL ; }
				else if (nTempAdjVarSpace > 0) { av = TempAdjVarSpace++ ; --nTempAdjVarSpace ; }
				else 
					return ERRORCODE_out_of_memory ;

				// update node [u]
				av->_NextAdjVar = NULL ;
				av->_V = v ;
				av->_IterationEdgeAdded = IterationIdx ; // edge iteration index equals the idx of the variable, in the order, that was eliminated during the iteration
				if (NULL == av_u_last) 
					_Nodes[u]._Neighbors = av ;
				else 
					av_u_last->_NextAdjVar = av ;
				av_u_last = av ;
				_Nodes[u]._Degree++ ;
				// update ElimScore later, so that all ElimScore reductions get done first
				}
			continue ;
			}
		if (X == av_u->_V) { // take av_u out of the adjacency list of u
			av_u_next = av_u->_NextAdjVar ;
			if (NULL == av_u_last) 
				{ _Nodes[u]._Neighbors = av_u_next ; }
			else 
				{ av_u_last->_NextAdjVar = av_u_next ; }
			// add av_u to the list of empty edges
			if (NULL == AVFLend) AdjVarFreeList = AVFLend = av_u ; else AVFLend->_NextAdjVar = av_u ; AVFLend = av_u ; av_u->_NextAdjVar = NULL ;
			av_u = av_u_next ;
			goto move_on ;
			}
		if (av_u->_V < av_X->_V) { // av_u->_V is adjacent to u but not to X; subtract 1 from u
			_Nodes[u]._MinFillScore-- ;
			av_u_last = av_u ;
			av_u = av_u->_NextAdjVar ;
			goto move_on ;
			}
		if (av_u->_V > av_X->_V) { // av_X->_V is adjacent to X and not adjacent to u; add an edge between (av_X->_V, u), before av_u in the list of u
			v = av_X->_V ;

			if (u < v) {
				// edges2add.Insert((u << 16) | v) ; // else edges2add.Insert((v << 16) | u) ; 2010-10-08 KK : add iff u<v, otherwise we would add twice
				_EdgeU[nEdgesAdded] = u ;
				_EdgeV[nEdgesAdded] = v ;
				nEdgesAdded++ ;
				}

			// fetch empty structure
			if (NULL != AdjVarFreeList) { av = AdjVarFreeList ; AdjVarFreeList = AdjVarFreeList->_NextAdjVar ; if (NULL == AdjVarFreeList) AVFLend = NULL ; }
			else if (nTempAdjVarSpace > 0) { av = TempAdjVarSpace++ ; --nTempAdjVarSpace ; }
			else 
				return ERRORCODE_out_of_memory ;

			// update node [u]
			av->_NextAdjVar = av_u ;
			av->_V = v ;
			av->_IterationEdgeAdded = IterationIdx ; // edge iteration index equals the idx of the variable, in the order, that was eliminated during the iteration
			if (NULL == av_u_last) {
				_Nodes[u]._Neighbors = av ;
				}
			else {
				av_u_last->_NextAdjVar = av ;
				}
			av_u_last = av ;
			_Nodes[u]._Degree++ ;
			// update ElimScore later, so that all ElimScore reductions get done first

			av_X = av_X->_NextAdjVar ;
			goto move_on ;
			}
		// now it must be that av_u->_V == av_X->_V; ignore this case because the edge (u, av_u->_V) stays
		av_u_last = av_u ;
		av_u = av_u->_NextAdjVar ;
		av_X = av_X->_NextAdjVar ;
		goto move_on ;
		}

	// add edges between nodes (that used to be) adj to X that don't have them yet
	for (i = 0 ; i < nEdgesAdded ; i++) {
		u = _EdgeU[i] ;
		v = _EdgeV[i] ;
		AdjustScoresForArcAddition(u, v, IterationIdx, avlVars2CheckScore) ;
		_Nodes[u]._EliminationScore += _Nodes[v]._LogK ; // variable u is now connected to v so its elimination score is higher
		_Nodes[v]._EliminationScore += _Nodes[u]._LogK ; // variable v is now connected to u so its elimination score is higher
		}
	_nFillEdges += nEdgesAdded ;

	// check variables whose MinFillScore changed whether they need to be moved to a different list.
	long pos = - 1, ul ;
	while (avlVars2CheckScore.GetNext(&ul, pos) != 0) 
		ProcessPostEliminationNodeListLocation(ul) ;

//for (AdjVar *avX = _Nodes[X]._Neighbors ; NULL != avX ; avX = avX->_NextAdjVar) {
//	u = avX->_V ;
//	if (0 == tempscores[u] && _Nodes[u]._MinFillScore != 0) 
//		printf("\nMinFill : score not persistent var=%d; before %d after %d", (int) X, (int) tempscores[u], (int) _Nodes[u]._MinFillScore) ;
//	}

	// dump X from the graph; remove all edges adj to X; use temporary storage to reuse edges adj to X.
	if (NULL != av_X_end) {
		if (NULL == AVFLend) 
			AdjVarFreeList = AVFLend = _Nodes[X]._Neighbors ;
		else 
			AVFLend->_NextAdjVar = _Nodes[X]._Neighbors ;
		AVFLend = av_X_end ;
		AVFLend->_NextAdjVar = NULL ;
		_Nodes[X]._Neighbors = NULL ;
		_Nodes[X]._Degree = 0 ;
		_Nodes[X]._EliminationScore = _Nodes[X]._LogK ;
		_Nodes[X]._MinFillScore = 0 ;
		}

	goto pick_next_var ;
}


int ARE::Graph::ComputeVariableEliminationOrder_Simple_wMinFillOnly(int WidthLimit, bool EarlyTermination_W, bool QuitAfterEasyIsDone, int EasyWidth, int n4RandomPick, double eRandomPick, CMauiAVLTreeSimple & avlVars2CheckScore, AdjVarMemoryDynamicManager & TempAdjVarSpace)
{
	_nFillEdges = 0 ;
	if (_nNodes < 1) 
		return 0 ;

	TempAdjVarSpace.Reset() ;
	if (WidthLimit > 1048576) 
		WidthLimit = 1048576 ;
	if (EasyWidth < 1) 
		EasyWidth = 1 ;

	int i, j, k, X, u, v ;

	int nRemaining = _nNodes - _OrderLength ;
	int IterationIdx = _OrderLength ;

	// this is a list of keeping edges that were part of the graph and were removed and that can be reused
	AdjVar *AdjVarFreeList = NULL ;
	AdjVar *AVFLend = NULL ;

#define maxNumVarsForPicking 256 // this is built-in hard limit of the number of variables we consider for picking; make it large enough.
	int nVarsToPickFrom ;
	int VarsToPickFrom[maxNumVarsForPicking] ; // make sure this array has at least maxNumVarsForPicking elements
	double VarsToPickFrom_ProbFactor[maxNumVarsForPicking] ; // make sure this array has at least maxNumVarsForPicking elements
	double VarsToPickFrom_Scores[maxNumVarsForPicking] ; // make sure this array has at least maxNumVarsForPicking elements; keep sorted in inc order.
	if (n4RandomPick < 1) 
		n4RandomPick = 1 ;
	else if (n4RandomPick > maxNumVarsForPicking) 
		n4RandomPick = maxNumVarsForPicking ;

	int nTotalEdgesAdded = 0 ;
	int nEdgesAdded = 0 ;

pick_next_var :

	if (1+nRemaining <= EasyWidth) {
		for (i = 0 ; i < _nTrivialNodes ; i++) {
			X = _TrivialNodesList[i] ;
			_VarType[X] = 0 ;
			_PosOfVarInList[X] = _OrderLength ;
			_VarElimOrder[_OrderLength++] = X ;
			if (_Nodes[X]._Degree > _VarElimOrderWidth) 
				_VarElimOrderWidth = _Nodes[X]._Degree ;
			}
		for (i = 0 ; i < _nMinFillScore0Nodes ; i++) {
			X = _MinFill0ScoreList[i] ;
			_VarType[X] = 0 ;
			_PosOfVarInList[X] = _OrderLength ;
			_VarElimOrder[_OrderLength++] = X ;
			if (_Nodes[X]._Degree > _VarElimOrderWidth) 
				_VarElimOrderWidth = _Nodes[X]._Degree ;
			}
		for (i = 0 ; i < _nRemainingNodes ; i++) {
			X = _RemainingNodesList[i] ;
			_VarType[X] = 0 ;
			_PosOfVarInList[X] = _OrderLength ;
			_VarElimOrder[_OrderLength++] = X ;
			if (_Nodes[X]._Degree > _VarElimOrderWidth) 
				_VarElimOrderWidth = _Nodes[X]._Degree ;
			}
		_nTrivialNodes = _nMinFillScore0Nodes = _nRemainingNodes = 0 ;
		return 0 ;
		}

	if (_nTrivialNodes > 0) {
		X = _TrivialNodesList[0] ;
		goto eliminate_picked_variable ;
		}
	else if (_nMinFillScore0Nodes > 0) {
		X = _MinFill0ScoreList[0] ;
		goto eliminate_picked_variable ;
		}

	if (QuitAfterEasyIsDone) 
		return 0 ;

	// **********************************************************************
	// Pool size is 1. collect nodes with the best score, and pick on randomly out of these.
	// **********************************************************************

	if (n4RandomPick < 2) {
		nVarsToPickFrom = 0 ;
		__int64 best_score = _I64_MAX ;
		for (i = 0 ; i < _nRemainingNodes ; i++) {
			u = _RemainingNodesList[i] ;
			if (EarlyTermination_W) {
				if (_Nodes[u]._Degree > WidthLimit) 
					// don't consider variables with larger width that known best width.
					// this may not be optimal, since we really minimize total elimination complexity, and space.
					// however, we want to quickly give up on an ordering computation, if it does not lead to a new better ordering that the previously known best.
					// we use the width as the key.
					continue ;
				}

			// There should be no variables with MinFillScore=0, although it is not important for the code here to work correctly.
			// We will pick n4RandomPick variables with the smallest EliminationComplexity.
			__int64 score = _Nodes[u]._MinFillScore ;
			if (score > best_score) 
				continue ;
			if (score < best_score) {
				nVarsToPickFrom = 1 ;
				VarsToPickFrom[0] = u ;
				best_score = score ;
				}
			else if (nVarsToPickFrom < maxNumVarsForPicking) {
				VarsToPickFrom[nVarsToPickFrom++] = u ;
				}
			}
		if (0 == nVarsToPickFrom) 
			return ERRORCODE_NoVariablesLeftToPickFrom ;
		else if (1 == nVarsToPickFrom) 
			i = 0 ;
		else 
			i = _RNG.randInt(nVarsToPickFrom-1) ;
		X = VarsToPickFrom[i] ;
		goto eliminate_picked_variable ;
		}

	// **********************************************************************
	// Pool picking
	// **********************************************************************

	nVarsToPickFrom = 0 ;
	for (i = 0 ; i < _nRemainingNodes ; i++) {
		u = _RemainingNodesList[i] ;
		if (EarlyTermination_W) {
			if (_Nodes[u]._Degree > WidthLimit) 
				// don't consider variables with larger width that known best width.
				// this may not be optimal, since we really minimize total elimination complexity, and space.
				// however, we want to quickly give up on an ordering computation, if it does not lead to a new better ordering that the previously known best.
				// we use the width as the key.
				continue ;
			}

		// There should be no variables with MinFillScore=0, although it is not important for the code here to work correctly.
		// We will pick n4RandomPick variables with the smallest EliminationComplexity.
		double score = _Nodes[u]._MinFillScore ;
		if (nVarsToPickFrom < n4RandomPick) {
			VarsToPickFrom_Scores[nVarsToPickFrom] = score ;
			VarsToPickFrom[nVarsToPickFrom] = u ;
			++nVarsToPickFrom ;
			}
		else {
			// find max
			k = 0 ;
			for (j = 1 ; j < nVarsToPickFrom ; j++) {
				if (VarsToPickFrom_Scores[j] > VarsToPickFrom_Scores[k]) 
					k = j ;
				}
			// if better than max, replace it
			if (score < VarsToPickFrom_Scores[k]) {
				VarsToPickFrom_Scores[k] = score ;
				VarsToPickFrom[k] = u ;
				}
			}
		}
	if (0 == nVarsToPickFrom) 
		return ERRORCODE_NoVariablesLeftToPickFrom ;

	// pick variable to eliminate
	if (nVarsToPickFrom > 1) {
		// we assume that no VarsToPickFrom_ProbFactor[] is 0.
		double nTotal = 0.0 ;
		for (i = 0 ; i < nVarsToPickFrom ; i++) {
			VarsToPickFrom_Scores[i] += 1.0 ;
			if (VarsToPickFrom_Scores[i] > 1000000.0) 
				// this check is necessary so that pow() won't get too large and throw exception.
				// also, note that score should not be 0!!! since pow() will throw exception on that too.
				VarsToPickFrom_Scores[i] = 1000000.0 ;
			// NOTE 2011-02-06 KK : MinComplexity case (2) right now score here is log. we may want to exp() it.
			VarsToPickFrom_ProbFactor[i] = pow(VarsToPickFrom_Scores[i], eRandomPick) ;
			nTotal += VarsToPickFrom_ProbFactor[i] ;
			}
		double r = _RNG.randExc(nTotal) ;
		for (i = 0 ; i < nVarsToPickFrom ; i++) {
			r -= VarsToPickFrom_ProbFactor[i] ;
			if (r < 0.0) 
				break ;
			}
		}
	else 
		i = 0 ;
	X = VarsToPickFrom[i] ;

eliminate_picked_variable :

	nTotalEdgesAdded += nEdgesAdded ;
	nEdgesAdded = 0 ;

	avlVars2CheckScore.EmptyQuick() ;

	/*
		DEF = width is number of variables in a cluster - 1.
	*/


	RemoveVarFromList(X) ;
	_VarType[X] = 0 ;
	_PosOfVarInList[X] = _OrderLength ;
	_VarElimOrder[_OrderLength++] = X ;
	--nRemaining ;

	if (_Nodes[X]._Degree > _VarElimOrderWidth) 
		_VarElimOrderWidth = _Nodes[X]._Degree ;

	if (EarlyTermination_W) {
		// check if the width is blown
		if (_VarElimOrderWidth >= WidthLimit) 
			return ERRORCODE_EliminationWidthTooLarge ;
		}

	AdjVar *av_X_end = NULL, *av ;

	if (0 == _Nodes[X]._Degree) {
		goto pick_next_var ;
		}

	IterationIdx = _OrderLength ;

	// reduce the graph : remove edges adj to X + add edges between variables adj to X that don't have an edge.
	// when removing an edge, there are two cost updates : 
	//		1) common neighbors of the two endpoints of the edge have +1 (in the future, when the common endpoint is eliminated, one more edge needs to be added)
	//		2) for each non-common neighbor, for both endpoints, the endpoint gets -1, since in the future the endpoint does not have to add an edge
	//		*) note that for us case here, common endpoints are also adj to X, and since they would get both +1 and -1, resulting in 0, and therefore, 
	//			we only have to add -1 for each adj node, not adj to X, for each node adj to X.
	// when adding an edge (u,v), the cost updates are :
	//		1) all common neighbors of u,v get -1 (they no longer have to add an edge in the future, if they were to be eliminated)
	//		2) u/v gets, for each non-common neighbors, +1 since a new edge between v/u and that non-common neighbor needs to be added

//	// TODO/NOTE : we use long to store edges, which leaves 16 bits for variable, limiting nNodes to 16.
//	edges2add.Empty() ;


	AdjVar *avX ;
	for (avX = _Nodes[X]._Neighbors ; NULL != avX ; avX = avX->_NextAdjVar) {
		av_X_end = avX ;
		u = avX->_V ;
		avlVars2CheckScore.Insert(u) ;
		_Nodes[u]._Degree-- ;

		// do a comparative scan of neighbor lists of X and u
		AdjVar *av_u = _Nodes[u]._Neighbors ;
		AdjVar *av_X = _Nodes[X]._Neighbors ;
		AdjVar *av_u_last = NULL, *av_u_next ;
move_on :
		if (NULL == av_X) { // all nodes left in av_u are adjacent to u but not to X
			for (; NULL != av_u ; av_u = av_u_next) {
				av_u_next = av_u->_NextAdjVar ;
				if (X == av_u->_V) { // take av_u out of the adjacency list of u
					if (NULL == av_u_last) 
						{ _Nodes[u]._Neighbors = av_u_next ; }
					else 
						{ av_u_last->_NextAdjVar = av_u_next ; }
					// add av_u to the list of empty edges
					if (NULL == AVFLend) AdjVarFreeList = AVFLend = av_u ; else AVFLend->_NextAdjVar = av_u ; AVFLend = av_u ; av_u->_NextAdjVar = NULL ;
					}
				else { // (u,X) edge will be gone; u no longer has to connect av_u->_V and X; subtract 1.
					_Nodes[u]._MinFillScore-- ;
					av_u_last = av_u ;
					}
				}
			continue ;
			}
		if (u == av_X->_V) {
			av_X = av_X->_NextAdjVar ;
			goto move_on ;
			}
		if (NULL == av_u) { // all nodes left in av_X are adjacent to X but not to u; add an edge for each, between (av_X->_V, u)
			for (; NULL != av_X ; av_X = av_X->_NextAdjVar) {
				v = av_X->_V ;
				if (u == v) continue ;

				if (u < v) {
					// edges2add.Insert((u << 16) | v) ; // else edges2add.Insert((v << 16) | u) ; 2010-10-08 KK : add iff u<v, otherwise we would add twice
					_EdgeU[nEdgesAdded] = u ;
					_EdgeV[nEdgesAdded] = v ;
					nEdgesAdded++ ;
					}

				// fetch empty structure
				if (NULL != AdjVarFreeList) { av = AdjVarFreeList ; AdjVarFreeList = AdjVarFreeList->_NextAdjVar ; if (NULL == AdjVarFreeList) AVFLend = NULL ; }
				else {
					av = TempAdjVarSpace.GetNextElement() ;
					if (NULL == av) 
						return ERRORCODE_out_of_memory ;
					}

				// update node [u]
				av->_NextAdjVar = NULL ;
				av->_V = v ;
				av->_IterationEdgeAdded = IterationIdx ; // edge iteration index equals the idx of the variable, in the order, that was eliminated during the iteration
				if (NULL == av_u_last) 
					_Nodes[u]._Neighbors = av ;
				else 
					av_u_last->_NextAdjVar = av ;
				av_u_last = av ;
				_Nodes[u]._Degree++ ;
				// update ElimScore later, so that all ElimScore reductions get done first
				}
			continue ;
			}
		if (X == av_u->_V) { // take av_u out of the adjacency list of u
			av_u_next = av_u->_NextAdjVar ;
			if (NULL == av_u_last) 
				{ _Nodes[u]._Neighbors = av_u_next ; }
			else 
				{ av_u_last->_NextAdjVar = av_u_next ; }
			// add av_u to the list of empty edges
			if (NULL == AVFLend) AdjVarFreeList = AVFLend = av_u ; else AVFLend->_NextAdjVar = av_u ; AVFLend = av_u ; av_u->_NextAdjVar = NULL ;
			av_u = av_u_next ;
			goto move_on ;
			}
		if (av_u->_V < av_X->_V) { // av_u->_V is adjacent to u but not to X; subtract 1 from u
			_Nodes[u]._MinFillScore-- ;
			av_u_last = av_u ;
			av_u = av_u->_NextAdjVar ;
			goto move_on ;
			}
		if (av_u->_V > av_X->_V) { // av_X->_V is adjacent to X and not adjacent to u; add an edge between (av_X->_V, u), before av_u in the list of u
			v = av_X->_V ;

			if (u < v) {
				// edges2add.Insert((u << 16) | v) ; // else edges2add.Insert((v << 16) | u) ; 2010-10-08 KK : add iff u<v, otherwise we would add twice
				_EdgeU[nEdgesAdded] = u ;
				_EdgeV[nEdgesAdded] = v ;
				nEdgesAdded++ ;
				}

			// fetch empty structure
			if (NULL != AdjVarFreeList) { av = AdjVarFreeList ; AdjVarFreeList = AdjVarFreeList->_NextAdjVar ; if (NULL == AdjVarFreeList) AVFLend = NULL ; }
			else {
				av = TempAdjVarSpace.GetNextElement() ;
				if (NULL == av) 
					return ERRORCODE_out_of_memory ;
				}

			// update node [u]
			av->_NextAdjVar = av_u ;
			av->_V = v ;
			av->_IterationEdgeAdded = IterationIdx ; // edge iteration index equals the idx of the variable, in the order, that was eliminated during the iteration
			if (NULL == av_u_last) {
				_Nodes[u]._Neighbors = av ;
				}
			else {
				av_u_last->_NextAdjVar = av ;
				}
			av_u_last = av ;
			_Nodes[u]._Degree++ ;
			// update ElimScore later, so that all ElimScore reductions get done first

			av_X = av_X->_NextAdjVar ;
			goto move_on ;
			}
		// now it must be that av_u->_V == av_X->_V; ignore this case because the edge (u, av_u->_V) stays
		av_u_last = av_u ;
		av_u = av_u->_NextAdjVar ;
		av_X = av_X->_NextAdjVar ;
		goto move_on ;
		}

	// add edges between nodes (that used to be) adj to X that don't have them yet
	for (i = 0 ; i < nEdgesAdded ; i++) {
		u = _EdgeU[i] ;
		v = _EdgeV[i] ;
		AdjustScoresForArcAddition(u, v, IterationIdx, avlVars2CheckScore) ;
		}
	_nFillEdges += nEdgesAdded ;

	// check variables whose MinFillScore changed whether they need to be moved to a different list.
	long pos = - 1, ul ;
	while (avlVars2CheckScore.GetNext(&ul, pos) != 0) 
		ProcessPostEliminationNodeListLocation(ul) ;

	// dump X from the graph; remove all edges adj to X; use temporary storage to reuse edges adj to X.
	if (NULL != av_X_end) {
		if (NULL == AVFLend) 
			AdjVarFreeList = AVFLend = _Nodes[X]._Neighbors ;
		else 
			AVFLend->_NextAdjVar = _Nodes[X]._Neighbors ;
		AVFLend = av_X_end ;
		AVFLend->_NextAdjVar = NULL ;
		_Nodes[X]._Neighbors = NULL ;
		_Nodes[X]._Degree = 0 ;
		_Nodes[X]._MinFillScore = 0 ;
		}

	goto pick_next_var ;
}

