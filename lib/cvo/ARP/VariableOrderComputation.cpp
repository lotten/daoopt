// CVO.cpp : Defines the entry point for the console application.
//

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <time.h>

#ifdef WINDOWS
#include <process.h>    /* _beginthread, _endthread */
#endif // WINDOWS

#include "AVLtreeSimple.hxx"
#include "ARPall.hxx"
#include "RandomProblemGenerator.hxx"

static int objCode = 0 ; // 0=width, 1=state space size
static int algCode = 0 ; // 1=minfill
static int nThreads = 0 ;
static int nRunsToDo = 0 ;
static time_t ttStart = 0, ttEnd = 0, ttToStop = 0 ;
static int LogIncrement = 1 ;
static int nRandomPick = 4 ;
static double eRandomPick = 0.5 ;
static bool EarlyTerminationOfBasic_W = false, EarlyTerminationOfBasic_C = false ;

// global copy of the problem; used by all threads as a starting point
static ARE::utils::RecursiveMutex MasterGraphMutex ;
static ARE::Graph OriginalGraph, MasterGraph ;
#ifdef LINUX
pthread_mutex_t nRunsSumMutex = PTHREAD_MUTEX_INITIALIZER ;
#endif
static volatile long nRunsSum = 0 ;
static long nRunsSum_Good = 0 ;
static ARE::AdjVar TempAdjVarSpace[TempAdjVarSpaceSize] ;

// best orderings : min-width
static int BestOrderN = 0 ;
static int *BestOrder_Order = NULL ;
static int BestOrder_Width = INT_MAX ;
static int BestOrder_nFillEdges = 0 ;
static double BestOrder_MaxSingleVarElimComplexity = DBL_MAX ;
static double BestOrder_Complexity = DBL_MAX ;
static double BestOrder_TotalNewFunctionStorageAsNumOfElements = DBL_MAX ;
static int BestOrder_nImprovements = 0 ;
static ARE::VariableEliminationOrderComputationResultSnapShot BestOrder_Improvements[1024] ;

static __int64 BestOrder_Width2CountMap[1024] ; // for widths [0,1023], how many times it was obtained
static double BestOrder_Width2MinComplexityMap[1024] ; // for widths [0,1023], smallest complexity
static double BestOrder_Width2MaxComplexityMap[1024] ; // for widths [0,1023], largest complexity

static void GetCurrentDT(char *strDT, time_t & ttNow)
{
	// get current date/time string so that we can log when stuff is happening
	if (0 == ttNow) time(&ttNow) ;
	struct tm *pTime = localtime(&ttNow) ;
	char *sDT = asctime(pTime) ; // from manual : The string result produced by asctime contains exactly 26 characters and has the form Wed Jan 02 02:03:55 1980\n\0.
	memcpy(strDT, sDT, 26) ;
	strDT[24] = 0 ;
}

