#include <stdlib.h>
#include <float.h>

#include "Globals.hxx"

#include "Sort.hxx"
#include "MersenneTwister.h"
#include "Problem.hxx"
#include "Function.hxx"
#include "FunctionTableBlock.hxx"
#include "Bucket.hxx"

static MTRand RNG ;

int ARE::Function::GetTableBinaryFilename(const std::string & Dir, std::string & fn)
{
	if (NULL == _Problem) 
		return 1 ;
	if (0 == Dir.length()) 
		return 1 ;
	fn = Dir ;
	if ('\\' != fn[fn.length()-1]) 
		fn += '\\' ;

	char s[256] ;
	sprintf(s, "%d", _IDX) ;
	fn += s ;
	fn += ".tbl" ;

	return 0 ;
}


int ARE::Function::SaveTableBinary(const std::string & Dir)
{
	if (_TableSize < 1 || NULL == _Table) 
		return 0 ;

	std::string fn ;
	if (0 != GetTableBinaryFilename(Dir, fn)) 
		return 1 ;
	if (0 == fn.length()) 
		return 1 ;

	FILE *fp = fopen(fn.c_str(), "wb") ;
	if (NULL == fp) 
		return 1 ;
	int sz = sizeof(ARE_Function_TableType)*_TableSize ;
	if (sz == fwrite(_Table, 1, sz, fp)) 
		_FileName = fn ;
	fclose(fp) ;

/*
	// this is debug code; it will load the table just saved and check that it matches this table.
	if (true) {
		double t[32000] ;
		fp = fopen(fn.c_str(), "rb") ;
		fread(t, 1, sz, fp) ;
		fclose(fp) ;
		if (0 != memcmp(t, _Table, sz)) {
			int here = 1 ;
			for (int j = 0 ; j < _TableSize ; j++) {
				if (0 != _isnan(_Table[j]) || 0 == _finite(_Table[j])) {
					int error = 1 ;
					}
				}
			for (int i = 0 ; i < _TableSize ; i++) {
				if (t[i] != _Table[i]) {
					int heee = 1 ;
					}
				}
			}
		}
*/

	return 0 ;
}


int ARE::Function::SaveXMLString(const char *prefixspaces, const char *tag, const std::string & Dir, std::string & S)
{
	char s[1024] ;
	int i ;
	if (_IDX >= 0) {
//		GetTableBinaryFilename(Dir, fn) ;
		sprintf(s, "%s<%s IDX=\"%d\" type=\"%s\" nArgs=\"%d\" scope=\"", prefixspaces, tag, _IDX, _Type.c_str(), _nArgs) ;
		}
	else 
		sprintf(s, "%s<%s type=\"%s\" nArgs=\"%d\" scope=\"", prefixspaces, tag, _Type.c_str(), _nArgs) ;
	S += s ;
	for (i = 0 ; i < _nArgs ; i++) {
		sprintf(s, "%d", _Arguments[i]) ;
		if (i > 0) 
			S += ';' ;
		S += s ;
		}
	sprintf(s, "\" tablecelltype=\"%s\"", ARE_Function_TableTypeString) ;
	S += s ;
	if (_TableSize >= 0) {
		sprintf(s, " TableSize=\"%I64d\"", _TableSize) ;
		S += s ;
		}
	if (_nTableBlocks >= 0) {
		sprintf(s, " nTableBlocks=\"%I64d\"", _nTableBlocks) ;
		S += s ;
		}
	if (_nTableBlocks >= 0) {
		sprintf(s, " TableBlockSize=\"%I64d\"", _TableBlockSize) ;
		S += s ;
		}
	if (_FileName.length() > 0) {
		sprintf(s, " filename=\"%s\"", _FileName.c_str()) ;
		S += s ;
		}
	S += "/>" ;

	return 0 ;
}


int ARE::Function::SetArguments(int N, int *Arguments, int ExcludeThisVar)
{
	_BayesianCPTChildVariable = -1 ;

	int i, n = N ;
	if (ExcludeThisVar >= 0) {
		for (i = 0 ; i < N ; i++) {
			if (Arguments[i] == ExcludeThisVar) 
				n-- ;
			}
		}

	if (n != _nArgs) {
		if (NULL != _Arguments) {
			delete [] _Arguments ;
			_Arguments = NULL ;
			}
		_nArgs = 0 ;
		}

	if (n <= 0) 
		return 0 ;

	if (NULL == _Arguments) {
		_Arguments = new int[n] ;
		if (NULL == _Arguments) 
			return 1 ;
		_nArgs = n ;
		}

	if (ExcludeThisVar < 0) 
		memcpy(_Arguments, Arguments, _nArgs*sizeof(int)) ;
	else {
		n = 0 ;
		for (i = 0 ; i < N ; i++) {
			if (Arguments[i] != ExcludeThisVar) 
				_Arguments[n++] = Arguments[i] ;
			}
		}
	if (ARE_Function_Type_BayesianCPT == _Type) 
		_BayesianCPTChildVariable = _Arguments[n-1] ;

	return 0 ;
}

