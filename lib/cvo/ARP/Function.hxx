#ifndef Function_HXX_INCLUDED
#define Function_HXX_INCLUDED

#include <stdlib.h>
#include <string>

#include "Mutex.h"
#include "Globals.hxx"
#include "Workspace.hxx"
#include "FunctionTableBlock.hxx"

namespace BucketElimination { class Bucket ; }
namespace ARE { class FunctionTableBlock ; }

#define ARE_Function_Type_BayesianCPT "BayesianCPT"

/*
	Addressing scheme in the table is as follows. Store sequentially values of argument value combinations 
	in order: 0,...,0; 0,...,1; 0,...,2; ...; 0,...,k-1; 0,...,1,0; 0,...,1,1; etc.
	In other words, as a k-base number starting from 0,...,0 and adding 1 at a time.
	E.g. a value combination (a,b) of a function (X, Y) has address b+K[Y]*a.
	Given a function f(X1, ..., Xm), and a combination of argument values, a formula to compute an index in the table is 
	v[Xm] + v[Xm-1]*K[Xm] + v[Xm-2]*K[Xm]*K[Xm-1] + ...
*/

namespace ARE
{

class ARP ;

class Function
{
protected :
	Workspace *_Workspace ;
	ARP *_Problem ;
	// index of this function wrt the owner
	// 1) for original functions of the problem, index in the problem array of functions
	int _IDX ;
	// type of the function; user/derived classes should fill this in; e.g. "BayesianCPT"
	std::string _Type ;
	// file with the function contents (table); could be NULL (e.g. Bayesian table loaded from UAI file); or could a binary table for the function.
	std::string _FileName ;
	// a flag indicating that this function is not relevant to the query, and can be skipped.
	// e.g. a CPT that contain no evidence variables, and that has no evidence variable descendants.
	bool _IsQueryIrrelevant ;
	// if case this fn is a bayesian CPT, the child variable.
	// normally child var in a CPT is the last variable, but sometimes we may reorder the fn scope, 
	// and may thus lose track of the child var.
	int _BayesianCPTChildVariable ;
public :
	inline Workspace *WS(void) const { return _Workspace ; }
	inline ARP *Problem(void) const { return _Problem ; }
	inline int IDX(void) const { return _IDX ; }
	inline const std::string & FileName(void) const { return _FileName ; }
	inline const std::string & Type(void) const { return _Type ; }
	inline void SetWS(Workspace *WS) { _Workspace = WS ; }
	inline void SetProblem(ARP *P) { _Problem = P ; }
	inline void SetIDX(int IDX) { _IDX = IDX ; }
	inline void SetType(const std::string & Type) { _Type = Type ; }
	inline void SetFileName(const std::string & FileName) { _FileName = FileName ; }
	inline bool IsQueryIrrelevant(void) const { return _IsQueryIrrelevant ; }
	inline void MarkAsQueryIrrelevant(void) { _IsQueryIrrelevant = true ; }
	inline void MarkAsQueryRelevant(void) { _IsQueryIrrelevant = false ; }
	inline int BayesianCPTChildVariable(void) const { return _BayesianCPTChildVariable ; }

protected :
	int _nArgs ; // number of variables in the function
	int *_Arguments ; // a list of variables; size of the array is N. in case of Bayesian CPTs, the child variable is last.
public :
	inline int N(void) const { return _nArgs ; }
	inline int Argument(int IDX) const { return _Arguments[IDX] ; }
	inline const int *Arguments(void) const { return _Arguments ; }
	int SetArguments(int n, int *Arguments)
	{
		if (n < 0) 
			return 1 ;
		Destroy() ;
		if (n < 1) 
			return 0 ;
		_Arguments = new int[n] ;
		if (NULL == _Arguments) 
			return 1 ;
		for (int i = 0 ; i < n ; i++) 
			_Arguments[i] = Arguments[i] ;
		_nArgs = n ;
		if (ARE_Function_Type_BayesianCPT == _Type) 
			_BayesianCPTChildVariable = _Arguments[n-1] ;
		return 0 ;
	}
	inline bool ContainsVariable(int V) const { for (int i = 0 ; i < _nArgs ; i++) { if (V == _Arguments[i]) return true ; } return false ; }
	void RemoveVariableFromArguments(int V)
	{
		for (int i = 0 ; i < _nArgs ; i++) {
			if (V == _Arguments[i]) {
				--_nArgs ;
				for (int j = i ; j < _nArgs ; j++) 
					_Arguments[j] = _Arguments[j+1] ;
				return ;
				}
			}
	}
	inline int GetHighestOrderedVariable(const int *Var2PosMap) const
	{
		if (_nArgs <= 0) 
			return -1 ;
		int i, v = _Arguments[0] ;
		for (i = 1 ; i < _nArgs ; i++) {
			if (Var2PosMap[_Arguments[i]] > Var2PosMap[v]) 
				v = _Arguments[i] ;
			}
		return v ;
	}