#ifdef WINDOWS
typedef unsigned int (*pWorkerThreadFn)(void *X) ;
static unsigned int __stdcall WorkerThreadFn(void *X) 
#elif defined (LINUX)
typedef void *(*pWorkerThreadFn)(void *X) ;
static void *WorkerThreadFn(void *X)
#endif 
{
	ARE::VOC_worker *w = (ARE::VOC_worker *) X ;
	CMauiAVLTreeSimple avlVars2CheckScore ;

	// initialize best order with to something bad; we don't want anything worse than that.
	int bestWidth = 255 ;
	double bestComplexity = DBL_MAX ;
	{
	ARE::utils::AutoLock lock(MasterGraphMutex) ;
	bestWidth = BestOrder_Width ;
	bestComplexity = BestOrder_Complexity ;
	}

	char best_order_criteria = 0 ; // 0=width, 1=complexity
	if (EarlyTerminationOfBasic_W) 
		best_order_criteria = 0 ;
	else if (EarlyTerminationOfBasic_C) 
		best_order_criteria = 1 ;

	printf("\nworker %2d start; objCode=%d alg=%d ...", (int) w->_IDX, (int) objCode, (int) w->_Algorithm) ;
	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\nworker %2d start; objCode=%d alg=%d ...", (int) w->_IDX, (int) objCode, (int) w->_Algorithm) ;
		fflush(ARE::fpLOG) ;
		}

	try {
		*(w->_G) = MasterGraph ;
		if (! w->_G->_IsValid) 
			goto done ;
		}
	catch (...) {
		printf("\nworker %2d prep0 exception ...", (int) w->_IDX) ;
		if (NULL != ARE::fpLOG) {
			fprintf(ARE::fpLOG, "\nworker %2d prep0 exception ...", (int) w->_IDX) ;
			fflush(ARE::fpLOG) ;
			}
		}

	time_t ttNow ; ttNow = 0 ;
	int res ;
	char strDT[64] ;
	int nCompleteRunsTodo ; nCompleteRunsTodo = 3 ;
	while (! w->_ThreadStop) {
		ttNow = 0 ;
		++(w->_nRunsDone) ;
#ifdef WINDOWS
		long v = InterlockedIncrement(&nRunsSum) ;
#else
		pthread_mutex_lock(&nRunsSumMutex) ;
		long v = ++nRunsSum ;
		pthread_mutex_unlock(&nRunsSumMutex) ;
#endif
		if (0 == (v % LogIncrement)) {
			GetCurrentDT(strDT, ttNow) ;
			printf("\n%s worker %2d will do run %d ...", strDT, (int) w->_IDX, (int) v) ;
			if (NULL != ARE::fpLOG) {
				fprintf(ARE::fpLOG, "\n%s worker %2d will do run %d ...", strDT, (int) w->_IDX, (int) v) ;
				fflush(ARE::fpLOG) ;
				}
			}
// DEBUGGG
//printf("\nworker %d starting; nRunsSum=%d ...", (int) w->_IDX, (int) v) ;
		try {
			bool earlyTerminationOk = nCompleteRunsTodo-- > 0 ? false : true ;
			res = w->_G->ComputeVariableEliminationOrder_Simple(w->_Algorithm, bestWidth, earlyTerminationOk && EarlyTerminationOfBasic_W, bestComplexity, earlyTerminationOk && EarlyTerminationOfBasic_C, false, 10, w->_nRandomPick, eRandomPick, avlVars2CheckScore, w->_TempAdjVarSpace, TempAdjVarSpaceSize) ;
			}
		catch (...) {
			GetCurrentDT(strDT, ttNow) ;
			printf("\nworker %2d exception (%s) ...", (int) w->_IDX, strDT) ;
			if (NULL != ARE::fpLOG) {
				fprintf(ARE::fpLOG, "\nworker %2d exception (%s) ...", (int) w->_IDX, strDT) ;
				fflush(ARE::fpLOG) ;
				goto done ;
				}
			}
// DEBUGGG
//		printf("\nworker %d finished res=%d; width=%d complexity=%I64d", (int) w->_IDX, (int) res, (int) w->_G->_VarElimOrderWidth, (__int64) w->_G->_TotalVarElimComplexity) ;
//		if (NULL != ARE::fpLOG) {
//			fprintf(ARE::fpLOG, "\nworker %d finished res=%d; width=%d complexity=%I64d", (int) w->_IDX, (int) res, (int) w->_G->_VarElimOrderWidth, (__int64) w->_G->_TotalVarElimComplexity) ;
//			fflush(ARE::fpLOG) ;
//			}
		try {
			ARE::utils::AutoLock lock(MasterGraphMutex) ;
//GetCurrentDT(strDT, ttNow) ;
//printf("\n%s worker %2d found width=%d complexity=%I64d space(#elements)=%I64d res=%d", strDT, (int) w->_IDX, (int) w->_G->_VarElimOrderWidth, (__int64) w->_G->_TotalVarElimComplexity, (__int64) w->_G->_TotalNewFunctionStorageAsNumOfElements, (int) res) ;
			if (0 == res) {
				++nRunsSum_Good ;
				if ((1==objCode && (w->_G->_TotalVarElimComplexity < BestOrder_Complexity || (fabs(w->_G->_TotalVarElimComplexity - BestOrder_Complexity) < 0.01 && w->_G->_VarElimOrderWidth < BestOrder_Width))) ||  
					(0==objCode && (w->_G->_VarElimOrderWidth < BestOrder_Width || (w->_G->_VarElimOrderWidth == BestOrder_Width && w->_G->_TotalVarElimComplexity < BestOrder_Complexity)))) {
					GetCurrentDT(strDT, ttNow) ;
					printf("\n%s worker %2d found better solution : width=%d complexity=%g space(#elements)=%g", strDT, (int) w->_IDX, (int) w->_G->_VarElimOrderWidth, (double) w->_G->_TotalVarElimComplexity, (double) w->_G->_TotalNewFunctionStorageAsNumOfElements) ;
					if (NULL != ARE::fpLOG) {
						fprintf(ARE::fpLOG, "\n%s worker %2d found better solution : width=%d complexity=%g space(#elements)=%g", strDT, (int) w->_IDX, (int) w->_G->_VarElimOrderWidth, (double) w->_G->_TotalVarElimComplexity, (double) w->_G->_TotalNewFunctionStorageAsNumOfElements) ;
						fflush(ARE::fpLOG) ;
						}
					BestOrder_Width = w->_G->_VarElimOrderWidth ;
					BestOrder_nFillEdges = w->_G->_nFillEdges ;
					BestOrder_MaxSingleVarElimComplexity = w->_G->_MaxVarElimComplexity ;
					BestOrder_Complexity = w->_G->_TotalVarElimComplexity ;
					BestOrder_TotalNewFunctionStorageAsNumOfElements = w->_G->_TotalNewFunctionStorageAsNumOfElements ;
					for (int i = 0 ; i < BestOrderN ; i++) 
						BestOrder_Order[i] = (w->_G->_VarElimOrder)[i] ;
					ARE::VariableEliminationOrderComputationResultSnapShot & result_record = BestOrder_Improvements[BestOrder_nImprovements++] ;
					result_record._dt = ttNow - ttStart ;
					result_record._width = w->_G->_VarElimOrderWidth ;
					result_record._complexity = w->_G->_TotalVarElimComplexity ;
					}
				if (EarlyTerminationOfBasic_W) {
					if (BestOrder_Width < bestWidth) 
						bestWidth = BestOrder_Width ;
					}
				if (EarlyTerminationOfBasic_C) {
					if (BestOrder_Complexity < bestComplexity) 
						bestComplexity = BestOrder_Complexity ;
					}
				if (w->_G->_VarElimOrderWidth >= 0 && w->_G->_VarElimOrderWidth < 1024) {
					BestOrder_Width2CountMap[w->_G->_VarElimOrderWidth]++ ;
					if (BestOrder_Width2MinComplexityMap[w->_G->_VarElimOrderWidth] > w->_G->_TotalVarElimComplexity) 
						BestOrder_Width2MinComplexityMap[w->_G->_VarElimOrderWidth] = w->_G->_TotalVarElimComplexity ;
					if (BestOrder_Width2MaxComplexityMap[w->_G->_VarElimOrderWidth] < w->_G->_TotalVarElimComplexity) 
						BestOrder_Width2MaxComplexityMap[w->_G->_VarElimOrderWidth] = w->_G->_TotalVarElimComplexity ;
					}
				}
			else {
				// DEBUGGG
//				printf("\nThread %d res=%d", w->_IDX, res) ;
				}
			if (nRunsSum >= nRunsToDo) {
// DEBUGGG
//				printf("\nworker %d out of runs (%d >= %d) ...", (int) w->_IDX, (int) nRunsSum, (int) nRunsToDo) ;
				goto done ;
				}
			if (ttToStop > 0) {
				if (0 == ttNow) time(&ttNow) ;
				if (ttNow >= ttToStop) {
// DEBUGGG
//					printf("\nworker %d out of time ...", (int) w->_IDX) ;
					goto done ;
					}
				}

			}
		catch (...) {
			GetCurrentDT(strDT, ttNow) ;
			printf("\nworker %d summary exception; %s ...", (int) w->_IDX, strDT) ;
			if (NULL != ARE::fpLOG) {
				fprintf(ARE::fpLOG, "\nworker %d summary exception; %s ...", (int) w->_IDX, strDT) ;
				fflush(ARE::fpLOG) ;
				}
			}

		try {
			*(w->_G) = MasterGraph ;
			}
		catch (...) {
			GetCurrentDT(strDT, ttNow) ;
			printf("\nworker %d prep exception; %s ...", (int) w->_IDX, strDT) ;
			if (NULL != ARE::fpLOG) {
				fprintf(ARE::fpLOG, "\nworker %d prep exception; %s ...", (int) w->_IDX, strDT) ;
				fflush(ARE::fpLOG) ;
				}
			}
		}

