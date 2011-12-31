#include <stdlib.h>

// Added by Vibhav for domain pruning implementation
//#include "Solver.h"  // TODO: disabled by Lars


#include "MersenneTwister.h"
#include "Globals.hxx"
#include "Function.hxx"
#include "Problem.hxx"
#include "ProblemGraphNode.hxx"

static MTRand RNG ;

int ARE::ARP::GetFilename(const std::string & Dir, std::string & fn)
{
	if (0 == _Name.length()) 
		return 1 ;
	if (0 == Dir.length()) 
		return 1 ;
	fn = Dir ;
	if ('\\' != fn[fn.length()-1]) 
		fn += '\\' ;
	fn += _Name ;
//	fn += ".xml" ;
	return 0 ;
}


int ARE::ARP::SaveUAI08(const std::string & Dir)
{
	if (NULL == _K) 
		return 1 ;

	std::string fn ;
	if (0 != GetFilename(Dir, fn)) 
		return 1 ;
	if (0 == fn.length()) 
		return 1 ;
	fn += ".uai" ;
	FILE *fp = fopen(fn.c_str(), "w") ;
	if (NULL == fp) 
		return 1 ;

	char s[1024] ;
	int i, j ;

	// save preamble

	sprintf(s, "BAYES\n%d", (int) _nVars) ;
	fwrite(s, 1, strlen(s), fp) ;

	fwrite("\n", 1, 1, fp) ;
	for (i = 0 ; i < _nVars ; i++) {
		if (i > 0) 
			sprintf(s, " %d", (int) _K[i]) ;
		else 
			sprintf(s, "%d", (int) _K[i]) ;
		fwrite(s, 1, strlen(s), fp) ;
		}

	sprintf(s, "\n%d", (int) _nFunctions) ;
	fwrite(s, 1, strlen(s), fp) ;

	for (i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (NULL == f) {
			fwrite("\n0", 1, 2, fp) ;
			continue ;
			}
		sprintf(s, "\n%d", (int) f->N()) ;
		fwrite(s, 1, strlen(s), fp) ;
		for (j = 0 ; j < f->N() ; j++) {
			sprintf(s, " %d", (int) f->Argument(j)) ;
			fwrite(s, 1, strlen(s), fp) ;
			}
		}

	// save function tables

	for (i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (NULL == f) 
			continue ;
		FunctionTableBlock *ftb = f->Table() ;
		if (NULL == ftb) {
			fwrite("\n\n0", 1, 3, fp) ;
			continue ;
			}
		__int64 ts = f->TableSize() ;
		if (ts < 0) {
			fwrite("\n\n0", 1, 3, fp) ;
			continue ;
			}
		sprintf(s, "\n\n%I64d", ts) ;
		fwrite(s, 1, strlen(s), fp) ;
		__int64 kN = 1, kR = _K[f->Argument(f->N()-1)] ;
		for (j = 0 ; j < f->N() - 1 ; j++) 
			kN *= _K[f->Argument(j)] ;
		// note that ts=kN*kR
		__int64 k, l, K = 0 ;
		for (k = 0 ; k < kN ; k++) {
			fwrite("\n", 1, 1, fp) ;
			for (l = 0 ; l < kR ; l++, K++) {
				sprintf(s, " %g", (double) ftb->Entry(K)) ;
				fwrite(s, 1, strlen(s), fp) ;
				}
			}
		}

	fclose(fp) ;
	return 0 ;
}


int ARE::ARP::SaveXML(const std::string & Dir)
{
	std::string fn ;
	if (0 != GetFilename(Dir, fn)) 
		return 1 ;
	if (0 == fn.length()) 
		return 1 ;
	fn += ".xml" ;
	FILE *fp = fopen(fn.c_str(), "w") ;
	if (NULL == fp) 
		return 1 ;

	char s[1024] ;
	int i ;

	sprintf(s, "<problem name=\"%s\" N=\"%d\" nF=\"%d\">", _Name.c_str(), (int) _nVars, (int) _nFunctions) ;
	fwrite(s, 1, strlen(s), fp) ;

	std::string temp ;
	if (NULL != _K) {
		temp = "\n <variables" ;
		// check if all domains are the same size
		int k = _K[0] ;
		for (i = 1 ; i < _nVars ; i++) {
			if (k != _K[i]) 
				break ;
			}
		if (i >= _nVars) {
			temp += " domainsize=\"" ;
			sprintf(s, "%d", k) ;
			temp += s ;
			temp += "\"" ;
			}
		else {
			temp += " domainsizelist=\"" ;
			for (i = 0 ; i < _nVars ; i++) {
				sprintf(s, "%d", _K[i]) ;
				if (i > 0) 
					temp += ';' ;
				temp += s ;
				}
			}
		temp += "/>" ;
		fwrite(temp.c_str(), 1, temp.length(), fp) ;
		}

	// save functions
	if (_nFunctions > 0 && NULL != _Functions) {
		std::string fFN ;
		for (i = 0 ; i < _nFunctions ; i++) {
			ARE::Function *f = _Functions[i] ;
			if (NULL == f) continue ;
			temp = "\n" ;
			if (0 == f->SaveXMLString(" ", "function", Dir, temp)) 
				fwrite(temp.c_str(), 1, temp.length(), fp) ;
/*
if (CheckFunctions()) {
int error = 1 ;
}
			f->SaveTableBinary(Dir) ;
if (CheckFunctions()) {
int error = 1 ;
}
*/
			}
		}

	// save variable orderings
	if (NULL != _MinDegreeOrdering_VarList) {
		fwrite("\n <variableordering>", 1, 20, fp) ;
		if (NULL != _MinDegreeOrdering_VarList) {
			temp = "\n  <mindegree width=\"" ;
			sprintf(s, "%d", _MinDegreeOrdering_InducedWidth) ;
			temp += s ;
			temp += "\" list=\"" ;
			for (i = 0 ; i < _nVars ; i++) {
				sprintf(s, "%d", _MinDegreeOrdering_VarList[i]) ;
				if (i > 0) 
					temp += ';' ;
				temp += s ;
				}
			temp += "\"/>" ;
			fwrite(temp.c_str(), 1, temp.length(), fp) ;
			}
		fwrite("\n </variableordering>", 1, 21, fp) ;
		}

	fwrite("\n</problem>", 1, 11, fp) ;
	fclose(fp) ;

	// do the actual save of functions as binary files
	for (i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (NULL == f) continue ;
		temp = "\n" ;
		f->SaveTableBinary(Dir) ;
		}

//if (CheckFunctions()) {
//int error = 1 ;
//}
	return 0 ;
}


int ARE::fileload_getnexttoken(char * & buf, int & L, char * & B, int & l)
{
	B = NULL ;
	l = 0 ;
	// find beginning
	int i ;
	for (i = 0 ; i < L ; i++) { if ('\r' != buf[i] && '\n' != buf[i] && ' ' != buf[i] && '\t' != buf[i]) break ; }
	buf += i ;
	L -= i ;
	if (L <= 0) 
		return 1 ;
	// find end
	B = buf ;
	for (l = 1 ; l < L ; l++) { if ('\r' == buf[l] || '\n' == buf[l] || ' ' == buf[l] || '\t' == buf[l]) break ; }
	buf += l ;
	L -= l ;
	return 0 ;
}

int ARE::ARP::LoadFromFile(const std::string & FileName)
{
	// uaiFN may have dir in it; extract filename.
	int i = FileName.length() - 1 ;
	for (; i >= 0 ; i--) {
#ifdef LINUX
		if ('\\' == FileName[i] || '/' == FileName[i]) 
#else
		if ('\\' == FileName[i] || '//' == FileName[i]) 
#endif
			break ;
		}
	std::string fn(FileName.substr(i+1)) ;

	if (NULL != ARE::fpLOG) 
		fprintf(ARE::fpLOG, "\nWill load problem from %s", FileName.c_str()) ;
	FILE *fp = fopen(FileName.c_str(), "r") ;
	if (NULL == fp) {
		fprintf(ARE::fpLOG, "\nfailed to open file; will quit ...") ;
		return ERRORCODE_cannot_open_file ;
		}
	// get file size
	fseek(fp, 0, SEEK_END) ;
	int filesize = ftell(fp) ;
	fseek(fp, 0, SEEK_SET) ;
//#define bufsize	 1048576
//#define bufsize	16777216
//#define bufsize	33554432
	char *buf = new char[filesize] ;
	if (NULL == buf) {
		fclose(fp) ;
		fprintf(ARE::fpLOG, "\nfailed to allocate buffer for file data; will quit ...") ;
		return ERRORCODE_cannot_allocate_buffer_size_too_large ;
		}
	int L = fread(buf, 1, filesize, fp) ;
	fclose(fp) ;
	if (filesize != L) {
		// we probably did not read in the whole file; buf is too small
		// todo : can use foef() or ferror.
		fprintf(ARE::fpLOG, "\nfailed to load file; will quit ...") ;
		delete [] buf ;
		return ERRORCODE_file_too_large ;
		}
	// figure our type of data : BAYES, MARKOV
	for (i = 0 ; i < L && '\n' != buf[i] ; i++) ;
	if (5 == i ? 0 == memcmp(buf, "BAYES", i) : false) 
		i = LoadUAIFormat(buf+6, L-6) ;
	else if (6 == i ? 0 == memcmp(buf, "MARKOV", i) : false) 
		i = LoadUAIFormat(buf+7, L-7) ;
	else {
		delete [] buf ;
		fprintf(ARE::fpLOG, "\nfile type unknown; will quit ...") ;
		return ERRORCODE_problem_type_unknown ;
		}
	delete [] buf ;
	if (0 != i) {
		fprintf(ARE::fpLOG, "\nLoad failed; res = %d", i) ;
		return ERRORCODE_generic ;
		}

	return 0 ;
}


