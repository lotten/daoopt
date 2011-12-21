#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "Sort.hxx"

#include <Function.hxx>
#include <BEworkspace.hxx>

BucketElimination::BEworkspace::BEworkspace(const char *BEEMDiskSpaceDirectory)
	:
	ARE::Workspace(BEEMDiskSpaceDirectory), 
	_VarOrder(NULL), 
	_VarPos(NULL), 
	_Var2BucketMapping(NULL), 
	_AnswerFactor(1.0), 
	_FnCombination_VarElimination_Type(VAR_ELIMINATION_TYPE_PROD_SUM), 
	_nBuckets(0), 
	_MaxNumChildren(-1), 
	_MaxNumVarsInBucket(-1),
	_nBucketsWithSingleChild_initial(-1), 
	_nBucketsWithSingleChild_final(-1), 
	_nBuckets_initial(-1), 
	_nBuckets_final(-1), 
	_nVarsWithoutBucket(-1), 
	_nConstValueFunctions(-1), 
	_DeleteUsedTables(false), 
	_BucketOrderToCompute(NULL) 
{
	if (! _IsValid) 
		return ;

	for (int i = 0 ; i < MAX_NUM_BUCKETS ; i++) 
		_Buckets[i] = NULL ;

	_IsValid = true ;
}


int BucketElimination::BEworkspace::Initialize(ARE::ARP & Problem, const int *VarOrderingAsVarList)
{
	if (! _IsValid || 0 != ARE::Workspace::Initialize(Problem)) 
		return 1 ;

	int i ;

	if (VAR_ELIMINATION_TYPE_SUM_MAX == _FnCombination_VarElimination_Type) 
		_AnswerFactor = 0.0 ;
	else 
		_AnswerFactor = 1.0 ;

	if (_Problem ? _Problem->N() > 0 : false) {
		_VarOrder = new int[_Problem->N()] ;
		for (i = 0 ; i < _Problem->N() ; i++) {
			_VarOrder[i] = -1 ;
			}

		// if ordering is not given, use one from the problem
		if (NULL == VarOrderingAsVarList) {
			if (NULL == _Problem->MinDegreeOrdering_VarList()) {
				if (0 != _Problem->ComputeMinDegreeOrdering()) 
					return 1 ;
				if (0 != _Problem->TestVariableOrdering(_Problem->MinDegreeOrdering_VarList(), _Problem->MinDegreeOrdering_VarPos())) 
					return 1 ;
				}

			if (NULL == _Problem->MinFillOrdering_VarList()) {
				if (0 != _Problem->ComputeMinFillOrdering(1, -1)) 
					return 1 ;
				if (0 != _Problem->TestVariableOrdering(_Problem->MinFillOrdering_VarList(), _Problem->MinFillOrdering_VarPos())) 
					return 1 ;
				}
			int minDegreeWidth = _Problem->MinDegreeOrdering_InducedWidth() ;
			if (minDegreeWidth < 0) minDegreeWidth = 999999 ;
			int minFillWidth = _Problem->MinFillOrdering_InducedWidth() ;
			if (minFillWidth < 0) minFillWidth = 999999 ;
			if (minDegreeWidth <= minFillWidth) 
				VarOrderingAsVarList = _Problem->MinDegreeOrdering_VarList() ;
			else 
				VarOrderingAsVarList = _Problem->MinFillOrdering_VarList() ;
			}

		if (0 != CreateBuckets(_Problem->N(), VarOrderingAsVarList, true)) 
			return 1 ;

		// verify integrity of functions
		for (i = 0 ; i < _Problem->nFunctions() ; i++) {
			ARE::Function *f = _Problem->getFunction(i) ;
			if (0 != f->CheckIntegrity()) 
				return 1 ;
			}
		for (i = 0 ; i < _nBuckets ; i++) {
			BucketElimination::Bucket *b = _Buckets[i] ;
			ARE::Function & f = b->OutputFunction() ;
			if (0 != f.CheckIntegrity()) 
				return 1 ;
			}
		}

	return 0 ;
}