/*
int ARE::Function::FactorizeArgumentDomains(void)
{
	if (NULL == _Problem) 
		return 1 ;
	_nArgumentDomainFactorization = 0 ;
	long F[MAX_NUM_FACTORS_PER_FN_DOMAIN] ;
	long N = 0 ;
	char f[128] ;
	int i, j ;
	for (i = _nArgs-2 ; i >= 0 ; i--) {
		char n = PrimeFactoringByTrialDivision(_Problem->K(_Arguments[i]), f) ;
		if ((N + n) > MAX_NUM_FACTORS_PER_FN_DOMAIN) 
			return 1 ;
		for (j = 0 ; j < n ; j++, N++) 
			F[N] = f[j] ;
		}
	QuickSortLong2(F, N) ;
	for (i = 0 ; i < N ; i++) 
		_ArgumentDomainFactorization[i] = F[i] ;
	_nArgumentDomainFactorization = N ;
	return 0 ;
}
*/


int ARE::Function::ConvertTableToLog(void)
{
	if (_ConstValue >= 0.0) {
		_ConstValue = log10(_ConstValue) ;
		if (_ConstValue > 0.0 && NULL != ARE::fpLOG) {
			fprintf(ARE::fpLOG, "\nFunction::ConvertTableToLog(): ERROR idx=%d ConstValue=%g (should be <= 0)", (int) _IDX, (double) _ConstValue) ;
			fflush(ARE::fpLOG) ;
			}
		}
	if (NULL == _Table) 
		return 0 ;
	ARE_Function_TableType *data = _Table->Data() ;
	if (NULL == data) 
		return 0 ;
	for (int i = 0 ; i < _Table->Size() ; i++) {
		data[i] = log10(data[i]) ;
		if (data[i] > 0.0 && NULL != ARE::fpLOG) {
			fprintf(ARE::fpLOG, "\nFunction::ConvertTableToLog(): ERROR idx=%d data[%d]=%g (should be <= 0)", (int) _IDX, i, (double) data[i]) ;
			fflush(ARE::fpLOG) ;
			}
		}
	return 0 ;
}


__int64 ARE::Function::ComputeTableSize(void) 
{
	if (_nArgs < 1) {
		_TableSize = 0 ;
		return 0 ;
		}
	_TableSize = 1 ;
	for (int i = 0 ; i < _nArgs ; i++) {
		_TableSize *= _Problem->K(_Arguments[i]) ;
		if (_TableSize < 0) {
			// overflow ?
			_TableSize = -1 ;
			return _TableSize ;
			}
		}
	return _TableSize ;
}


__int64 ARE::Function::ComputeTableSpace(void) 
{
	ComputeTableSize() ;
	if (_TableSize <= 0) 
		return 0 ;
	return sizeof(ARE_Function_TableType)*_TableSize ;
}

/*
int ARE::Function::GenerateRandomBayesianSignature(int N, int ChildIDX, int & nFVL, int *FreeVariableList)
{
	return 0 ;
}
*/

int ARE::Function::GenerateRandomBayesianSignature(int N, int ChildIDX)
{
	if (ARE_Function_Type_BayesianCPT != _Type) 
		return 1 ;
	if (N < 1 || ChildIDX < 0) 
		return 1 ;
	if (ChildIDX < N-1) 
		// cannot pick N-1 parents for the CPT
		return 1 ;

	// indeces run [0,(Problem->N)-1]; 
	// we assume that children of CPTs are from [P,(Problem->N)-1] and the parents of a CPT are [0,ChildIDX-1].

	int i, j ;

	Destroy() ;
	_nArgs = N ;

	_Arguments = new int[_nArgs] ;
	if (NULL == _Arguments) goto failed ;
	for (i = 0 ; i < _nArgs ; i++) 
		_Arguments[i] = -1 ;

	// place the child as the last argument
	_Arguments[_nArgs-1] = ChildIDX ;
	_BayesianCPTChildVariable = ChildIDX ;

	if (_nArgs > 1) {
		int number_of_parents = _nArgs-1 ;
		if (number_of_parents == ChildIDX) {
			for (i = 0 ; i < number_of_parents ; i++) 
				_Arguments[i] = i ;
			}
		else {
			for (i = 0 ; i < number_of_parents ;) {
				_Arguments[i] = RNG.randInt(ChildIDX-1) ;
				// check that this parent is not already generated
				for (j = 0 ; j < i ; j++) {
					if (_Arguments[j] == _Arguments[i]) break ;
					}
				if (j == i) i++ ;
				}
			}
		}

	ComputeTableSize() ;

	return 0 ;
failed :
	Destroy() ;
	return 1 ;
}


