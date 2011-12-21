#include <stdlib.h>
#include <memory.h>

#include <Function.hxx>
#include <Bucket.hxx>
#include <BEworkspace.hxx>


BucketElimination::Bucket::Bucket(void)
	:
	_Workspace(NULL),
	_IDX(-1), 
	_nVars(0), 
	_Vars(NULL), 
	_VarsSpace(0), 
	_ParentBucket(NULL), 
	_RootBucket(NULL), 
	_DistanceToRoot(-1), 
	_Height(-1), 
	_nOriginalFunctions(0), 
	_OriginalFunctions(NULL), 
	_OriginalWidth(-1), 
	_OriginalSignature(NULL), 
	_nChildBucketFunctions(0), 
	_ChildBucketFunctions(NULL), 
	_ChildBucketFunctionArraySize(0), 
	_Width(-1), 
	_Signature(NULL), 
	_OutputFunctionBlockComputationResultSize(0), 
	_nOutputFunctionBlocks(-1), 
	_OutputFunctionBlockComputationResult(NULL), 
	_nOutputFunctionBlocksComputed(0)
{
}


BucketElimination::Bucket::~Bucket(void)
{
	Destroy() ;
}


BucketElimination::Bucket::Bucket(BEworkspace & WS, int IDX, int V)
	:
	_Workspace(&WS),
	_IDX(IDX), 
	_nVars(0), 
	_Vars(NULL), 
	_VarsSpace(0), 
	_ParentBucket(NULL), 
	_RootBucket(NULL), 
	_DistanceToRoot(-1), 
	_Height(-1), 
	_nOriginalFunctions(0), 
	_OriginalFunctions(NULL), 
	_OriginalWidth(-1), 
	_OriginalSignature(NULL), 
	_nChildBucketFunctions(0), 
	_ChildBucketFunctions(NULL), 
	_ChildBucketFunctionArraySize(0), 
	_Width(-1), 
	_Signature(NULL), 
	_OutputFunctionBlockComputationResultSize(0), 
	_nOutputFunctionBlocks(-1), 
	_OutputFunctionBlockComputationResult(NULL), 
	_nOutputFunctionBlocksComputed(0)
{
	// if Var is given, add it
	if (V >= 0) 
		AddVar(V) ;
	// fix up output function of the bucket
	_OutputFunction.Initialize(_Workspace, _Workspace->Problem(), -1) ;
	_OutputFunction.SetOriginatingBucket(this) ;
}


void BucketElimination::Bucket::Destroy(void)
{
	if (NULL != _OriginalFunctions) {
		delete [] _OriginalFunctions ;
		_OriginalFunctions = NULL ;
		}
	if (NULL != _OriginalSignature) {
		delete [] _OriginalSignature ;
		_OriginalSignature = NULL ;
		}
	if (NULL != _ChildBucketFunctions) {
		delete [] _ChildBucketFunctions ;
		_ChildBucketFunctions = NULL ;
		_ChildBucketFunctionArraySize = 0 ;
		}
	if (NULL != _Signature) {
		delete [] _Signature ;
		_Signature = NULL ;
		}
	_OutputFunction.Destroy() ;
	if (NULL != _OutputFunctionBlockComputationResult) {
		delete [] _OutputFunctionBlockComputationResult ;
		_OutputFunctionBlockComputationResult = NULL ;
		}
	_OutputFunctionBlockComputationResultSize = 0 ;
	_Width = -1 ;
	_nChildBucketFunctions = 0 ;
	_OriginalWidth = -1 ;
	_nOriginalFunctions = 0 ;
	_nVars = 0 ;
	if (NULL != _Vars) {
		delete [] _Vars ;
		_Vars = NULL ;
		}
	_VarsSpace = 0 ;
}


