#ifndef _SLS4MPE_GLOBAL_H_
#define _SLS4MPE_GLOBAL_H_

#include "DEFINES.h"
#ifdef ENABLE_SLS

namespace sls4mpe {
class ProbabilityTable; // just such that we know the name;
class Variable; // just such that we know the name;
class AssignmentManager; // just such that we know the name;
}
/************************************/
/* Compilation flags                */
/************************************/

#ifdef WIN32
	#define NT 1 
#endif

/************************************/
/* Standard includes                */
/************************************/
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <cstdlib>
#include <cmath>
#include <csignal>
#include <cassert>
#include <vector>
#include <fstream>

namespace sls4mpe {
/************************************/
/* Internal constants               */
/************************************/

#define TRUE 1
#define FALSE 0
#define NOVALUE -1

#define BIG 100000000
#define DOUBLE_BIG 1e100
#define LINE_LEN 1000
#define MAX_VARNAME_LENGTH 10000
#define INITIAL_MBSET_SIZE 10
//=== An assingment is only better than another if it is at least EPS better
//=== DON'T SET THIS TOO LOW !!!
//=== With 1e-7, there are already problems with numerical instabilities for
//=== larger problems and long runtimes. The caching makes this effect worse !
const double EPS = 1e-5; 

const double log_zero = -BIG/10000;
const int ALGO_GN = 0;
const int ALGO_GLS = 1;
const int ALGO_ILS = 2;
const int ALGO_HYBRID = 3;
const int ALGO_TABU = 4;
const int ALGO_MB = -1;

const int INIT_RANDOM = 0;
const int INIT_MB = 1;
const int INIT_EXTERNAL = 2;

const int CACHING_NONE = 0;
const int CACHING_INDICES = 1;
const int CACHING_SCORE = 2;
const int CACHING_GOOD_VARS = 3;
const int CACHING_GOOD_PQ = 4;

const int PERTUBATION_RANDOM_VARS_RANDOM_FLIP = 0;
const int PERTUBATION_RANDOM_VARS_SAMPLED_FLIP = 1;
const int PERTUBATION_RANDOM_POTS_RANDOM_INDEX = 2;
const int PERTUBATION_RANDOM_POTS_SAMPLED_INDEX = 3;
const int PERTUBATION_SAMPLED_POTS_RANDOM_INDEX = 4;
const int PERTUBATION_SAMPLED_POTS_SAMPLED_INDEX = 5;
const int PERTUBATION_MB = 6;

const int ACC_RW = 0;
const int ACC_BETTER = 1;
const int ACC_RESTART = 2;
const int ACC_BETTER_RW = 3;
const int ACC_BEST_WORSENING = 4;
const int ACC_RW_AFTER_N = 5;
const int ACC_RW_AFTER_N2 = 6;

#define MAX(A,B) ((A) > (B) ? (A) : (B))
#define MIN(A,B) ((A) < (B) ? (A) : (B))

struct instantiation{
	int var;
	int value;
};

//Parameter:
extern int maxRuns;
extern double maxTime;
extern int maxIterations;
extern long maxSteps;

extern int caching;
extern int init_algo;
extern int pertubationType;

extern int noout;
extern int output_res;
extern bool onlyConvertToBNT;

extern int output_to_stdout;
extern int output_lm;
extern bool justStats;
extern int output_runstats;
extern int output_trajectory;

extern char sls_filename[1000];
extern char res_filename[1000];
extern char network_filename[1000];
extern char traj_it_filename[1000];
extern char traj_fl_filename[1000];
extern FILE *outfile;
extern FILE *resfile;
extern FILE *traj_it_file;
extern FILE *traj_fl_file;

// old ILS params
extern double tmult;
extern double tdiv;
extern double tmin;
extern double tbase;
extern double T;
extern int pertubationType;
extern int num_vns_pertubation_strength;
extern int mbPertubation;
extern bool vns;
extern int restartNumFactor;
extern bool pertubationFixVars;
extern bool pertubation_rel;
extern double psp_base;
extern double psa_base;
extern int tl;
extern int accCriterion;
extern double worseningInterval;
extern double accNoise;

//old GLS params
extern double glsPenaltyIncrement;
extern int glsAspiration;

// actual ILS params;
extern int pertubation_strength;
extern double preprocessingSizeBound;
extern double maxMBWeight;
extern double glsSmooth;
extern int glsInterval;
extern int noise;
extern double cutoff;

extern double run_time_so_far;

extern bool outputBestMPE;
extern AssignmentManager assignmentManager;
extern double log_prob;
extern int num_vars;
extern int num_pots;
extern bool* isgoodvar;
extern struct fheap *heapOfGoodVars;
extern int glsReal;
extern int algo;
extern double glsPenaltyMultFactor;

extern ProbabilityTable** probTables;
extern Variable** variables;
extern AssignmentManager assignmentManager;

const int NUM_BP_VARS = 100000;
extern int externalInitValues[NUM_BP_VARS];

extern bool verbose;

}  // sls4mpe

#endif
#endif