int ARE::Function::FillInRandomBayesianTable(void)
{
	if (ARE_Function_Type_BayesianCPT != _Type) 
		return 1 ;
	if (NULL == _Arguments || _nArgs < 1) 
		return 1 ;

	// indeces run [0,(Problem->N)-1]; 
	// we assume that children of CPTs are from [P,(Problem->N)-1] and the parents of a CPT are [0,ChildIDX-1].

	int i, j ;

	// compute table size; allocate table
	ComputeTableSize() ;
//	DestroyTable() ;
	if (0 != AllocateInMemoryTableBlock()) 
		{ Destroy() ; return 1 ; }
	ARE_Function_TableType *table = _Table->Data() ;
	if (NULL == table) 
		{ Destroy() ; return 1 ; }
	int number_of_parents = _nArgs - 1 ;
	int ChildIDX = _Arguments[number_of_parents] ;

	// generate table contents
	int parent_value_combination_array[MAX_NUM_ARGUMENTS_PER_FUNCTION] ; // up to 'MAX_NUM_ARGUMENTS_PER_FUNCTION' parents allowed
	if (number_of_parents > 0) {
		for (j = 0 ; j < number_of_parents - 1 ; j++) parent_value_combination_array[j] = 0 ;
		parent_value_combination_array[number_of_parents - 1] = -1 ;
		}
	// process all parent value combinations
	int k = _Problem->K(ChildIDX) ;
	for (j = 0 ; j < _TableSize ; j += k) {
		// enumerate parent value combination. find next combination from the current.
		for (i = number_of_parents - 1 ; i >= 0 ; i--) {
			if (++parent_value_combination_array[i] < _Problem->K(_Arguments[i])) break ;
			parent_value_combination_array[i] = 0 ;
			}

		ARE_Function_TableType x = (ARE_Function_TableType) 0.0 ; // normalization constant
		ARE_Function_TableType p_of_x ;
		for (i = 0 ; i < k ; i++) {
			p_of_x = 0.0001 + RNG.rand() ;
			x += table[j + i] = (ARE_Function_TableType) p_of_x ;
			}
		// check if all zeros. add some constant to x and to the prob-matrix.
		if (x < 1.0e-15) {
			x += (ARE_Function_TableType) 0.1 ;
			table[RNG.randInt(k-1)] += (ARE_Function_TableType) 0.1 ;
			}
		for (i = 0 ; i < k ; i++) table[j + i] /= x ;
		}

	return 0 ;
failed :
	Destroy() ;
	return 1 ;
}


int ARE::Function::CheckTable(void)
{
	if (NULL != _Table) 
		return _Table->CheckData() ;
	return 0 ;
}


int ARE::Function::CheckIntegrity(void)
{
	if (NULL == _Problem) 
		return 1 ;

	int i, n = _Problem->N() ;
	for (i = 0 ; i < _nArgs ; i++) {
		if (_Arguments[i] < 0 || _Arguments[i] >= n) 
			return 2 ;
		}

	if (NULL != _Table) 
		return _Table->CheckData() ;

	return 0 ;
}


void ARE::Function::DestroyFTBList(void)
{
	// release all disk-blocks; lock disk-block list.
	if (NULL != _Workspace) {
//		ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;
		while (NULL != _FTBsInMemory) {
			FunctionTableBlock *ftb = _FTBsInMemory ;
			_FTBsInMemory = _FTBsInMemory->NextFTBInFunction() ;
			ftb->AttachNextFTBInFunction(NULL) ;
			ftb->AttachReferenceFunction(NULL) ;
			delete ftb ;
			}
		}
}


void ARE::Function::DestroyTable(void)
{
	if (NULL != _Table) {
		delete _Table ;
		_Table = NULL ;
		}
}


int ARE::Function::AllocateInMemoryTableBlock(void)
{
	// release all disk-blocks; lock disk-block list.
	if (NULL != _Workspace) {
//		ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;
		while (NULL != _FTBsInMemory) {
			FunctionTableBlock *ftb = _FTBsInMemory ;
			_FTBsInMemory = _FTBsInMemory->NextFTBInFunction() ;
			ftb->AttachNextFTBInFunction(NULL) ;
			delete ftb ;
			}
		}

	// make sure table size is known
	_TableSize = ComputeTableSize() ;
	// since table is allocated in memory, nBlocks and Size are 0.
	_nTableBlocks = _TableBlockSize = 0 ;

	// if no table, make sure it does not exist
	if (0 == _TableSize) {
		if (NULL != _Table) { delete _Table ; _Table = NULL ; }
		return 0 ;
		}

	// if a table of the right size already exists, keep it.
	if (NULL != _Table) {
		if (_Table->Size() != _TableSize || 0 != _Table->IDX()) {
			delete _Table ;
			_Table = NULL ;
			}
		}

	if (NULL == _Table) {
		_Table = new FunctionTableBlock(this) ;
		if (NULL == _Table) 
			return 1 ;
		}
	_Table->Initialize(this, 0) ;

	// allocate data block
	if (NULL == _Table->Data()) {
		if (0 != _Table->AllocateData()) {
			delete _Table ;
			_Table = NULL ;
			return 1 ;
			}
		}

	return 0 ;
}


int ARE::Function::DestroyInMemoryTableBlock(void)
{
	// make sure table size is known
	if (_TableSize < 0) 
		_TableSize = ComputeTableSize() ;

	if (0 == _nTableBlocks) 
		_nTableBlocks = _TableBlockSize = 0 ;
	if (NULL != _Table) { delete _Table ; _Table = NULL ; }
	return 0 ;
}