int BucketElimination::Bucket::SaveXMLString(const char *prefixspaces, const std::string & Dir, std::string & S)
{
	char s[1024] ;
	std::string temp ;
	int i, j ;
	sprintf(s, "%s<bucket IDX=\"%d\" nVars=\"%d\" Vars=\"", prefixspaces, _IDX, _nVars) ;
	S += s ;
	for (i = 0 ; i < _nVars ; i++) {
		sprintf(s, "%d", _Vars[i]) ;
		if (i > 0) 
			S += ';' ;
		S += s ;
		}
	S += '"' ;
	if (NULL !=_ParentBucket) {
		sprintf(s, " parentbucket=\"%d\"", _ParentBucket->IDX()) ;
		S += s ;
		}
	S += ">" ;

	// save original functions
	if (_nOriginalFunctions > 0 && NULL != _OriginalFunctions) {
		sprintf(s, "\n%s <originalfunctions n=\"%d\" list=\"", prefixspaces, _nOriginalFunctions) ;
		S += s ;
		for (i = 0 ; i < _nOriginalFunctions ; i++) {
			ARE::Function *f = _OriginalFunctions[i] ;
			if (NULL == f) continue ;
			sprintf(s, "%d", f->IDX()) ;
			if (i > 0) 
				S += ';' ;
			S += s ;
			}
		S += "\"/>" ;
		}

	// save incoming child bucket functions
	if (_nChildBucketFunctions > 0 && NULL != _ChildBucketFunctions) {
		sprintf(s, "%s ", prefixspaces) ;
		for (i = 0 ; i < _nChildBucketFunctions ; i++) {
			ARE::Function *f = _ChildBucketFunctions[i] ;
			if (NULL == f) continue ;
			S += "\n" ;
			// bucketfunctions don't have IDX; set the IDX of the originating bucket as the IDX of this function
			int idx = f->IDX() ;
			Bucket *b = f->OriginatingBucket() ;
			if (NULL != b) 
				f->SetIDX(b->IDX()) ;
			f->SaveXMLString(s, "childbucketfn", Dir, S) ;
			f->SetIDX(idx) ;
			}
		}

	// serialize _OutputFunction
	if (_OutputFunction.N() > 0) {
		sprintf(s, "%s ", prefixspaces) ;
		S += "\n" ;
		_OutputFunction.SaveXMLString(s, "ownbucketfn", Dir, S) ;
		}

	// serialize _OutputFunctionBlockComputationResult
	if (_OutputFunction.N() > 0 && NULL != _OutputFunctionBlockComputationResult) {
		__int64 nTB = _OutputFunction.nTableBlocks() ;
		int size = (7 + nTB) >> 3 ;
		sprintf(s, "\n%s <ownbucketfncomputationresult nComputed=\"%I64d/%I64d\" bits=\"", prefixspaces, _nOutputFunctionBlocksComputed, nTB) ;
		S += s ;
		int n = 0 ;
		for (i = 0 ; i < size && n < nTB ; i++) {
			for (j = 0 ; j < 8 && n < nTB ; j++, n++) {
				int bit = _OutputFunctionBlockComputationResult[i] & (1 << j) ;
				if (0 != bit) 
					S += '1' ;
				else 
					S += '0' ;
				}
			}
		S += "\"/>" ;
		}

	sprintf(s, "\n%s</bucket>", prefixspaces) ;
	S += s ;

	return 0 ;
}


int BucketElimination::Bucket::SetOriginalFunctions(int N, ARE::Function *FNs[]) 
{
	if (NULL != _OriginalFunctions) {
		delete [] _OriginalFunctions ;
		_OriginalFunctions = NULL ;
		}
	if (NULL != _OriginalSignature) {
		delete [] _OriginalSignature ;
		_OriginalSignature = NULL ;
		}
	_OriginalWidth = 0 ;

	if (N < 1) 
		return 0 ;

	int i, j, k ;

	_OriginalFunctions = new ARE::Function*[N] ;
	if (NULL == _OriginalFunctions) 
		{ Destroy() ; return 1 ; }
	_nOriginalFunctions = N ;
	for (i = 0 ; i < _nOriginalFunctions ; i++) 
		_OriginalFunctions[i] = FNs[i] ;

	// compute approx width
	if (0 == _nOriginalFunctions) 
		return 0 ;
	int n = 0 ;
	for (i = 0 ; i < _nOriginalFunctions ; i++) 
		n += _OriginalFunctions[i]->N() ;
	// n is an upper bound on the width

	// compute width/signature
	_OriginalSignature = new int[n] ;
	if (NULL == _OriginalSignature) 
		goto failed ;
	_OriginalWidth = 0 ;
	for (i = 0 ; i < _nOriginalFunctions ; i++) {
		ARE::Function *f = _OriginalFunctions[i] ;
		f->SetBEBucket(this) ;
		for (j = 0 ; j < f->N() ; j++) {
			int v = f->Argument(j) ;
			for (k = 0 ; k < _OriginalWidth ; k++) 
				{ if (_OriginalSignature[k] == v) break ; }
			if (k < _OriginalWidth) 
				continue ;
			_OriginalSignature[_OriginalWidth++] = v ;
			}
		}

	return 0 ;
failed :
	Destroy() ;
	return 1 ;
}