void BucketElimination::BEworkspace::DestroyBuckets(void)
{
	if (_VarOrder) {
		delete [] _VarOrder ;
		_VarOrder = NULL ;
		}
	if (_VarPos) {
		delete [] _VarPos ;
		_VarPos = NULL ;
		}
	if (_Var2BucketMapping) {
		delete [] _Var2BucketMapping ;
		_Var2BucketMapping = NULL ;
		}
	if (NULL != _BucketOrderToCompute) {
		delete [] _BucketOrderToCompute ;
		_BucketOrderToCompute = NULL ;
		}
	_nVars = 0 ;
	for (int i = 0 ; i < MAX_NUM_BUCKETS ; i++) {
		if (NULL != _Buckets[i]) {
			delete _Buckets[i] ;
			_Buckets[i] = NULL ;
			}
		}
	_nBuckets = 0 ;
//	_AnswerFactor = 1.0 ;
//	_FnCombination_VarElimination_Type = VAR_ELIMINATION_TYPE_PROD_SUM ;
	_nVarsWithoutBucket = -1 ;
//	_nConstValueFunctions = -1 ;
}


int BucketElimination::BEworkspace::CheckBucketTreeIntegrity(void)
{
	int i, j, k ;

	for (i = 0 ; i < _nBuckets ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		BucketElimination::Bucket *B = b->ParentBucket() ;
		// check indeces are within range
		if (b->IDX() < 0 || b->IDX() >= _nBuckets) 
			goto failed ;
		if (NULL != B ? B->IDX() < 0 || B->IDX() >= _nBuckets : false) 
			goto failed ;
		// check width is computed
		if (b->Width() < 0) 
			goto failed ;
		// check output function
		ARE::Function & f = b->OutputFunction() ;
		if (f.BEBucket() != B) 
			goto failed ;
		if (f.N() > 0 && NULL == B) 
			goto failed ;
		if (0 == f.N() && NULL != B) 
			goto failed ;
		// check bVar, Signature, f->args are unique
		for (j = 0 ; j < b->nVars() ; j++) {
			int u = b->Var(j) ;
			for (k = j+1 ; k < b->nVars() ; k++) {
				if (u == b->Var(k)) 
					goto failed ;
				}
			}
		const int *bSig = b->Signature() ;
		for (j = 0 ; j < b->Width() ; j++) {
			int u = bSig[j] ;
			for (k = j+1 ; k < b->Width() ; k++) {
				if (u == bSig[k]) 
					goto failed ;
				}
			}
		const int *fArgs = f.Arguments() ;
		for (j = 0 ; j < f.N() ; j++) {
			int u = fArgs[j] ;
			for (k = j+1 ; k < f.N() ; k++) {
				if (u == fArgs[k]) 
					goto failed ;
				}
			}
		// check f->args and bVars are disjoint
		for (j = 0 ; j < f.N() ; j++) {
			int u = fArgs[j] ;
			for (k = 0 ; k < b->nVars() ; k++) {
				if (u == b->Var(k)) 
					goto failed ;
				}
			}
		for (j = 0 ; j < b->nVars() ; j++) {
			int u = b->Var(j) ;
			for (k = 0 ; k < f.N() ; k++) {
				if (u == fArgs[k]) 
					goto failed ;
				}
			}
		// check all f->args are in signature
		for (j = 0 ; j < f.N() ; j++) {
			int u = fArgs[j] ;
			for (k = 0 ; k < b->Width() ; k++) {
				if (u == bSig[k]) 
					break ;
				}
			if (k >= b->Width()) 
				goto failed ;
			}
		// bVar + nFN + width much match
		if (0 == b->nOriginalFunctions() + b->nChildBucketFunctions()) {
			if (0 != f.N()) 
				goto failed ;
			if (0 != b->Width()) 
				goto failed ;
			}
		else if (b->Width() != f.N() + b->nVars()) 
			goto failed ;
		// check children
		for (j = 0 ; j < b->nChildBucketFunctions() ; j++) {
			ARE::Function *cf = b->ChildBucketFunction(j) ;
			if (NULL == cf) 
				goto failed ;
			if (b != cf->BEBucket()) 
				goto failed ;
			if (NULL == cf->OriginatingBucket()) 
				goto failed ;
			if (cf->OriginatingBucket()->ParentBucket() != b) 
				goto failed ;
			}
		}

	return 0 ;
failed :
	fprintf(ARE::fpLOG, "\nERROR : bucket tree integrity check") ;
	fflush(ARE::fpLOG) ;
	return 1 ;
}