int ARE::Function::ReOrderArguments(int nAF, const int *AF, int nAB, const int *AB)
{
	if (_nArgs < 1 || NULL == _Arguments) 
		return 0 ;
	if (_nArgs > MAX_NUM_VARIABLES_PER_BUCKET) 
		return 1 ;

	// compute new order
//	int *neworder = new int[_nArgs << 1] ;
//	if (NULL == neworder) 
//		return 1 ;
	int neworder[MAX_NUM_VARIABLES_PER_BUCKET] ;
//	int *new2old_mapping = neworder + _nArgs ;
	int new2old_mapping[MAX_NUM_VARIABLES_PER_BUCKET] ;
	int i, j, b = 0, e = _nArgs ;
	// separate variables of _Arguments into : in AF vs. in AB
	for (i = 0 ; i < _nArgs ; i++) {
		int a = _Arguments[i] ;
		for (j = 0 ; j < nAF ; j++) {
			if (AF[j] == a) 
				break ;
			}
		if (j < nAF) 
			// a is in AF
			neworder[b++] = a ;
		else 
			// a is in AB (or must be in AB)
			neworder[--e] = a ;
		}
	// it should be that b==e; neworder[0,e) are arguments in _Arguments that are in AF, neworder[e,_nArgs) are arguments in _Arguments that are in AB.
	// sort neworder[0,e)/neworder[e,_nArgs) so that it agrees with the variable order in AF/AB.
	b = 0 ;
	for (i = 0 ; i < nAF ; i++) {
		int a = AF[i] ;
		for (j = b ; j < e ; j++) {
			if (neworder[j] == a) 
				break ;
			}
		if (j >= e) 
			// a is not in _Arguments[]
			continue ;
		// place a in the next position (pointed to by 'b') in neworder by swapping
		if (b != j) {
			neworder[j] = neworder[b] ;
			neworder[b] = a ;
			}
		++b ;
		}
	b = e ;
	for (i = 0 ; i < nAB ; i++) {
		int a = AB[i] ;
		for (j = b ; j < _nArgs ; j++) {
			if (neworder[j] == a) 
				break ;
			}
		if (j >= _nArgs) 
			// a is not in _Arguments[]
			continue ;
		// place a in the next position (pointed to by 'b') in neworder by swapping
		if (b != j) {
			neworder[j] = neworder[b] ;
			neworder[b] = a ;
			}
		++b ;
		}

	// check if order has changed
	if (0 == memcmp(_Arguments, neworder, _nArgs*sizeof(int))) {
//		delete [] neworder ;
		return 0 ;
		}

#ifdef _DEBUG
	if (NULL != ARE::fpLOG) {
		char s[256] ;
		if (NULL == _OriginatingBucket) 
			sprintf(s, "\n   o function %d arguments are being reordered (bucket=%d)", (int) _IDX, (int) _BEBucket->IDX()) ;
		else 
			sprintf(s, "\n   n function %d arguments are being reordered (bucket=%d)", (int) _OriginatingBucket->IDX(), (int) _BEBucket->IDX()) ;
		fwrite(s, 1, strlen(s), ARE::fpLOG) ;
		fflush(ARE::fpLOG) ;
		}
#endif // _DEBUG

	// compute new2old_mapping = for each var in the new ordering, its position in the old (current) ordering
	for (i = 0 ; i < _nArgs ; i++) {
		int a = neworder[i] ;
		for (j = 0 ; j < _nArgs ; j++) {
			if (_Arguments[j] == a) { new2old_mapping[i] = j ; break ; }
			}
		}

	// fix table
	ARE_Function_TableType *newtable = NULL ;
	ARE_Function_TableType *oldtable = (NULL != _Table) ? _Table->Data() : NULL ;
	if (_TableSize > 0 && NULL != oldtable) {
		newtable = new ARE_Function_TableType[_TableSize] ;
		if (NULL == newtable) {
//			delete [] neworder ;
			return 1 ;
			}

		// enumerate all combinations of arguments, using new order.
		// for each combination, compute adr in old table and pull value

		static int value_combination_array_NEW[MAX_NUM_ARGUMENTS_PER_FUNCTION] ; // up to 'MAX_NUM_ARGUMENTS_PER_FUNCTION' arguments allowed
		static int value_combination_array_OLD[MAX_NUM_ARGUMENTS_PER_FUNCTION] ; // up to 'MAX_NUM_ARGUMENTS_PER_FUNCTION' arguments allowed
		for (j = 0 ; j < _nArgs ; j++) value_combination_array_NEW[j] = 0 ;
		value_combination_array_NEW[_nArgs - 1] = -1 ;
		__int64 oldadr ;
		for (j = 0 ; j < _TableSize ; j++) {
			// enumerate value combination. find next combination from the current.
			EnumerateNextArgumentsValueCombination(_nArgs, neworder, value_combination_array_NEW, _Problem->K()) ;
			// convert new combination into old combination
			for (i = 0 ; i < _nArgs ; i++) 
				value_combination_array_OLD[new2old_mapping[i]] = value_combination_array_NEW[i] ;

			// fetch the value of the table cell from the old table and store in the new table
			oldadr = ARE::ComputeFnTableAdr(_nArgs, _Arguments, value_combination_array_OLD, _Problem->K()) ;
//			if (oldadr < 0 || oldadr >= _TableSize) 
//				{ int bad = 1 ; }
			newtable[j] = oldtable[oldadr] ;
			}

		_Table->SetData(_TableSize, newtable) ;
		}

	// install new arguments list
	memcpy(_Arguments, neworder, _nArgs*sizeof(int)) ;
//	delete [] neworder ;

	return 0 ;
}


