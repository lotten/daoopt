#include <stdlib.h>

#include "MersenneTwister.h"
#include "Function.hxx"
#include "Problem.hxx"
#include "ProblemGraphNode.hxx"
#include "Workspace.hxx"
#include "BEworkspace.hxx"

void ARE::Workspace::AddErrorExplanation(ARE::Function *f, ARE::FunctionTableBlock *ftb)
{
	ARE::Explanation *e = new ARE::Explanation(ARE::Explantion_Error) ;
	if (NULL == e) 
		SetFatalError() ;
	else {
		if (NULL != f) 
			e->_Bucket = f->OriginatingBucket() ;
		if (NULL != ftb) 
			e->_FTBidx = ftb->IDX() ;
		AddExplanation(*e) ;
		}
}


void ARE::Workspace::AddExplanation(ARE::Explanation & E)
{
	E._Next = _ExplanationList ;
	_ExplanationList = &E ;
}


bool ARE::Workspace::HasErrorExplanation(void)
{
	if (_HasFatalError) 
		return true ;
	for (ARE::Explanation *e = _ExplanationList ; NULL != e ; e = e->_Next) {
		if (ARE::Explantion_NONE == e->_Type) 
			return true ;
		}
	return false ;
}


void ARE::Workspace::LogStatistics(time_t ttStart, time_t ttFinish)
{
	if (NULL != ARE::fpLOG) {
		char s[1024] ;
		__int64 i64 ;
		int i ;

		BucketElimination::BEworkspace *bews = dynamic_cast<BucketElimination::BEworkspace *>(this) ;

		// compute runtime
		time_t runtime = ttFinish - ttStart ;
		long d = runtime ;
		int hour = runtime/3600 ;
		d -= hour*3600 ;
		int min = d/60 ;
		d -= min*60 ;
		int sec = d ;

		time_t ttNow ;
		time(&ttNow) ;
		struct tm *pTime = localtime(&ttNow) ;
		char *sDT = asctime(pTime) ; // from manual : The string result produced by asctime contains exactly 26 characters and has the form Wed Jan 02 02:03:55 1980\n\0.
		fprintf(ARE::fpLOG, "\nBEEM Task Generator : no more FTBs to compute; runtime=%d:%d:%d DT=%s", hour, min, sec, sDT) ;

		__int64 curSpace = CurrentDiskMemorySpaceCached() ;
		__int64 maxSpace = MaximumDiskMemorySpaceCached() ;
		int nInMemory = nDiskTableBlocksInMemory() ;
		int maxInMemory = MaximumNumConcurrentDiskTableBlocksInMemory() ;
		bool hasErrors = HasErrorExplanation() ;
		fprintf(ARE::fpLOG, "\nBEEM Task Generator : stats : \n   hasErrors=%c \n   nTableBlocksLoaded=%I64d nTableBlocksSaved=%I64d \n   CurrentDiskMemorySpaceCached=%I64d MaximumDiskMemorySpaceCached=%I64d \n   CurrentNumDiskBlocksInMemory=%d maxNumConcurDiskBlocksInMemory=%d", 
			(char) (hasErrors ? 'Y' : 'N'), 
			(__int64) nTableBlocksLoaded(), 
			(__int64) nTableBlocksSaved(), 
			curSpace, maxSpace, nInMemory, maxInMemory) ;
		__int64 tw = InputTableBlocksWaitPeriodTotal() ;
		__int64 nBlocksWaited = nInputTableBlocksWaited() ;
		if (NULL != bews) {
			fprintf(ARE::fpLOG, "\n   nBuckets_initial=%d nBuckets_final=%d", (int) bews->nBuckets_initial(), (int) bews->nBuckets_final()) ;
			fprintf(ARE::fpLOG, "\n   nBucketsWithSingleChild_initial=%d nBucketsWithSingleChild_final=%d", (int) bews->nBucketsWithSingleChild_initial(), (int) bews->nBucketsWithSingleChild_final()) ;
			fprintf(ARE::fpLOG, "\n   nVarsWithoutBucket=%d nConstValueFunctions=%d", (int) bews->nVarsWithoutBucket(), (int) bews->nConstValueFunctions()) ;
			}
		fprintf(ARE::fpLOG, "\n   InputTableBlockWaiting=(%I64d times, %I64d millisec) ", nBlocksWaited, tw) ;
		double x = _InputTableGetTimeTotal/1000.0 ;
		fprintf(ARE::fpLOG, "\n   InputTableGetTimeTotal=%g sec", x) ;
		x = _FileLoadTimeTotal/1000.0 ;
		fprintf(ARE::fpLOG, "\n   FileLoadTimeTotal=%g sec", x) ;
		x = _FileSaveTimeTotal/1000.0 ;
		fprintf(ARE::fpLOG, "\n   FileSaveTimeTotal=%g sec", x) ;
		x = _FTBComputationTimeTotal/1000.0 ;
		fprintf(ARE::fpLOG, "\n   FTBComputationTimeTotal=%g sec", x) ;
		x = _FTBComputationTimeTotal/1000.0 ;
		fprintf(ARE::fpLOG, "\n   runtime=%d sec (%d:%d:%d)", (int) runtime, (int) hour, (int) min, (int) sec) ;
		x = runtime > 0 ? ((double) _FTBComputationTimeTotal + tw)/((double) 1000.0*runtime) : -1.0 ;
		fprintf(ARE::fpLOG, "\n   avg num threads busy+wait = %g", x) ;
		double art = _FTBComputationTimeTotal - _FileSaveTimeTotal - _FileLoadTimeTotal ;
		x = runtime > 0 ? ((double) art)/((double) 1000.0*runtime) : -1.0 ;
		fprintf(ARE::fpLOG, "\n   avg num threads actually computing data = %g", x) ;
/*
		for (i64 = 0 ; i64 < nBlocksWaited ; i64++) {
			int BucketIDX ; __int64 BlockIDX ;
			InputTableBlockWaitDetails(i64, BucketIDX, BlockIDX) ;
			fprintf(ARE::fpLOG, "\nFTB wait %I64d = %d %I64d", i64, BucketIDX, BlockIDX) ;
			}
*/
		if (NULL != bews) {
			fprintf(ARE::fpLOG, "\nNumber of FTBs loaded per bucket") ;
			for (i = 0 ; i < bews->nBuckets() ; i++) {
				BucketElimination::Bucket *b = bews->getBucket(i) ;
				Function & f = b->OutputFunction() ;
				__int64 nblocks = f.nTableBlocks() ;
				fprintf(ARE::fpLOG, "\n bucket %d OUT : n blocks = %I64d, n times loaded %d", (int) i, nblocks, _nFTBsLoadedPerBucket[i]) ;
				}
			}
		fflush(ARE::fpLOG) ;
		}
}