int BucketElimination::BEworkspace::CreateBuckets(int N, const int *VariableOrder, bool CreateSuperBuckets)
{
	DestroyBuckets() ;
	if (N < 1) 
		return 0 ;

	int i, j, n, ret = 1 ;

	// ***************************************************************************************************
	// allocate space; initialize
	// ***************************************************************************************************

	_VarOrder = new int[N] ;
	_VarPos = new int[N] ;
	_Var2BucketMapping = new BucketElimination::Bucket*[N] ;
	_BucketOrderToCompute = new long[N] ;
	if (NULL == _VarOrder || NULL == _VarPos || NULL == _Var2BucketMapping || NULL == _BucketOrderToCompute) {
		DestroyBuckets() ;
		return 1 ;
		}
	_nVars = N ;
	for (i = 0 ; i < _nVars ; i++) {
		_VarOrder[i] = VariableOrder[i] ;
		_VarPos[_VarOrder[i]] = i ;
		_Var2BucketMapping[i] = NULL ;
		}

	// ***************************************************************************************************
	// create initial bucket tree structure
	// ***************************************************************************************************

	for (i = 0 ; i < _nVars ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] = new BucketElimination::Bucket(*this, i, _VarOrder[i]) ;
		if (NULL == b) {
			DestroyBuckets() ;
			return 1 ;
			}
		if (1 != b->nVars()) {
			DestroyBuckets() ;
			return 1 ;
			}
		_Var2BucketMapping[_VarOrder[i]] = b ;
		}
	_nBuckets = _nVars ;

	// ***************************************************************************************************
	// create initial function assignment
	// ***************************************************************************************************

	int NF = _Problem->nFunctions() ;
	if (0 == NF) 
		return 0 ;
	for (i = 0 ; i < NF ; i++) {
		ARE::Function *f = _Problem->getFunction(i) ;
		f->SetBEBucket(NULL) ;
		f->SetOriginatingBucket(NULL) ;
		}

	ARE::Function **fl = new ARE::Function*[NF] ;
	if (NULL == fl) 
		return 1 ;

	// process buckets, from last to first
	int nfPlaced = 0 ;
	int nfQirrelevant  = 0 ;
	for (i = 0 ; i < NF ; i++) {
		ARE::Function *f = _Problem->getFunction(i) ;
		if (f->IsQueryIrrelevant()) 
			++nfQirrelevant ;
		}
	for (i = _nBuckets - 1 ; i >= 0 ; i--) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		if (b->nVars() < 1) 
			// this should not happen
			goto failed ;
		// find functions whose highest indexed variable is b->Var(0)
		n = 0 ;
		ARE::ARPGraphNode *node = _Problem->GraphNode(b->Var(0)) ;
		if (NULL == node) 
			goto failed ;
		for (j = 0 ; j < node->nAdjacentFunctions() ; j++) {
			ARE::Function *f = node->AdjacentFunction(j) ;
			if (NULL == f) 
				continue ;
			if (f->IsQueryIrrelevant()) 
				continue ;
			if (NULL != f->BEBucket()) 
				// this means f was placed in a later bucket; e.g. b->Var(0) is not the highest-ordered variable in f.
				continue ;
/*
			int v = f->GetHighestOrderedVariable(_VarPos) ;
			if (v < 0) 
				continue ;
			if (_VarPos[v] != i) 
				// this means b->Var(0) is not the highest-ordered variable in f; e.g. f was placed in a later bucket
				continue ;
*/
			fl[n++] = f ;
			}
