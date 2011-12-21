// CVO.cpp : Defines the entry point for the console application.
//

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <set>

#ifdef WINDOWS
#include <process.h>    /* _beginthread, _endthread */
#endif // WINDOWS

#include "cvo.h"

int CVO::Sample_SingleThreaded_MinFill(const std::string & uaifile, int nIterations)
{
	int ret = 1, i, j, k ;

	ARE::ARP p("") ;

	// uaiFN may have dir in it; extract filename.
	i = uaifile.length() - 1 ;
	for (; i >= 0 ; i--) {
#ifdef LINUX
		if ('\\' == uaifile[i] || '/' == uaifile[i]) break ;
#else
    if ('\\' == uaifile[i] || '//' == uaifile[i]) break ;
#endif
		}
	std::string fn = uaifile.substr(i+1) ;

	p.SetName(fn) ;
	if (0 != p.LoadFromFile(uaifile)) {
		ret = 2 ;
		return ret ;
		}
	if (p.N() < 1) {
		ret = 3 ;
		return ret ;
		}

	std::vector<std::set<int>> fn_signatures ;
	for (i = 0 ; i < p.nFunctions() ; i++) {
		ARE::Function *f = p.getFunction(i) ;
		if (NULL == f) continue ;
		std::set<int> fn_sig ;
		for (j = 0 ; j < f->N() ; j++) {
			int u = f->Argument(j) ;
			set<int>::iterator it = fn_sig.find(u) ;
			if (it == fn_sig.end()) 
				fn_sig.insert(u) ;
			}
		fn_signatures.push_back(fn_sig) ;
		}
	int nVars = p.N() ;
	int BestWidthKnown = INT_MAX ;

	time_t ttStart, ttEnd ;
	time(&ttStart) ;

	{
	// *********************** BEGIN iterative MinFill *********************** 
	/*
		3 variables declared/set outside of this scope are used here :
		1) IN : nVars
		2) IN : fn_signatures
		3) OUT : BestWidthKnown
	*/

	ARE::Graph MasterGraph ;
	MasterGraph.Create(nVars, fn_signatures) ;
	if (! MasterGraph._IsValid) {
		return 4 ;
		}

	static CMauiAVLTreeSimple avlVars2CheckScore ; // internally used variable
	static ARE::AdjVar TempAdjVarSpace[TempAdjVarSpaceSize] ; // internally used variable

	// do all the easy eliminations; this will give us a starting point for large-scale randomized searches later.
	j = MasterGraph.ComputeVariableEliminationOrder_Simple_wMinFillOnly(INT_MAX, false, true, 10, 1, 0.0, avlVars2CheckScore, TempAdjVarSpace, TempAdjVarSpaceSize) ;
	MasterGraph.ReAllocateEdges() ;

	ARE::Graph g ; // declare g outside of 'for' scope so that constructor/destructor is run only once
	BestWidthKnown = INT_MAX ;
	for (i = 0 ; i < nIterations ; i++) {
		// MasterGraph will be used as the starting point for MinFill searches.
		g = MasterGraph ;
		j = g.ComputeVariableEliminationOrder_Simple_wMinFillOnly(BestWidthKnown, true, false, 10, 1, 0.0, avlVars2CheckScore, TempAdjVarSpace, TempAdjVarSpaceSize) ;
		if (0 != j) 
			continue ;
		/*
			g._VarElimOrderWidth is the width of the ordering
			g._VarElimOrder is the ordering
		*/
		if (g._VarElimOrderWidth < BestWidthKnown) {
			// TODO : handle ordering with smallest width than ever before
			BestWidthKnown = g._VarElimOrderWidth ;
			}
		else if (g._VarElimOrderWidth == BestWidthKnown) {
			// TODO : handle ordering with smallest width seen before
			}
		}

	// *********************** END iterative MinFill *********************** 
	}

	time(&ttEnd) ;
	time_t d = ttEnd - ttStart ;
	printf("\n\n BEST w=%d nIterations=%d time=%d", (int) BestWidthKnown, (int) i, (int) d) ;

done :

	return ret ;
}