done :
	w->_ThreadHandle = 0 ;
	w->_ThreadIsRunning = false ;
#ifdef WINDOWS
	_endthreadex(0) ;
	return 0  ;
#else
	return NULL ;
#endif
}


int ARE::ComputeVariableEliminationOrder(
	// IN
	const std::string & uaifile, 
	int objcode, 
	int algcode, 
	int nthreads, 
	int nrunstodo, 
	long TimeLimitInSeconds, 
	int nRP, 
	double eRP, 
	bool PerformSingletonConsistencyChecking, 
	bool earlyterminationofbasic_W, 
	bool earlyterminationofbasic_C, 
	// OUT
	ARE::VariableEliminationOrder & BestOrder
	)
{
	int ret = 1 ;

	char strDT[64] ;
	time_t ttNow = 0 ;
	GetCurrentDT(strDT, ttNow) ;
	printf("\n%s Start ...", strDT) ;
	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\n%s Start ...", strDT) ;
		fflush(ARE::fpLOG) ;
		}

	std::string fn ;
	ARE::ARP p("") ;
	int nWorkers = 0 ;
	ARE::VOC_worker *Workers = NULL ;
	CMauiAVLTreeSimple avlVars2CheckScore ;

	algCode = algcode ;
	if (algCode < 0 || algCode > 2) 
		algCode = 0 ;
	objCode = objcode ;
	if (0 != objCode && 1 != objCode) 
		objCode = 0 ;

	nRunsToDo = nrunstodo ;
	if (nRunsToDo < 1) 
		nRunsToDo = 1 ;
	else if (nRunsToDo > 1000000000) 
		nRunsToDo = 1000000000 ;

	if (TimeLimitInSeconds < 0) 
		TimeLimitInSeconds = 0 ;
	else if (TimeLimitInSeconds > 86400) 
		TimeLimitInSeconds = 86400 ;

	eRandomPick = eRP ;
	nRandomPick = nRP ;
	if (nRandomPick < 1) 
		nRandomPick = 1 ;

	EarlyTerminationOfBasic_W = earlyterminationofbasic_W ;
	EarlyTerminationOfBasic_C = earlyterminationofbasic_C ;

	if (uaifile.length() < 1) 
		return 1 ;

	nThreads = nthreads ;
	if (nThreads > nRunsToDo-1) 
		nThreads = nRunsToDo-1 ;
	else if (nThreads < 1) 
		nThreads = 1 ;