/*
		for (j = 0 ; j < NF ; j++) {
			ARE::Function *f = _Problem->Function(j) ;
			if (NULL == f) 
				continue ;
			int v = f->GetHighestOrderedVariable(_VarPos) ;
			if (v < 0) 
				continue ;
			if (_VarPos[v] != i) 
				continue ;
			fl[n++] = f ;
			}
*/
		nfPlaced += n ;
		b->SetOriginalFunctions(n, fl) ;
		}

	// collect all const-functions; add as factor to the workspace
	{
	double neutral_value = (VAR_ELIMINATION_TYPE_SUM_MAX == _FnCombination_VarElimination_Type) ? 0.0 : 1.0 ;
	char combination_type = (VAR_ELIMINATION_TYPE_SUM_MAX == _FnCombination_VarElimination_Type) ? 1 : 0 ; // 0=prod, 1=sum
	ARE_Function_TableType const_value = neutral_value ;
	_nConstValueFunctions = 0 ;
	for (i = 0 ; i < NF ; i++) {
		ARE::Function *f = _Problem->getFunction(i) ;
		if (0 == f->N()) {
			if (0 == combination_type) 
				const_value *= f->ConstValue() ;
			else if (1 == combination_type) 
				const_value += f->ConstValue() ;
			++_nConstValueFunctions ;
			}
		}
	if (fabs(const_value - neutral_value) > 0.000001) 
		AddAnswerFactor(combination_type, const_value) ;

// DEBUGGGG
if (NULL != ARE::fpLOG) {
fprintf(ARE::fpLOG, "\nBEws::CreateBuckets neutral_value=%g combination_type=%d const_value=%g _FnCombination_VarElimination_Type=%d", neutral_value, (int) combination_type, (double) const_value, (int) _FnCombination_VarElimination_Type) ;
fflush(ARE::fpLOG) ;
}
	}

	// test all functions are processed
	if ((nfPlaced + nfQirrelevant + _nConstValueFunctions) != NF) {
		for (i = 0 ; i < NF ; i++) {
			ARE::Function *f = _Problem->getFunction(i) ;
			int break_point_here = 1 ;
			}
		goto failed ;
		}

	// ***************************************************************************************************
	// create a bucket-function for each bucket, from last to first (bottom up on the bucket tree)
	// ***************************************************************************************************

	BucketElimination::Bucket *b ;
	for (i = _nBuckets - 1 ; i >= 0 ; i--) {
		b = _Buckets[i] ;
		if (b->ComputeSignature()) 
			goto failed ;
		if (0 != b->ComputeOutputFunctionWithScopeWithoutTable()) 
			goto failed ;
		}

	// eliminate buckets that have no functions in them
	_nVarsWithoutBucket = 0 ;
	for (i = _nBuckets - 1 ; i >= 0 ; i--) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		if (0 == b->nOriginalFunctions() + b->nChildBucketFunctions()) {
			for (j = 0 ; j < b->nVars() ; j++) {
				_Var2BucketMapping[b->Var(j)] = NULL ;
				++_nVarsWithoutBucket ;
				}
			--_nBuckets ;
			for (j = b->IDX() ; j < _nBuckets ; j++) {
				_Buckets[j] = _Buckets[j+1] ;
				_Buckets[j]->SetIDX(j) ;
				}
			_Buckets[_nBuckets] = NULL ;
			delete b ;
			}
		}

	if (0 != CheckBucketTreeIntegrity()) 
		goto failed ;

	_nBuckets_initial = _nBuckets ;
	_nBucketsWithSingleChild_initial = ComputeNBucketsWithSingleChild() ;

	// ***************************************************************************************************
	// traverse the bucket-tree bottom up; at each step, check if buckets can be merged.
	// in particular, we work with candidate buckets, where a candidate is something 
	// that is either a leaf (in the bucket tree) or has more than 1 child.
	// if a parent of a candidate has no other children (than the candidate), 
	// then the parent can be merged into the candidate.
	// ***************************************************************************************************

	n = 0 ;
	BucketElimination::Bucket *buckets[MAX_NUM_BUCKETS] ;