int BucketElimination::Bucket::AddOriginalFunctions(int N, ARE::Function *FNs[]) 
{
	if (N < 1) 
		return 0 ;

	int i, j, k ;

	// check if functions in FNs are already in this bucket
	for (i = N-1 ; i >= 0 ; i--) {
		ARE::Function *f = FNs[i] ;
		for (j = 0 ; j < _nOriginalFunctions ; j++) {
			if (_OriginalFunctions[j] == f) {
				FNs[i] = FNs[--N] ;
				break ;
				}
			}
		}
	if (N < 1) 
		return 0 ;

	// reallocate original functions array
	int space = _nOriginalFunctions + N ;
	ARE::Function **fnlist = new ARE::Function*[space] ;
	if (NULL == fnlist) 
		return 1 ;
	for (i = 0 ; i < _nOriginalFunctions ; i++) 
		fnlist[i] = _OriginalFunctions[i] ;
	for (; i < space ; i++) 
		fnlist[i] = FNs[i-_nOriginalFunctions] ;
	delete [] _OriginalFunctions ;
	_OriginalFunctions = fnlist ;
	_nOriginalFunctions = space ;

	// compute approx width
	int n = 0 ;
	for (i = 0 ; i < _nOriginalFunctions ; i++) 
		n += _OriginalFunctions[i]->N() ;
	// n is an upper bound on the width

	// compute width/signature
	if (NULL != _OriginalSignature) {
		delete [] _OriginalSignature ;
		_OriginalSignature = NULL ;
		}
	_OriginalSignature = new int[n] ;
	if (NULL == _OriginalSignature) 
		goto failed ;
	_OriginalWidth = 0 ;
	for (i = 0 ; i < _nOriginalFunctions ; i++) {
		ARE::Function *f = _OriginalFunctions[i] ;
		f->SetBEBucket(this) ;
		for (j = 0 ; j < f->N() ; j++) {
			int v = f->Argument(j) ;
			for (k = 0 ; k < _OriginalWidth ; k++) 
				{ if (_OriginalSignature[k] == v) break ; }
			if (k < _OriginalWidth) 
				continue ;
			_OriginalSignature[_OriginalWidth++] = v ;
			}
		}

	return 0 ;
failed :
	Destroy() ;
	return 1 ;
}


int BucketElimination::Bucket::AddChildBucketFunction(ARE::Function & F)
{
	// check if we have enough space
	if (_nChildBucketFunctions+1 > _ChildBucketFunctionArraySize) {
		int newsize = _ChildBucketFunctionArraySize + 8 ;
		ARE::Function **newspace = new ARE::Function*[newsize] ;
		if (NULL == newspace) 
			return 1 ;
		if (_nChildBucketFunctions > 0) 
			memcpy(newspace, _ChildBucketFunctions, sizeof(ARE::Function *)*_nChildBucketFunctions) ;
		delete []_ChildBucketFunctions ;
		_ChildBucketFunctions = newspace ;
		_ChildBucketFunctionArraySize = newsize ;
		}

	_ChildBucketFunctions[_nChildBucketFunctions++] = &F ;

	return 0 ;
}


int BucketElimination::Bucket::RemoveChildBucketFunction(ARE::Function & F)
{
	int i ;
	for (i = _nChildBucketFunctions - 1 ; i >= 0 ; i--) {
		if (&F == _ChildBucketFunctions[i]) {
			_ChildBucketFunctions[i] = _ChildBucketFunctions[--_nChildBucketFunctions] ;
			F.SetBEBucket(NULL) ;
			}
		}
	return 0 ;
}


int BucketElimination::Bucket::ComputeSignature(void)
{
	if (NULL != _Signature) {
		delete [] _Signature ;
		_Signature = NULL ;
		}
	_Width = 0 ;

	// compute approx width
	if (_OriginalWidth < 0) {
		_Width = -1 ;
		return 1 ;
		}
	if (0 == _OriginalWidth && 0 == _nChildBucketFunctions) 
		return 0 ;
	int i, n = _OriginalWidth ;
	for (i = 0 ; i < _nChildBucketFunctions ; i++) 
		n += _ChildBucketFunctions[i]->N() ;
	// n is an upper bound on the width

	// OriginalSignature is part of Signature
	_Signature = new int[n] ;
	if (NULL == _Signature) 
		goto failed ;
	if (_OriginalWidth > 0) {
		memcpy(_Signature, _OriginalSignature, _OriginalWidth*sizeof(int)) ;
		_Width = _OriginalWidth ;
		}

	// add scopes of bucketfunctions to the signature
	int j, k ;
	for (i = 0 ; i < _nChildBucketFunctions ; i++) {
		ARE::Function *f = _ChildBucketFunctions[i] ;
		for (j = 0 ; j < f->N() ; j++) {
			int v = f->Argument(j) ;
			for (k = 0 ; k < _Width ; k++) 
				{ if (_Signature[k] == v) break ; }
			if (k < _Width) 
				continue ;
			_Signature[_Width++] = v ;
			}
		}

	return 0 ;
failed :
	return 1 ;
}


