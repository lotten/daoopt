#ifndef ProblemGraphNode_HXX_INCLUDED
#define ProblemGraphNode_HXX_INCLUDED

#include <Function.hxx>

namespace ARE
{

class ARPGraphNode
{
protected :
	int _V ;
	int _Degree ;
	int *_AdjacentVariables ;
	int _nAdjacentFunctions ;
	Function **_AdjacentFunctions ;
public :
	inline int V(void) const { return _V ; }
	inline int nAdjacentFunctions(void) const { return _nAdjacentFunctions ; }
	inline int nAdjacentFunctions_QueryReleventFunctionsOnly(void) 
	{
		int n = 0 ;
		for (int i = 0 ; i < _nAdjacentFunctions ; i++) {
			if (_AdjacentFunctions[i]->IsQueryIrrelevant()) 
				continue ;
			++n ;
			}
		return n ;
	}
	inline Function *AdjacentFunction_QueryRelevantFunctionsOnly(int IDX) const
	{
		int n = 0 ;
		for (int i = 0 ; i < _nAdjacentFunctions ; i++) {
			if (_AdjacentFunctions[i]->IsQueryIrrelevant()) 
				continue ;
			if (n == IDX) 
				return _AdjacentFunctions[i] ;
			++n ;
			}
		return NULL ;
	}
	inline Function *AdjacentFunction(int IDX) const { return _AdjacentFunctions[IDX] ; }
	inline int Degree(void) const { return _Degree ; }
	inline int AdjacentVariable(int IDX) const { return _AdjacentVariables[IDX] ; }
	inline bool IsAdjacent(int Variable) { for (int i = 0 ; i < _Degree ; i++) { if (_AdjacentVariables[i] == Variable) return true ; } return false ; }
	inline bool IsAdjacent_QueryReleventFunctionsOnly(int Variable) 
	{
		for (int i = 0 ; i < _nAdjacentFunctions ; i++) {
			if (_AdjacentFunctions[i]->IsQueryIrrelevant()) 
				continue ;
			if (_AdjacentFunctions[i]->ContainsVariable(Variable)) 
				return true ;
			}
		return false ;
	}
	inline void SetV(int V) { _V = V ; }
	int SetN(int nAdjacentFunctions) 
	{
		if (nAdjacentFunctions < 0) nAdjacentFunctions = 0 ;
		if (_nAdjacentFunctions == nAdjacentFunctions) 
			return 0 ;
		if (NULL != _AdjacentFunctions) 
			{ delete [] _AdjacentFunctions ; _AdjacentFunctions = NULL ; _nAdjacentFunctions = 0 ; }
		if (nAdjacentFunctions > 0) {
			_AdjacentFunctions = new Function*[nAdjacentFunctions] ;
			if (NULL == _AdjacentFunctions) 
				return 1 ;
			}
		_nAdjacentFunctions = nAdjacentFunctions ;
		for (int i = 0 ; i < _nAdjacentFunctions ; i++) 
			_AdjacentFunctions[i] = NULL ;
		return 0 ;
	}
	inline void SetFunction(int IDX, Function *fn) { _AdjacentFunctions[IDX] = fn ; }
	int ComputeAdjacentVariables(void)
	{
		if (NULL != _AdjacentVariables) {
			delete [] _AdjacentVariables ;
			_AdjacentVariables = NULL ;
			}
		_Degree = 0 ;

		int i, n = 0 ;
		for (i = 0 ; i < _nAdjacentFunctions ; i++) 
			n += (_AdjacentFunctions[i]->N() - 1) ; // subtract 1 since _V also is an argument of the fn, but we don't want to count it since it is not an adj var
		// n is an upper bound on the degree
		if (0 == n) 
			return 0 ;

		_AdjacentVariables = new int[n] ;
		if (NULL == _AdjacentVariables) 
			return 1 ;
		int j, k ;
		for (i = 0 ; i < _nAdjacentFunctions ; i++) {
			Function *f = _AdjacentFunctions[i] ;
			for (j = 0 ; j < f->N() ; j++) {
				int v = f->Argument(j) ;
				if (v == _V) continue ;
				for (k = 0 ; k < _Degree ; k++) 
					{ if (_AdjacentVariables[k] == v) break ; }
				if (k < _Degree) 
					continue ;
				_AdjacentVariables[_Degree++] = v ;
				}
			}

		return 0 ;
	}

	int test(void)
	{
		int i, j, k ;

		if (_V < 0 || _Degree < 0) 
			return 1 ;

		// check there are no duplicate functions
		for (i = 0 ; i < _nAdjacentFunctions ; i++) {
			for (j = 0 ; j < i ; j++) {
				if (_AdjacentFunctions[i] == _AdjacentFunctions[j]) 
					return 2 ;
				}
			}

		// check that each function contains _V
		for (i = 0 ; i < _nAdjacentFunctions ; i++) {
			Function *f = _AdjacentFunctions[i] ;
			for (j = 0 ; j < f->N() ; j++) 
				{ if (_V == f->Argument(j)) break ; }
			if (j >= f->N()) 
				return 3 ;
			}

		// check that arguments of functions are among the adj vars
		for (i = 0 ; i < _nAdjacentFunctions ; i++) {
			Function *f = _AdjacentFunctions[i] ;
			for (j = 0 ; j < f->N() ; j++) {
				int v = f->Argument(j) ;
				if (_V == v) continue ;
				for (k = 0 ; k < _Degree ; k++) 
					{ if (_AdjacentVariables[k] == v) break ; }
				if (k >= _Degree) 
					return 4 ;
				}
			}

		return 0 ;
	}

public :
	ARPGraphNode(void)
		:
		_V(-1), 
		_Degree(0), 
		_AdjacentVariables(NULL), 
		_nAdjacentFunctions(0), 
		_AdjacentFunctions(NULL)
	{
	}
	~ARPGraphNode(void)
	{
		if (_AdjacentFunctions) 
			delete [] _AdjacentFunctions ;
		if (_AdjacentVariables) 
			delete [] _AdjacentVariables ;
	}
} ;

} // namespace ARE

#endif // ProblemGraphNode_HXX_INCLUDED