int ARE::ARP::LoadUAIFormat(char *buf, int L)
{
	if (NULL == buf) 
		return 1 ;

	int ret = 1, i, j ;
	char *BUF = NULL ;
	int *K = NULL ;

	Destroy() ;

	{

	char *token = NULL ;
	int l = 0 ;
/*
	if (0 != fileload_getnexttoken(buf, L, token, l)) 
		goto done ;
	if (5 != l ? true : 0 != memcmp(token, "BAYES", l)) 
		goto done ;
*/
	// get # of variables
	if (0 != fileload_getnexttoken(buf, L, token, l)) 
		goto done ;
	int N = atoi(token) ;
	if (N < 1) 
		goto done ;

	// get domain size of each variable
	K = new int[N] ;
	if (NULL == K) goto done ;
	for (i = 0 ; i < N ; i++) {
		if (0 != fileload_getnexttoken(buf, L, token, l)) 
			goto done ;
		K[i] = atoi(token) ;
		}
	if (0 != SetN(N)) 
		goto done ;
	if (0 != SetK(K)) 
		goto done ;

	// get # of functions
	if (0 != fileload_getnexttoken(buf, L, token, l)) 
		goto done ;
	_nFunctions = atoi(token) ;
	if (_nFunctions < 1) 
		{ _nFunctions = 0 ; ret = 0 ; goto done ; }
	_Functions = new ARE::Function*[_nFunctions] ;
	if (NULL == _Functions) 
		goto done ;
	for (i = 0 ; i < _nFunctions ; i++) 
		_Functions[i] = NULL ;

	// read in function signatures
	int A[MAX_NUM_ARGUMENTS_PER_FUNCTION] ;
	for (i = 0 ; i < _nFunctions ; i++) {
		if (0 != fileload_getnexttoken(buf, L, token, l)) 
			goto done ;
		int nA = atoi(token) ;
		if (0 == nA) 
			// this means function is missing essentially
			continue ;
		if (nA < 0) 
			goto done ;
		_Functions[i] = new ARE::Function(NULL, this, i) ;
		if (NULL == _Functions[i]) 
			goto done ;
		_Functions[i]->SetType(ARE_Function_Type_BayesianCPT) ;
		for (j = 0 ; j < nA ; j++) {
			if (0 != fileload_getnexttoken(buf, L, token, l)) 
				goto done ;
			A[j] = atoi(token) ;
			if (A[j] < 0 || A[j] >= _nVars) 
				goto done ;
			}
		if (0 != _Functions[i]->SetArguments(nA, A, -1)) 
			goto done ;
		}

	// create function tables
	for (i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (NULL == f) continue ;
		f->AllocateInMemoryTableBlock() ;
		ARE::FunctionTableBlock *ftb = f->Table() ;
		if (NULL == ftb) goto done ;
		ARE_Function_TableType *data = ftb->Data() ;
		if (NULL == data) goto done ;
		}

	// load function tables
	for (i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (NULL == f) continue ;
		ARE::FunctionTableBlock *ftb = f->Table() ;
		if (NULL == ftb) goto done ;
		ARE_Function_TableType *data = ftb->Data() ;
		if (NULL == data) goto done ;
		// load # of entries
		if (0 != fileload_getnexttoken(buf, L, token, l)) 
			goto done ;
		int n = atoi(token) ;
		if (n != f->TableSize()) 
			goto done ;
		for (j = 0 ; j < n ; j++) {
			if (0 != fileload_getnexttoken(buf, L, token, l)) 
				goto done ;
			double x = atof(token) ;
			data[j] = x ;
			}
		}

	ret = 0 ;

	}

done :
	if (NULL != BUF) 
		delete [] BUF ;
	if (NULL != K) 
		delete [] K ;
	if (0 != ret) 
		Destroy() ;
	return ret ;
}


int ARE::ARP::LoadUAIFormat_Evidence(const char *fn)
{
	int ret = 1, i, j ;
	char *buf = NULL, *BUF = NULL ;
	int *K = NULL ;

	FILE *fp = fopen(fn, "r") ;
	if (NULL == fp) 
		return 1 ;

	{

#define bufsize 256000
	BUF = buf = new char[bufsize] ;
	if (NULL == buf) 
		goto done ;

	int L = fread(buf, 1, bufsize, fp) ;
	if (bufsize == L) 
		// we probably did not read in the whole file; buf is too small
		// todo : can use foef() or ferror.
		goto done ;

	char *token = NULL ;
	int l = 0 ;

	// get # of evidence
	if (0 != fileload_getnexttoken(buf, L, token, l)) 
		goto done ;
	int N = atoi(token) ;
	if (N < 1) 
		goto doneok ;

	// need _K and _Value arrays to exist
	if (NULL == _K || NULL == _Value) 
		goto done ;

	// read in evidence
	for (i = 0 ; i < N ; i++) {
		if (0 != fileload_getnexttoken(buf, L, token, l)) 
			goto done ;
		int var = atoi(token) ;
		if (0 != fileload_getnexttoken(buf, L, token, l)) 
			goto done ;
		int val = atoi(token) ;
		if (var < 0 || var >= _nVars) 
			goto done ;
		if (val < 0 || val >= _K[var]) 
			goto done ;
		_Value[var] = val ;
		}

	}

doneok : 
	_EvidenceFileName = fn ;
	ret = 0 ;
done :
	if (NULL != BUF) 
		delete [] BUF ;
	fclose(fp) ;
	return ret ;
}


__int64 ARE::ARP::ComputeFunctionSpace(void)
{
	__int64 n ;
	_FunctionsSpace = 0 ;

	for (int i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (NULL == f) continue ;
		n = f->ComputeTableSpace() ;
		if (n > 0) 
			_FunctionsSpace += n ;
		}

	return _FunctionsSpace ;
}


int ARE::ARP::ConvertFunctionsToLog(void)
{
	if (_FunctionsAreConvertedToLog) 
		return 0 ;
	_FunctionsAreConvertedToLog = true ;
	for (int i = 0 ; i < nFunctions() ; i++) {
		ARE::Function *f = getFunction(i) ;
		if (NULL != f) 
			f->ConvertTableToLog() ;
		}
	return 0 ;
}


int ARE::ARP::CheckFunctions(void)
{
	for (int i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (NULL == f) continue ;
		if (f->CheckTable()) 
			return 1 ;
		}
	return 0 ;
}


int ARE::ARP::FillInFunctionTables(void)
{
	for (int i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (NULL == f) continue ;
		if (f->ComputeTableSize() <= 0) 
			// this function does not have a table
			continue ;
		FunctionTableBlock *ftb = f->Table() ;
		if (NULL != ftb) 
			// this function already has a table
			continue ;
		if (0 != f->FillInRandomBayesianTable()) 
			return 2 ;
		}

	return 0 ;
}