	// This function is used when a bucket elimination computes the outgoing (from the bucket) function, by enumerating over all its argument value combinations. 
	// _Arguments[_nArgs-1] is the variable being eliminated in the bucket, and thus is not among the argument of the outgoing function; 
	// all other variables in _Arguments[] are also arguments of the outgoing function.
	// Also, the order of variables in _Arguments[] and argumentsof(outgoingfunction) are the same (BE should have 
	// reordered arguments of this function to agree with the bucket function arguments). 
	// Given a bucket-function argument values combination, this function computes the adr, wrt the table of this function, 
	// of the corresponding table cell in this table. Obviously, given an adr (argument value combination) of the 
	// bucket function, corresponding adr in the table of this function is different, since these two functions have different scopes.
	// since VarSuperSet does not specify the value of _Arguments[_nArgs-1], this function returns adr wrt this fn corresponding to argument value combination where _Arguments[_nArgs-1]=0.
	inline __int64 ComputeFnTableAdrEx(int NSuperSet, const int *VarSuperSet, const int *ValSuperSet, const int *DomainSizes)
	{
		// _Arguments and VarSuperSet have the same order of variables, except _Arguments[_nArgs-1] is not in VarSuperSet.
		// Therefore _nArgs <= NSuperSet, with the exception of the last var in _Arguments[].
		if (_nArgs < 1) 
			return -1 ;
		if (NSuperSet < 1) 
			return 0 ;
		int i = NSuperSet - 1 ;
		int k = _nArgs - 2 ;
		__int64 adr = 0 ;
		__int64 j = 1 ;
		for (; i >= 0 ; i--) {
			if (VarSuperSet[i] == _Arguments[k]) {
				j *= DomainSizes[_Arguments[k+1]] ;
				adr += j*ValSuperSet[i] ;
				--k ;
				}
			}
		return adr ;
	}
	// This function is used when a super bucket elimination computes the outgoing (from the super bucket) function, by enumerating over all its argument value combinations. 
	// The order of variables in _Arguments[] and argumentsof(outgoingfunction) are the same (BE should have reordered arguments of this function to agree with the bucket function arguments). 
	// Given a bucket-function arguments values combination, this function computes the adr, wrt the table of this function, 
	// of the corresponding table cell in this table. Obviously, given an adr (argument value combination) of the 
	// bucket function, corresponding adr in the table of this function is different, since these two functions have different scopes.
	// We assume that VarSuperSet and _Arguments are in the same order.
	inline __int64 ComputeFnTableAdrExN(int NSuperSet, const int *VarSuperSet, const int *ValSuperSet, const int *DomainSizes)
	{
		if (_nArgs < 1) 
			return -1 ;
		if (NSuperSet < 1) 
			return 0 ;
		int i = NSuperSet - 1 ;
		__int64 adr = -1 ;
		int k = _nArgs - 1 ;
		for (; i >= 0 ; i--) {
			if (VarSuperSet[i] == _Arguments[k]) {
				adr = ValSuperSet[i] ;
				--k ;
				--i ;
				break ;
				}
			}
		__int64 j = 1 ;
		for (; i >= 0 ; i--) {
			if (VarSuperSet[i] == _Arguments[k]) {
				j *= DomainSizes[_Arguments[k+1]] ;
				adr += j*ValSuperSet[i] ;
				--k ;
				}
			}
		return adr ;
	}

	int SetArguments(int N, int *Arguments, int ExcludeThisVar) ;

	// **************************************************************************************************
	// when running Bucket Elimination, functions belong to a bucket
	// **************************************************************************************************

protected :
	BucketElimination::Bucket *_BEBucket ;
public :
	inline BucketElimination::Bucket *BEBucket(void) const { return _BEBucket ; }
	inline void SetBEBucket(BucketElimination::Bucket *B) { _BEBucket = B ; }

	// **************************************************************************************************
	// A function table may be stored in memory (in which case there is 1 block of size=_TableSize), 
	// or on a disk, in which case there are _nTableBlocks blocks of size _TableBlockSize 
	// (i.e. all table blocks are of equal size).
	// **************************************************************************************************

	// _OriginatingBucket is used when this function is generated during Bucket Elimination
protected :
	BucketElimination::Bucket *_OriginatingBucket ;
public :
	inline BucketElimination::Bucket *OriginatingBucket(void) const { return _OriginatingBucket ; }
	inline void SetOriginatingBucket(BucketElimination::Bucket *B) { _OriginatingBucket = B ; }

protected :