int ARE::Function::RemoveVariable(int Var, int Val)
{
	if (_nArgs < 1 || NULL == _Arguments) 
		return 0 ;

	// compute new args array
	int neworder[MAX_NUM_VARIABLES_PER_BUCKET] ;
	int new2old_mapping[MAX_NUM_VARIABLES_PER_BUCKET] ;
	int i, j ;
	int n = 0 ;
	int VarIdx = -1 ;
	for (i = 0 ; i < _nArgs ; i++) {
		int a = _Arguments[i] ;
		if (a == Var) 
			VarIdx = i ;
		else 
			neworder[n++] = a ;
		}
	if (VarIdx < 0) 
		return 1 ;

	if (n < 1) {
		// this fn is a const-value function
		if (NULL != _Table) 
			_ConstValue = _Table->Entry(Val) ;
		else 
			_ConstValue = 1.0 ;
		return 0 ;
		}

	// compute new2old_mapping = for each var in the new ordering, its position in the old (current) ordering
	for (i = 0 ; i < n ; i++) {
		int a = neworder[i] ;
		for (j = 0 ; j < _nArgs ; j++) {
			if (_Arguments[j] == a) { new2old_mapping[i] = j ; break ; }
			}
		}

	// dump all table blocks
	DestroyFTBList() ;
	__int64 newtablesize = 1 ;
	for (i = 0 ; i < n ; i++) 
		newtablesize *= _Problem->K(neworder[i]) ;

	// fix table
	ARE_Function_TableType *newtable = NULL ;
	ARE_Function_TableType *oldtable = (NULL != _Table) ? _Table->Data() : NULL ;
	if (_TableSize > 0 && NULL != oldtable) {
		newtable = new ARE_Function_TableType[newtablesize] ;
		if (NULL == newtable) 
			return 1 ;

		// enumerate all combinations of arguments, using new order.
		// for each combination, compute adr in old table and pull value

		static int value_combination_array_NEW[MAX_NUM_ARGUMENTS_PER_FUNCTION] ; // up to 'MAX_NUM_ARGUMENTS_PER_FUNCTION' arguments allowed
		static int value_combination_array_OLD[MAX_NUM_ARGUMENTS_PER_FUNCTION] ; // up to 'MAX_NUM_ARGUMENTS_PER_FUNCTION' arguments allowed
		value_combination_array_OLD[VarIdx] = Val ;
		for (j = 0 ; j < n ; j++) value_combination_array_NEW[j] = 0 ;
		value_combination_array_NEW[n - 1] = -1 ;
		__int64 oldadr ;
		for (j = 0 ; j < newtablesize ; j++) {
			// enumerate value combination. find next combination from the current.
			EnumerateNextArgumentsValueCombination(n, neworder, value_combination_array_NEW, _Problem->K()) ;
			// convert new combination into old combination
			for (i = 0 ; i < n ; i++) 
				value_combination_array_OLD[new2old_mapping[i]] = value_combination_array_NEW[i] ;

			// TODO : if var elimination by summation is needed, do it here

			// fetch the value of the table cell from the old table and store in the new table
			oldadr = ARE::ComputeFnTableAdr(_nArgs, _Arguments, value_combination_array_OLD, _Problem->K()) ;
//			if (oldadr < 0 || oldadr >= _TableSize) 
//				{ int bad = 1 ; }
			newtable[j] = oldtable[oldadr] ;
			}

		_Table->SetData(newtablesize, newtable) ;
		}

	// install new arguments list
	for (i = VarIdx ; i < n ; i++) 
		_Arguments[i] = _Arguments[i+1] ;
	--_nArgs ;

	return 0 ;
}