int ARE::ARP::ComputeQueryRelevance_VarElimination(ARE_Function_TableType & Factor, char VarElimOp, char CombinationOp)
{
	// we assume Factor is initialed

	int i, j, k ;

	if (0 != VarElimOp && 1 != VarElimOp) 
		return 1 ;

	double neutral_value = (0 == CombinationOp) ? 1.0 : 0.0 ;

	if (NULL != ARE::fpLOG) 
		fprintf(ARE::fpLOG, "\nARP::ComputeQueryRelevance_VarElimination() combination_type=%d VarElimOp=%d neutral_value=%g", (int) CombinationOp, (int) VarElimOp, neutral_value) ;

	_nFunctionsIrrelevant = 0 ;
	for (i = 0 ; i < _nFunctions ; i++) 
		_Functions[i]->MarkAsQueryRelevant() ;

	if (_nFunctions < 1 || _nVars < 1) 
		return 0 ;

	int nLeaves = 0 ;
//	ARE::Function **FNs = new ARE::Function*[_nFunctions] ;
	int *Leaves = new int[_nVars] ;
	if (NULL == Leaves) 
		return 1 ;
	char *VarIsInQueue = new char [_nVars] ;
	if (NULL == VarIsInQueue) 
		{ delete [] Leaves ; return 1 ; }
	for (i = 0 ; i < _nVars ; i++) 
		VarIsInQueue[i] = 0 ; // 0=not in queue 1=in queue

	// construct an initial list of variables that participate in 1 fn only
	for (i = 0 ; i < _nVars ; i++) {
		ARPGraphNode & adj = _GraphAdjacencyMatrix[i] ;
		if (adj.nAdjacentFunctions() > 1) 
			// cannot be a leaf if more than 1 adj function
			continue ;
		if (adj.nAdjacentFunctions() < 1) 
			// this is a singleton variable; ignore it.
			continue ;
		Leaves[nLeaves++] = i ;
		VarIsInQueue[i] = 1 ;
		}

	while (nLeaves > 0) {
		int var = Leaves[--nLeaves] ;
		VarIsInQueue[var] = 0 ;
		ARPGraphNode & adj = _GraphAdjacencyMatrix[var] ;
		if (1 != adj.nAdjacentFunctions_QueryReleventFunctionsOnly()) {
			if (NULL != ARE::fpLOG) 
				fprintf(ARE::fpLOG, "\nWARNING : var %d is being analyzed for adj-fn query relevance, and 1 != adj.nAdjacentFunctions_QueryReleventFunctionsOnly()", var) ;
			continue ;
			}

		ARE_Function_TableType elim_value = -1.0 ;

		ARE::Function *f = adj.AdjacentFunction_QueryRelevantFunctionsOnly(0) ;
		ARE::FunctionTableBlock *ftb = f->Table() ;
		if (NULL == ftb) 
			goto dump_table ;

		// check that if we eliminated 'var' from this fn, all entries would be the same (some const value).
		{

		int Val[MAX_NUM_VARIABLES_PER_BUCKET], val[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the current value combination of the arguments of this function (table block).
		int Var[MAX_NUM_VARIABLES_PER_BUCKET] ; // this is the current value combination of the arguments of this function (table block).
		int iVar = -1, n ;
		__int64 idx, size = 1 ;
		for (i = n = 0 ; i < f->N() ; i++) {
			Val[i] = val[i] = 0 ;
			int u = f->Argument(i) ;
			if (var == u) 
				{ iVar = i ; continue ; }
			Var[n++] = u ;
			size *= _K[u] ;
			}
		bool fn_is_relevant = false ;
		for (idx = 0 ; idx < size ; idx++) {
			for (i = 0 ; i < n ; i++) 
				val[i + (i < iVar ? 0 : 1)] = Val[i] ;
			double v = 0.0 ;
			for (k = 0 ; k < _K[var] ; k++, idx++) {
				val[iVar] = k ;
				idx = ComputeFnTableAdr(f->N(), f->Arguments(), val, _K) ;
				if (0 == VarElimOp) 
					v += ftb->Entry(idx) ;
				else if (1 == VarElimOp) 
					{ if (ftb->Entry(idx) > v) v = ftb->Entry(idx) ; }
				}
			if (elim_value < 0.0) {
				elim_value = v ;
				}
			else if (fabs(v - elim_value) > 0.000001) {
				fn_is_relevant = true ;
				break ;
				}
			ARE::EnumerateNextArgumentsValueCombination(n, Var, Val, _K) ;
			}
/*
		ARE::Function *f = GetCPT(var) ;
		if (NULL == f) {
			// this is an error
			if (NULL != ARE::fpLOG) 
				fprintf(ARE::fpLOG, "\nERROR : var %d is being analyzed for adj-fn query relevance, and it has no CPT", var) ;
			continue ;
			}
		ARE::FunctionTableBlock *ftb = f->Table() ;
		if (NULL == ftb) 
			goto dump_table ;
		__int64 n = f->TableSize() ;
		int cVar = f->BayesianCPTChildVariable() ;
		__int64 idx ;
		bool fn_is_relevant = false ;
		for (idx = 0 ; idx < n ; idx++) {
			double sum = 0.0 ;
			for (k = 0 ; k < _K[var] ; k++, idx++) 
				sum += ftb->Entry(idx) ;
			if (fabs(sum - 1.0) > 0.000001) {
				fn_is_relevant = true ;
				break ;
				}
			}
*/
		if (fn_is_relevant) 
			continue ;
		}

dump_table :
		if (f->IsQueryIrrelevant()) {
			// this is an error
			if (NULL != ARE::fpLOG) 
				fprintf(ARE::fpLOG, "\nERROR : var %d is being analyzed for adj-fn query relevance, and its CPT %d is already marked as irrelevant", var, f->IDX()) ;
			}
		else {
			f->MarkAsQueryIrrelevant() ;
			if (fabs(elim_value - neutral_value) > 0.000001) {
				if (0 == CombinationOp) 
					Factor *= elim_value ;
				else if (1 == CombinationOp) 
					Factor += elim_value ;
				}
			++_nFunctionsIrrelevant ;
			}
		// check if other variables in the fn need checking
		for (k = 0 ; k < f->N() ; k++) {
			int u = f->Argument(k) ;
			if (u == var) 
				continue ;
			if (0 != VarIsInQueue[u]) 
				continue ;
			ARPGraphNode & u_adj = _GraphAdjacencyMatrix[u] ;
			if (1 != u_adj.nAdjacentFunctions_QueryReleventFunctionsOnly()) 
				continue ;
			Leaves[nLeaves++] = u ;
			VarIsInQueue[u] = 1 ;
			}
		}

/*
static int S = 0 ;
static int E = 0 ;
j = 0 ;
for (i = 0 ; i < _nFunctions ; i++) {
	ARE::Function *f = _Functions[i] ;
	if (! f->IsQueryIrrelevant()) 
		continue ;
	if (j < S || j > E) 
		f->MarkAsQueryRelevant() ;
	++j ;
	}
*/

	delete [] VarIsInQueue ;
	delete [] Leaves ;
	return 0 ;
}


int ARE::ARP::ComputeBayesianAncestors(int V, int *AncestorFlags, int *Workspace)
{
	int i, j ;

	for (i = 0 ; i < _nVars ; i++) 
		AncestorFlags[i] = 0 ;

	Workspace[0] = V ;
	int n = 1 ;
	while (n > 0) {
		int v = Workspace[0] ;
		Workspace[0] = Workspace[--n] ;
		if (0 != AncestorFlags[v]) 
			continue ;
		AncestorFlags[v] = 1 ;
		ARE::Function *f = GetCPT(v) ;
		if (NULL == f) 
			continue ;
		for (i = f->N() - 2 ; i >= 0 ; i--) {
			int u = f->Argument(i) ;
			if (0 != AncestorFlags[u]) 
				continue ;
			for (j = 0 ; j < n ; j++) {
				if (Workspace[j] == u) 
					break ;
				}
			if (j >= n) 
				Workspace[n++] = u ;
			}
		}

	return 0 ;
}


int ARE::ARP::GenerateRandomUniformBayesianNetworkStructure(int N, int K, int P, int C, int ProblemCharacteristic)
{
	Destroy() ;

	int i, j ;

	// check if parameters are valid
	if (N < 1 || K < 1 || K > MAX_NUM_VALUES_PER_VAR_DOMAIN) return 1 ;
	if (C < 0 ||P < 1) return 1 ;
	if (N <= P) return 1 ;
	if (P+1 > MAX_NUM_ARGUMENTS_PER_FUNCTION) return 1 ;

	// safety check.
	// notice, that C cannot be larger than N-P since for last P variables in the ordering,
	// we don't have enough parents to put in the parent set.
	if (C > N - P) C = N - P ;

	_nVars = N ;
	_K = new int[_nVars] ;
	if (NULL == _K) 
		goto failed ;
	_Value = new int[_nVars] ;
	if (NULL == _Value) 
		goto failed ;
	for (i = 0 ; i < _nVars ; i++) {
		_K[i] = K ;
		_Value[i] = -1 ;
		}
	_nFunctions = 0 ;
	_Functions = new ARE::Function*[_nVars] ;
	if (NULL == _Functions) goto failed ;
	for (i = 0 ; i < _nVars ; i++) 
		_Functions[i] = NULL ;

	if (1 == ProblemCharacteristic) {
		int ret = 1 ;
		int *space = new int[4*_nVars] ;
		if (NULL == space) 
			goto failed ;
		// idea : 
		// 1) maintain a set of current-priors ; i.e. variables that are parents of a CPT such that these variables themselves have no CPT
		// 2) recursively, pick a var from the set of current-priors, and then generate parent as follows: 
		//    pick a var, randomly among all variables, and check if current child is an ancestor of this picked var;
		//    if yes, this picked var is no good.
		int *L = space ; // L is the set of leaves
		int *A = space + _nVars ; // ancestor bits
		int *W = space + 2*_nVars ; // workspace bits
		int *PL = space + 3*_nVars ; // potential parent list for child

		L[0] = _nVars-1 ;
		int nL = 1 ;

		while (_nFunctions < C) {
			// pick child
			int child = -1 ;
			if (nL <= 0) goto probchar1_done ;
			else if (1 == nL) { child = L[0] ; nL = 0 ; }
			else { int idx = RNG.randInt(nL-1) ; child = L[idx] ; L[idx] = L[--nL] ; }
			// generate a list of possible parents
			int nPL = 0 ;
			for (i = 0 ; i < _nVars ; i++) {
				if (child == i) continue ;
				ComputeBayesianAncestors(i, A, W) ;
				if (0 == A[child]) 
					PL[nPL++] = i ;
				}
			// PL is now a list of potential parents; generate actual parents
			int args[MAX_NUM_ARGUMENTS_PER_FUNCTION] ;
			if (nPL < P) 
				goto probchar1_done ;
			else if (nPL == P) {
				for (i = 0 ; i < P ; i++) args[i] = PL[i] ;
				nPL = 0 ;
				}
			else {
				for (i = 0 ; i < P ; i++) {
					int idx = RNG.randInt(nPL-1) ;
					args[i] = PL[idx] ;
					PL[idx] = PL[--nPL] ;
					}
				}

			// create function
			ARE::Function *f = new ARE::Function(NULL, this, child) ;
			if (NULL == f) goto probchar1_done ;
			f->SetType(ARE_Function_Type_BayesianCPT) ;
			_Functions[child] = f ;
			++_nFunctions ;
			args[P] = child ;
			if (0 != f->SetArguments(P+1, args)) 
				goto probchar1_done ;

			// add arguments of f to L
			for (i = 0 ; i < P ; i++) {
				int v = args[i] ;
				if (NULL != _Functions[v]) 
					continue ;
				for (j = 0 ; j < nL ; j++) { if (v == L[j]) break ; }
				if (j >= nL) 
					L[nL++] = v ;
				}
			}

		ret = 0 ;
probchar1_done :
		delete [] space ;
		if (0 != ret) 
			goto failed ;
		}
	else {
		// create C conditional probabilities
		while (_nFunctions < C) {
			// we will let the CSP_Bayesian_PT constructor create the parent set and the probability matrix for each conditional probability.
			// indeces run [0,N-1]; we assume that children of CPTs are from [P,N-1] and the parents of a CPT are [0,ChildIDX-1].
			// randomly pick a childIDX.
			int child = P + RNG.randInt(_nVars - P-1) ;
			// check if a conditional probability has been created for this variable
			if (NULL != _Functions[child]) continue ;
			ARE::Function *f = new ARE::Function(NULL, this, child) ;
			if (NULL == f) goto failed ;
			f->SetType(ARE_Function_Type_BayesianCPT) ;
			_Functions[child] = f ;
			++_nFunctions ;
			if (0 != f->GenerateRandomBayesianSignature(P+1, child)) goto failed ;
//			if (0 != f->GenerateRandomBayesianTable(P+1, child)) goto failed ;
			}
		}

	// create prior probability matrices for variables which have no conditional probability matrix
	for (i = 0 ; i < _nVars ; i++) {
		if (NULL != _Functions[i]) continue ;
		ARE::Function *f = new ARE::Function(NULL, this, i) ;
		if (NULL == f) goto failed ;
		f->SetType(ARE_Function_Type_BayesianCPT) ;
		_Functions[i] = f ;
		++_nFunctions ;
		if (0 != f->GenerateRandomBayesianSignature(1, i)) goto failed ;
//		if (0 != f->GenerateRandomBayesianTable(1, i)) goto failed ;
		}

	return 0 ;
failed :
	Destroy() ;
	return 1 ;
}


void ARE::ARP::DestroyGraph(void)
{
	if (NULL != _GraphAdjacencyMatrix) {
		delete [] _GraphAdjacencyMatrix ;
		_GraphAdjacencyMatrix = NULL ;
		}
	_nConnectedComponents = _nSingletonVariables = 0 ;
}


int ARE::ARP::ComputeGraph(void)
{
	DestroyGraph() ;
	if (_nVars < 1) 
		return 0 ;

	int i, j ;

	{

	_GraphAdjacencyMatrix = new ARPGraphNode[_nVars] ;
	if (NULL == _GraphAdjacencyMatrix) goto failed ;
	// process one variable at a time
	for (i = 0 ; i < _nVars ; i++) {
		int n = 0 ;
		// compute degree of i
		for (j = 0 ; j < _nFunctions ; j++) {
			ARE::Function *f = _Functions[j] ;
			if (NULL == f) continue ;
			if (f->ContainsVariable(i)) 
				n++ ;
			}
		// allocate function list
		ARPGraphNode & node = _GraphAdjacencyMatrix[i] ;
		if (0 != node.SetN(n)) 
			goto failed ;
		node.SetV(i) ;
		n = 0 ;
		// fill in function list
		for (j = 0 ; j < _nFunctions ; j++) {
			ARE::Function *f = _Functions[j] ;
			if (NULL == f) continue ;
			if (f->ContainsVariable(i)) 
				node.SetFunction(n++, f) ;
			}
		// finish
		node.ComputeAdjacentVariables() ;
		}

	// run test of nodes
	for (i = 0 ; i < _nVars ; i++) {
		ARPGraphNode & node = _GraphAdjacencyMatrix[i] ;
		if (node.test() > 0) {
			int error = 1 ;
			}
		}

	// compute num of singleton variables
	for (i = 0 ; i < _nVars ; i++) {
		ARPGraphNode & node = _GraphAdjacencyMatrix[i] ;
		if (node.Degree() < 0) {
			int error = 1 ;
			}
		else if (0 == node.Degree()) 
			_nSingletonVariables++ ;
		}

	// compute number of connected components
	int *temp = new int[3*_nVars] ;
	if (NULL == temp) goto failed ;
	for (i = 0 ; i < _nVars ; i++) temp[i] = 0 ;
	for (i = 0 ; i < _nVars ; i++) {
		if (0 != temp[i]) continue ;
		// variable i belongs to a new component; do DFS of the component starting from i.
		temp[i] = ++_nConnectedComponents ;
		int *nAdj = temp+_nVars ; // for each variable, number of ajacent variables we have traversed
		int *vStack = temp+2*_nVars ; // variable stack
		vStack[0] = i ;
		nAdj[0] = 0 ;
		j = 0 ; // stacksize - 1
		while (j >= 0) {
			int v = vStack[j] ;
			ARPGraphNode & node = _GraphAdjacencyMatrix[v] ;
			if (nAdj[j] >= node.Degree()) { --j ; continue ; }
			int u = node.AdjacentVariable(nAdj[j]++) ;
			if (temp[u]) continue ;
			temp[u] = _nConnectedComponents ;
			vStack[++j] = u ;
			nAdj[j] = 0 ;
			}
		}
	delete [] temp ;

	}

	return 0 ;
failed :
	DestroyGraph() ;
	return 1 ;
}


void ARE::ARP::DestroyMinFillOrdering(void)
{
	if (NULL != _MinFillOrdering_VarList) {
		delete [] _MinFillOrdering_VarList ;
		_MinFillOrdering_VarList = NULL ;
		}
	if (NULL != _MinFillOrdering_VarPos) {
		delete [] _MinFillOrdering_VarPos ;
		_MinFillOrdering_VarPos = NULL ;
		}
	_MinFillOrdering_InducedWidth = -1 ;
}


void ARE::ARP::DestroyMinDegreeOrdering(void)
{
	if (NULL != _MinDegreeOrdering_VarList) {
		delete [] _MinDegreeOrdering_VarList ;
		_MinDegreeOrdering_VarList = NULL ;
		}
	if (NULL != _MinDegreeOrdering_VarPos) {
		delete [] _MinDegreeOrdering_VarPos ;
		_MinDegreeOrdering_VarPos = NULL ;
		}
	_MinDegreeOrdering_InducedWidth = -1 ;
}


int ARE::ARP::TestVariableOrdering(const int *VarList, const int *Var2PosMap)
{
	int i, n, m ;

	// check no var/pos is out of bounds
	for (i = 0 ; i < _nVars ; i++) {
		if (VarList[i] < 0 || VarList[i] >= _nVars) 
			return 1 ;
		if (Var2PosMap[i] < 0 || Var2PosMap[i] >= _nVars) 
			return 2 ;
		}

	// check sum of VarList/Var2PosMap; should be (N choose 2) = N*(N-1)/2
	n = m = 0 ;
	for (i = 0 ; i < _nVars ; i++) {
		n += VarList[i] ;
		m += Var2PosMap[i] ;
		}
	int test = (_nVars*(_nVars-1)) >> 1 ;
	if (n != test) 
		return 3 ;
	if (m != test) 
		return 3 ;

	return 0 ;
}


int ARE::ARP::EliminateEvidenceVariable(int Var, int Val)
{
	int i ;

	for (i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (NULL == f) continue ;
		if (! f->ContainsVariable(Var)) continue ;
		f->RemoveVariable(Var, Val) ;
		}

	return 0 ;
}


int ARE::ARP::EliminateSingletonDomainVariables(void)
{
	int i, j ;

	printf("\np.EliminateSingletonDomainVariables(): start, N=%d ...", (int) _nVars) ;
	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\np.EliminateSingletonDomainVariables(): start, N=%d ...", (int) _nVars) ;
		fflush(ARE::fpLOG) ;
		}

	_nSingletonDomainVariables = 0 ;
	for (i = 0 ; i < _nVars ; i++) {
		if (_K[i] > 1) continue ;
		++_nSingletonDomainVariables ;
		for (j = 0 ; j < _nFunctions ; j++) {
			ARE::Function *f = _Functions[j] ;
			if (NULL == f) continue ;
			if (! f->ContainsVariable(i)) continue ;
			// since it is singleton domain, we don't have to do anything (e.g. change fn table), 
			// just remove the variable from the scope.
#ifdef _DEBUG
			if (NULL != ARE::fpLOG) {
				fprintf(ARE::fpLOG, "\np.EliminateSingletonDomainVariables(): removing var %d from fn %d ...", i, (int) f->IDX()) ;
				fflush(ARE::fpLOG) ;
				}
#endif
			f->RemoveVariableFromArguments(i) ;
			// if the fn has no arguments, make it a const function
			if (0 == f->N()) {
#ifdef _DEBUG
				if (NULL != ARE::fpLOG) {
					fprintf(ARE::fpLOG, "\np.EliminateSingletonDomainVariables(): fn %d has no arguments left ...", (int) f->IDX()) ;
					fflush(ARE::fpLOG) ;
					}
#endif
				if (NULL != f->Table()) 
					f->ConstValue() = f->Table()->Entry(0) ;
				else 
					f->ConstValue() = 1.0 ;
				}
//			f->RemoveEvidenceVariable(i, _K[i]) ;
			}
		}

	printf("\np.EliminateSingletonDomainVariables(): done; nSingletonDomainVariables=%d ...", (int) _nSingletonDomainVariables) ;
	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\np.EliminateSingletonDomainVariables(): done; nSingletonDomainVariables=%d ...", (int) _nSingletonDomainVariables) ;
		fflush(ARE::fpLOG) ;
		}

	return 0 ;
}