__int64 BucketElimination::Bucket::ComputeProcessingComplexity(void)
{
	__int64 n = 1 ;
	for (int i = 0 ; i < _Width ; i++) {
		n *= _Workspace->Problem()->K(_Signature[i]) ;
		}
	return n ;
}


int BucketElimination::Bucket::ComputeOutputFunctionWithScopeWithoutTable(void)
{
	// dump current bucket fn; cleanup.
	if (NULL != _ParentBucket) {
		_ParentBucket->RemoveChildBucketFunction(_OutputFunction) ;
		_ParentBucket = NULL ;
		}
	_OutputFunction.Destroy() ;

	if (_Width < 0) {
		if (0 != ComputeSignature()) 
			return 1 ;
		}
	if (_Width <= _nVars) 
		// _Width=0 can only be when this bucket has no functions
		// _Width=_nVars means all functions in this bucket have only _Vars in their scope; this bucket has a function with const-value; in this case
		// we will still compute the const-value, but the output function does not go anywhere.
		return 0 ;
	if (_Width > MAX_NUM_VARIABLES_PER_BUCKET) 
		return 1 ;

	// make a local copy, while ignoring bucket variables
	int vlist[MAX_NUM_VARIABLES_PER_BUCKET] ;
	int i, j, n = 0 ;
	for (i = 0 ; i < _Width ; i++) {
		int v = _Signature[i] ;
		for (j = 0 ; j < _nVars ; j++) {
			if (_Vars[j] == v) 
				break ;
			}
		if (j < _nVars) 
			continue ;
		vlist[n++] = v ;
		}

	// create and initialize bucket function
	if (0 != _OutputFunction.SetArguments(n, vlist)) 
		goto failed ;
	if (_nOriginalFunctions > 0) 
		_OutputFunction.SetType(_OriginalFunctions[0]->Type()) ;
	else if (_nChildBucketFunctions > 0) 
		_OutputFunction.SetType(_ChildBucketFunctions[0]->Type()) ;

	// find appropriate parent bucket and assign the bucket function to it
	{
	const int *varpos = _Workspace->VarPos() ;
	int v = _OutputFunction.GetHighestOrderedVariable(varpos) ;
	Bucket *parentbucket = _Workspace->MapVar2Bucket(v) ;
	if (NULL == parentbucket) 
		// this is not supposed to happen
		goto failed ;
	if (parentbucket->IDX() >= _IDX) 
		// this is not supposed to happen
		goto failed ;
	if (0 != parentbucket->AddChildBucketFunction(_OutputFunction)) 
		goto failed ;
	_OutputFunction.SetBEBucket(parentbucket) ;

	// make sure this bucket knows its parent bucket
	_ParentBucket = parentbucket ;
	}

	return 0 ;
failed :
	if (_ParentBucket) {
		_ParentBucket->RemoveChildBucketFunction(_OutputFunction) ;
		_ParentBucket = NULL ;
		}
	_OutputFunction.Destroy() ;
	return 1 ;
}


int BucketElimination::Bucket::NoteOutputFunctionComputationCompletion(void)
{
	int i ;

	// child bucket output functions are no longer needed; release them.
	for (i = 0 ; i < _nChildBucketFunctions ; i++) {
		ARE::Function *f = _ChildBucketFunctions[i] ;
		if (NULL == f) continue ;
		// if needed, delete input function (tables)
		f->ReleaseAllFTBs(_Workspace->DeleteUsedTables()) ;
		}

	return 0 ;
}


