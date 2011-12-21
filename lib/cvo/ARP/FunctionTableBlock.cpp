#include <stdlib.h>
#include <float.h>

#include "MersenneTwister.h"
#include "Globals.hxx"
#include "Problem.hxx"
#include "FunctionTableBlock.hxx"
#include "Bucket.hxx"
//#include <BEEMworkspace.hxx>


ARE::FunctionTableBlock::~FunctionTableBlock(void)
{
	DestroyData() ;

/*
	// notify inputs that they are no longer needed; if input has no users, delete it.
	if (NULL != _Inputs) {
		for (int i = 0 ; i < _nInputs ; i++) {
			FunctionTableBlock *ftb = _Inputs[i] ;
			ftb->ReferenceFunction()->ReleaseFTB(*ftb, this) ;
			}
		delete [] _Inputs ;
		}
*/

/*
	// tell users that this input is gone
	if (NULL != _Users) {
		for (int i = 0 ; i < _nUsers ; i++) {
			FunctionTableBlock *ftb = _Users[i] ;
			ftb->RemoveInput(*this) ;
			}
		delete [] _Users ;
		}
*/

	// remove from the reference function
	if (NULL != _ReferenceFunction && 0 != _IDX) 
		_ReferenceFunction->RemoveFTB(*this) ;
}


int ARE::FunctionTableBlock::Initialize(ARE::Function *ReferenceFunction, __int64 IDX)
{
	_ReferenceFunction = ReferenceFunction ;
	__int64 nBlocks = _ReferenceFunction->nTableBlocks() ;
	if (nBlocks < 0) 
		return 1 ;
	if (0 == nBlocks && 0 != IDX) 
		return 1 ;
	else if (nBlocks > 0 && IDX < 1) 
		return 1 ;
	_IDX = IDX ;
	int size = (0 == nBlocks) ? _ReferenceFunction->TableSize() : _ReferenceFunction->TableBlockSize() ;
	if (0 == _IDX) 
		_StartAdr = 0 ;
	else 
		_StartAdr = (_IDX-1)*size ;
	_EndAdr = _StartAdr + size ;
	_Filename.erase() ;
	return 0 ;
}


int ARE::FunctionTableBlock::AllocateData(void)
{
	if (NULL == _ReferenceFunction) 
		return 1 ;
	__int64 nBlocks = _ReferenceFunction->nTableBlocks() ;
	if (nBlocks < 0) 
		return 1 ;
	int size = (0 == nBlocks) ? _ReferenceFunction->TableSize() : _ReferenceFunction->TableBlockSize() ;

	if (size <= 0) 
		DestroyData() ;
	else if (_Size != size) {
		DestroyData() ;
		try {
			_Data = new ARE_Function_TableType[size] ;
			}
		catch (...) {
			}
		if (NULL == _Data) 
			return 1 ;
		_Size = size ;
		}
	return 0 ;
}


double ARE::FunctionTableBlock::SumEntireData(void)
{
	if (NULL == _Data) 
		return -1.0 ;
	if (_Size < 1) 
		return -2.0 ;

	double sum = 0.0 ;
	for (int i = 0 ; i < _Size ; i++) {
/*
#ifdef _DEBUG
		if (fabs(_Data[i]) > 1.0e+10) 
			{ int bug = 1 ; }
#endif // _DEBUG
*/
		sum += _Data[i] ;
		}
	return sum ;
}


int ARE::FunctionTableBlock::CheckData(void)
{
	if (NULL != _Data) {
		for (int i = 0 ; i < _Size ; i++) {
//			if (0 != _isnan(_Data[i]))
			if (_Data[i] != _Data[i]) // tests for NaN
				// don't check if 0 == _finite(_Data[i])), since we may have -INF when the table is converted to log
				return 1 ;
			}
		}

	return 0 ;
}


int ARE::FunctionTableBlock::ComputeFilename(void)
{
	if (0 == _Filename.length()) {
		if (NULL == _ReferenceFunction) 
			return 1 ;
		Workspace *ws = _ReferenceFunction->WS() ;
		if (NULL == ws) 
			return 1 ;
		const std::string & dir = ws->DiskSpaceDirectory() ;
		if (0 == dir.length()) 
			return 1 ;
		BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
		if (NULL == b) 
			return 1 ;
		_Filename = dir ;
		if ('\\' != _Filename[_Filename.length()-1]) 
			_Filename += '\\' ;

		// format : <bucket idx>-<tablesize>-<total num blocks>-<block idx>-<start adr>-<size>

		char s[256] ;
		sprintf(s, "%d", (int) b->IDX()) ;
		_Filename += s ;

		_Filename += '-' ;
		sprintf(s, "%I64d", (__int64) _ReferenceFunction->TableSize()) ;
		_Filename += s ;

		_Filename += '-' ;
		sprintf(s, "%I64d", (__int64) _ReferenceFunction->nTableBlocks()) ;
		_Filename += s ;

		_Filename += '-' ;
		sprintf(s, "%I64d", (__int64) _IDX) ;
		_Filename += s ;

		_Filename += '-' ;
		sprintf(s, "%I64d", (__int64) _StartAdr) ;
		_Filename += s ;

		_Filename += '-' ;
		sprintf(s, "%I64d", (__int64) _Size) ;
		_Filename += s ;
		}
	return 0 ;
}


int ARE::FunctionTableBlock::SaveData(void)
{
	if (NULL == _ReferenceFunction) 
		return 1 ;
	if (0 == _ReferenceFunction->nTableBlocks()) 
		// FT is in-memory; skip the save
		return 0 ;
	Workspace *ws = _ReferenceFunction->WS() ;
	if (NULL == _Data) 
		return 1 ;
	if (0 == _Filename.length()) 
		ComputeFilename() ;
	FILE *fp = fopen(_Filename.c_str(), "wb") ;
	if (NULL == fp) 
		return 1 ;
	DWORD tcS = GETCLOCK() ;
	size_t res = fwrite(_Data, sizeof(ARE_Function_TableType), _Size, fp) ;
	DWORD tElapsed = GETCLOCK() - tcS ;
	if (NULL != ws) 
		ws->NoteFileSaveTime(tElapsed) ;
	if (res != _Size) {
		// TODO : do something more
		int error = ferror(fp) ;
		return 1 ;
		}
	fclose(fp) ;
	if (NULL != ws) 
		ws->IncrementnTableBlocksSaved() ;
	return 0 ;
}


int ARE::FunctionTableBlock::LoadData(void)
{
	if (NULL == _Data) 
		return 1 ;
	if (0 == _Filename.length()) 
		ComputeFilename() ;
	FILE *fp = fopen(_Filename.c_str(), "rb") ;
	if (NULL == fp) 
		return 1 ;
	int n = fread(_Data, sizeof(ARE_Function_TableType), _Size, fp) ;
	fclose(fp) ;
	if (n != _Size) 
		return 1 ;
	if (NULL != _ReferenceFunction) {
		Workspace *ws = _ReferenceFunction->WS() ;
		if (NULL != ws) {
			ws->IncrementnTableBlocksLoaded(NULL != _ReferenceFunction->OriginatingBucket() ? _ReferenceFunction->OriginatingBucket()->IDX() : -1) ;
			}
		}
	return 0 ;
}