// DEBUGGG
//nThreads = 1 ;

	// at least one of nRunsToDo/TimeLimitInSeconds has to be given
	if (nRunsToDo < 1 && TimeLimitInSeconds < 1) 
		nRunsToDo = 1 ;

	// uaiFN may have dir in it; extract filename.
	int i = uaifile.length() - 1 ;
	for (; i >= 0 ; i--) {
#ifdef LINUX
		if ('\\' == uaifile[i] || '/' == uaifile[i]) break ;
#else
    if ('\\' == uaifile[i] || '//' == uaifile[i]) break ;
#endif
		}
	fn = uaifile.substr(i+1) ;

	p.SetName(fn) ;
	if (0 != p.LoadFromFile(uaifile)) {
		ret = 2 ;
		printf("\nload failed ...") ;
		goto done ;
		}
	if (p.N() < 1) {
		ret = 3 ;
		printf("\nN=%d; will exit ...", p.N()) ;
		goto done ;
		}
/*
	if (p.N() > (1 << 16)) {
//		printf("\nN=%d; currently nVars limit is 64K since AVL tree (edgesadded) with key type long is used to store 2 variable indeces; will exit ...", p.N()) ;
		printf("\nN=%d; currently nVars limit is 64K since heap uses 2 bytes for cost and 2 bytes for var index, for total 4 bytes; change heap key to __in64; will exit ...", p.N()) ;
		goto done ;
		}
*/

	ttNow = 0 ;
	GetCurrentDT(strDT, ttNow) ;
	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\n%s File loaded; start preprocessing ...", strDT) ;
		fflush(ARE::fpLOG) ;
		}

	// eliminate singleton-domain variables; do this before ordering is computed; this is easy and should be done by any algorithm processing the network
	p.EliminateSingletonDomainVariables() ;

	// prune domains of variables by checking which values participate in no complete assignment with probability>0.
	// basically, we compute a minimal domain for each variable.
	if (PerformSingletonConsistencyChecking) {
		int nNewSingletonDomainVariablesAfterSingletonConsistency = 0 ;
		if (-1 == p.ComputeSingletonConsistency(nNewSingletonDomainVariablesAfterSingletonConsistency)) {
			// domain of some variable is empty
			printf("\np.ComputeSingletonConsistency(): domain of some variable is empty; will quit ...") ;
			if (NULL != ARE::fpLOG) {
				fprintf(ARE::fpLOG, "\np.ComputeSingletonConsistency(): domain of some variable is empty; will quit ...") ;
				fflush(ARE::fpLOG) ;
				}
			goto done ;
			}
		printf("\np.ComputeSingletonConsistency(): nNewSingletonDomainVariablesAfterSingletonConsistency = %d", nNewSingletonDomainVariablesAfterSingletonConsistency) ;
		if (NULL != ARE::fpLOG) {
			fprintf(ARE::fpLOG, "\np.ComputeSingletonConsistency(): nNewSingletonDomainVariablesAfterSingletonConsistency = %d", nNewSingletonDomainVariablesAfterSingletonConsistency) ;
			fflush(ARE::fpLOG) ;
			}
		if (nNewSingletonDomainVariablesAfterSingletonConsistency > 0) 
			p.EliminateSingletonDomainVariables() ;
		}
	else {
		printf("\nSingleton-Consistency check not requested ...") ;
		if (NULL != ARE::fpLOG) 
			fprintf(ARE::fpLOG, "\nSingleton-Consistency check not requested ...") ;
		}

	BestOrder_Order = new int[p.N()] ;
	if (NULL == BestOrder_Order) {
		ret = 4 ;
		printf("\nfailed to allocate memory for order...") ;
		if (NULL != ARE::fpLOG) {
			fprintf(ARE::fpLOG, "\nfailed to allocate memory for order...") ;
			fflush(ARE::fpLOG) ;
			}
		goto done ;
		}

	// create problem graph
	OriginalGraph.Create(p) ;
	if (! OriginalGraph._IsValid) {
		ret = 5 ;
		goto done ;
		}
	BestOrderN = p.N() ;

	// do all the easy eliminations; this will give us a starting point for large-scale randomized searches later.
	MasterGraph = OriginalGraph ;
	i = MasterGraph.ComputeVariableEliminationOrder_Simple(0, INT_MAX, false, DBL_MAX, false, true, 10, 1, 0.0, avlVars2CheckScore, TempAdjVarSpace, TempAdjVarSpaceSize) ;
	MasterGraph.ReAllocateEdges() ;

	ttNow = 0 ;
	GetCurrentDT(strDT, ttNow) ;
	ttStart = ttNow ;
	if (TimeLimitInSeconds > 0) 
		ttToStop = ttStart + TimeLimitInSeconds ;
	printf("\n%s Preprocessing done; ready to run ...", strDT) ;
	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\n%s Preprocessing done; ready to run ...", strDT) ;
		fflush(ARE::fpLOG) ;
		}

	// execute one run here to get some real bound on width/complexity.
	{
	ARE::Graph g ;
	g = MasterGraph ;
	++nRunsSum ;
	i = g.ComputeVariableEliminationOrder_Simple(0, INT_MAX, false, DBL_MAX, false, false, 10, 1, 0.0, avlVars2CheckScore, TempAdjVarSpace, TempAdjVarSpaceSize) ;
	BestOrder_nImprovements = 1 ;
	ARE::VariableEliminationOrderComputationResultSnapShot & result_record_W = BestOrder_Improvements[0] ;
	result_record_W._dt = ttNow - ttStart ;
	result_record_W._width = g._VarElimOrderWidth ;
	result_record_W._complexity = g._TotalVarElimComplexity ;
	if (0 == i) {
		printf("\nInitial computation width=%d; MaxSingleVarElimComplexity=%g, TotalVarElimComplexity=%g, TotalNewFunctionStorageAsNumOfElements=%g", (int) g._VarElimOrderWidth, (double) g._MaxVarElimComplexity, (double) g._TotalVarElimComplexity, (double) g._TotalNewFunctionStorageAsNumOfElements) ;
		if (NULL != ARE::fpLOG) {
			fprintf(ARE::fpLOG, "\nInitial computation width=%d; MaxSingleVarElimComplexity=%g, TotalVarElimComplexity=%g, TotalNewFunctionStorageAsNumOfElements=%g", (int) g._VarElimOrderWidth, (double) g._MaxVarElimComplexity, (double) g._TotalVarElimComplexity, (double) g._TotalNewFunctionStorageAsNumOfElements) ;
			fflush(ARE::fpLOG) ;
			}
		++nRunsSum_Good ;
		BestOrder_Width = g._VarElimOrderWidth ;
		BestOrder_nFillEdges = g._nFillEdges ;
		BestOrder_MaxSingleVarElimComplexity = g._MaxVarElimComplexity ;
		BestOrder_Complexity = g._TotalVarElimComplexity ;
		BestOrder_TotalNewFunctionStorageAsNumOfElements = g._TotalNewFunctionStorageAsNumOfElements ;
		for (i = 0 ; i < BestOrderN ; i++) {
			BestOrder_Order[i] = g._VarElimOrder[i] ;
			}
		}
	else {
		printf("\nInitial computation failed; res=%d; will continue ...", i) ;
		if (NULL != ARE::fpLOG) {
			fprintf(ARE::fpLOG, "\nInitial computation failed; res=%d; will continue ...", i) ;
			fflush(ARE::fpLOG) ;
			}
		}
	}

	if (nRunsSum >= nRunsToDo) 
		goto done ;
	if (nThreads < 1) {
		if (NULL != ARE::fpLOG) {
			fprintf(ARE::fpLOG, "\nNo threads; will quit ...") ;
			fflush(ARE::fpLOG) ;
			goto done ;
			}
		}