int ARE::Function::RemoveVariableValue(int Var, int Val)
{
	if (_nArgs < 1 || NULL == _Arguments) 
		return 0 ;

	int i, j ;

	// dump all table blocks
	DestroyFTBList() ;
	__int64 newtablesize = 1 ;
	int VarIdx = -1 ;
	for (i = 0 ; i < _nArgs ; i++) {
		int a = _Arguments[i] ;
		if (a == Var) {
			VarIdx = i ;
			newtablesize *= _Problem->K(a) - 1 ;
			}
		else 
			newtablesize *= _Problem->K(a) ;
		}
	if (VarIdx < 0) 
		// this should not happen
		return 1 ;
	if (newtablesize < 1) {
		// domain of the variable is empty; function is invalid
		_Table->SetData(0, NULL) ;
		return 0 ;
		}

	// fix table
	ARE_Function_TableType *newtable = NULL ;
	ARE_Function_TableType *oldtable = (NULL != _Table) ? _Table->Data() : NULL ;
	int *newDomainSizes = NULL ;
	if (_TableSize > 0 && NULL != oldtable) {
		newDomainSizes = new int[_Problem->N()] ;
		if (NULL == newDomainSizes) 
			return 1 ;
		newtable = new ARE_Function_TableType[newtablesize] ;
		if (NULL == newtable) {
			delete [] newDomainSizes ;
			return 1 ;
			}

		// enumerate all combinations of arguments; copy all combinations from old table to new table, unless Var=Val.

		static int value_combination_array_OLD[MAX_NUM_ARGUMENTS_PER_FUNCTION] ; // up to 'MAX_NUM_ARGUMENTS_PER_FUNCTION' arguments allowed
		static int value_combination_array_NEW[MAX_NUM_ARGUMENTS_PER_FUNCTION] ; // up to 'MAX_NUM_ARGUMENTS_PER_FUNCTION' arguments allowed
		static int domains_NEW[MAX_NUM_VALUES_PER_VAR_DOMAIN] ;
		for (j = 0 ; j < _nArgs ; j++) value_combination_array_OLD[j] = 0 ;
		value_combination_array_OLD[_nArgs - 1] = -1 ;
		for (j = 0 ; j < _Problem->N() ; j++) 
			newDomainSizes[j] = _Problem->K(j) ;
		newDomainSizes[Var] = _Problem->K(Var) - 1 ;
		__int64 oldadr, newadr ;
		for (j = 0 ; j < _TableSize ; j++) {
			// enumerate (old) value combination. find next combination from the current.
			EnumerateNextArgumentsValueCombination(_nArgs, _Arguments, value_combination_array_OLD, _Problem->K()) ;
			if (Val == value_combination_array_OLD[VarIdx]) 
				// Var=Val; skip this combination.
				continue ;

			// create corresponding new table combination
			for (i = 0 ; i < _nArgs ; i++) 
				value_combination_array_NEW[i] = value_combination_array_OLD[i] ;
			if (value_combination_array_NEW[VarIdx] > Val) 
				value_combination_array_NEW[VarIdx]-- ;

			// fetch the value of the table cell from the old table and store in the new table
			oldadr = ARE::ComputeFnTableAdr(_nArgs, _Arguments, value_combination_array_OLD, _Problem->K()) ;
			newadr = ARE::ComputeFnTableAdr(_nArgs, _Arguments, value_combination_array_NEW, newDomainSizes) ;
			newtable[newadr] = oldtable[oldadr] ;
			}

		_Table->SetData(newtablesize, newtable) ;
		}

	if (NULL == newtable) {
		delete [] newtable ;
		return 1 ;
		}
	if (NULL == newDomainSizes) {
		delete [] newDomainSizes ;
		return 1 ;
		}

	return 0 ;
}


int ARE::Function::ComputeTableBlockSize(__int64 MaxBlockSize, int nComputingThreads)
{
	if (MaxBlockSize < 32) 
		MaxBlockSize = 32 ;

	if (NULL != _Table) {
		// this table is in memory; no need for table blocks
		_nTableBlocks = 0 ;
		_TableBlockSize = 0 ;
		return 0 ;
		}
	if (_nArgs <= 0) {
		// this fn has no arguments
		_nTableBlocks = 0 ;
		_TableBlockSize = 0 ;
		return 0 ;
		}
	if (NULL == _Arguments) 
		// this should not happen
		return 1 ;
	_TableSize = ComputeTableSize() ;
	if (_TableSize <= 0) 
		// this should not happen
		return 0 ;

	if (nComputingThreads < 1) {
		// this must mean that table is held in-memory; use just one block.
		_nTableBlocks = 0 ;
		_TableBlockSize = 0 ;
		return 0 ;
		}

	// NOTES : if the table size is trivial (e.g. 287 bytes), use just one block in-memory.
	__int64 TrivialBlockSize = 8192 ;
	if (TrivialBlockSize > MaxBlockSize) 
		TrivialBlockSize = MaxBlockSize ;
	if (_TableSize <= TrivialBlockSize) {
		_nTableBlocks = 0 ;
		_TableBlockSize = 0 ;
		return 0 ;
		}
	// otherwise, if the table size is non-trivial, brake it into blocks such that the number of blocks is at least as large as number of threads

	int i, j ;

	// NOTE : we don't want to break the table (into blocks) in the middle.
	// wa want to break it as close to the highest order var as possible.
	// e.g. these two arguments combinations should be in the same block
	// 0 2 1 0 1 0 1 1 0 2 0 0 0 0 adr = 131072, block 2
	// 0 2 1 0 1 0 1 1 0 0 0 0 0 0 adr = 130944, block 1
	// since they differ in argument 4, a parent function that needs this child function, 
	// refers to both of these argument combinations within on block, 
	// and thus when computing 1 parent FTB, child FTB needs to be swapped in/out all the time.

/*
	// factorize argument domains
	if (_nArgs > 1) {
		if (_nArgumentDomainFactorization < 1 ? 0 != FactorizeArgumentDomains() : false) 
			return 1 ;
		}
	else 
		_nArgumentDomainFactorization = 0 ;
	__int64 check = _Problem->K(_Arguments[_nArgs-1]) ;
	for (i = 0 ; i < _nArgumentDomainFactorization ; i++) 
		check *= _ArgumentDomainFactorization[i] ;
	if (check != _TableSize) 
		return 1 ;
*/

	// make a local copy of domain sizes; keep start/end indeces (as inclusive).
	// we will work the domain size array.
	int iS = 0, iE = _nArgs-2 ;
	int domains[MAX_NUM_ARGUMENTS_PER_FUNCTION] ;
	for (i = iS ; i <= iE ; i++) 
		domains[i] = _Problem->K(_Arguments[i]) ;
	_nTableBlocks = 1 ;
	_TableBlockSize = _Problem->K(_Arguments[_nArgs-1]) ;

	// make sure block size is at least the size of a trivial block
	for (; iE >= iS && _TableBlockSize < TrivialBlockSize ; iE--) 
		_TableBlockSize *= domains[iE] ;

	// if more than 1 thread, make sure that there are enough blocks vs NumThreads.
	if (nComputingThreads > 1) {
		for (; iS <= iE ; iS++) {
			char f[128] ;
			char n = PrimeFactoringByTrialDivision(domains[iS], f) ;
			if (n < 1) 
				return 1 ;
			for (j = 0 ; j < n ; j++) {
				_nTableBlocks *= f[j] ;
				if (_nTableBlocks > nComputingThreads) 
					break ;
				}
			if (j++ < n) {
				// refresh domains[iS] as the product of unused factors
				domains[iS] = 1 ;
				for (; j < n ; j++) 
					domains[iS] *= f[j] ;
				break ;
				}
			}
		}
	for (; iE >= iS ; iE--) {
		__int64 size = _TableBlockSize * domains[iE] ;
		if (size > MaxBlockSize) 
			break ;
		_TableBlockSize = size ;
		}
	for (; iS <= iE ; iS++) 
		_nTableBlocks *= domains[iS] ;

/*
	// table size is K1*K2*...*Kn; we will split it in two and compute 
	// _TableBlockSize=Kn*Kn-1*...Ki and _nTableBlocks=Ki-1*...*K2*K1.
	// also, we will try to get as close to maxsize as possible.
	_TableBlockSize = 1 ;
	for (i = _nArgs-1 ; i >= 0 ; i--) {
		int k = _Problem->K(_Arguments[i]) ;
		__int64 size = _TableBlockSize * k ;
		if (size > MaxBlockSize) 
			break ;
		_TableBlockSize = size ;
		}
//	_nTableBlocks = _TableSize/_TableBlockSize ;
	_nTableBlocks = 1 ;
	for (; i >= 0 ; i--) {
		int k = _Problem->K(_Arguments[i]) ;
		_nTableBlocks *= k ;
		}
*/

	if (_TableSize != _nTableBlocks*_TableBlockSize) 
		return 1 ;

	return 0 ;
}