int ARE::ARP::ComputeMinDegreeOrdering(void)
{
	DestroyMinDegreeOrdering() ;
	if (_nVars < 1) 
		return 0 ;
	if (NULL == _GraphAdjacencyMatrix) 
		return 1 ;

	int i, j, k ;

	// we will keep track, for each variable, which variables are its neighbors.
	// given i and j (i < j) will keep track of if i and j are neighbors.
	// adj info for i is kept as a list of adj variables.
	// head of the list is stored in '_MinDegreeOrdering_VarPos' -> if starts at m, then we store -(m+2); 
	// if _MinDegreeOrdering_VarPos[n] >= 0, then that var is already in the ordering.
//	int adj_space_size = 30000 ;
//	int adj_space_size = 16384 ;
#define ORD_COMP_ADJ_SPACE_SIZE 32768
	int adj_space_size = ORD_COMP_ADJ_SPACE_SIZE ;
	// adj space should be counted in pairs :
	//		[i] is the value,
	//		[i+1] is a pointer to the next in the list.
	int *degrees = new int[_nVars] ;
	int *adj_space = new int[adj_space_size] ;
	if (NULL == degrees || NULL == adj_space) goto failed ;

	{

	_MinDegreeOrdering_VarList = new int[_nVars] ;
	_MinDegreeOrdering_VarPos = new int[_nVars] ;
	if (NULL == _MinDegreeOrdering_VarPos || NULL == _MinDegreeOrdering_VarList) goto failed ;

	// forget the induced width computed earlier
	_MinDegreeOrdering_InducedWidth = -1 ;

	// initialize the space
	int empty_adj_space = 0 ;
	int number_of_empty_cells = adj_space_size >> 1 ;
	for (i = 1 ; i < adj_space_size - 2 ; i += 2) 
		adj_space[i] = i + 1 ;
	// NIL at the end
	adj_space[adj_space_size - 1] = -1 ;

	// during each iteration, we will pick a variable that has the smallest degree

	// mark each variable as not in the ordering yet
	for (i = 0 ; i < _nVars ; i++) {
		_MinDegreeOrdering_VarPos[i] = -1 ;
		// use this array for counting the degree of unordered variables
		_MinDegreeOrdering_VarList[i] = 0 ;
		}

	// count degrees
	for (i = 0 ; i < _nVars ; i++) 
		degrees[i] = 0 ;

	// fill in initial graph
	for (j = 0 ; j < _nVars ; j++) {
		ARPGraphNode & node_j = _GraphAdjacencyMatrix[j] ;
		for (i = j+1 ; i < _nVars ; i++) {
			if (! node_j.IsAdjacent_QueryReleventFunctionsOnly(i)) continue ;
			// check space
			if (number_of_empty_cells < 2) {
				int new_size = adj_space_size + ORD_COMP_ADJ_SPACE_SIZE ;
				int *new_adj_space = new int[new_size] ;
				if (NULL == new_adj_space) 
					goto failed ;
				memcpy(new_adj_space, adj_space, adj_space_size*sizeof(int)) ;
				// generate structure for new space
				number_of_empty_cells += ORD_COMP_ADJ_SPACE_SIZE >> 1 ;
				for (k = adj_space_size + 1 ; k < new_size - 2 ; k += 2) {
					new_adj_space[k] = k + 1 ;
					}
				new_adj_space[new_size - 1] = empty_adj_space ;
				empty_adj_space = adj_space_size ;
				// let go of old space
				delete [] adj_space ;
				adj_space = new_adj_space ;
				adj_space_size = new_size ;
				}

			// add a node.
			// ***************************
			// get a cell for i.
			// ***************************
			int cell = empty_adj_space ;
			empty_adj_space = adj_space[empty_adj_space + 1] ;
			--number_of_empty_cells ;
			adj_space[cell] = j ;
			// check if i has nothing
			if (-1 == _MinDegreeOrdering_VarPos[i]) 
				adj_space[cell + 1] = -1 ;
			else 
				adj_space[cell + 1] = -(_MinDegreeOrdering_VarPos[i] + 2) ;
			_MinDegreeOrdering_VarPos[i] = -(cell + 2) ;
			// count degree
			++degrees[i] ;
			// ***************************
			// get a cell for j.
			// ***************************
			cell = empty_adj_space ;
			empty_adj_space = adj_space[empty_adj_space + 1] ;
			--number_of_empty_cells ;
			adj_space[cell] = i ;
			// check if j has nothing
			if (-1 == _MinDegreeOrdering_VarPos[j]) 
				adj_space[cell + 1] = -1 ;
			else 
				adj_space[cell + 1] = -(_MinDegreeOrdering_VarPos[j] + 2) ;
			_MinDegreeOrdering_VarPos[j] = -(cell + 2) ;
			// count degree
			++degrees[j] ;
			}
		}

	// find the max degree
	int max_degree = -INT_MAX ;

	// we will fill in the ordering from the end.
	// from the end, we will put min-degree nodes, except degree 0,
	// which go in the beginning.
	int end = _nVars - 1 ;
	while (end >= 0) {
		// among variables that are not yet in the ordering, find one that has the smallest degree.
		int min_degree_value = INT_MAX ;
		int min_degree_var = -1 ;
		for (j = 0 ; j < _nVars ; j++) {
			if (_MinDegreeOrdering_VarPos[j] >= 0) continue ; // variable already in ordering
			if (degrees[j] < min_degree_value) {
				min_degree_value = degrees[j] ;
				min_degree_var = j ;
				}
			}
		// check if nothing left
		if (min_degree_var < 0) 
			break ; // no variable found
		if (max_degree < min_degree_value) 
			max_degree = min_degree_value ;

		// remove all references from others to this
		for (j = 0 ; j < _nVars ; j++) {
			if (min_degree_var == j) continue ;
			if (_MinDegreeOrdering_VarPos[j] >= 0) continue ; // variable already in ordering
			int previous = -1 ;
			for (i = -_MinDegreeOrdering_VarPos[j] - 2 ; -1 != i ; i = adj_space[i+1]) {
				if (min_degree_var == adj_space[i]) {
					// remove min_degree_var from list
					if (-1 == previous)
						_MinDegreeOrdering_VarPos[j] = -(adj_space[i+1] + 2) ;
					else
						adj_space[previous+1] = adj_space[i+1] ;
					adj_space[i+1] = empty_adj_space ;
					empty_adj_space = i ;
					++number_of_empty_cells ;
					// reduce degree
					--degrees[j] ;
					break ;
					}
				previous = i ;
				}
			}

		// connect neighbors
		for (j = -_MinDegreeOrdering_VarPos[min_degree_var] - 2 ; -1 != j ; j = adj_space[j+1]) {
			for (i = adj_space[j+1] ; -1 != i ; i = adj_space[i+1]) {
				int u = adj_space[i] ;
				int v = adj_space[j] ;
				// check if u and v are already connected
				for (k = -_MinDegreeOrdering_VarPos[u] - 2 ; -1 != k ; k = adj_space[k+1]) {
					if (adj_space[k] == v) break ;
					}
				if (k >= 0) continue ; // already connected

				// check space
				if (number_of_empty_cells < 2) {
					int new_size = adj_space_size + ORD_COMP_ADJ_SPACE_SIZE ;
					int *new_adj_space = new int[new_size] ;
					if (NULL == new_adj_space) goto failed ;
					memcpy(new_adj_space, adj_space, adj_space_size*sizeof(int)) ;
					// generate structure for new space
					number_of_empty_cells += ORD_COMP_ADJ_SPACE_SIZE >> 1 ;
					for (k = adj_space_size + 1 ; k < new_size - 2 ; k += 2) {
						new_adj_space[k] = k + 1 ;
						}
					new_adj_space[new_size - 1] = empty_adj_space ;
					empty_adj_space = adj_space_size ;
					// let go of old space
					delete [] adj_space ;
					adj_space = new_adj_space ;
					adj_space_size = new_size ;
					}

				// ***************************
				// get a cell for u.
				// ***************************
				int cell = empty_adj_space ;
				empty_adj_space = adj_space[empty_adj_space + 1] ;
				--number_of_empty_cells ;
				adj_space[cell] = v ;
				// check if u has nothing
				if (-1 == _MinDegreeOrdering_VarPos[u]) 
					adj_space[cell + 1] = -1 ;
				else 
					adj_space[cell + 1] = -_MinDegreeOrdering_VarPos[u] - 2 ;
				_MinDegreeOrdering_VarPos[u] = -(cell + 2) ;
				// count degree
				++degrees[u] ;
				// ***************************
				// get a cell for v.
				// ***************************
				cell = empty_adj_space ;
				empty_adj_space = adj_space[empty_adj_space + 1] ;
				--number_of_empty_cells ;
				adj_space[cell] = u ;
				// check if v has nothing
				if (-1 == _MinDegreeOrdering_VarPos[v]) 
					adj_space[cell + 1] = -1 ;
				else 
					adj_space[cell + 1] = -_MinDegreeOrdering_VarPos[v] - 2 ;
				_MinDegreeOrdering_VarPos[v] = -(cell + 2) ;
				// count degree
				++degrees[v] ;
				}
			}

		// release space from this var
		int first = -_MinDegreeOrdering_VarPos[min_degree_var] - 2 ;
		int last = -1 ;
		int count = 0 ;
		for (j = first ; -1 != j ; j = adj_space[j+1]) {
			last = j ;
			++count ;
			}
		if (last >= 0) {
			adj_space[last+1] = empty_adj_space ;
			empty_adj_space = first ;
			number_of_empty_cells += count ;
			}

		// min_degree_var is the next variable in the ordering
		_MinDegreeOrdering_VarPos[min_degree_var] = end ;
		_MinDegreeOrdering_VarList[end] = min_degree_var ;
		--end ;
		}

	delete [] degrees ;
	delete [] adj_space ;

	ComputeInducedWidth(_MinDegreeOrdering_VarList, _MinDegreeOrdering_VarPos, _MinDegreeOrdering_InducedWidth) ;

#ifdef _DEBUG
	if (NULL != ARE::fpLOG) {
		char s[256] ;
		sprintf(s, "\nMin-Degree ordering width = %d varlist : ", (int) _MinDegreeOrdering_InducedWidth) ;
		fwrite(s, 1, strlen(s), ARE::fpLOG) ;
		for (j = 0 ; j < _nVars ; j++) {
			sprintf(s, " %d", (int) _MinDegreeOrdering_VarList[j]) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			}
		}
#endif // _DEBUG

	return 0 ;

	}

failed :
	if (NULL != degrees) 
		delete [] degrees ;
	if (NULL != adj_space) 
		delete [] adj_space ;
	DestroyMinDegreeOrdering() ;
	return 1 ;
}


