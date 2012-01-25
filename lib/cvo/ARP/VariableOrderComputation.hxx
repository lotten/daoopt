#ifndef ARE_VariableOrderComputation_HXX_INCLUDED
#define ARE_VariableOrderComputation_HXX_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "Graph.hxx"

namespace ARE
{

#define ARE_TempAdjVarSpaceSize 100000

class VariableEliminationOrderComputationResultSnapShot
{
public :
	int _dt ;
	int _width ;
	double _complexity ;
public :
	void operator=(const VariableEliminationOrderComputationResultSnapShot & O)
	{
		_dt = O._dt ;
		_width = O._width ;
		_complexity = O._complexity ;
	}
public :
	VariableEliminationOrderComputationResultSnapShot(void) 
		:
		_dt(0), 
		_width(0), 
		_complexity(0.0)
	{
	}
} ;

class VariableEliminationOrder
{
public :
	int _nVars ;
	int *_VarListInElimOrder ;
	int _Width ;
	double _Complexity ; // log of
	double _TotalNewFunctionStorageAsNumOfElements ; // log of
	int _nRunsDone ;
	int _nRunsGood ;
	int _nFillEdges ;
public :
	__int64 _Width2CountMap[1024] ; // for widths [0,1023], how many times it was obtained
	double _Width2MinComplexityMap[1024] ; // for widths [0,1023], log of smallest complexity
	double _Width2MaxComplexityMap[1024] ; // for widths [0,1023], log of largest complexity
public :
	int _nImprovements ;
	ARE::VariableEliminationOrderComputationResultSnapShot _Improvements[1024] ;
public :
	inline int Initialize(int n)
	{
		if (NULL != _VarListInElimOrder) { delete [] _VarListInElimOrder ; _VarListInElimOrder = NULL ; }
		_nVars = 0 ;
		_Width = 0 ;
		_Complexity = 0.0 ;
		_TotalNewFunctionStorageAsNumOfElements = 0.0 ;
		_nRunsDone = _nRunsGood = 0 ;
		_nFillEdges = 0 ;
		_nImprovements = 0 ;
		if (n > 0) {
			_VarListInElimOrder = new int[n] ;
			if (NULL == _VarListInElimOrder) 
				return 1 ;
			_nVars = n ;
			}
		for (int i = 0 ; i < 1024 ; i++) {
			_Width2CountMap[i] = 0 ;
			_Width2MinComplexityMap[i] = DBL_MAX ;
			_Width2MaxComplexityMap[i] = 0 ;
			}
		return 0 ;
	}
public :
	VariableEliminationOrder(void)
		: 
		_nVars(0), 
		_VarListInElimOrder(NULL), 
		_Width(0), 
		_Complexity(0.0), 
		_TotalNewFunctionStorageAsNumOfElements(0.0), 
		_nRunsDone(0), 
		_nRunsGood(0), 
		_nFillEdges(0), 
		_nImprovements(0)
	{
	}
	~VariableEliminationOrder(void)
	{
		if (NULL != _VarListInElimOrder) 
			delete [] _VarListInElimOrder ;
	}
} ;

class VOC_worker
{
public :
	int _IDX ;
	Graph *_G ;
public :
	char _Algorithm ; // 0=MinFill, 1=MinDegree(MinInducedWidth), 2=MinComplexity
	int _nRandomPick ;
#ifdef WINDOWS
	uintptr_t _ThreadHandle ;
#elif defined (LINUX)
	pthread_t _ThreadHandle ;
#endif 
	bool _ThreadIsRunning ;
	bool _ThreadStop ;
	int _nRunsDone ;
	AdjVar _TempAdjVarSpace[ARE_TempAdjVarSpaceSize] ;
public :
	VOC_worker(int idx = -1, Graph *G = NULL)
		:
		_IDX(idx), 
		_G(G), 
		_Algorithm(0), 
		_nRandomPick(1), 
		_ThreadHandle(0), 
		_ThreadIsRunning(false), 
		_ThreadStop(true), 
		_nRunsDone(0)
	{
	}
	~VOC_worker(void)
	{
		if (NULL != _G) 
			delete _G ;
	}
} ;

int ComputeVariableEliminationOrder(
	// IN
	const std::string & uaifile, 
	int objCode, 
	int algCode, 
	int nThreads, 
	int nRunsToDo, 
	long TimeLimitInSeconds, 
	int nRandomPick, 
	double eRandomPick, 
	bool PerformSingletonConsistencyChecking, 
	bool EarlyTerminationOfBasic_W, 
	bool EarlyTerminationOfBasic_C, 
	// OUT
	VariableEliminationOrder & BestOrder 
	) ;

} // namespace ARE

#endif // ARE_VariableOrderComputation_HXX_INCLUDED