// DEBUGGGG
CreateSuperBuckets = false ;
	if (CreateSuperBuckets) {
		for (i = 0 ; i < _nBuckets ; i++) {
			BucketElimination::Bucket *b = _Buckets[i] ;
			if (1 == b->nChildBucketFunctions()) 
				continue ;
			BucketElimination::Bucket *B = b->ParentBucket() ;
			if (NULL == B) 
				continue ;
			if (1 != B->nChildBucketFunctions()) 
				continue ;
			buckets[n++] = b ;
			}
		}

//int nMergers = 0 ;

	while (n > 0) {
		BucketElimination::Bucket *b = buckets[--n] ;
		BucketElimination::Bucket *B = b->ParentBucket() ;
		if (NULL == B) 
			continue ;
		// if B has no other children, other than b, B can be eliminated, by merging its original functions into b.
// TODO : check that b does not get too big (in terms of #Vars and PROD-of-domain-sizes-of-Vars).
		if (1 != B->nChildBucketFunctions()) 
			// B has other children
			continue ;
//fprintf(ARE::fpLOG, "\nSUPERBUCKETS adding B=%d to b=%d", B->IDX(), b->IDX()) ;
//fflush(ARE::fpLOG) ;
		// check that adding B to b does not increase complexity too much
		__int64 cmplxty_b = b->ComputeProcessingComplexity() ;
		__int64 cmplxty = cmplxty_b ;
		for (i = 0 ; i < B->Width() ; i++) {
			int vB = (B->Signature())[i] ;
			for (j = 0 ; j < b->Width() ; j++) {
				int vb = (b->Signature())[j] ;
				if (vB == vb) break ;
				}
			if (j < b->Width()) continue ;
			cmplxty *= _Problem->K(vB) ;
			}
		if (cmplxty >= (1 << 20)) {
			__int64 cmplxty_B = B->ComputeProcessingComplexity() ;
			if (cmplxty > (cmplxty_B + cmplxty_b)) {
				// merging would increase complexity too much, above a threshold;
				// don't merge; check B instead.
				if (NULL != ARE::fpLOG) 
					fprintf(ARE::fpLOG, "\nCANNOT merge bucket B=%d into b=%d, complexities would grow from %I64d+%I64d to %I64d", 
					B->IDX(), b->IDX(), cmplxty_B, cmplxty_b, cmplxty) ;
				buckets[n++] = B ;
				continue ;
				}
			}
		// add variables of B to b
		for (i = 0 ; i < B->nVars() ; i++) {
			j = B->Var(i) ;
			if (0 != b->AddVar(j)) 
				goto failed ;
			_Var2BucketMapping[j] = b ;
			}
		// remove b's bucketfunction from B
		B->RemoveChildBucketFunction(b->OutputFunction()) ;
		b->SetParentBucket(NULL) ;
		// add original functions of B to b
		if (b->AddOriginalFunctions(B->nOriginalFunctions(), B->OriginalFunctionsArray())) 
			goto failed ;
		if (b->ComputeSignature()) 
			goto failed ;
		// recompute bucketfunction of b
		if (0 != b->ComputeOutputFunctionWithScopeWithoutTable()) 
			goto failed ;
		// run a check
		if (b->Width() != b->OutputFunction().N() + b->nVars()) 
			{ int error = 1 ; }
		// detach B from its parent
		BucketElimination::Bucket *pB = B->ParentBucket() ;
		if (NULL != pB) 
			pB->RemoveChildBucketFunction(B->OutputFunction()) ;
		B->SetParentBucket(NULL) ;
		// shrink the _Buckets array
		j = _nBuckets - 1 ;
// DEBUGGG
//int BIDX = B->IDX() ;
		for (i = B->IDX() ; i < j ; i++) {
			_Buckets[i] = _Buckets[i+1] ;
			_Buckets[i]->SetIDX(i) ;
			}
		_nBuckets = j ;
		_Buckets[j] = NULL ;
		// delete B
		delete B ;
		// b needs to be rechecked
		buckets[n++] = b ;
// merger; DEBUGGGG
//fprintf(ARE::fpLOG, "\nELIM B=%d", BIDX) ;
//if (++nMergers >= 31) 
//	break ;
		}

	// ***************************************************************************************************
	// check the bucket tree
	// ***************************************************************************************************

	if (0 != CheckBucketTreeIntegrity()) 
		goto failed ;

	// ***************************************************************************************************
	// set root ptr; set distance2root
	// ***************************************************************************************************

	for (i = 0 ; i < _nBuckets ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		BucketElimination::Bucket *B = b->ParentBucket() ;
		if (NULL == B) {
			b->SetDistanceToRoot(0) ;
			b->SetRootBucket(b) ;
			}
		else {
			b->SetDistanceToRoot(B->DistanceToRoot() + 1) ;
			b->SetRootBucket(B->RootBucket()) ;
			}
		b->SetHeight(-1) ;
		}

	// ***************************************************************************************************
	// set height
	// ***************************************************************************************************

	for (i = _nBuckets - 1 ; i >= 0 ; i--) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		BucketElimination::Bucket *B = b->ParentBucket() ;
		if (b->Height() < 0) 
			b->SetHeight(0) ;
		if (NULL != B) {
			int h = b->Height() + 1 ;
			if (B->Height() < h) 
				B->SetHeight(h) ;
			}
		}

	_nBuckets_final = _nBuckets ;
	_nBucketsWithSingleChild_final = ComputeNBucketsWithSingleChild() ;

	ret = 0 ;