//goto done ;

	// initialize some stats space
	for (i = 0 ; i < 1024 ; i++) {
		BestOrder_Width2CountMap[i] = 0 ;
		BestOrder_Width2MinComplexityMap[i] = DBL_MAX ;
		BestOrder_Width2MaxComplexityMap[i] = 0 ;
		}

	// create workers; don't let them run yet
	Workers = new ARE::VOC_worker[nThreads] ;
	if (NULL == Workers) {
		ret = 6 ;
		goto done ;
		}
	nWorkers = nThreads ;
	for (i = 0 ; i < nThreads ; i++) {
		Workers[i]._IDX = i ;
		Workers[i]._G = new ARE::Graph ;
		if (NULL == Workers[i]._G) {
			ret = 7 ;
			goto done ;
			}
//		Workers[i]._Algorithm = i % 3 ;
		Workers[i]._Algorithm = algCode ;
//		Workers[i]._nRandomPick = (0 == (i & 1)) ? 1 : nRandomPick ;
		Workers[i]._nRandomPick = nRandomPick ;
		}

	LogIncrement = nRunsToDo/20 ;
	if (LogIncrement < 1) LogIncrement = 1 ;
	for (i = 0 ; i < nThreads ; i++) {
		Workers[i]._ThreadIsRunning = true ;
		Workers[i]._ThreadStop = false ;
#ifdef WINDOWS
		Workers[i]._ThreadHandle = _beginthreadex(NULL, 0, WorkerThreadFn, &(Workers[i]), 0, NULL) ;
#else
		pthread_create(&(Workers[i]._ThreadHandle), NULL, WorkerThreadFn, &(Workers[i])) ; // TODO third argument
#endif 
		if (0 == Workers[i]._ThreadHandle) {
			Workers[i]._ThreadIsRunning = false ;
			Workers[i]._ThreadStop = true ;
			printf("\nFAILED to create thread %d", i) ;
			if (NULL != ARE::fpLOG) {
				fprintf(ARE::fpLOG, "\nFAILED to create thread %d", i) ;
				fflush(ARE::fpLOG) ;
				}
			ret = 8 ;
			goto done ;
			}
		}

	while (true) {
		SLEEP(100) ;
		int nRunning = 0 ;
		for (i = 0 ; i < nThreads ; i++) {
			if (0 == Workers[i]._ThreadHandle) continue ;
			++nRunning ;
			}
		if (nRunning <= 0) 
			break ;
		}
	printf("\nall threads have closed ...") ;
	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\nall threads have closed ...") ;
		fflush(ARE::fpLOG) ;
		}