int ARE::ARP::ComputeMinFillOrdering(int nRandomTries, int BestKnownWidth)
{
	if (_nVars < 1) 
		return 0 ;
	if (nRandomTries < 2) {
		ComputeMinFillOrdering() ;
		goto done ;
		}

	// run n tries; save the best
	{
	int *VarList = new int[_nVars] ;
	if (NULL == VarList) 
		return 1 ;
	int w = INT_MAX ;
	for (int i = 0 ; i < nRandomTries ; i++) {
/*
if (NULL != ARE::fpLOG && 0 == i%100) 
{
fprintf(ARE::fpLOG, "\nMin-fill try %d ...", i) ;
fflush(ARE::fpLOG) ;
}
*/
		if (0 != ComputeMinFillOrdering()) 
			continue ;
		if (_MinFillOrdering_InducedWidth < w) {
			w = _MinFillOrdering_InducedWidth ;
			for (int j = 0 ; j < _nVars ; j++) VarList[j] = _MinFillOrdering_VarList[j] ;
			if (w <= BestKnownWidth) 
				break ;
			}
		}
	if (w < _MinFillOrdering_InducedWidth) {
		for (int j = 0 ; j < _nVars ; j++) {
			_MinFillOrdering_VarList[j] = VarList[j] ;
			_MinFillOrdering_VarPos[_MinFillOrdering_VarList[j]] = j ;
			}
		_MinFillOrdering_InducedWidth = w ;
		}
	delete [] VarList ;
	}

done :
#ifdef _DEBUG
	if (NULL != ARE::fpLOG) {
		char s[256] ;
		sprintf(s, "\nMin-Fill   ordering width = %d varlist : ", (int) _MinFillOrdering_InducedWidth) ;
		fwrite(s, 1, strlen(s), ARE::fpLOG) ;
		for (int j = 0 ; j < _nVars ; j++) {
			sprintf(s, " %d", (int) _MinFillOrdering_VarList[j]) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			}
		}
#endif // _DEBUG

	return 0 ;
}