int BucketElimination::Bucket::ReorderFunctionScopesForExternalMemory(bool IncludeOriginalFunctions, bool IncludeNewFunctions)
{
	// destroy table of all childbucketfunctions of this bucket;
	// reordering of scopes should be done in the beginning when these tables don't exist yet, 
	// so this should not be a problem.
	int i ;
	if (IncludeNewFunctions) {
		for (i = 0 ; i < _nChildBucketFunctions ; i++) {
			ARE::Function *f = _ChildBucketFunctions[i] ;
			f->DestroyTable() ;
			f->DestroyFTBList() ;
			}
		}

	// reorder scopes
	if (_OutputFunction.N() < 1) {
		if (IncludeNewFunctions) {
			for (i = 0 ; i < _nChildBucketFunctions ; i++) {
				if (0 != _ChildBucketFunctions[i]->ReOrderArguments(_nVars, _Vars, 0, NULL)) 
					return 1 ;
				}
			}
		if (IncludeOriginalFunctions) {
			for (i = 0 ; i < _nOriginalFunctions ; i++) {
				if (0 != _OriginalFunctions[i]->ReOrderArguments(_nVars, _Vars, 0, NULL)) 
					return 1 ;
				}
			}
		}
	else {
		if (IncludeNewFunctions) {
			for (i = 0 ; i < _nChildBucketFunctions ; i++) {
				if (0 != _ChildBucketFunctions[i]->ReOrderArguments(_OutputFunction.N(), _OutputFunction.Arguments(), _nVars, _Vars)) 
					return 1 ;
				}
			}
		if (IncludeOriginalFunctions) {
			for (i = 0 ; i < _nOriginalFunctions ; i++) {
				if (0 != _OriginalFunctions[i]->ReOrderArguments(_OutputFunction.N(), _OutputFunction.Arguments(), _nVars, _Vars)) 
					return 1 ;
				}
			}
		}

	return 0 ;
}


int BucketElimination::Bucket::AllocateOutputFunctionBlockComputationResult(__int64 MaxBlockSize, int nComputingThreads)
{
	ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;
	if (_OutputFunction.nTableBlocks() < 0) {
		if (0 != _OutputFunction.ComputeTableBlockSize(MaxBlockSize, nComputingThreads)) 
			return 1 ;
		}
	_nOutputFunctionBlocks = _OutputFunction.nTableBlocks() ;
	if (_nOutputFunctionBlocks < 0) 
		// this means fn has no table 
		return 0 ;
	int size = _nOutputFunctionBlocks > 0 ? (7 + _nOutputFunctionBlocks) >> 3 : 1 ;
	if (_OutputFunctionBlockComputationResultSize == size) {
		memset(_OutputFunctionBlockComputationResult, 0, _OutputFunctionBlockComputationResultSize) ;
		_nOutputFunctionBlocksComputed = 0 ;
		return 0 ;
		}
	if (NULL != _OutputFunctionBlockComputationResult) 
		delete [] _OutputFunctionBlockComputationResult ;
	_OutputFunctionBlockComputationResult = new unsigned char[size] ;
	if (NULL == _OutputFunctionBlockComputationResult) 
		return 0 ;
	_OutputFunctionBlockComputationResultSize = size ;
	memset(_OutputFunctionBlockComputationResult, 0, _OutputFunctionBlockComputationResultSize) ;
	_nOutputFunctionBlocksComputed = 0 ;
	return 0 ;
}


bool BucketElimination::Bucket::IsOutputFunctionBlockComputed(__int64 IDX)
{
	ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;
	if (0 == IDX) {
		// this means function table has 1 in-memory block
		return _nOutputFunctionBlocksComputed > 0 ;
		}
	--IDX ; // disk memeory block indeces run [1,nBlocks], but we need [0,nBlocks)
	return _OutputFunctionBlockComputationResult[IDX >> 3] & (1 << (IDX & 7)) ;
}


__int64 BucketElimination::Bucket::nOutputFunctionBlocksComputed(void) const
{
	ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;
	return _nOutputFunctionBlocksComputed ;
}


void BucketElimination::Bucket::MarkOutputFunctionBlockComputed(__int64 IDX)
{
	ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;
	if (0 == IDX) {
		// this means function table has 1 in-memory block
		if (0 == _nOutputFunctionBlocksComputed) 
			_nOutputFunctionBlocksComputed = 1 ;
		NoteOutputFunctionComputationCompletion() ;
		return ;
		}
	if (! IsOutputFunctionBlockComputed(IDX)) {
		--IDX ; // disk memeory block indeces run [1,nBlocks], but we need [0,nBlocks)
		_OutputFunctionBlockComputationResult[IDX >> 3] |= (1 << (IDX & 7)) ;
		++_nOutputFunctionBlocksComputed ;
		}
	if (_nOutputFunctionBlocksComputed >= _nOutputFunctionBlocks) 
		NoteOutputFunctionComputationCompletion() ;
}
