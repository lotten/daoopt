#ifndef ARP_HXX_INCLUDED
#define ARP_HXX_INCLUDED

#include <stdlib.h>
#include <string>
#include <vector>

#include "Function.hxx"
#include "ProblemGraphNode.hxx"
using namespace std ;

namespace ARE // Automated Reasoning Engine
{

class ARP // Automated Reasoning Problem
{
protected :
	std::string _Name ;
public :
	inline const std::string & Name(void) const { return _Name ; }
	inline void SetName(const std::string & Name) { _Name = Name ; }

protected :
	std::string _FileName ;
	std::string _EvidenceFileName ;
public :
	inline const std::string & FileName(void) const { return _FileName ; }
	inline const std::string & EvidenceFileName(void) const { return _EvidenceFileName ; }
	inline void SetFileName(const std::string & FileName) { _FileName = FileName ; }

protected :
	int _nVars ; // number of variables; indeces run [0,_nVars-1]
	int *_K ; // domain size of each variable; size of this array is _nVars.
	int *_Value ; // current value of each variable; for var i, legal values run [0, _K[i]-1].
	int _nSingletonDomainVariables ;
public :
	inline int N(void) const { return _nVars ; }
	inline int K(int IDX) const { return _K[IDX] ; }
	inline const int *K(void) const { return _K ; }
	inline int Value(int IDX) const { return _Value[IDX] ; }
	inline int nSingletonDomainVariables(void) const { return _nSingletonDomainVariables ; }

	inline int SetN(int N)
	{
		if (_nVars < 0) 
			return 1 ;
		if (_nVars == N) 
			return 0 ;
		_nVars = N ;
		if (NULL != _K) { delete [] _K ; _K = NULL ; }
		if (NULL != _Value) { delete [] _Value ; _Value = NULL ; }
		if (_nVars > 0) {
			_K = new int[_nVars] ;
			_Value = new int[_nVars] ;
			if (NULL == _K || NULL == _Value) {
				Destroy() ;
				return 1 ;
				}
			for (int i = 0 ; i < _nVars ; i++) {
				_K[i] = -1 ;
				_Value[i] = -1 ;
				}
			}
		return 0 ;
	}
	inline int SetK(int *K)
	{
		if (NULL == _K) 
			return 1 ;
		for (int i = 0 ; i < _nVars ; i++) 
			_K[i] = K[i] ;
		return 0 ;
	}

	// given evidence Var=Val, this function will eliminate the evidence variable from all functions
	int EliminateEvidenceVariable(int Var, int Val) ;

	// eliminate all variables, whose domain has just one value, from all functions.
	// all functions that have no arguments left, as the result, will be turned into const functions.
	int EliminateSingletonDomainVariables(void) ;

	// ***************************************************************************************************
	// Functions of the problem
	// ***************************************************************************************************

protected :

	bool _FunctionsAreConvertedToLog ;
	int _nFunctions ; // number of functions in the  problem.
	Function **_Functions ; // a list of  functions.
	__int64 _FunctionsSpace ; // space, in bytes, taken up by all functions.
	int _nFunctionsIrrelevant ; // number of functions in the  problem, not relevent to the query.

public :

	inline bool FunctionsAreConvertedToLog(void) const { return _FunctionsAreConvertedToLog ; }
	inline int nFunctions(void) const { return _nFunctions ; }
	inline ARE::Function *getFunction(int IDX) const { return _Functions[IDX] ; }
	inline __int64 FunctionsSpace(void) const { return _FunctionsSpace ; }
	inline int nFunctionsIrrelevant(void) const { return _nFunctionsIrrelevant ; }

	// in bytes, the space required
	__int64 ComputeFunctionSpace(void) ;

	int ConvertFunctionsToLog(void) ;
	
	// return non-0 iff something is wrong with any of the functions
	int CheckFunctions(void) ;

	// find Bayesian CPT for the given variable
	inline ARE::Function *GetCPT(int V)
	{
		for (int i = 0 ; i < _nFunctions ; i++) {
			ARE::Function *f = _Functions[i] ;
			if (NULL == f) continue ;
			if (V != f->BayesianCPTChildVariable()) 
				continue ;
			return f ;
			}
		return NULL ;
	}

	// given a variable, it will compute all ancestors, assuming functions are bayesian
	int ComputeBayesianAncestors(int V, int *AncestorFlags, int *Workspace) ;

	// compute which functions are relevant/irrelevant to the query; we process recursively from leaves up, checking variables that are in a single function.
	// VarElimOp=0 means sum, VarElimOp=1 means max.
	// we assume Factor is initialized.
	int ComputeQueryRelevance_VarElimination(ARE_Function_TableType & Factor, char VarElimOp, char CombinationOp) ;