int ARE::ARP::ComputeMinFillOrdering(void)
{
	DestroyMinFillOrdering() ;
	if (_nVars < 1) 
		return 0 ;
	if (NULL == _GraphAdjacencyMatrix) 
		return 1 ;

	int i, j, k, l ;

	// we will keep track, for each variable, which variables are its neighbors.
	// given i and j (i < j) will keep track of if i and j are neighbors.
	// adj info for i is kept as a list of adj variables.
	// head of the list is stored in '_MinFillOrdering_VarPos' -> if starts at m, then we store -(m+2); 
	// if _MinFillOrdering_VarPos[n] >= 0, then that var is already in the ordering.
//	int adj_space_size = 30000 ;
//	int adj_space_size = 16384 ;
#define ORD_COMP_ADJ_SPACE_SIZE 32768
	int adj_space_size = ORD_COMP_ADJ_SPACE_SIZE ;
	// adj space should be counted in pairs :
	//		[i] is the value,
	//		[i+1] is a pointer to the next in the list.
	int *degrees = new int[_nVars] ;
	int *adj_space = new int[adj_space_size] ;
	if (NULL == degrees || NULL == adj_space) goto failed ;

	{

	_MinFillOrdering_VarList = new int[_nVars] ;
	_MinFillOrdering_VarPos = new int[_nVars] ;
	if (NULL == _MinFillOrdering_VarPos || NULL == _MinFillOrdering_VarList) goto failed ;

	// forget the induced width computed earlier
	_MinFillOrdering_InducedWidth = -1 ;

	// during each iteration, we will pick a variable that has the smallest number of fill-in edges

	// mark each variable as not in the ordering yet
	for (i = 0 ; i < _nVars ; i++) {
		_MinFillOrdering_VarPos[i] = -1 ;
		// use this array for counting the fill-in number of unordered variables
		_MinFillOrdering_VarList[i] = 0 ;
		}

	// initialize the space
	int empty_adj_space = 0 ;
	int number_of_empty_cells = adj_space_size >> 1 ;
	for (i = 1 ; i < adj_space_size - 2 ; i += 2) 
		adj_space[i] = i + 1 ;
	// NIL at the end
	adj_space[adj_space_size - 1] = -1 ;

	// count degrees
	for (i = 0 ; i < _nVars ; i++) 
		degrees[i] = 0 ;

	// fill in initial graph
	for (j = 0 ; j < _nVars ; j++) {
		ARPGraphNode & node_j = _GraphAdjacencyMatrix[j] ;
		for (i = j+1 ; i < _nVars ; i++) {
			if (! node_j.IsAdjacent_QueryReleventFunctionsOnly(i)) continue ;
			// check space
			if (number_of_empty_cells < 2) {
				int new_size = adj_space_size + ORD_COMP_ADJ_SPACE_SIZE ;
				int *new_adj_space = new int[new_size] ;
				if (NULL == new_adj_space) 
					goto failed ;
				memcpy(new_adj_space, adj_space, adj_space_size*sizeof(int)) ;
				// generate structure for new space
				number_of_empty_cells += ORD_COMP_ADJ_SPACE_SIZE >> 1 ;
				for (k = adj_space_size + 1 ; k < new_size - 2 ; k += 2) {
					new_adj_space[k] = k + 1 ;
					}
				new_adj_space[new_size - 1] = empty_adj_space ;
				empty_adj_space = adj_space_size ;
				// let go of old space
				delete [] adj_space ;
				adj_space = new_adj_space ;
				adj_space_size = new_size ;
				}

			// add a node.
			// ***************************
			// get a cell for i.
			// ***************************
			int cell = empty_adj_space ;
			empty_adj_space = adj_space[empty_adj_space + 1] ;
			--number_of_empty_cells ;
			adj_space[cell] = j ;
			// check if i has nothing
			if (-1 == _MinFillOrdering_VarPos[i]) 
				adj_space[cell + 1] = -1 ;
			else 
				adj_space[cell + 1] = -(_MinFillOrdering_VarPos[i] + 2) ;
			_MinFillOrdering_VarPos[i] = -(cell + 2) ;
			// count degree
			++degrees[i] ;
			// ***************************
			// get a cell for j.
			// ***************************
			cell = empty_adj_space ;
			empty_adj_space = adj_space[empty_adj_space + 1] ;
			--number_of_empty_cells ;
			adj_space[cell] = i ;
			// check if j has nothing
			if (-1 == _MinFillOrdering_VarPos[j]) 
				adj_space[cell + 1] = -1 ;
			else 
				adj_space[cell + 1] = -(_MinFillOrdering_VarPos[j] + 2) ;
			_MinFillOrdering_VarPos[j] = -(cell + 2) ;
			// count degree
			++degrees[j] ;
			}
		}

	// we will fill in the ordering from the end.
	// from the end, we will put min-fillin nodes.
	int end = _nVars - 1 ;
	while (end >= 0) {
		// among variables that are not yet in the ordering, find one that has the smallest fillin.
		int min_fillin_value = INT_MAX ;
		int min_fillin_var = -1 ;
		for (j = 0 ; j < _nVars ; j++) {
			if (_MinFillOrdering_VarPos[j] >= 0) continue ; // variable already in ordering
			// compute for all pairs of adjacent nodes, how many are not connected; this is the fillin value
			int fillinvalue = 0 ;
			if (degrees[j] > 1) {
//				fillinvalue = (degrees[j]*(degrees[j]-1)) >> 1 ;
				for (i = -_MinFillOrdering_VarPos[j] - 2 ; -1 != i ; i = adj_space[i+1]) {
					for (k = adj_space[i+1] ; -1 != k ; k = adj_space[k+1]) {
						int u = adj_space[i] ;
						int v = adj_space[k] ;
						// u and v are two variables adjacent to j; check if u and v are already connected.
						for (l = -_MinFillOrdering_VarPos[u] - 2 ; -1 != l ; l = adj_space[l+1]) {
							if (adj_space[l] == v) break ;
							}
						if (l < 0) 
							// not connected
							fillinvalue++ ;
						}
					}
				}
			if (fillinvalue < min_fillin_value) {
				min_fillin_value = fillinvalue ;
				min_fillin_var = j ;
				}
			// if equal, break tries randomly
			else if (fillinvalue == min_fillin_value) {
				if (RNG.rand() > 0.5) {
					min_fillin_value = fillinvalue ;
					min_fillin_var = j ;
					}
				}
			}
		// check if nothing left
		if (min_fillin_var < 0) 
			break ; // no variable found

		// remove all references from others to this
		for (j = 0 ; j < _nVars ; j++) {
			if (min_fillin_var == j) continue ;
			if (_MinFillOrdering_VarPos[j] >= 0) continue ; // variable already in ordering
			int previous = -1 ;
			for (i = -_MinFillOrdering_VarPos[j] - 2 ; -1 != i ; i = adj_space[i+1]) {
				if (min_fillin_var == adj_space[i]) {
					// remove min_fillin_var from list
					if (-1 == previous)
						_MinFillOrdering_VarPos[j] = -(adj_space[i+1] + 2) ;
					else
						adj_space[previous+1] = adj_space[i+1] ;
					adj_space[i+1] = empty_adj_space ;
					empty_adj_space = i ;
					++number_of_empty_cells ;
					// reduce degree
					--degrees[j] ;
					break ;
					}
				previous = i ;
				}
			}

		// connect neighbors
		for (j = -_MinFillOrdering_VarPos[min_fillin_var] - 2 ; -1 != j ; j = adj_space[j+1]) {
			for (i = adj_space[j+1] ; -1 != i ; i = adj_space[i+1]) {
				int u = adj_space[i] ;
				int v = adj_space[j] ;
				// check if u and v are already connected
				for (k = -_MinFillOrdering_VarPos[u] - 2 ; -1 != k ; k = adj_space[k+1]) {
					if (adj_space[k] == v) break ;
					}
				if (k >= 0) continue ; // already connected

				// check space
				if (number_of_empty_cells < 2) {
					int new_size = adj_space_size + ORD_COMP_ADJ_SPACE_SIZE ;
					int *new_adj_space = new int[new_size] ;
					if (NULL == new_adj_space) goto failed ;
					memcpy(new_adj_space, adj_space, adj_space_size*sizeof(int)) ;
					// generate structure for new space
					number_of_empty_cells += ORD_COMP_ADJ_SPACE_SIZE >> 1 ;
					for (k = adj_space_size + 1 ; k < new_size - 2 ; k += 2) {
						new_adj_space[k] = k + 1 ;
						}
					new_adj_space[new_size - 1] = empty_adj_space ;
					empty_adj_space = adj_space_size ;
					// let go of old space
					delete [] adj_space ;
					adj_space = new_adj_space ;
					adj_space_size = new_size ;
					}

				// ***************************
				// get a cell for u.
				// ***************************
				int cell = empty_adj_space ;
				empty_adj_space = adj_space[empty_adj_space + 1] ;
				--number_of_empty_cells ;
				adj_space[cell] = v ;
				// check if u has nothing
				if (-1 == _MinFillOrdering_VarPos[u]) 
					adj_space[cell + 1] = -1 ;
				else 
					adj_space[cell + 1] = -_MinFillOrdering_VarPos[u] - 2 ;
				_MinFillOrdering_VarPos[u] = -(cell + 2) ;
				// count degree
				++degrees[u] ;
				// ***************************
				// get a cell for v.
				// ***************************
				cell = empty_adj_space ;
				empty_adj_space = adj_space[empty_adj_space + 1] ;
				--number_of_empty_cells ;
				adj_space[cell] = u ;
				// check if v has nothing
				if (-1 == _MinFillOrdering_VarPos[v]) 
					adj_space[cell + 1] = -1 ;
				else 
					adj_space[cell + 1] = -_MinFillOrdering_VarPos[v] - 2 ;
				_MinFillOrdering_VarPos[v] = -(cell + 2) ;
				// count degree
				++degrees[v] ;
				}
			}

		// release space from this var
		int first = -_MinFillOrdering_VarPos[min_fillin_var] - 2 ;
		int last = -1 ;
		int count = 0 ;
		for (j = first ; -1 != j ; j = adj_space[j+1]) {
			last = j ;
			++count ;
			}
		if (last >= 0) {
			adj_space[last+1] = empty_adj_space ;
			empty_adj_space = first ;
			number_of_empty_cells += count ;
			}

		// min_fillin_var is the next variable in the ordering
		_MinFillOrdering_VarPos[min_fillin_var] = end ;
		_MinFillOrdering_VarList[end] = min_fillin_var ;
		--end ;
		}

	delete [] degrees ;
	delete [] adj_space ;

	ComputeInducedWidth(_MinFillOrdering_VarList, _MinFillOrdering_VarPos, _MinFillOrdering_InducedWidth) ;

/*
	if (NULL != ARE::fpLOG) {
		char s[256] ;
		sprintf(s, "\nMin-Fill   ordering width = %d varlist : ", (int) _MinFillOrdering_InducedWidth) ;
		fwrite(s, 1, strlen(s), ARE::fpLOG) ;
		for (j = 0 ; j < _nVars ; j++) {
			sprintf(s, " %d", (int) _MinFillOrdering_VarList[j]) ;
			fwrite(s, 1, strlen(s), ARE::fpLOG) ;
			}
		}
*/

	return 0 ;

	}

failed :
	DestroyMinFillOrdering() ;
	return 1 ;
}


