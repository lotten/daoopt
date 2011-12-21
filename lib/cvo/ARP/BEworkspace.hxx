#ifndef BEworkspace_HXX_INCLUDED
#define BEworkspace_HXX_INCLUDED

#include <Function.hxx>
#include <Problem.hxx>
#include <Bucket.hxx>
#include <Workspace.hxx>

namespace ARE { class Workspace ; }

namespace BucketElimination
{

class BEworkspace : public ARE::Workspace
{

protected :

	int _nVars ; // number of variables
	int *_VarOrder ; // order of variables, as an array of variable indeces; _VarOrder[0] is first bucket, _VarOrder[N-1] is last bucket.
	int *_VarPos ; // positions of variables in the order.
	Bucket **_Var2BucketMapping ; // bucket for each variable

public :

	inline int N(void) const { return _nVars ; }
	inline const int *VarOrder(void) const { return _VarOrder ; }
	inline const int *VarPos(void) const { return _VarPos ; }
	inline Bucket *MapVar2Bucket(int v) const { return _Var2BucketMapping[v] ; }

	// ****************************************************************************************
	// Query.
	// ****************************************************************************************

protected :

	ARE_Function_TableType _AnswerFactor ;
	char _FnCombination_VarElimination_Type ;

public :

	inline ARE_Function_TableType AnswerFactor(void) const { return _AnswerFactor ; }
	inline void AddAnswerFactor(char comb_type, ARE_Function_TableType & F) { if (0 == comb_type) _AnswerFactor *= F ; else if (1 == comb_type) _AnswerFactor += F ; }
	inline void SetFnCombination_VarElimination_Type(char FnCombination_VarElimination_Type) { _FnCombination_VarElimination_Type = FnCombination_VarElimination_Type ; }

	// ****************************************************************************************
	// Bucket-tree structure.
	// ****************************************************************************************

	int _nBuckets ;
	Bucket *_Buckets[MAX_NUM_BUCKETS] ;
	int _MaxNumChildren ; // maximum number of children of any bucket
	int _MaxNumVarsInBucket ; // maximum number of variables in any bucket
	int _nBucketsWithSingleChild_initial ; // number of buckets with a single child; these buckets can be eliminated by using superbuckets.
	int _nBucketsWithSingleChild_final ; // number of buckets with a single child; these buckets can be eliminated by using superbuckets.
	int _nBuckets_initial ;
	int _nBuckets_final ;
	int _nVarsWithoutBucket ;
	int _nConstValueFunctions ;
	bool _DeleteUsedTables ; // delete tables used (child tables when parent bucket is computed)

	// an ordered list of buckets to compute; typically this list is ordered in the order of decreasing height (of the bucket), 
	// and the computation is carried out from last-to-first.
	// the allocated size of this array is _nVars.
	long *_BucketOrderToCompute ;

public :

	inline int nBuckets(void) const { return _nBuckets ; }
	inline BucketElimination::Bucket *getBucket(int IDX) const { return _Buckets[IDX] ; }
	inline int MaxNumChildren(void) const { return _MaxNumChildren ; }
	inline int MaxNumVarsInBucket(void) const { return _MaxNumVarsInBucket ; }
	inline int nBucketsWithSingleChild_initial(void) const { return _nBucketsWithSingleChild_initial ; }
	inline int nBucketsWithSingleChild_final(void) const { return _nBucketsWithSingleChild_final ; }
	inline int nBuckets_initial(void) const { return _nBuckets_initial ; }
	inline int nBuckets_final(void) const { return _nBuckets_final ; }
	inline int nVarsWithoutBucket(void) const { return _nVarsWithoutBucket ; }
	inline int nConstValueFunctions(void) const { return _nConstValueFunctions ; }
	inline bool DeleteUsedTables(void) const { return _DeleteUsedTables ; }
	inline int CurrentBucketIDX(int IDX) const { return _BucketOrderToCompute[IDX] ; }
	inline void SetDeleteUsedTables(bool v) { _DeleteUsedTables = v ; }
	int AssignProblemFunctionsToBuckets(void) ;
	int ComputeMaxNumChildren(void) ;
	int ComputeMaxNumVarsInBucket(void) ;
	int ComputeNBucketsWithSingleChild(void) ;
	int CheckBucketTreeIntegrity(void) ;

	void DestroyBuckets(void) ;
	virtual int CreateBuckets(int N, const int *VariableOrder, bool CreateSuperBuckets) ;

	// create an order in which buckets should be processed
	virtual int CreateComputationOrder(void) ;

public :

	int GetMaxBucketFunctionWidth(void) ;

	// return number of entries over all tables
	__int64 ComputeNewFunctionSpace(void) ;

	// return complexity of computing all new functions; this is defined as the sum over all buckets, of the product of domain sizes of variables in the bucket
	__int64 ComputeNewFunctionComputationComplexity(void) ;

	// simulate execution and compute the minimum amount of space required (for new functions).
	// this assumes that when a bucket is processed, all input function (tables) are deleted.
	__int64 SimulateComputationAndComputeMinSpace(bool IgnoreInMemoryTables) ;

	// run regular BE on the bucket-tree
	virtual int RunSimple(char FnCombination_VarElimination_Type) ;

	// **************************************************************************************************
	// Problem generation
	// **************************************************************************************************

public :

	// this function generates a random uniform Bayesian network for the given parameters.
	// it will not fill in any function tables.
	int GenerateRandomBayesianNetworkStructure(
		int N, // # of variables
		int K, // same domain size for all variables
		int P, // # of parents per CPT
		int C,  // # of CPTs; variables that are not a child in a CPT will get a prior.
		int ProblemCharacteristic // 0 = totally random, 1 = 1 leaf node
		) ;

public :

	virtual int Initialize(ARE::ARP & Problem, const int *VarOrderingAsVarList) ;
	BEworkspace(const char *BEEMDiskSpaceDirectory) ;
	virtual ~BEworkspace(void)
	{
		DestroyBuckets() ;
	}
} ;

// this function generates given number of random uniform Bayesian networks for the given parameters, 
// within specified complexity bounds.
// problems will be saved in uai format.
// return value is number of problem generated/saved.
int GenerateRandomBayesianNetworksWithGivenComplexity(
	int nProblems, 
	int N, // # of variables
	int K, // same domain size for all variables
	int P, // # of parents per CPT
	int C,  // # of CPTs; variables that are not a child in a CPT will get a prior.
	int ProblemCharacteristic, // 0 = totally random, 1 = 1 leaf node
	__int64 MinSpace, 
	__int64 MaxSpace
	) ;

} // namespace BucketElimination

#endif // BEworkspace_HXX_INCLUDED