bool ARE::Function::IsTableBlockForAdrAvailable(__int64 Adr, __int64 & IDX)
{
	IDX = -1 ;

	if (Adr < 0 || Adr >= _TableSize) 
		// out of range
		return false ;

	// check if the table is stored in memory (as opposed to the disk)
	if (0 == _nTableBlocks) {
		IDX = 0 ;
		return _OriginatingBucket->IsOutputFunctionBlockComputed(0) ;
		}

	if (NULL == _OriginatingBucket || _TableBlockSize < 1) 
		return false ;

	// compute index of the block containing this address
	IDX = Adr/_TableBlockSize ;
	++IDX ; // disk memeory block indeces run [1,nBlocks]

	// check if it has been computed.
	return _OriginatingBucket->IsOutputFunctionBlockComputed(IDX) ;
}

bool ARE::Function::IsTableBlockAvailable(__int64 IDX)
{
	if (NULL != _OriginatingBucket) 
		return _OriginatingBucket->IsOutputFunctionBlockComputed(IDX) ;
	// TODO : handle more cases
	return false ;
}

/*
void ARE::Function::AddFTB(ARE::FunctionTableBlock & FTB)
{
	ARE::utils::AutoLock lock(_FTBlistMutex) ;
	FTB.AttachNextFTBInFunction(_FTBsInMemory) ;
	_FTBsInMemory = &FTB ;
	if (NULL != _Workspace) 
		_Workspace->Increment_CurrentDiskMemorySpaceCached(FTB.Size()) ;
}
*/

void ARE::Function::RemoveFTB(ARE::FunctionTableBlock & FTB)
{
	if (NULL == _Workspace) 
		return ;
//	ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;
	FunctionTableBlock *previous_ftb = NULL ;
	for (FunctionTableBlock *ftb = _FTBsInMemory ; NULL != ftb ; ftb = ftb->NextFTBInFunction()) {
		if (&FTB == ftb) {
			if (previous_ftb) previous_ftb->AttachNextFTBInFunction(ftb->NextFTBInFunction()) ;
			else _FTBsInMemory = ftb->NextFTBInFunction() ;
			ftb->AttachNextFTBInFunction(NULL) ;
			if (NULL != _Workspace) 
				_Workspace->NoteDiskMemoryBlockUnLoaded(sizeof(ARE_Function_TableType)*ftb->Size()) ;
			}
		previous_ftb = ftb ;
		}
}