	// singleton consistency; for probabilistic network, check each domain value against 0-probabilities.
	// return value  0 means ok.
	// return value -1 means domain of some variable became empty.
	int ComputeSingletonConsistency(int & nNewSingletonDomainVariables) ;
	// The following function returns is_consistent[i][j] where i indexes variables and j indexes the domains of variables
	// is_consistent[i][j] is true if the variable value combination is consistent and false otherwise
	void SingletonConsistencyHelper(vector<vector<bool> > & is_consistent) ;

protected :
	ARPGraphNode *_GraphAdjacencyMatrix ; // for each variable, a list of adjacent variables, functions, etc.
	int _nConnectedComponents ;
	int _nSingletonVariables ;
public :
	inline int Degree(int IDX) const { return _GraphAdjacencyMatrix ? _GraphAdjacencyMatrix[IDX].Degree() : 0 ; }
	inline ARPGraphNode *GraphNode(int IDX) const { return _GraphAdjacencyMatrix ? _GraphAdjacencyMatrix + IDX : NULL ; }
	inline int nConnectedComponents(void) const { return _nConnectedComponents ; }
	inline int nSingletonVariables(void) const { return _nSingletonVariables ; }
	int ComputeGraph(void) ;
	void DestroyGraph(void) ;

protected :
	int _MinDegreeOrdering_InducedWidth ; // MinDegreeOrdering : width of the ordering.
	int *_MinDegreeOrdering_VarList ; // MinDegreeOrdering : a list of variables, in the order.
	int *_MinDegreeOrdering_VarPos ; // MinDegreeOrdering : for each variable, its position in the order.
	int _MinFillOrdering_InducedWidth ; // MinFillOrdering : width of the ordering.
	int *_MinFillOrdering_VarList ; // MinFillOrdering : a list of variables, in the order.
	int *_MinFillOrdering_VarPos ; // MinFillOrdering : for each variable, its position in the order.
protected :
	int ComputeMinFillOrdering(void) ; // this is for internal use; use ComputeMinFillOrdering(int nRandomTries)
public :
	inline int MinDegreeOrdering_InducedWidth(void) const { return _MinDegreeOrdering_InducedWidth ; }
	inline const int *MinDegreeOrdering_VarList(void) const { return _MinDegreeOrdering_VarList ; }
	inline const int *MinDegreeOrdering_VarPos(void) const { return _MinDegreeOrdering_VarPos ; }
	inline int MinFillOrdering_InducedWidth(void) const { return _MinFillOrdering_InducedWidth ; }
	inline const int *MinFillOrdering_VarList(void) const { return _MinFillOrdering_VarList ; }
	inline const int *MinFillOrdering_VarPos(void) const { return _MinFillOrdering_VarPos ; }
	int ComputeMinDegreeOrdering(void) ;
	void DestroyMinDegreeOrdering(void) ;
	void DestroyMinFillOrdering(void) ;
	int ComputeMinFillOrdering(int nRandomTries, int BestKnownWidth) ;
	int ComputeInducedWidth(const int *VarList, const int *Var2PosMap, int & InducedWidth) ;
	int TestVariableOrdering(const int *VarList, const int *Var2PosMap) ;

public :
	virtual int SaveXML(const std::string & Dir) ;
	virtual int SaveUAI08(const std::string & Dir) ;
	int GetFilename(const std::string & Dir, std::string & fn) ; // filename will not have an extension

	// this function generates a random Bayesian network structure, that is it does not generate actual tables, 
	// just the variables (arguments) for each function (CPT).
	// tables can be generated by calling FillInFunctionTables().
	int GenerateRandomUniformBayesianNetworkStructure(int N, int K, int P, int C, int ProblemCharacteristic) ;

	// this function checks if all functions have tables; if not, it will fill in the table, checking the type. 
	// e.g. if the function is a Bayesian CPT, it will fill it in appropriately.
	int FillInFunctionTables(void) ;

	// load from file.
	int LoadFromFile(const std::string & FileName) ;

	// this function loads problem from UAI format file; we assume data is already in buf.
	int LoadUAIFormat(char *buf, int L) ;

	// this function loads evidence file.
	// file format is : <nEvidence> followed by nEvidence times <evidVar> <evidValue>.
	// it will set evidence as the current value of the variable.
	int LoadUAIFormat_Evidence(const char *fn) ;

public :

	void Destroy(void)
	{
		DestroyGraph() ;
		DestroyMinDegreeOrdering() ;
		DestroyMinFillOrdering() ;
		if (NULL != _Functions) {
			for (int i = 0 ; i < _nFunctions ; i++) {
				if (NULL != _Functions[i]) 
					delete _Functions[i] ;
				}
			delete [] _Functions ;
			_Functions = NULL ;
			}
		_FunctionsAreConvertedToLog = false ;
		_nFunctions = 0 ;
		_nFunctionsIrrelevant = -1 ;
		if (NULL != _K) {
			delete [] _K ;
			_K = NULL ;
			}
		if (NULL != _Value) {
			delete [] _Value ;
			_Value = NULL ;
			}
		_nVars = 0 ;
		_nSingletonDomainVariables = -1 ;
		_FileName.erase() ;
	}
	ARP(const std::string & Name)
		:
		_Name(Name), 
		_nVars(0), 
		_K(NULL), 
		_Value(NULL), 
		_nSingletonDomainVariables(-1), 
		_FunctionsAreConvertedToLog(false), 
		_nFunctions(0),
		_Functions(NULL), 
		_FunctionsSpace(0), 
		_nFunctionsIrrelevant(-1), 
		_GraphAdjacencyMatrix(NULL), 
		_nConnectedComponents(0), 
		_nSingletonVariables(0), 
		_MinDegreeOrdering_InducedWidth(-1), 
		_MinDegreeOrdering_VarList(NULL), 
		_MinDegreeOrdering_VarPos(NULL), 
		_MinFillOrdering_InducedWidth(-1), 
		_MinFillOrdering_VarList(NULL), 
		_MinFillOrdering_VarPos(NULL)
	{
	}
	virtual ~ARP(void)
	{
		Destroy() ;
	}
} ;

int fileload_getnexttoken(char * & buf, int & L, char * & B, int & l) ;

} // namespace ARE

#endif // ARP_HXX_INCLUDED