failed :
	if (NULL != fl) 
		delete [] fl ;
	return ret ;
}


int BucketElimination::BEworkspace::CreateComputationOrder(void)
{
	int i ;

	if (NULL == _BucketOrderToCompute || _nBuckets < 1) 
		return 1 ;

	// sort by height descending
	long *keys = new long[_nBuckets] ;
	if (NULL == keys) 
		return 1 ;
	for (i = 0 ; i < _nBuckets ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		keys[i] = b->Height() ;
		_BucketOrderToCompute[i] = b->IDX() ;
		}
	long left[32], right[32] ;
	QuickSortLong_Descending(keys, _nBuckets, _BucketOrderToCompute, left, right) ;
	delete [] keys ;

	return 0 ;
}


__int64 BucketElimination::BEworkspace::ComputeNewFunctionSpace(void)
{
	__int64 n = 0 ;
	for (int i = 0 ; i < _nBuckets ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		ARE::Function & f = b->OutputFunction() ;
		n += f.ComputeTableSpace() ;
		}
	return n ;
}


__int64 BucketElimination::BEworkspace::ComputeNewFunctionComputationComplexity(void)
{
	__int64 N = 0 ;
	for (int i = 0 ; i < _nBuckets ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		int w = b->Width() ;
		__int64 n = 1 ;
		const int *vars = b->Signature() ;
		for (int j = 0 ; j < w ; j++) 
			n *= _Problem->K(vars[j]) ;
		N += n ;
		}
	return N ;
}