int ARE::FunctionTableBlock::CheckBEEMInputTablesExist(Function * & FunctionMissing, __int64 & BlockIDX)
{
	FunctionMissing = NULL ;
	BlockIDX = -1 ;

	if (NULL == _ReferenceFunction) 
		return -1 ;
	const int *refFNarguments = _ReferenceFunction->Arguments() ;
	int nA = _ReferenceFunction->N() ;
	if (nA < 0) 
		// this should not happen
		return -1 ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	if (NULL == b) 
		return -1 ;
	int nTotalFunctions = b->nOriginalFunctions() + b->nChildBucketFunctions() ;
	if (nTotalFunctions < 1) 
		return 0 ;
	ARP *problem = _ReferenceFunction->Problem() ;
	if (NULL == problem) 
		return -1 ;

	int i, j ;

	if (0 == nA) {
		// this means the function is a constant (i.e. has no arguments); check inputs exist in entirety.
		for (j = 0 ; j < b->nChildBucketFunctions() ; j++) {
			Function *f = b->ChildBucketFunction(j) ;
			if (NULL == f) continue ;
			BucketElimination::Bucket *ob = f->OriginatingBucket() ;
			if (NULL == ob) 
				// error
				return -1 ;
			if (0 == f->nTableBlocks() ? ob->nOutputFunctionBlocksComputed() < 1 : false) {
				// this means entire table is in memory
				BlockIDX = 0 ;
				FunctionMissing = f ;
				return 1 ;
				}
			if (ob->nOutputFunctionBlocksComputed() < f->nTableBlocks()) {
				BlockIDX = 0 ;
				FunctionMissing = f ;
				return 1 ;
				}
			}
		return 0 ;
		}

	// check a few entries in this table block, e.g. first/last
	__int64 entries2check[2] ;
	int Nentries2check = 2 ;
	entries2check[0] = _StartAdr ;
	entries2check[1] = _EndAdr - 1 ;
	int values[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the current value combination of the arguments of this function (table block).
	int vars[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the list of variables : vars2Keep + vars2Eliminate
	int nAtotal = nA + b->nVars() ;
	for (i = 0 ; i < nA ; i++) 
		vars[i] = refFNarguments[i] ;
	for (; i < nAtotal ; i++) {
		vars[i] = b->Var(i - nA) ;
		values[i] = 0 ;
		}
	for (i = 0 ; i < Nentries2check ; i++) {
		ComputeArgCombinationFromFnTableAdr(entries2check[i], nA, vars, values, problem->K()) ;
/* 2009-10-29 KK : assume original functions are always available
		for (j = 0 ; j < b->nOriginalFunctions() ; j++) {
			Function *f = b->OriginalFunction(j) ;
			if (NULL == f) continue ;
			if (0 == f->nTableBlocks()) 
				// this means entire table is in memory
				continue ;
			__int64 adr = f->ComputeFnTableAdrExN(nAtotal, vars, values, problem->K()) ;
			if (! f->IsTableBlockForAdrAvailable(adr, BlockIDX)) {
				FunctionMissing = f ;
				return 1 ;
				}
			}
*/
		for (j = 0 ; j < b->nChildBucketFunctions() ; j++) {
			Function *f = b->ChildBucketFunction(j) ;
			if (NULL == f) continue ;
			__int64 adr = f->ComputeFnTableAdrExN(nAtotal, vars, values, problem->K()) ;
			if (! f->IsTableBlockForAdrAvailable(adr, BlockIDX)) {
				FunctionMissing = f ;
				return 1 ;
				}
			}
		}

	return 0 ;
}


int ARE::FunctionTableBlock::ComputeDataBEEM_ProdSum_singleV(Function * & MissingFunction, __int64 & MissingBlockIDX)
{
	MissingFunction = NULL ;
	MissingBlockIDX = -1 ;

	int ret = 1 ;

	// for now (2009-07-14), assume that this function table block is in a function computed by BE.
	// this means we will be enumerating over all _ReferenceFunction argument combinations, 
	// for each argument combination eliminating all 
	if (NULL == _ReferenceFunction) 
		return 1 ;
	if (_IDX < 0) 
		return 1 ;
	int nA = _ReferenceFunction->N() ;
	if (nA < 0) {
		// this should not happen
		return 0 ;
		}
	if (nA > MAX_NUM_VARIABLES_PER_BUCKET) 
		return 3 ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	if (NULL == b) 
		return 1 ;
	Workspace *ws = _ReferenceFunction->WS() ;
	if (NULL == ws) 
		return 1 ;
	if (1 != b->nVars()) 
		return 1 ;
	int bVar = b->Var(0) ;
	if (bVar < 0) 
		return 1 ;
	int nTotalFunctions = b->nOriginalFunctions() + b->nChildBucketFunctions() ;
	if (nTotalFunctions < 1) {
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 1.0 ;
// DEBUGGGG
//if (b->IDX() >= 283) {
//	fprintf(ARE::fpLOG, "\nFTB b=%d const=%g", (int) b->IDX(), V) ;
//	}
		return 0 ;
		}
	if (nTotalFunctions > MAX_NUM_FUNCTIONS_PER_BUCKET) 
		return 3 ;
	ARP *problem = _ReferenceFunction->Problem() ;
	if (NULL == problem) 
		return 1 ;
	// get the domain size of bVar; we will eliminate bVar; for that we will need to enumerate over its domain.
	// this assumes that all values in the domain are allowed.
	// if some values are not allowed, or bVar is evidence variable (i.e. its has 1 fixed values), it requires special processing.
	int bVarK = problem->K(bVar) ;

	int j, k, n ;

	if (0 == nA) {
		// this means the function is a constant (i.e. has no arguments); assume there is 1 variable in the bucket, since there must be at least 1 function in the bucket at this point.
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 0.0 ;
		for (j = 0 ; j < bVarK ; j++) {
			ARE_Function_TableType value = 1.0 ;
			for (k = 0 ; k < nTotalFunctions ; k++) {
				Function *f = (k < b->nOriginalFunctions()) ? b->OriginalFunction(k) : b->ChildBucketFunction(k - b->nOriginalFunctions()) ;
				if (0 == f->N()) 
					value *= f->ConstValue() ;
				else if (1 == f->N()) 
					value *= ((f->Table())->Data())[j] ;
				else 
					return 1 ;
				}
			V += value ;
			}
		b->MarkOutputFunctionBlockComputed(_IDX) ;
#ifdef _DEBUG
		if (NULL != ARE::fpLOG) {
			ARE::utils::AutoLock lock(ARE::LOGMutex) ;
			char s[256] ;
			sprintf(s, "\n   worker computed b=%d ftb=%I64d constvalue=%g", (int) b->IDX(), (__int64) _IDX, (double) V) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			fflush(ARE::fpLOG) ;
			}
#endif // _DEBUG
// DEBUGGGG
//if (b->IDX() >= 283) {
//	fprintf(ARE::fpLOG, "\nFTB b=%d const=%g", (int) b->IDX(), V) ;
//	}
		return 0 ;
		}

	// make sure array for data is allocated
	AllocateData() ;
	if (NULL == _Data) 
		return 1 ;

	int args[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the current value combination of the arguments of this function (table block).
	__int64 adrlist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is the adr wrt input function table, corresponding to the output function argument value combination.
	Function *flist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input functions
	FunctionTableBlock *ftblist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input function table blocks, regardless of whether they are in-memory or external(disk)-memory.
	
	{

	// generate initial argument value combination, corresponding to the start-adr of this function table block.
	// fill in function list; check ptr's so that we don't have to check for NULL later in the main loop; etc.
	ComputeArgCombinationFromFnTableAdr(_StartAdr, nA, _ReferenceFunction->Arguments(), args, problem->K()) ;
	for (j = n = 0 ; j < nTotalFunctions ; j++) {
		flist[n] = (j < b->nOriginalFunctions()) ? b->OriginalFunction(j) : b->ChildBucketFunction(j - b->nOriginalFunctions()) ;
		if (NULL == flist[n]) 
			continue ;
		__int64 adr = flist[n]->ComputeFnTableAdrEx(nA, _ReferenceFunction->Arguments(), args, problem->K()) ;
		k = flist[n]->GetFTB(adr, this, ftblist[n], MissingBlockIDX) ;
		if (4 == k) 
			// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
			{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = flist[n] ; goto done ; }
		if (NULL == ftblist[n]) 
			// generic error
			goto done ;
		++n ;
		}
	nTotalFunctions = n ;
	if (nTotalFunctions < 1) {
// DEBUGGGG
//if (b->IDX() >= 283) {
//	fprintf(ARE::fpLOG, "\nFTB b=%d ---", (int) b->IDX()) ;
//	}
		return 0 ;
		}

	// compute each table entry.
	// note we will not keep track of inputs; we should do this for disk-function-tables, but we maintain an input list locally, 
	// and when done, release all the inputs using our local bookkeeping.
	__int64 i ;
	for (i = 0 ; i < _Size ; i++) {
		// make sure we have the right block for each input function
		for (j = 0 ; j < nTotalFunctions ; j++) {
			Function *f = flist[j] ;
			__int64 adr = f->ComputeFnTableAdrEx(nA, _ReferenceFunction->Arguments(), args, problem->K()) ;
			// check if the block we already have is good; in not, fetch a new block.
			if (ftblist[j]->StartAdr() > adr || ftblist[j]->EndAdr() <= adr) {
				// release current block; if this is an external-memory block and has no other users, destroy it.
				f->ReleaseFTB(*ftblist[j], this) ;
				ftblist[j] = NULL ;
				// get new block; if it is an external-memory block, load it.
				// this will mark us (this fn) as the user of the block, so that it won't be automatically unloaded by anybody.
				k = f->GetFTB(adr, this, ftblist[j], MissingBlockIDX) ;
				if (4 == k) 
					// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
					{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
				if (NULL == ftblist[j]) 
					// generic error
					goto done ;
				}
			adrlist[j] = adr - (ftblist[j]->StartAdr()) ;
			}

		// Enumerate over all bVar values. Note that given a fixed argument value combination of this function (table block), 
		// (which fixes values of all variables in all input functions, with the axception of bVar), all bVar values 
		// are always within the same function table block (by construction).
		// note that ComputeFnTableAdrEx() returns adr corresponding to bVar=0.
		_Data[i] = 0.0 ;
		for (j = 0 ; j < bVarK ; j++) {
			ARE_Function_TableType value = 1.0 ;
			for (k = 0 ; k < nTotalFunctions ; k++) {
				value *= ftblist[k]->Entry(adrlist[k]+j) ;
				}
			_Data[i] += value ;
			}

		// go to next argument value combination
		EnumerateNextArgumentsValueCombination(_ReferenceFunction->N(), _ReferenceFunction->Arguments(), args, problem->K()) ;
		}

// DEBUGGGG
//if (b->IDX() >= 283) {
//	fprintf(ARE::fpLOG, "\nFTB b=%d out =", (int) b->IDX()) ;
//	for (j = 0 ; j < _Size ; j++) fprintf(ARE::fpLOG, "%g", (double) _Data[j]) ;
//	}

	// save in a file
	SaveData() ;

	// if this is an exernal-memory block, we can destroy the table, since its contents are saved on the disk.
	// however, we leave this to the calling fn.

	ret = 0 ;

	}

done :

	// release function blocks
	for (j = 0 ; j < nTotalFunctions ; j++) {
		if (NULL != ftblist[j]) 
			ftblist[j]->ReferenceFunction()->ReleaseFTB(*ftblist[j], this) ;
		}

	// mark this block as computed; do this after the table block is saved.
	if (0 == ret) {
#ifdef _DEBUG
		if (NULL != ARE::fpLOG) {
			ARE::utils::AutoLock lock(ARE::LOGMutex) ;
			double x = SumEntireData() ;
			char s[256] ;
			int wIDX = -1 ;
			sprintf(s, "\n   worker idx=%d computed b=%d ftb=%I64d sum=%g size=%I64d", wIDX, (int) b->IDX(), (__int64) _IDX, (double) x, (__int64) _Size) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			fflush(ARE::fpLOG) ;
			}
#endif // _DEBUG
		b->MarkOutputFunctionBlockComputed(_IDX) ;
		}

	return ret ;
}


int ARE::FunctionTableBlock::ComputeDataBEEM_ProdMax_singleV(Function * & MissingFunction, __int64 & MissingBlockIDX)
{
	MissingFunction = NULL ;
	MissingBlockIDX = -1 ;

	int ret = 1 ;

	// for now (2009-07-14), assume that this function table block is in a function computed by BE.
	// this means we will be enumerating over all _ReferenceFunction argument combinations, 
	// for each argument combination eliminating all 
	if (NULL == _ReferenceFunction) 
		return 1 ;
	if (_IDX < 0) 
		return 1 ;
	int nA = _ReferenceFunction->N() ;
	if (nA < 0) {
		// this should not happen
		return 0 ;
		}
	if (nA > MAX_NUM_VARIABLES_PER_BUCKET) 
		return 3 ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	if (NULL == b) 
		return 1 ;
	Workspace *ws = _ReferenceFunction->WS() ;
	if (NULL == ws) 
		return 1 ;
	if (1 != b->nVars()) 
		return 1 ;
	int bVar = b->Var(0) ;
	if (bVar < 0) 
		return 1 ;
	int nTotalFunctions = b->nOriginalFunctions() + b->nChildBucketFunctions() ;
	if (nTotalFunctions < 1) {
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 1.0 ;
		return 0 ;
		}
	if (nTotalFunctions > MAX_NUM_FUNCTIONS_PER_BUCKET) 
		return 3 ;
	ARP *problem = _ReferenceFunction->Problem() ;
	if (NULL == problem) 
		return 1 ;
	// get the domain size of bVar; we will eliminate bVar; for that we will need to enumerate over its domain.
	// this assumes that all values in the domain are allowed.
	// if some values are not allowed, or bVar is evidence variable (i.e. its has 1 fixed values), it requires special processing.
	int bVarK = problem->K(bVar) ;

	int j, k, n ;

	if (0 == nA) {
		// this means the function is a constant (i.e. has no arguments); assume there is 1 variable in the bucket, since there must be at least 1 function in the bucket at this point.
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 0.0 ;
		for (j = 0 ; j < bVarK ; j++) {
			ARE_Function_TableType value = 1.0 ;
			for (k = 0 ; k < nTotalFunctions ; k++) {
				Function *f = (k < b->nOriginalFunctions()) ? b->OriginalFunction(k) : b->ChildBucketFunction(k - b->nOriginalFunctions()) ;
				if (0 == f->N()) 
					value *= f->ConstValue() ;
				else if (1 == f->N()) 
					value *= ((f->Table())->Data())[j] ;
				else 
					return 1 ;
				}
			if (value > V) V = value ;
			}
		b->MarkOutputFunctionBlockComputed(_IDX) ;
#ifdef _DEBUG
		if (NULL != ARE::fpLOG) {
			ARE::utils::AutoLock lock(ARE::LOGMutex) ;
			char s[256] ;
			sprintf(s, "\n   worker computed b=%d ftb=%I64d constvalue=%g", (int) b->IDX(), (__int64) _IDX, (double) V) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			fflush(ARE::fpLOG) ;
			}
#endif // _DEBUG
		return 0 ;
		}

	// make sure array for data is allocated
	AllocateData() ;
	if (NULL == _Data) 
		return 1 ;

	int args[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the current value combination of the arguments of this function (table block).
	__int64 adrlist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is the adr wrt input function table, corresponding to the output function argument value combination.
	Function *flist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input functions
	FunctionTableBlock *ftblist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input function table blocks, regardless of whether they are in-memory or external(disk)-memory.

	{

	// generate initial argument value combination, corresponding to the start-adr of this function table block.
	// fill in function list; check ptr's so that we don't have to check for NULL later in the main loop; etc.
	ComputeArgCombinationFromFnTableAdr(_StartAdr, nA, _ReferenceFunction->Arguments(), args, problem->K()) ;
	for (j = n = 0 ; j < nTotalFunctions ; j++) {
		flist[n] = (j < b->nOriginalFunctions()) ? b->OriginalFunction(j) : b->ChildBucketFunction(j - b->nOriginalFunctions()) ;
		if (NULL == flist[n]) 
			continue ;
		__int64 adr = flist[n]->ComputeFnTableAdrEx(nA, _ReferenceFunction->Arguments(), args, problem->K()) ;
		k = flist[n]->GetFTB(adr, this, ftblist[n], MissingBlockIDX) ;
		if (4 == k) 
			// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
			{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = flist[n] ; goto done ; }
		if (NULL == ftblist[n]) 
			// generic error
			goto done ;
		++n ;
		}
	nTotalFunctions = n ;
	if (nTotalFunctions < 1) 
		return 0 ;

	// compute each table entry.
	// note we will not keep track of inputs; we should do this for disk-function-tables, but we maintain an input list locally, 
	// and when done, release all the inputs using our local bookkeeping.
	__int64 i ;
	for (i = 0 ; i < _Size ; i++) {
		// make sure we have the right block for each input function
		for (j = 0 ; j < nTotalFunctions ; j++) {
			Function *f = flist[j] ;
			__int64 adr = f->ComputeFnTableAdrEx(nA, _ReferenceFunction->Arguments(), args, problem->K()) ;
			// check if the block we already have is good; in not, fetch a new block.
			if (ftblist[j]->StartAdr() > adr || ftblist[j]->EndAdr() <= adr) {
				// release current block; if this is an external-memory block and has no other users, destroy it.
				f->ReleaseFTB(*ftblist[j], this) ;
				ftblist[j] = NULL ;
				// get new block; if it is an external-memory block, load it.
				// this will mark us (this fn) as the user of the block, so that it won't be automatically unloaded by anybody.
				k = f->GetFTB(adr, this, ftblist[j], MissingBlockIDX) ;
				if (4 == k) 
					// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
					{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
				if (NULL == ftblist[j]) 
					// generic error
					goto done ;
				}
			adrlist[j] = adr - (ftblist[j]->StartAdr()) ;
			}

		// Enumerate over all bVar values. Note that given a fixed argument value combination of this function (table block), 
		// (which fixes values of all variables in all input functions, with the axception of bVar), all bVar values 
		// are always within the same function table block (by construction).
		// note that ComputeFnTableAdrEx() returns adr corresponding to bVar=0.
		_Data[i] = 0.0 ;
		for (j = 0 ; j < bVarK ; j++) {
			ARE_Function_TableType value = 1.0 ;
			for (k = 0 ; k < nTotalFunctions ; k++) {
				value *= ftblist[k]->Entry(adrlist[k]+j) ;
				}
			if (value > _Data[i]) _Data[i] = value ;
			}

		// go to next argument value combination
		EnumerateNextArgumentsValueCombination(_ReferenceFunction->N(), _ReferenceFunction->Arguments(), args, problem->K()) ;
		}

	// save in a file
	SaveData() ;

	// if this is an exernal-memory block, we can destroy the table, since its contents are saved on the disk.
	// however, we leave this to the calling fn.

	ret = 0 ;

	}

done :

	// release function blocks
	for (j = 0 ; j < nTotalFunctions ; j++) {
		if (NULL != ftblist[j]) 
			ftblist[j]->ReferenceFunction()->ReleaseFTB(*ftblist[j], this) ;
		}

	// mark this block as computed; do this after the table block is saved.
	if (0 == ret) {
#ifdef _DEBUG
		if (NULL != ARE::fpLOG) {
			ARE::utils::AutoLock lock(ARE::LOGMutex) ;
			double x = SumEntireData() ;
			char s[256] ;
			int wIDX = -1 ;
			sprintf(s, "\n   worker idx=%d computed b=%d ftb=%I64d sum=%g size=%I64d", wIDX, (int) b->IDX(), (__int64) _IDX, (double) x, (__int64) _Size) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			fflush(ARE::fpLOG) ;
			}
#endif // _DEBUG
		b->MarkOutputFunctionBlockComputed(_IDX) ;
		}

	return ret ;
}


int ARE::FunctionTableBlock::ComputeDataBEEM_SumMax_singleV(Function * & MissingFunction, __int64 & MissingBlockIDX)
{
	MissingFunction = NULL ;
	MissingBlockIDX = -1 ;

	int ret = 1 ;

	// for now (2009-07-14), assume that this function table block is in a function computed by BE.
	// this means we will be enumerating over all _ReferenceFunction argument combinations, 
	// for each argument combination eliminating all 
	if (NULL == _ReferenceFunction) 
		return 1 ;
	if (_IDX < 0) 
		return 1 ;
	int nA = _ReferenceFunction->N() ;
	if (nA < 0) {
		// this should not happen
		return 0 ;
		}
	if (nA > MAX_NUM_VARIABLES_PER_BUCKET) 
		return 3 ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	if (NULL == b) 
		return 1 ;
	Workspace *ws = _ReferenceFunction->WS() ;
	if (NULL == ws) 
		return 1 ;
	if (1 != b->nVars()) 
		return 1 ;
	int bVar = b->Var(0) ;
	if (bVar < 0) 
		return 1 ;
	int nTotalFunctions = b->nOriginalFunctions() + b->nChildBucketFunctions() ;
	if (nTotalFunctions < 1) {
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 1.0 ;
		return 0 ;
		}
	if (nTotalFunctions > MAX_NUM_FUNCTIONS_PER_BUCKET) 
		return 3 ;
	ARP *problem = _ReferenceFunction->Problem() ;
	if (NULL == problem) 
		return 1 ;
	// get the domain size of bVar; we will eliminate bVar; for that we will need to enumerate over its domain.
	// this assumes that all values in the domain are allowed.
	// if some values are not allowed, or bVar is evidence variable (i.e. its has 1 fixed values), it requires special processing.
	int bVarK = problem->K(bVar) ;

	int j, k, n ;

	if (0 == nA) {
		// this means the function is a constant (i.e. has no arguments); assume there is 1 variable in the bucket, since there must be at least 1 function in the bucket at this point.
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = ARE::neg_infinity ;
		for (j = 0 ; j < bVarK ; j++) {
			ARE_Function_TableType value = 0.0 ;
			for (k = 0 ; k < nTotalFunctions ; k++) {
				Function *f = (k < b->nOriginalFunctions()) ? b->OriginalFunction(k) : b->ChildBucketFunction(k - b->nOriginalFunctions()) ;
				if (0 == f->N()) 
					value += f->ConstValue() ;
				else if (1 == f->N()) 
					value += ((f->Table())->Data())[j] ;
				else 
					return 1 ;
				}
			if (value > V) V = value ;
			}
		b->MarkOutputFunctionBlockComputed(_IDX) ;
#ifdef _DEBUG
		if (NULL != ARE::fpLOG) {
			ARE::utils::AutoLock lock(ARE::LOGMutex) ;
			char s[256] ;
			sprintf(s, "\n   worker computed b=%d ftb=%I64d constvalue=%g", (int) b->IDX(), (__int64) _IDX, (double) V) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			fflush(ARE::fpLOG) ;
			}
#endif // _DEBUG
		return 0 ;
		}

	// make sure array for data is allocated
	AllocateData() ;
	if (NULL == _Data) 
		return 1 ;

	int args[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the current value combination of the arguments of this function (table block).
	__int64 adrlist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is the adr wrt input function table, corresponding to the output function argument value combination.
	Function *flist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input functions
	FunctionTableBlock *ftblist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input function table blocks, regardless of whether they are in-memory or external(disk)-memory.

	{

	// generate initial argument value combination, corresponding to the start-adr of this function table block.
	// fill in function list; check ptr's so that we don't have to check for NULL later in the main loop; etc.
	ComputeArgCombinationFromFnTableAdr(_StartAdr, nA, _ReferenceFunction->Arguments(), args, problem->K()) ;
	for (j = n = 0 ; j < nTotalFunctions ; j++) {
		flist[n] = (j < b->nOriginalFunctions()) ? b->OriginalFunction(j) : b->ChildBucketFunction(j - b->nOriginalFunctions()) ;
		if (NULL == flist[n]) 
			continue ;
		__int64 adr = flist[n]->ComputeFnTableAdrEx(nA, _ReferenceFunction->Arguments(), args, problem->K()) ;
		k = flist[n]->GetFTB(adr, this, ftblist[n], MissingBlockIDX) ;
		if (4 == k) 
			// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
			{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = flist[n] ; goto done ; }
		if (NULL == ftblist[n]) 
			// generic error
			goto done ;
		++n ;
		}
	nTotalFunctions = n ;
	if (nTotalFunctions < 1) 
		return 0 ;

	// compute each table entry.
	// note we will not keep track of inputs; we should do this for disk-function-tables, but we maintain an input list locally, 
	// and when done, release all the inputs using our local bookkeeping.
	__int64 i ;
	for (i = 0 ; i < _Size ; i++) {
		// make sure we have the right block for each input function
		for (j = 0 ; j < nTotalFunctions ; j++) {
			Function *f = flist[j] ;
			__int64 adr = f->ComputeFnTableAdrEx(nA, _ReferenceFunction->Arguments(), args, problem->K()) ;
			// check if the block we already have is good; in not, fetch a new block.
			if (ftblist[j]->StartAdr() > adr || ftblist[j]->EndAdr() <= adr) {
				// release current block; if this is an external-memory block and has no other users, destroy it.
				f->ReleaseFTB(*ftblist[j], this) ;
				ftblist[j] = NULL ;
				// get new block; if it is an external-memory block, load it.
				// this will mark us (this fn) as the user of the block, so that it won't be automatically unloaded by anybody.
				k = f->GetFTB(adr, this, ftblist[j], MissingBlockIDX) ;
				if (4 == k) 
					// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
					{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
				if (NULL == ftblist[j]) 
					// generic error
					goto done ;
				}
			adrlist[j] = adr - (ftblist[j]->StartAdr()) ;
			}

		// Enumerate over all bVar values. Note that given a fixed argument value combination of this function (table block), 
		// (which fixes values of all variables in all input functions, with the axception of bVar), all bVar values 
		// are always within the same function table block (by construction).
		// note that ComputeFnTableAdrEx() returns adr corresponding to bVar=0.
		_Data[i] = ARE::neg_infinity ;
		for (j = 0 ; j < bVarK ; j++) {
			ARE_Function_TableType value = 0.0 ;
			for (k = 0 ; k < nTotalFunctions ; k++) {
				value += ftblist[k]->Entry(adrlist[k]+j) ;
				}
			if (value > _Data[i]) _Data[i] = value ;
			}
		if (_Data[i] > 0.0 && NULL != ARE::fpLOG) {
			fprintf(ARE::fpLOG, "\nFTB::SumMax(): ERROR data[%d]=%g blocksize=%d (should be <= 0) bVarK=%d bIDX=%d nTotalFunctions=%d", i, (double) _Data[i], (int) Size(), (int) bVarK, (int) b->IDX(), (int) nTotalFunctions) ;
			for (k = 0 ; k < nTotalFunctions ; k++) {
				fprintf(ARE::fpLOG, "\n     ftb blocksize=%d fIDX=%d", (int) ftblist[k]->Size(), (int) ftblist[k]->ReferenceFunction()->IDX()) ;
				}
			_Data[i] = ARE::neg_infinity ;
			fprintf(ARE::fpLOG, "\n     _Data[i]=%f _FPCLASS_NINF=%g log10(0.0)=%g", (double) _Data[i], (double) _FPCLASS_NINF, (double) neg_infinity) ;
			for (j = 0 ; j < bVarK ; j++) {
				ARE_Function_TableType value = 0.0 ;
				fprintf(ARE::fpLOG, "\n     bVar value = %d value=%g _Data[i]=%g", (int) j, (double) value, (double) _Data[i]) ;
				for (k = 0 ; k < nTotalFunctions ; k++) {
					value += ftblist[k]->Entry(adrlist[k]+j) ;
					fprintf(ARE::fpLOG, "\n       %g value=%g adr=%d", (double) ftblist[k]->Entry(adrlist[k]+j), (double) value, (int) adrlist[k]+j) ;
					}
				if (value > _Data[i]) _Data[i] = value ;
				fprintf(ARE::fpLOG, "\n     bVar value = %d _Data[i]=%g", (int) j, (double) _Data[i]) ;
				}
			fprintf(ARE::fpLOG, "\n") ;
			fflush(ARE::fpLOG) ;
			_exit(1) ;
			}

		// go to next argument value combination
		EnumerateNextArgumentsValueCombination(_ReferenceFunction->N(), _ReferenceFunction->Arguments(), args, problem->K()) ;
		}

	// save in a file
	SaveData() ;

	// if this is an exernal-memory block, we can destroy the table, since its contents are saved on the disk.
	// however, we leave this to the calling fn.

	ret = 0 ;

	}

done :

	// release function blocks
	for (j = 0 ; j < nTotalFunctions ; j++) {
		if (NULL != ftblist[j]) 
			ftblist[j]->ReferenceFunction()->ReleaseFTB(*ftblist[j], this) ;
		}

	// mark this block as computed; do this after the table block is saved.
	if (0 == ret) {
#ifdef _DEBUG
		if (NULL != ARE::fpLOG) {
			ARE::utils::AutoLock lock(ARE::LOGMutex) ;
			double x = SumEntireData() ;
			char s[256] ;
			int wIDX = -1 ;
			sprintf(s, "\n   worker idx=%d computed b=%d ftb=%I64d sum=%g size=%I64d", wIDX, (int) b->IDX(), (__int64) _IDX, (double) x, (__int64) _Size) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			fflush(ARE::fpLOG) ;
			}
#endif // _DEBUG
		b->MarkOutputFunctionBlockComputed(_IDX) ;
		}

	return ret ;
}


int ARE::FunctionTableBlock::ComputeDataBEEM_ProdSum(Function * & MissingFunction, __int64 & MissingBlockIDX)
{
	// DEBUGGGGG
//	if (NULL != ARE::fpLOG) {
//		fprintf(ARE::fpLOG, "\nFTB::ComputeDataBEEM_ProdSum()") ;
//		fflush(ARE::fpLOG) ;
//		}
	MissingFunction = NULL ;
	MissingBlockIDX = -1 ;

/*
// DEBUGGGGG
if (NULL != ARE::fpLOG) {
	ARE::utils::AutoLock lock(ARE::LOGMutex) ;
	char s[256] ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	sprintf(s, "\n   worker idx=%d going to compute bucket %d ftb %I64d", wIDX, (int) b->IDX(), (__int64) (this->_IDX)) ;
	fwrite(s, 1, strlen(s), ARE::fpLOG) ;
	fflush(ARE::fpLOG) ;
	}
*/
	int ret = 1, j, k ;

	// ***************************************************************************************************
	// for now (2009-07-14), assume that this function table block is in a function computed by BE.
	// this means we will be enumerating over all _ReferenceFunction argument combinations, 
	// for each argument combination eliminating all variables of the bucket.
	// ***************************************************************************************************

	// check/detect simple data structure errors
	if (NULL == _ReferenceFunction) 
		return ERRORCODE_generic ;
	const int *refFNarguments = _ReferenceFunction->Arguments() ;
	if (_IDX < 0) 
		return ERRORCODE_generic ;
	ARP *problem = _ReferenceFunction->Problem() ;
	if (NULL == problem) 
		return ERRORCODE_generic ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	if (NULL == b) 
		return ERRORCODE_generic ;
	Workspace *ws = _ReferenceFunction->WS() ;
	if (NULL == ws) 
		return ERRORCODE_generic ;
	if (b->nVars() < 1) 
		return ERRORCODE_generic ;
	int nA = _ReferenceFunction->N() ;
	if (nA < 0) {
		// this should not happen
		return 0 ;
		}
	if (1 == b->nVars()) 
		return ComputeDataBEEM_ProdSum_singleV(MissingFunction, MissingBlockIDX) ;

	// DEBUGGGGG
	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\nFTB::ComputeDataBEEM_ProdSum() general ...") ;
		fflush(ARE::fpLOG) ;
		}

	// ***************************************************************************************************
	// handle special case when there are no functions in the bucket
	// ***************************************************************************************************

	int nTotalFunctions = b->nOriginalFunctions() + b->nChildBucketFunctions() ;
	if (nTotalFunctions < 1) {
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 1.0 ;
		return 0 ;
		}

	// ***************************************************************************************************
	// check if there are too many variables/functions
	// ***************************************************************************************************

	if (nA > MAX_NUM_VARIABLES_PER_BUCKET) 
		return ERRORCODE_too_many_variables ;
	if (nTotalFunctions > MAX_NUM_FUNCTIONS_PER_BUCKET) 
		return ERRORCODE_too_many_functions ;

	// ***************************************************************************************************
	// handle the special case when all variables are eliminated; e.g. there is no output fn, just a const value.
	// ***************************************************************************************************

	int values[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the current value combination of the arguments of this function (table block).
	Function *flist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input functions
	FunctionTableBlock *ftblist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input function table blocks, regardless of whether they are in-memory or external(disk)-memory.

	{

	for (j = 0 ; j < nTotalFunctions ; j++) {
		flist[j] = (j < b->nOriginalFunctions()) ? b->OriginalFunction(j) : b->ChildBucketFunction(j - b->nOriginalFunctions()) ;
		if (NULL == flist[j]) 
			return ERRORCODE_fn_ptr_is_NULL ;
		}

	__int64 ElimIDX, ElimSize = 1 ;
	for (j = 0 ; j < b->nVars() ; j++) 
		ElimSize *= problem->K(b->Var(j)) ;

	if (0 == nA) {
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 0.0 ;
		for (j = 0 ; j < b->nVars() ; j++) 
			values[j] = 0 ;
		for (ElimIDX = 0 ; ElimIDX < ElimSize ; ElimIDX++) {
			ARE_Function_TableType value = 1.0 ;
			for (j = 0 ; j < nTotalFunctions ; j++) {
				Function *f = flist[j] ;
				if (0 == f->N()) 
					value *= f->ConstValue() ;
				else {
					FunctionTableBlock * & ftb = ftblist[j] ;
					__int64 adr = f->ComputeFnTableAdrExN(b->nVars(), b->VarsArray(), values, problem->K()) ;
					k = f->GetFTB(adr, this, ftb, MissingBlockIDX) ;
					if (4 == k) 
						// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
						{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
					if (NULL == ftb) 
						// generic error
						{ ret = ERRORCODE_input_FTB_computed_but_failed_to_fetch ; goto done ; }
					value *= ftb->Entry(adr - ftb->StartAdr()) ;
					}
				}
			V += value ;
			// go to next argument value combination
			EnumerateNextArgumentsValueCombination(b->nVars(), b->VarsArray(), values, problem->K()) ;
			}
#ifdef _DEBUG
		if (NULL != ARE::fpLOG) {
			ARE::utils::AutoLock lock(ARE::LOGMutex) ;
			char s[256] ;
			sprintf(s, "\n   worker computed b=%d ftb=%I64d constvalue=%g", (int) b->IDX(), (__int64) _IDX, (double) V) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			fflush(ARE::fpLOG) ;
			}
#endif // _DEBUG
		ret = 0 ;
		goto done ;
		}

	// ***************************************************************************************************
	// handle general case when there are variables to eliminate and variables to keep
	// ***************************************************************************************************

	// make sure array for data is allocated
	AllocateData() ;
	if (NULL == _Data) 
		return 1 ;

	int vars[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the list of variables : vars2Keep + vars2Eliminate
	int nAtotal = nA + b->nVars() ;
	for (j = 0 ; j < nA ; j++) {
		vars[j] = refFNarguments[j] ;
		values[j] = 0 ;
		}
	for (; j < nAtotal ; j++) {
		vars[j] = b->Var(j - nA) ;
		values[j] = 0 ;
		}

	// generate initial argument value combination, corresponding to the start-adr of this function table block.
	// fill in function list; check ptr's so that we don't have to check for NULL later in the main loop; etc.
	ComputeArgCombinationFromFnTableAdr(_StartAdr, nA, _ReferenceFunction->Arguments(), values, problem->K()) ;
	for (j = 0 ; j < nTotalFunctions ; j++) {
		Function *f = flist[j] ;
		FunctionTableBlock * & ftb = ftblist[j] ;
		__int64 adr = f->ComputeFnTableAdrExN(nAtotal, vars, values, problem->K()) ;
		k = f->GetFTB(adr, this, ftb, MissingBlockIDX) ;
		if (4 == k) 
			// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
			{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
		if (NULL == ftb) 
			// generic error
			{ ret = ERRORCODE_input_FTB_computed_but_failed_to_fetch ; goto done ; }
		}

	__int64 KeepIDX ;
	for (KeepIDX = 0 ; KeepIDX < _Size ; KeepIDX++) {
		for (j = nA ; j < nAtotal ; j++) 
			values[j] = 0 ;
		_Data[KeepIDX] = 0.0 ;
		for (ElimIDX = 0 ; ElimIDX < ElimSize ; ElimIDX++) {
			ARE_Function_TableType value = 1.0 ;
			for (j = 0 ; j < nTotalFunctions ; j++) {
				Function *f = flist[j] ;
				FunctionTableBlock * & ftb = ftblist[j] ;
				// make sure we have the right block from each input function
				__int64 adr = f->ComputeFnTableAdrExN(nAtotal, vars, values, problem->K()) ;
				// check if the block we already have is good; in not, fetch a new block.
				if (ftb->StartAdr() > adr || ftb->EndAdr() <= adr) {
					// release current block; if this is an external-memory block and has no other users, destroy it.
					f->ReleaseFTB(*ftb, this) ;
					ftb = NULL ;
					// get new block; if it is an external-memory block, load it.
					// this will mark us (this fn) as the user of the block, so that it won't be automatically unloaded by anybody.
					k = f->GetFTB(adr, this, ftb, MissingBlockIDX) ;
					if (4 == k) 
						// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
						{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
					if (NULL == ftb) 
						// generic error
						{ ret = ERRORCODE_input_FTB_computed_but_failed_to_fetch ; goto done ; }
					}
				value *= ftb->Entry(adr - ftb->StartAdr()) ;
				}
			_Data[KeepIDX] += value ;
			// go to next argument value combination
			EnumerateNextArgumentsValueCombination(nAtotal, vars, values, problem->K()) ;
			}
		}

	// save in a file
	SaveData() ;

/*
// DEBUGGGG
if (NULL != ARE::fpLOG) {
	char s[256] ;
	sprintf(s, "\nworker computed b idx=%d ftb idx=%I64d", (int) b->IDX(), (__int64) _IDX) ;
	fwrite(s, 1, strlen(s), ARE::fpLOG) ;
	fflush(ARE::fpLOG) ;
	}
*/

	// if this is an exernal-memory block, we can destroy the table, since its contents are saved on the disk.
	// however, we leave this to the calling fn.

	ret = 0 ;

#ifdef _DEBUG
	if (NULL != ARE::fpLOG) {
		ARE::utils::AutoLock lock(ARE::LOGMutex) ;
		double x = SumEntireData() ;
		char s[256] ;
		int wIDX = -1 ;
		sprintf(s, "\n   worker idx=%d computed b=%d ftb=%I64d sum=%g size=%I64d", wIDX, (int) b->IDX(), (__int64) _IDX, (double) x, (__int64) _Size) ;
		fwrite(s, 1, strlen(s), ARE::fpLOG) ;
		fflush(ARE::fpLOG) ;
		}
#endif // _DEBUG

	}

done :

	// release function blocks
	for (j = 0 ; j < nTotalFunctions ; j++) {
		FunctionTableBlock *ftb = ftblist[j] ;
		if (NULL != ftb) 
			ftb->ReferenceFunction()->ReleaseFTB(*ftb, this) ;
		}

	// mark this block as computed; do this after the table block is saved.
	if (0 == ret) 
		b->MarkOutputFunctionBlockComputed(_IDX) ;

	return ret ;
}


int ARE::FunctionTableBlock::ComputeDataBEEM_ProdMax(Function * & MissingFunction, __int64 & MissingBlockIDX)
{
	// DEBUGGGGG
//	if (NULL != ARE::fpLOG) {
//		fprintf(ARE::fpLOG, "\nFTB::ComputeDataBEEM_ProdMax()") ;
//		fflush(ARE::fpLOG) ;
//		}
	MissingFunction = NULL ;
	MissingBlockIDX = -1 ;

/*
// DEBUGGGGG
if (NULL != ARE::fpLOG) {
	ARE::utils::AutoLock lock(ARE::LOGMutex) ;
	char s[256] ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	sprintf(s, "\n   worker idx=%d going to compute bucket %d ftb %I64d", wIDX, (int) b->IDX(), (__int64) (this->_IDX)) ;
	fwrite(s, 1, strlen(s), ARE::fpLOG) ;
	fflush(ARE::fpLOG) ;
	}
*/
	int ret = 1, j, k ;

	// ***************************************************************************************************
	// for now (2009-07-14), assume that this function table block is in a function computed by BE.
	// this means we will be enumerating over all _ReferenceFunction argument combinations, 
	// for each argument combination eliminating all variables of the bucket.
	// ***************************************************************************************************

	// check/detect simple data structure errors
	if (NULL == _ReferenceFunction) 
		return ERRORCODE_generic ;
	const int *refFNarguments = _ReferenceFunction->Arguments() ;
	if (_IDX < 0) 
		return ERRORCODE_generic ;
	ARP *problem = _ReferenceFunction->Problem() ;
	if (NULL == problem) 
		return ERRORCODE_generic ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	if (NULL == b) 
		return ERRORCODE_generic ;
	Workspace *ws = _ReferenceFunction->WS() ;
	if (NULL == ws) 
		return ERRORCODE_generic ;
	if (b->nVars() < 1) 
		return ERRORCODE_generic ;
	int nA = _ReferenceFunction->N() ;
	if (nA < 0) {
		// this should not happen
		return 0 ;
		}
	if (1 == b->nVars()) 
		return ComputeDataBEEM_ProdMax_singleV(MissingFunction, MissingBlockIDX) ;

	// ***************************************************************************************************
	// handle special case when there are no functions in the bucket
	// ***************************************************************************************************

	int nTotalFunctions = b->nOriginalFunctions() + b->nChildBucketFunctions() ;
	if (nTotalFunctions < 1) {
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 1.0 ;
		return 0 ;
		}

	// ***************************************************************************************************
	// check if there are too many variables/functions
	// ***************************************************************************************************

	if (nA > MAX_NUM_VARIABLES_PER_BUCKET) 
		return ERRORCODE_too_many_variables ;
	if (nTotalFunctions > MAX_NUM_FUNCTIONS_PER_BUCKET) 
		return ERRORCODE_too_many_functions ;

	// ***************************************************************************************************
	// handle the special case when all variables are eliminated; e.g. there is no output fn, just a const value.
	// ***************************************************************************************************

	int values[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the current value combination of the arguments of this function (table block).
	Function *flist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input functions
	FunctionTableBlock *ftblist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input function table blocks, regardless of whether they are in-memory or external(disk)-memory.

	{

	for (j = 0 ; j < nTotalFunctions ; j++) {
		flist[j] = (j < b->nOriginalFunctions()) ? b->OriginalFunction(j) : b->ChildBucketFunction(j - b->nOriginalFunctions()) ;
		if (NULL == flist[j]) 
			return ERRORCODE_fn_ptr_is_NULL ;
		}

	__int64 ElimIDX, ElimSize = 1 ;
	for (j = 0 ; j < b->nVars() ; j++) 
		ElimSize *= problem->K(b->Var(j)) ;

	if (0 == nA) {
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 0.0 ;
		for (j = 0 ; j < b->nVars() ; j++) 
			values[j] = 0 ;
		for (ElimIDX = 0 ; ElimIDX < ElimSize ; ElimIDX++) {
			ARE_Function_TableType value = 1.0 ;
			for (j = 0 ; j < nTotalFunctions ; j++) {
				Function *f = flist[j] ;
				if (0 == f->N()) 
					value *= f->ConstValue() ;
				else {
					FunctionTableBlock * & ftb = ftblist[j] ;
					__int64 adr = f->ComputeFnTableAdrExN(b->nVars(), b->VarsArray(), values, problem->K()) ;
					k = f->GetFTB(adr, this, ftb, MissingBlockIDX) ;
					if (4 == k) 
						// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
						{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
					if (NULL == ftb) 
						// generic error
						{ ret = ERRORCODE_input_FTB_computed_but_failed_to_fetch ; goto done ; }
					value *= ftb->Entry(adr - ftb->StartAdr()) ;
					}
				}
			if (value > V) V = value ;
			// go to next argument value combination
			EnumerateNextArgumentsValueCombination(b->nVars(), b->VarsArray(), values, problem->K()) ;
			}
#ifdef _DEBUG
		if (NULL != ARE::fpLOG) {
			ARE::utils::AutoLock lock(ARE::LOGMutex) ;
			char s[256] ;
			sprintf(s, "\n   worker computed b=%d ftb=%I64d constvalue=%g", (int) b->IDX(), (__int64) _IDX, (double) V) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			fflush(ARE::fpLOG) ;
			}
#endif // _DEBUG
		ret = 0 ;
		goto done ;
		}

	// ***************************************************************************************************
	// handle general case when there are variables to eliminate and variables to keep
	// ***************************************************************************************************

	// make sure array for data is allocated
	AllocateData() ;
	if (NULL == _Data) 
		return 1 ;

	int vars[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the list of variables : vars2Keep + vars2Eliminate
	int nAtotal = nA + b->nVars() ;
	for (j = 0 ; j < nA ; j++) {
		vars[j] = refFNarguments[j] ;
		values[j] = 0 ;
		}
	for (; j < nAtotal ; j++) {
		vars[j] = b->Var(j - nA) ;
		values[j] = 0 ;
		}

	// generate initial argument value combination, corresponding to the start-adr of this function table block.
	// fill in function list; check ptr's so that we don't have to check for NULL later in the main loop; etc.
	ComputeArgCombinationFromFnTableAdr(_StartAdr, nA, _ReferenceFunction->Arguments(), values, problem->K()) ;
	for (j = 0 ; j < nTotalFunctions ; j++) {
		Function *f = flist[j] ;
		FunctionTableBlock * & ftb = ftblist[j] ;
		__int64 adr = f->ComputeFnTableAdrExN(nAtotal, vars, values, problem->K()) ;
		k = f->GetFTB(adr, this, ftb, MissingBlockIDX) ;
		if (4 == k) 
			// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
			{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
		if (NULL == ftb) 
			// generic error
			{ ret = ERRORCODE_input_FTB_computed_but_failed_to_fetch ; goto done ; }
		}

	__int64 KeepIDX ;
	for (KeepIDX = 0 ; KeepIDX < _Size ; KeepIDX++) {
		for (j = nA ; j < nAtotal ; j++) 
			values[j] = 0 ;
		_Data[KeepIDX] = 0.0 ;
		for (ElimIDX = 0 ; ElimIDX < ElimSize ; ElimIDX++) {
			ARE_Function_TableType value = 1.0 ;
			for (j = 0 ; j < nTotalFunctions ; j++) {
				Function *f = flist[j] ;
				FunctionTableBlock * & ftb = ftblist[j] ;
				// make sure we have the right block from each input function
				__int64 adr = f->ComputeFnTableAdrExN(nAtotal, vars, values, problem->K()) ;
				// check if the block we already have is good; in not, fetch a new block.
				if (ftb->StartAdr() > adr || ftb->EndAdr() <= adr) {
					// release current block; if this is an external-memory block and has no other users, destroy it.
					f->ReleaseFTB(*ftb, this) ;
					ftb = NULL ;
					// get new block; if it is an external-memory block, load it.
					// this will mark us (this fn) as the user of the block, so that it won't be automatically unloaded by anybody.
					k = f->GetFTB(adr, this, ftb, MissingBlockIDX) ;
					if (4 == k) 
						// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
						{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
					if (NULL == ftb) 
						// generic error
						{ ret = ERRORCODE_input_FTB_computed_but_failed_to_fetch ; goto done ; }
					}
				value *= ftb->Entry(adr - ftb->StartAdr()) ;
				}
			if (value > _Data[KeepIDX]) _Data[KeepIDX] = value ;
			// go to next argument value combination
			EnumerateNextArgumentsValueCombination(nAtotal, vars, values, problem->K()) ;
			}
		}

	// save in a file
	SaveData() ;

/*
// DEBUGGGG
if (NULL != ARE::fpLOG) {
	char s[256] ;
	sprintf(s, "\nworker computed b idx=%d ftb idx=%I64d", (int) b->IDX(), (__int64) _IDX) ;
	fwrite(s, 1, strlen(s), ARE::fpLOG) ;
	fflush(ARE::fpLOG) ;
	}
*/

	// if this is an exernal-memory block, we can destroy the table, since its contents are saved on the disk.
	// however, we leave this to the calling fn.

	ret = 0 ;

#ifdef _DEBUG
	if (NULL != ARE::fpLOG) {
		ARE::utils::AutoLock lock(ARE::LOGMutex) ;
		double x = SumEntireData() ;
		char s[256] ;
		int wIDX = -1 ;
		sprintf(s, "\n   worker idx=%d computed b=%d ftb=%I64d sum=%g size=%I64d", wIDX, (int) b->IDX(), (__int64) _IDX, (double) x, (__int64) _Size) ;
		fwrite(s, 1, strlen(s), ARE::fpLOG) ;
		fflush(ARE::fpLOG) ;
		}
#endif // _DEBUG

	}

done :

	// release function blocks
	for (j = 0 ; j < nTotalFunctions ; j++) {
		FunctionTableBlock *ftb = ftblist[j] ;
		if (NULL != ftb) 
			ftb->ReferenceFunction()->ReleaseFTB(*ftb, this) ;
		}

	// mark this block as computed; do this after the table block is saved.
	if (0 == ret) 
		b->MarkOutputFunctionBlockComputed(_IDX) ;

	return ret ;
}


int ARE::FunctionTableBlock::ComputeDataBEEM_SumMax(Function * & MissingFunction, __int64 & MissingBlockIDX)
{
	// DEBUGGGGG
//	if (NULL != ARE::fpLOG) {
//		fprintf(ARE::fpLOG, "\nFTB::ComputeDataBEEM_SumMax()") ;
//		fflush(ARE::fpLOG) ;
//		}
	MissingFunction = NULL ;
	MissingBlockIDX = -1 ;

/*
// DEBUGGGGG
if (NULL != ARE::fpLOG) {
	ARE::utils::AutoLock lock(ARE::LOGMutex) ;
	char s[256] ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	sprintf(s, "\n   worker idx=%d going to compute bucket %d ftb %I64d", wIDX, (int) b->IDX(), (__int64) (this->_IDX)) ;
	fwrite(s, 1, strlen(s), ARE::fpLOG) ;
	fflush(ARE::fpLOG) ;
	}
*/
	int ret = 1, j, k ;

	// ***************************************************************************************************
	// for now (2009-07-14), assume that this function table block is in a function computed by BE.
	// this means we will be enumerating over all _ReferenceFunction argument combinations, 
	// for each argument combination eliminating all variables of the bucket.
	// ***************************************************************************************************

	// check/detect simple data structure errors
	if (NULL == _ReferenceFunction) 
		return ERRORCODE_generic ;
	const int *refFNarguments = _ReferenceFunction->Arguments() ;
	if (_IDX < 0) 
		return ERRORCODE_generic ;
	ARP *problem = _ReferenceFunction->Problem() ;
	if (NULL == problem) 
		return ERRORCODE_generic ;
	BucketElimination::Bucket *b = _ReferenceFunction->OriginatingBucket() ;
	if (NULL == b) 
		return ERRORCODE_generic ;
	Workspace *ws = _ReferenceFunction->WS() ;
	if (NULL == ws) 
		return ERRORCODE_generic ;
	if (b->nVars() < 1) 
		return ERRORCODE_generic ;
	int nA = _ReferenceFunction->N() ;
	if (nA < 0) {
		// this should not happen
		return 0 ;
		}
	if (1 == b->nVars()) 
		return ComputeDataBEEM_SumMax_singleV(MissingFunction, MissingBlockIDX) ;

	// ***************************************************************************************************
	// handle special case when there are no functions in the bucket
	// ***************************************************************************************************

	int nTotalFunctions = b->nOriginalFunctions() + b->nChildBucketFunctions() ;
	if (nTotalFunctions < 1) {
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = 1.0 ;
		return 0 ;
		}

	// ***************************************************************************************************
	// check if there are too many variables/functions
	// ***************************************************************************************************

	if (nA > MAX_NUM_VARIABLES_PER_BUCKET) 
		return ERRORCODE_too_many_variables ;
	if (nTotalFunctions > MAX_NUM_FUNCTIONS_PER_BUCKET) 
		return ERRORCODE_too_many_functions ;

	// ***************************************************************************************************
	// handle the special case when all variables are eliminated; e.g. there is no output fn, just a const value.
	// ***************************************************************************************************

	int values[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the current value combination of the arguments of this function (table block).
	Function *flist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input functions
	FunctionTableBlock *ftblist[MAX_NUM_FUNCTIONS_PER_BUCKET] ; // this is a list of input function table blocks, regardless of whether they are in-memory or external(disk)-memory.

	{

	for (j = 0 ; j < nTotalFunctions ; j++) {
		flist[j] = (j < b->nOriginalFunctions()) ? b->OriginalFunction(j) : b->ChildBucketFunction(j - b->nOriginalFunctions()) ;
		if (NULL == flist[j]) 
			return ERRORCODE_fn_ptr_is_NULL ;
		}

	__int64 ElimIDX, ElimSize = 1 ;
	for (j = 0 ; j < b->nVars() ; j++) 
		ElimSize *= problem->K(b->Var(j)) ;

	if (0 == nA) {
		ARE_Function_TableType & V = _ReferenceFunction->ConstValue() ;
		V = ARE::neg_infinity ;
		for (j = 0 ; j < b->nVars() ; j++) 
			values[j] = 0 ;
		for (ElimIDX = 0 ; ElimIDX < ElimSize ; ElimIDX++) {
			ARE_Function_TableType value = 0.0 ;
			for (j = 0 ; j < nTotalFunctions ; j++) {
				Function *f = flist[j] ;
				if (0 == f->N()) 
					value += f->ConstValue() ;
				else {
					FunctionTableBlock * & ftb = ftblist[j] ;
					__int64 adr = f->ComputeFnTableAdrExN(b->nVars(), b->VarsArray(), values, problem->K()) ;
					k = f->GetFTB(adr, this, ftb, MissingBlockIDX) ;
					if (4 == k) 
						// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
						{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
					if (NULL == ftb) 
						// generic error
						{ ret = ERRORCODE_input_FTB_computed_but_failed_to_fetch ; goto done ; }
					value += ftb->Entry(adr - ftb->StartAdr()) ;
					}
				}
			if (value > V) V = value ;
			// go to next argument value combination
			EnumerateNextArgumentsValueCombination(b->nVars(), b->VarsArray(), values, problem->K()) ;
			}
#ifdef _DEBUG
		if (NULL != ARE::fpLOG) {
			ARE::utils::AutoLock lock(ARE::LOGMutex) ;
			char s[256] ;
			sprintf(s, "\n   worker computed b=%d ftb=%I64d constvalue=%g", (int) b->IDX(), (__int64) _IDX, (double) V) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			fflush(ARE::fpLOG) ;
			}
#endif // _DEBUG
		ret = 0 ;
		goto done ;
		}

	// ***************************************************************************************************
	// handle general case when there are variables to eliminate and variables to keep
	// ***************************************************************************************************

	// make sure array for data is allocated
	AllocateData() ;
	if (NULL == _Data) 
		return 1 ;

	int vars[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the list of variables : vars2Keep + vars2Eliminate
	int nAtotal = nA + b->nVars() ;
	for (j = 0 ; j < nA ; j++) {
		vars[j] = refFNarguments[j] ;
		values[j] = 0 ;
		}
	for (; j < nAtotal ; j++) {
		vars[j] = b->Var(j - nA) ;
		values[j] = 0 ;
		}

	// generate initial argument value combination, corresponding to the start-adr of this function table block.
	// fill in function list; check ptr's so that we don't have to check for NULL later in the main loop; etc.
	ComputeArgCombinationFromFnTableAdr(_StartAdr, nA, _ReferenceFunction->Arguments(), values, problem->K()) ;
	for (j = 0 ; j < nTotalFunctions ; j++) {
		Function *f = flist[j] ;
		FunctionTableBlock * & ftb = ftblist[j] ;
		__int64 adr = f->ComputeFnTableAdrExN(nAtotal, vars, values, problem->K()) ;
		k = f->GetFTB(adr, this, ftb, MissingBlockIDX) ;
		if (4 == k) 
			// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
			{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
		if (NULL == ftb) 
			// generic error
			{ ret = ERRORCODE_input_FTB_computed_but_failed_to_fetch ; goto done ; }
		}

	__int64 KeepIDX ;
	for (KeepIDX = 0 ; KeepIDX < _Size ; KeepIDX++) {
		for (j = nA ; j < nAtotal ; j++) 
			values[j] = 0 ;
		_Data[KeepIDX] = ARE::neg_infinity ;
		for (ElimIDX = 0 ; ElimIDX < ElimSize ; ElimIDX++) {
			ARE_Function_TableType value = 0.0 ;
			for (j = 0 ; j < nTotalFunctions ; j++) {
				Function *f = flist[j] ;
				FunctionTableBlock * & ftb = ftblist[j] ;
				// make sure we have the right block from each input function
				__int64 adr = f->ComputeFnTableAdrExN(nAtotal, vars, values, problem->K()) ;
				// check if the block we already have is good; in not, fetch a new block.
				if (ftb->StartAdr() > adr || ftb->EndAdr() <= adr) {
					// release current block; if this is an external-memory block and has no other users, destroy it.
					f->ReleaseFTB(*ftb, this) ;
					ftb = NULL ;
					// get new block; if it is an external-memory block, load it.
					// this will mark us (this fn) as the user of the block, so that it won't be automatically unloaded by anybody.
					k = f->GetFTB(adr, this, ftb, MissingBlockIDX) ;
					if (4 == k) 
						// this block has not been computed yet; set proper return code and be done; we will abandon the computation.
						{ ret = ERRORCODE_input_FTB_not_computed ; MissingFunction = f ; goto done ; }
					if (NULL == ftb) 
						// generic error
						{ ret = ERRORCODE_input_FTB_computed_but_failed_to_fetch ; goto done ; }
					}
				value += ftb->Entry(adr - ftb->StartAdr()) ;
				}
			if (value > _Data[KeepIDX]) _Data[KeepIDX] = value ;
			// go to next argument value combination
			EnumerateNextArgumentsValueCombination(nAtotal, vars, values, problem->K()) ;
			}
		}

	// save in a file
	SaveData() ;

/*
// DEBUGGGG
if (NULL != ARE::fpLOG) {
	char s[256] ;
	sprintf(s, "\nworker computed b idx=%d ftb idx=%I64d", (int) b->IDX(), (__int64) _IDX) ;
	fwrite(s, 1, strlen(s), ARE::fpLOG) ;
	fflush(ARE::fpLOG) ;
	}
*/

	// if this is an exernal-memory block, we can destroy the table, since its contents are saved on the disk.
	// however, we leave this to the calling fn.

	ret = 0 ;

#ifdef _DEBUG
	if (NULL != ARE::fpLOG) {
		ARE::utils::AutoLock lock(ARE::LOGMutex) ;
		double x = SumEntireData() ;
		char s[256] ;
		int wIDX = -1 ;
		sprintf(s, "\n   worker idx=%d computed b=%d ftb=%I64d sum=%g size=%I64d", wIDX, (int) b->IDX(), (__int64) _IDX, (double) x, (__int64) _Size) ;
		fwrite(s, 1, strlen(s), ARE::fpLOG) ;
		fflush(ARE::fpLOG) ;
		}
#endif // _DEBUG

	}

done :

	// release function blocks
	for (j = 0 ; j < nTotalFunctions ; j++) {
		FunctionTableBlock *ftb = ftblist[j] ;
		if (NULL != ftb) 
			ftb->ReferenceFunction()->ReleaseFTB(*ftb, this) ;
		}

	// mark this block as computed; do this after the table block is saved.
	if (0 == ret) 
		b->MarkOutputFunctionBlockComputed(_IDX) ;

	return ret ;
}