/*
	if (TimeLimitInSeconds > 0) {
		Sleep(1000*TimeLimitInSeconds) ;
		for (i = 0 ; i < nThreads ; i++) 
			Workers[i]._ThreadStop = true ;
		for (i = 0 ; i < nThreads ; i++) {
			if (0 == Workers[i]._ThreadHandle) continue ;
			DWORD r = WaitForSingleObject((HANDLE) Workers[i]._ThreadHandle, 10) ;
			if (WAIT_TIMEOUT == r) 
				TerminateThread((HANDLE) Workers[i]._ThreadHandle, 0) ;
			CloseHandle((HANDLE) Workers[i]._ThreadHandle) ;
			Workers[i]._ThreadHandle = 0 ;
			}
		}
*/

	ret = 0 ;
done :
	time(&ttEnd) ;
	printf("\ndone; time=%dsec ...", (int) ttEnd - ttStart) ;
	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\ndone; time=%dsec ...", (int) ttEnd - ttStart) ;
		fflush(ARE::fpLOG) ;
		}
//	for (i = 0 ; i < nWorkers ; i++) 
//		nRuns += Workers[i]._nRunsDone ;
	if (NULL != Workers) 
		delete [] Workers ;
	if (BestOrder_Width < INT_MAX) {
		if (0 != BestOrder.Initialize(BestOrderN)) 
			return 1 ;

		BestOrder._Width = BestOrder_Width ;
		BestOrder._Complexity = BestOrder_Complexity ;
		BestOrder._TotalNewFunctionStorageAsNumOfElements = BestOrder_TotalNewFunctionStorageAsNumOfElements ;
		BestOrder._nFillEdges = BestOrder_nFillEdges ;
		BestOrder._nRunsDone = nRunsSum ;
		BestOrder._nRunsGood = nRunsSum_Good ;
		for (i = 0 ; i < BestOrderN ; i++) 
			BestOrder._VarListInElimOrder[i] = BestOrder_Order[i] ;
		BestOrder._nImprovements = BestOrder_nImprovements ;
		for (i = 0 ; i < BestOrder._nImprovements ; i++) 
			BestOrder._Improvements[i] = BestOrder_Improvements[i] ;
		for (i = 0 ; i < 1024 ; i++) {
			BestOrder._Width2CountMap[i] = BestOrder_Width2CountMap[i] ;
			BestOrder._Width2MinComplexityMap[i] = BestOrder_Width2MinComplexityMap[i] ;
			BestOrder._Width2MaxComplexityMap[i] = BestOrder_Width2MaxComplexityMap[i] ;
			}
		}
	else {
		BestOrder.Initialize(0) ;
		}

	// remove redundant fill edges
	if (false && BestOrder_Width > 0) {
		ARE::Graph g ;
		g = OriginalGraph ;
		g._OrderLength = BestOrderN ;
		for (i = 0 ; i < BestOrderN ; i++) {
			int v_i = BestOrder_Order[i] ;
			g._VarType[i] = 0 ;
			g._PosOfVarInList[v_i] = i ;
			g._VarElimOrder[i] = v_i ;
			}
		g._nFillEdges = BestOrder_nFillEdges ;
		g.RemoveRedundantFillEdges() ;
		int nEdgesRemoved = (OriginalGraph._nEdges + BestOrder_nFillEdges) - g._nEdges ;
		printf("\n# of redundant fill edges = %d", nEdgesRemoved) ;
		}

	return ret ;
}