int ARE::Function::ReleaseFTB(FunctionTableBlock & FTB, FunctionTableBlock *User)
{
	if (this != FTB.ReferenceFunction()) 
		return 1 ;
	if (0 == _nTableBlocks) 
		// this table has 1 block that is in-memory
		return 0 ;

	// get the mutex; block other threads from modifying the ftb list
	if (NULL == _Workspace) 
		return 1 ;
//	ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;

	if (NULL != User) {
		FTB.RemoveUser(*User) ;
		if (FTB.nUsers() <= 0) {
			RemoveFTB(FTB) ;
			delete &FTB ;
			}
		}
	else {
		if (FTB.nUsers() <= 0) {
			RemoveFTB(FTB) ;
			delete &FTB ;
			}
		}

	return 0 ;
}


int ARE::Function::ReleaseAllFTBs(bool DeleteFiles)
{
	if (0 == _nTableBlocks) 
		// this table has 1 block that is in-memory
		return 0 ;

	// get the mutex; block other threads from modifying the ftb list
	if (NULL == _Workspace) 
		return 1 ;
//	ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;

	// if needed delete FTB files. use a wildcard to do it in one step.
	// when changing this code, make sure the files are permanently deleted, not put in recycle bin.
	if (DeleteFiles && NULL != _OriginatingBucket && _Workspace->DiskSpaceDirectory().length() > 0) {
		const std::string & dir = _Workspace->DiskSpaceDirectory() ;
		std::string c("del ") ;
		c += dir ;
		if ('\\' != c[c.length()-1]) 
			c += '\\' ;
		char s[256] ;
		sprintf(s, "%d-*", (int) _OriginatingBucket->IDX()) ;
		c += s ;
		system(c.c_str()) ;
		}

	while (NULL != _FTBsInMemory) {
		FunctionTableBlock *ftb = _FTBsInMemory ;
		_FTBsInMemory = _FTBsInMemory->NextFTBInFunction() ;
		ftb->AttachNextFTBInFunction(NULL) ;
//		if (DeleteFiles && ftb->Filename().length() > 0) 
//			DeleteFileA(ftb->Filename().c_str()) ;
		_Workspace->NoteDiskMemoryBlockUnLoaded(sizeof(ARE_Function_TableType)*ftb->Size()) ;
		delete ftb ;
		}

	return 0 ;
}


int ARE::Function::GetFTB(__int64 Adr, FunctionTableBlock *User, ARE::FunctionTableBlock * & FTB, __int64 & idx)
{
	idx = -1 ;
	int ret = 1, i ;
	DWORD tElapsed ;

	FTB = NULL ;

	if (Adr < 0 || Adr >= _TableSize) 
		// out of range
		return 1 ;

	// check if the table is stored in memory (as opposed to the disk)
	if (0 == _nTableBlocks) {
		FTB = _Table ;
		idx = 0 ;
		return 0 ;
		}

	if (_TableBlockSize < 1) 
		// error; for disk memory blocks, block size must be known
		return 1 ;

	// get the mutex; block other threads from modifying the ftb list
	if (NULL == _Workspace) 
		return 1 ;
//	ARE::utils::AutoLock lock(_Workspace->FTBMutex()) ;

	// check if the block is already in memory
	FunctionTableBlock *ftb ;
	for (ftb = _FTBsInMemory ; NULL != ftb ; ftb = ftb->NextFTBInFunction()) {
		if (ftb->StartAdr() <= Adr && Adr < ftb->EndAdr()) {
			if (NULL != User) 
				ftb->AddUser(*User) ;
			FTB = ftb ;
			idx = FTB->IDX() ;
			return 0 ;
			}
		}

	// compute index of the block containing this address
	idx = Adr/_TableBlockSize ;
//	__int64 startadr = idx*_TableBlockSize ;
	++idx ; // disk memeory block indeces run [1,nBlocks]

	// this block is not in memory; check if it has been computed.
	if (NULL != _OriginatingBucket) {
		if (! _OriginatingBucket->IsOutputFunctionBlockComputed(idx)) 
			return 4 ;
		}

	DWORD tcS = GETCLOCK(), tcFL ;
	ARE_Function_TableType *data = NULL ;

	// ftb not in memory; need to create/allocate and load.
	ftb = new FunctionTableBlock(this) ;
	if (NULL == ftb) 
		{ ret = 1 ; goto done ; }
	ftb->Initialize(this, idx) ;
	ftb->AllocateData() ; // startadr, _TableBlockSize) ;
	data = ftb->Data() ;
	if (NULL == data) 
		{ delete ftb ; ret = 2 ; goto done ; }

	// load
	tcFL = GETCLOCK() ;
	i = ftb->LoadData() ;
	tElapsed = GETCLOCK() - tcFL ;
	_Workspace->NoteFileLoadTime(tElapsed) ;
	if (0 != i) 
		{ delete ftb ; ret = 3 ; goto done ; }

	// add to the list
	ftb->AttachNextFTBInFunction(_FTBsInMemory) ;
	_FTBsInMemory = ftb ;
	_Workspace->NoteDiskMemoryBlockLoaded(sizeof(ARE_Function_TableType)*ftb->Size()) ;
	if (NULL != User) 
		ftb->AddUser(*User) ;

	FTB = ftb ;
	idx = FTB->IDX() ;

done :
	tElapsed = GETCLOCK() - tcS ;
	_Workspace->NoteInputTableGetTime(tElapsed) ;
	return ret ;
}