	// size of the function table, as a number of elements; this is a function of domain size of arguments.
	__int64 _TableSize ;
	// number of table blocks; initially -1 (unknown); 0 means table is stored in memory; >0 means table is stored externally.
	__int64 _nTableBlocks ;
	// size of the table block, as a number of elements; all table blocks are of equal size.
	// initially -1 (unknown); 0 means table is stored in memory; >0 means table is stored externally.
	__int64 _TableBlockSize ;
	// _Table may be NULL even though _TableSize is known.
	FunctionTableBlock *_Table ;
//	// an ordered list of primes of the domain sizes, except for the domain of the last argument.
//	// we exclude the last argument, since it is the one that Bucket Elimination enumerates over when eliminating a variable.
//	char _nArgumentDomainFactorization ;
//	char _ArgumentDomainFactorization[MAX_NUM_FACTORS_PER_FN_DOMAIN] ;

public :

	inline __int64 TableSize(void) const { return _TableSize ; }
	inline __int64 nTableBlocks(void) const { return _nTableBlocks ; }
	inline __int64 TableBlockSize(void) const { return _TableBlockSize ; }
	inline FunctionTableBlock *Table(void) { return _Table ; }
	inline void SetnTableBlocks(__int64 nTableBlocks)
	{
		_nTableBlocks = nTableBlocks ;
		if (0 == nTableBlocks) 
			_TableBlockSize = 0 ;
		else 
			_TableBlockSize = _TableSize/nTableBlocks ;
	}

	int ConvertTableToLog(void) ;

//	// compute _ArgumentDomainFactorization
//	int FactorizeArgumentDomains(void) ;
	// compute table size, based on domain sizes of arguments; this is the number of elements in the table.
	__int64 ComputeTableSize(void) ;
	// compute table size, based on domain sizes of arguments; this is in bytes.
	__int64 ComputeTableSpace(void) ;
	// reset the table block size
	void ResetTableBlockSize(void) { _nTableBlocks = -1 ; _TableBlockSize = -1 ; }
	// compute _nTableBlocks and _TableBlockSize
	int ComputeTableBlockSize(__int64 MaxBlockSizeInNumberOfCells, int nComputingThreads) ;
	// return non-0 iff something is wrong with table
	int CheckTable(void) ;
	// Save the table of this function as a binary file
	virtual int SaveTableBinary(const std::string & Dir) ;
	// table will be stored in memory; allocate table
	int AllocateInMemoryTableBlock(void) ;
	// in-memoty table will be destroyed
	int DestroyInMemoryTableBlock(void) ;
	// this function checks whether the table block corresponding to the given adr (i.e. block which contains the entry with the given adr) is available (in-memory or on-disk).
	bool IsTableBlockForAdrAvailable(__int64 Adr, __int64 & IDX) ;
	bool IsTableBlockAvailable(__int64 IDX) ;

	// *******************************************************************************************************
	// A list of FTBs currently in memory (cached).
	// _FTBMutex mutex of the workspace is used to protect access.
	// *******************************************************************************************************

protected :
	// A list of FTBs that are currently in memory
	FunctionTableBlock *_FTBsInMemory ;
public :
	void RemoveFTB(FunctionTableBlock & FTB) ;
	// get function table block of this function that contains an entry with the given index; if requested, add a user.
	// return values :
	//  0 : ok
	// +1 : generic error
	// +2 : data table could not be created
	// +3 : file load error
	// +4 : requested block has not been computed yet
	int GetFTB(
		// IN
		__int64 Adr, FunctionTableBlock *User, 
		// OUT
		FunctionTableBlock * & FTB, 
		__int64 & FTBIDX
		) ;
	// release table block from the given user.
	int ReleaseFTB(FunctionTableBlock & FTB, FunctionTableBlock *User) ;
	// release all FTBs
	int ReleaseAllFTBs(bool DeleteFiles) ;

	// *************************************************************************************************************************
	// function has const value iff it has no arguments
	// *************************************************************************************************************************

protected :

	ARE_Function_TableType _ConstValue ;

public :

	inline ARE_Function_TableType & ConstValue(void) { return _ConstValue ; }

public :

	// this function allocates and fills in random Bayesian table.
	// it assumes that the last argument is the child variable in the CPT.
	int FillInRandomBayesianTable(void) ;

	// this function creates signature (i.e. variables/arguments) of this function, assuming it is a Bayesian CPT.
	// it will place the child as the last variable.
	// other variables are picked randomly, assuming their indeces are less than ChildIDX.
	int GenerateRandomBayesianSignature(int N, int ChildIDX) ;
//	int GenerateRandomBayesianSignature(int N, int ChildIDX, int & nFVL, int *FreeVariableList) ;

