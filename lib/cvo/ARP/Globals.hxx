#ifndef ARE_Globals_HXX_INCLUDED
#define ARE_Globals_HXX_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "Mutex.h"

#ifdef LINUX
typedef int64_t __int64;
typedef unsigned int DWORD;
#include <cstring>
#include <cfloat>

#include <limits>
#define _FPCLASS_NINF (- std::numeric_limits<double>::infinity())
#define _I64_MAX (std::numeric_limits<__int64>::max())
#endif


// macro to get system time
#ifdef LINUX
#define GETCLOCK() clock()
#define SLEEP(X) usleep(1000*X)
#else
#define GETCLOCK() GetTickCount()
#define SLEEP(X) Sleep(X)
#endif

namespace ARE
{

// Variable Elimination Complexity = # operations to eliminate a variable (from a bucket/cluster e.g.).
// this typically is the product of domain sizes of all variables involved.
// set single-var elimination complexity limit at 10^15 = 1000 trillion
#define InfiniteSingleVarElimComplexity 1000000000000000LL
#define InfiniteVarElimComplexity       1000000000000000LL
#define InfiniteSingleVarElimComplexity_log 999.0
#define InfiniteVarElimComplexity_log       999.0
// note that _I64_MAX = 9,223,372,036,854,775,807
//          out limit =     1,000,000,000,000,000
// note that we want enough distance between MaxSingleVarElimComplexity and _I64_MAX so that can detect going over MaxSingleVarElimComplexity without overflowing the int64.

#define MAX_NUM_VARIABLES_PER_BUCKET	128
#define MAX_NUM_FUNCTIONS_PER_BUCKET	256
#define MAX_NUM_ARGUMENTS_PER_FUNCTION	128
#define MAX_NUM_VALUES_PER_VAR_DOMAIN	100
#define MAX_NUM_FACTORS_PER_FN_DOMAIN	64
#define MAX_NUM_BUCKETS 65536
#define MAX_DEGREE_OF_GRAPH_NODE		1024
#define MAX_NUM_VARIABLES_PER_PROBLEM   1000000

#define ERRORCODE_generic									100
#define ERRORCODE_too_many_variables						101
#define ERRORCODE_too_many_functions						102
#define ERRORCODE_input_FTB_not_computed					103
#define ERRORCODE_input_FTB_computed_but_failed_to_fetch	104
#define ERRORCODE_fn_ptr_is_NULL							105
#define ERRORCODE_cannot_open_file							106
#define ERRORCODE_cannot_allocate_buffer_size_too_large		107
#define ERRORCODE_file_too_large							108
#define ERRORCODE_problem_type_unknown						109
#define ERRORCODE_EliminationComplexityWayTooLarge			110
#define ERRORCODE_EliminationWidthTooLarge					111
#define ERRORCODE_EliminationComplexityTooLarge				112
#define ERRORCODE_out_of_memory								113
#define ERRORCODE_memory_allocation_failure					114
#define ERRORCODE_NoVariablesLeftToPickFrom					115

#define VAR_ELIMINATION_TYPE_PROD_SUM 0
#define VAR_ELIMINATION_TYPE_PROD_MAX 1
#define VAR_ELIMINATION_TYPE_SUM_MAX 2

extern FILE *fpLOG ;
extern ARE::utils::RecursiveMutex LOGMutex ;

extern const double neg_infinity ;
extern       double pos_infinity ;

// returns 0 iff ok, error code otherwise.
int LoadVarOrderFromFile(
	// IN
	const std::string & fnVO, 
	int N,
	// OUT
	int *VarListBeforeEvidence, 
	int & Width
	) ;

int Initialize(void) ;

} // namespace ARE

// number factorization; done by division (not very efficient; ok for small numbers).
// output is sorted in increasing order.
char PrimeFactoringByTrialDivision(char N, char factors[128]) ;

#endif // ARE_Globals_HXX_INCLUDED