__int64 BucketElimination::BEworkspace::SimulateComputationAndComputeMinSpace(bool IgnoreInMemoryTables)
{
	int i ;

	// compute table sizes
	for (i = _nBuckets - 1 ; i >= 0 ; i--) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		// add the space of the output function of this bucket
		ARE::Function & f = b->OutputFunction() ;
		f.ComputeTableSize() ;
		for (int j = 0 ; j < b->nChildBucketFunctions() ; j++) {
			ARE::Function *cf = b->ChildBucketFunction(j) ;
			if (NULL != cf) 
				cf->ComputeTableSize() ;
			}
		}

	// compute maximum space for any bucket
	__int64 space = 0, maxbucketspace = 0 ;
	for (i = _nBuckets - 1 ; i >= 0 ; i--) {
		int idx = _BucketOrderToCompute[i] ;
		BucketElimination::Bucket *b = _Buckets[idx] ;
		// add the space of the output function of this bucket
		ARE::Function & f = b->OutputFunction() ;
		__int64 s = f.TableSize() ;
		if (s > 0) 
			space = s ;
		else 
			space = 0 ;
		for (int j = 0 ; j < b->nChildBucketFunctions() ; j++) {
			ARE::Function *cf = b->ChildBucketFunction(j) ;
			if (NULL == cf) continue ;
			if (cf->TableSize() <= 0) continue ;
			space += cf->TableSize() ;
			}
		if (space > maxbucketspace) 
			maxbucketspace = space ;
		}

	space = 0 ;
	__int64 maxspace = 0 ;
	for (i = _nBuckets - 1 ; i >= 0 ; i--) {
		int idx = _BucketOrderToCompute[i] ;
		BucketElimination::Bucket *b = _Buckets[idx] ;
		// add the space of the output function of this bucket
		ARE::Function & f = b->OutputFunction() ;
		__int64 s = f.TableSize() ;
		if (s > 0 && f.nTableBlocks() > 0) {
			space += s ;
			if (space > maxspace) 
				maxspace = space ;
			}
		// subtract the space of this bucket's input functions
		for (int j = 0 ; j < b->nChildBucketFunctions() ; j++) {
			ARE::Function *cf = b->ChildBucketFunction(j) ;
			if (NULL == cf) continue ;
			if (cf->TableSize() <= 0) continue ;
			if (IgnoreInMemoryTables && 0 == cf->nTableBlocks()) 
				// this input table block is in-memory and will not be deleted
				continue ;
			space -= cf->TableSize() ;
			}
		}

	return maxspace ;
}


int BucketElimination::BEworkspace::GetMaxBucketFunctionWidth(void)
{
	int n = 0 ;
	for (int i = 0 ; i < _nBuckets ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		ARE::Function & f = b->OutputFunction() ;
		if (f.N() > n) n = f.N() ;
		}
	return n ;
}


int BucketElimination::BEworkspace::ComputeMaxNumVarsInBucket(void)
{
	_MaxNumVarsInBucket = 0 ;
	for (int i = 0 ; i < _nBuckets ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		if (b->Width() > _MaxNumVarsInBucket) 
			_MaxNumVarsInBucket = b->Width() ;
		}
	return _MaxNumVarsInBucket ;
}


int BucketElimination::BEworkspace::ComputeNBucketsWithSingleChild(void)
{
	int n = 0 ;
	for (int i = 0 ; i < _nBuckets ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		if (1 == b->nChildBucketFunctions()) 
			n++ ;
		}
	return n ;
}


int BucketElimination::BEworkspace::ComputeMaxNumChildren(void)
{
	_MaxNumChildren = 0 ;
	for (int i = 0 ; i < _nBuckets ; i++) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		if (b->nChildBucketFunctions() > _MaxNumChildren) 
			_MaxNumChildren = b->nChildBucketFunctions() ;
		}
	return _MaxNumChildren ;
}


int BucketElimination::BEworkspace::RunSimple(char FnCombination_VarElimination_Type)
{
	int i ;

	// mark all bucket functions as in-memory
	for (i = _nBuckets - 1 ; i >= 0 ; i--) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		ARE::Function & f = b->OutputFunction() ;
		b->AllocateOutputFunctionBlockComputationResult(1000000, 0) ;
		f.ComputeTableSize() ;
		f.SetnTableBlocks(0) ;
		f.AllocateInMemoryTableBlock() ;
		}

	// compute all bucket functions
	ARE::Function *MissingFunction ;
	__int64 MissingBlockIDX ;
	for (i = _nBuckets - 1 ; i >= 0 ; i--) {
		BucketElimination::Bucket *b = _Buckets[i] ;
		ARE::Function & f = b->OutputFunction() ;
		ARE::FunctionTableBlock *ftb = f.Table() ;
		if (NULL == ftb) 
			// this is error
			continue ;
		if (VAR_ELIMINATION_TYPE_PROD_SUM == FnCombination_VarElimination_Type) 
			ftb->ComputeDataBEEM_ProdSum(MissingFunction, MissingBlockIDX) ;
		else if (VAR_ELIMINATION_TYPE_PROD_MAX == FnCombination_VarElimination_Type) 
			ftb->ComputeDataBEEM_ProdMax(MissingFunction, MissingBlockIDX) ;
		else if (VAR_ELIMINATION_TYPE_SUM_MAX == FnCombination_VarElimination_Type) 
			ftb->ComputeDataBEEM_SumMax(MissingFunction, MissingBlockIDX) ;
		}

	return 0 ;
}