static int mark_two_variables_as_adj_in_induced_width_computation(int i, int j, int **adj_matrix, int *adj_list_length, int adj_list_initial_length) 
{
	int k, l ;

	// check if adj list exists.
	if (NULL == adj_matrix[i]) {
		// adj list does not exist. allocate memory, mark j as adjacent to i.
		adj_matrix[i] = new int[adj_list_initial_length] ;
		if (NULL == adj_matrix[i]) return 1 ;
		adj_list_length[i] = adj_list_initial_length ;
		(adj_matrix[i])[0] = j ;
		for (k = 1 ; k < adj_list_length[i] ; k++) (adj_matrix[i])[k] = -1 ;
		}
	else {
		// adj list does exist.
		// check if j is already there.
		for (k = 0 ; k < adj_list_length[i] ; k++) {
			if ((adj_matrix[i])[k] < 0) break ;
			if ((adj_matrix[i])[k] == j) return 0 ;
			}
		// j not there
		if (k >= adj_list_length[i]) {
			// out of space
			int new_length = adj_list_length[i] + adj_list_initial_length ;
			int *temp_adj = new int[new_length] ;
			if (NULL == temp_adj) return 1 ;
			// copy existing list
			for (l = 0 ; l < adj_list_length[i] ; l++) temp_adj[l] = (adj_matrix[i])[l] ;
			for (; l < new_length ; l++) temp_adj[l] = -1 ;
			// free old memory
			delete [] adj_matrix[i] ;
			// set new length
			adj_list_length[i] = new_length ;
			// attach new memory
			adj_matrix[i] = temp_adj ;
			}
		// store adj info
		(adj_matrix[i])[k] = j ;
		}

	return 0 ;
}