	// this will reorder arguments of this function, so that its arguments agree with the order of variables in AF(ront)/AB(ack).
	// we assume that the arguments of this function are in AF+AB; however, AF+AB may have variables not in this function, e.g. AF+AB is a superset of this-function->Arguments.
	// ReOrderArguments() is called by a parent bucket on the bucket-function from a child bucket, 
	// with parent bucket's keep/eliminate variables as F/B lists.
	// this function arguments will be split along F/B.
	// moreover, order of arguments of this function split along F/B will agree with F/B.
	int ReOrderArguments(int nAF, const int *AF, int nAB, const int *AB) ;

	// this function will remove the given evidence variable from this function
	int RemoveVariable(int Variable, int ValueRemaining) ;

	// this function will remove the given value of the given variable; 
	// this function is used when domains of variables are pruned.
	int RemoveVariableValue(int Variable, int ValueRemoved) ;

	// serialize this function as XML; append it to the given string.
	virtual int SaveXMLString(const char *prefixspaces, const char *tag, const std::string & Dir, std::string & S) ;

	// get the filename of the table of this function, when saved as binary
	int GetTableBinaryFilename(const std::string & Dir, std::string & fn) ;

	// return non-0 iff something is wrong with the function
	int CheckIntegrity(void) ;

public :

	void Initialize(Workspace *WS, ARP *Problem, int IDX)
	{
		Destroy() ;
		_Workspace = WS ;
		_Problem = Problem ;
		_IDX = IDX ;
	}

	void DestroyFTBList(void) ;
	void DestroyTable(void) ;
	void Destroy(void)
	{
		DestroyTable() ;
		DestroyFTBList() ;
		if (NULL != _Arguments) {
			delete [] _Arguments ;
			_Arguments = NULL ;
			}
		_nArgs = 0 ;
		_BayesianCPTChildVariable = -1 ;
		_IsQueryIrrelevant = false ;
	}
	Function(void)
		:
		_Workspace(NULL), 
		_Problem(NULL), 
		_IDX(-1), 
		_IsQueryIrrelevant(false), 
		_BayesianCPTChildVariable(-1), 
		_nArgs(0), 
		_Arguments(NULL), 
		_BEBucket(NULL), 
		_OriginatingBucket(NULL), 
		_TableSize(-1), 
		_nTableBlocks(-1), 
		_TableBlockSize(-1), 
		_Table(NULL), 
//		_nArgumentDomainFactorization(0),
		_FTBsInMemory(NULL), 
		_ConstValue(-1.0)
	{
	}
	Function(Workspace *WS, ARP *Problem, int IDX)
		:
		_Workspace(WS), 
		_Problem(Problem), 
		_IDX(IDX), 
		_IsQueryIrrelevant(false), 
		_BayesianCPTChildVariable(-1), 
		_nArgs(0), 
		_Arguments(NULL), 
		_BEBucket(NULL), 
		_OriginatingBucket(NULL), 
		_TableSize(-1), 
		_nTableBlocks(-1), 
		_TableBlockSize(-1), 
		_Table(NULL), 
//		_nArgumentDomainFactorization(0),
		_FTBsInMemory(NULL), 
		_ConstValue(-1.0)
	{
	}
	virtual ~Function(void)
	{
		Destroy() ;
	}
} ;

// Variables[0 ... N), CurrentValueCombination[0 ... N), DomainSizes[0 ... problem->N)
inline __int64 ComputeFnTableAdr(int nVariables, const int *Variables, const int *CurrentValueCombination, const int *DomainSizes)
{
	if (nVariables < 1) 
		return -1 ;
	int i = nVariables - 1 ;
	__int64 adr = CurrentValueCombination[i] ;
	__int64 j = 1 ;
	for (--i ; i >= 0 ; i--) {
		j *= DomainSizes[Variables[i+1]] ;
		adr += j*CurrentValueCombination[i] ;
		}
	return adr ;
}

// Variables[0 ... N), CurrentValueCombination[0 ... N), DomainSizes[0 ... problem->N)
inline void ComputeArgCombinationFromFnTableAdr(__int64 ADR, int nVariables, const int *Variables, int *CurrentValueCombination, const int *DomainSizes)
{
	__int64 adr ;
	for (int i = nVariables-1 ; i >= 0 ; i--) {
		adr = ADR/DomainSizes[Variables[i]] ;
		CurrentValueCombination[i] = ADR - adr*DomainSizes[Variables[i]] ;
		ADR = adr ;
		}
}

// Arguments[0 ... N), CurrentValueCombination[0 ... N), DomainSizes[0 ... problem->N)
inline void EnumerateNextArgumentsValueCombination(int nArguments, const int *Arguments, int *CurrentValueCombination, const int *DomainSizes)
{
	for (int i = nArguments - 1 ; i >= 0 ; i--) {
		if (++CurrentValueCombination[i] < DomainSizes[Arguments[i]]) return ;
		CurrentValueCombination[i] = 0 ;
		}
}

} // namespace ARE

#endif // Function_HXX_INCLUDED