int BucketElimination::BEworkspace::GenerateRandomBayesianNetworkStructure(int N, int K, int P, int C, int ProblemCharacteristic)
{
	if (NULL == _Problem) 
		return 1 ;
	_Problem->Destroy() ;

	if (0 != _Problem->GenerateRandomUniformBayesianNetworkStructure(N, K, P, C, ProblemCharacteristic)) 
		{ return 1 ; }

	if (0 != _Problem->ComputeGraph()) 
		{ return 1 ; }
	int nComponents = _Problem->nConnectedComponents() ;
	if (nComponents > 1) 
		{ return 1 ; }

	if (0 != _Problem->ComputeMinDegreeOrdering()) 
		{ return 1 ; }

	int width = _Problem->MinDegreeOrdering_InducedWidth() ;
//	if (width < MinWidth || width > MaxWidth) 
//		{ return 1 ; }

	if (0 != _Problem->TestVariableOrdering(_Problem->MinDegreeOrdering_VarList(), _Problem->MinDegreeOrdering_VarPos())) 
		{ return 1 ; }

	if (0 != CreateBuckets(_Problem->N(), _Problem->MinDegreeOrdering_VarList(), true)) 
		{ return 1 ; }

	__int64 spaceO = _Problem->ComputeFunctionSpace() ;
	__int64 spaceN = ComputeNewFunctionSpace() ;
	__int64 space = spaceO + spaceN ;
//	if (space < MinMemory || space > MaxMemory) 
//		{ return 1 ; }

	return 0 ;
}


int BucketElimination::GenerateRandomBayesianNetworksWithGivenComplexity(int nProblems, int N, int K, int P, int C, int ProblemCharacteristic, __int64 MinSpace, __int64 MaxSpace)
{
	ARE::ARP p("test") ;
	// create BEEM workspace; this includes BE workspace.
	BEworkspace ws(NULL) ;
	ws.Initialize(p, NULL) ;

	time_t ttNow, ttBeginning ;
	time(&ttBeginning) ;
	time_t dMax = 3600 ;

	char s[256] ;
	int i ;
	for (i = 0 ; i < nProblems ; ) {
		time(&ttNow) ;
		time_t d = ttNow - ttBeginning ;
		if (d >= dMax) 
			break ;

		ws.DestroyBuckets() ;
		ws.GenerateRandomBayesianNetworkStructure(N, K, P, C, ProblemCharacteristic) ;

		if (p.nConnectedComponents() > 1) 
			continue ;

		int width = p.MinDegreeOrdering_InducedWidth() ;
		__int64 spaceO = p.ComputeFunctionSpace() ;
		__int64 spaceN = ws.ComputeNewFunctionSpace() ;
		__int64 space = spaceO + spaceN ;

		if (space < MinSpace || space > MaxSpace) 
			continue ;

		// generate nice name
		sprintf(s, "random-test-problem-%d-Space=%I64d", (int) ++i, space) ;
		p.SetName(s) ;

		// fill in original functions
		p.FillInFunctionTables() ;
		if (p.CheckFunctions()) 
			{ int error = 1 ; }

		// save UAI08 format
		p.SaveUAI08("c:\\UCI\\problems") ;
		}

	return i ;
}