int ARE::ARP::ComputeInducedWidth(const int *VarList, const int *Var2PosMap, int & InducedWidth)
{
	InducedWidth = -1 ;

	// check that the number of variables is positive
	if (_nVars < 1) 
		return 0 ;
	// check that an ordering is given
	if (NULL == VarList || NULL == Var2PosMap) 
		return 1 ;

	int return_value = 1 ;
	int i, j ;

	// we will use this variable to allocate memory to an adjacency list, and
	// if memory runs out, reallocate.
	const int adj_list_initial_length = 32 ;
	// for every variable, we will keep track of the adjacent variables
	int **adj_matrix = NULL ;
	// for every variable, we will keep track of the length of the adjacency list
	int *adj_list_length = NULL ;
	// this is the maximum width we are computing
	int max_width = 0 ;
	int max_width_variable = -1 ;

	// allocate memory
	adj_matrix = new int*[_nVars] ;
	if (NULL == adj_matrix) goto done ;
	for (i = 0 ; i < _nVars ; i++) adj_matrix[i] = NULL ;
	adj_list_length = new int[_nVars] ;
	if (NULL == adj_list_length) goto done ;
	for (i = 0 ; i < _nVars ; i++) adj_list_length[i] = 0 ;

	// ***************************************************************************************************
	// for every variable, compute its initial adjacency list, based on the given constraint graph.
	// once this is done, we don't need the original constraint graph any more and we won't reference it.
	// ***************************************************************************************************
	for (i = 0 ; i < _nVars ; i++) {
		ARPGraphNode & node_i = _GraphAdjacencyMatrix[i] ;
		for (int j = 0 ; j < _nVars ; j++) {
			if (i == j) continue ;

			// check if these two variables are connected
			if (! node_i.IsAdjacent_QueryReleventFunctionsOnly(j)) continue ;

			// now we need to add this bit of information that i and j are adjacent to the
			// adjcency lists. but first we need to check if there is enough memory.
			// do this for both variables
			// for variable i.
			if (mark_two_variables_as_adj_in_induced_width_computation(i, j, adj_matrix, adj_list_length, adj_list_initial_length)) goto done ;
			// for variable j.
			if (mark_two_variables_as_adj_in_induced_width_computation(j, i, adj_matrix, adj_list_length, adj_list_initial_length)) goto done ;
			}
		}

	// ***************************************************************************************************
	// go backwards in the ordering, and for every variable, connect all of its adjacent variables
	// that come before it in the ordering.
	// at the same time, keep track of the width of every variable.
	// ***************************************************************************************************
	for (i = _nVars - 1 ; i >= 0 ; i--) {
		// get the name of the variable
		int v = VarList[i] ;
		// check if this variable has an adj list
		if (NULL == adj_matrix[v]) continue ;

		// compute the width of this variable
		int width = 0 ;
		for (j = 0 ; j < adj_list_length[v] ; j++) {
			int vr = (adj_matrix[v])[j] ;
			if (vr < 0) break ;
			// check if this variable comes in the ordering before v
			if (Var2PosMap[vr] < i) width++ ;
			}
		// store the width, if maximum
		if (max_width < width) {
			max_width = width ;
			max_width_variable = v ;
			}

		// for any two parents of this variable, such that both are before it in the ordering, connect them.
		for (j = 0 ; j < adj_list_length[v] ; j++) {
			if ((adj_matrix[v])[j] < 0) break ;
			// check if this variable comes in the ordering before v
			if (Var2PosMap[(adj_matrix[v])[j]] >= i) continue ;
			for (int k = 0 ; k < adj_list_length[v] ; k++) {
				if ((adj_matrix[v])[k] < 0) break ;
				if (k == j) continue ;
				// check if this variable comes in the ordering before v
				if (Var2PosMap[(adj_matrix[v])[k]] >= i) continue ;

				// connect these two variables.
				// NOTE : we only need to add adjacency information to the variable that comes
				// second in the ordering
				int v1 = (adj_matrix[v])[j] ;
				int v2 = (adj_matrix[v])[k] ;
				if (Var2PosMap[v1] < Var2PosMap[v2]) {
					// add for v2
					if (mark_two_variables_as_adj_in_induced_width_computation(v2, v1, adj_matrix, adj_list_length, adj_list_initial_length)) goto done ;
					}
				else {
					// add for v1
					if (mark_two_variables_as_adj_in_induced_width_computation(v1, v2, adj_matrix, adj_list_length, adj_list_initial_length)) goto done ;
					}
				}
			}

		// delete space owned by this variable. it is no longer needed.
		if (adj_matrix[v]) {
			delete [] adj_matrix[v] ;
			adj_matrix[v] = NULL ;
			}
		}

	// store max width
	InducedWidth = max_width ;

	// successfully done
	return_value = 0 ;

done :

	// free memory
	if (adj_matrix) {
		for (i = 0 ; i < _nVars ; i++) {
			if (adj_matrix[i]) delete [] adj_matrix[i] ;
			}
		delete [] adj_matrix ;
		}
	if (adj_list_length) delete [] adj_list_length ;

	return return_value ;
}


int ARE::ARP::ComputeSingletonConsistency(int & nNewSingletonDomainVariables)
{
	nNewSingletonDomainVariables = 0 ;

	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\nARP::ComputeSingletonConsistency() begin") ;
		}

	vector<vector<bool> > is_consistent ;
	SingletonConsistencyHelper(is_consistent) ;

	for (int i = 0 ; i < _nVars ; i++) {
		int old_k = _K[i] ;
		for (int j = _K[i] - 1 ; j >= 0 ; j--) {
			if (is_consistent[i][j]) 
				continue ;
			// assignment variable[i] = value[j] is inconsistent with the rest of the problem; eliminate this value from the domain of the variable.
			for (int k = 0 ; k < _nFunctions ; k++) {
				ARE::Function *f = _Functions[k] ;
				if (NULL == f) continue ;
				if (! f->ContainsVariable(i)) 
					continue ;
				f->RemoveVariableValue(i, j) ;
				if (NULL != ARE::fpLOG) {
					fprintf(ARE::fpLOG, "\nComputeSingletonConsistency(): removing var %d value %d f=%d", i, j, (int) f->IDX()) ;
					fflush(ARE::fpLOG) ;
					}
				}
			// remove this value from the domain
			// check if domain size became 0
			if (--_K[i] < 1) 
				return -1 ;
			}
		if (old_k > 1 && _K[i] < 1) 
			nNewSingletonDomainVariables++ ;
		}

	if (NULL != ARE::fpLOG) {
		fprintf(ARE::fpLOG, "\nARP::ComputeSingletonConsistency() end") ;
		}

	return 0 ;
}


void ARE::ARP::SingletonConsistencyHelper(vector<vector<bool> > & is_consistent)
{
	int i, j, k ;

	// Constructing the CNF for singleton consistency
	// We have \sum_{i=1}^{N} #values(X_i) Boolean variables, namely a variable for each variable-value pair
	
	// minisat solver
	Solver S ;

	// The vector stores mapping from variable-value pairs to Boolean variables
	vector<vector<int> > var2cnfvar(_nVars) ;
	int count = 0 ;
	for (i = 0 ; i < _nVars ; i++) {
		var2cnfvar[i] = vector<int>(_K[i]) ;
		for (j = 0 ; j < _K[i] ; j++) 
			var2cnfvar[i][j] = count++ ;
		}
	// Initialize the variables of minisat
	for (i = 0 ; i < count ; i++) 
		S.newVar() ;

	// Each solution of the CNF corresponds to a unique consistent assignment to the variables.
	// Add clauses to ensure that each variable takes exactly one value (i.e. at least and at most one)
	for (i = 0 ; i < _nVars ; i++) {
		vec<Lit> lits ;
		// For each variable, we have a clause which ensures that each variable takes at least one value
		for (j = 0 ; j < _K[i] ; j++) 
			lits.push(Lit(var2cnfvar[i][j])) ;
		S.addClause(lits) ;
		// We have K[i]*(K[i]-1)/2 clauses which ensure that each variable takes at most one value
		for (j = 0 ; j < _K[i] ; j++) {
			for (k = j + 1 ; k < _K[i] ; k++) {
				lits.clear() ;
				lits.push(~Lit(var2cnfvar[i][j])) ;
				lits.push(~Lit(var2cnfvar[i][k])) ;
				S.addClause(lits) ;
				}
			}
		}

	// For each zero probability in each function, add a clause
	int value_assignment[MAX_NUM_ARGUMENTS_PER_FUNCTION] ;
	for (i = 0 ; i < _nFunctions ; i++) {
		ARE::Function *f = _Functions[i] ;
		if (0 != f->nTableBlocks()) 
			continue ; // error; this fn stored externally, cannot use it here.
		FunctionTableBlock *table = f->Table() ;
		vec<Lit> lits ;
		for (j = 0 ; j < f->TableSize() ; j++) {
			if (table->Entry(j) <= 0.000001) {
				lits.clear() ;
				// convert table address j to an assignment to the scope
				ComputeArgCombinationFromFnTableAdr(j, f->N(), f->Arguments(), value_assignment, _K) ;
				// Add a clause which is a negation of the current tuple assignment
				// E.g. If the tuple is (X=2,Y=3), then add a clause !X2 V !Y3 where X2 and Y3 are Boolean variables
				// corresponding to X=2 and Y=3 respectively
				// This will ensure that X=2 and Y=3 never occur together in a solution
				for (k = 0 ; k < f->N() ; k++) 
					lits.push(~Lit(var2cnfvar[f->Arguments()[k]][value_assignment[k]])) ;
				S.addClause(lits) ;
				}
			}
		}

	// solve for each variable-value pair, by forcing the variable to have the given value
	is_consistent = vector<vector<bool> >(_nVars) ;
	vec<Lit> assumps ;
	for (i = 0 ; i < _nVars ; i++) {
		is_consistent[i] = vector<bool>(_K[i]) ;
		for (j = 0 ; j < _K[i] ; j++) {
			assumps.clear() ;
			assumps.push(Lit(var2cnfvar[i][j])) ;
			if (S.solve(assumps))
				is_consistent[i][j] = true ;
			}
		}

	// done
}

