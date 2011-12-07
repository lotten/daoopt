#ifndef _SLS4MPE_MAIN_ALGO_H_
#define _SLS4MPE_MAIN_ALGO_H_

#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/Variable.h"
#include "sls4mpe/ProbabilityTable.h"
#include "sls4mpe/AssignmentManager.h"

namespace sls4mpe {

void printStats();

void print_parameters();
void print_assignment();
void print_end_problem();
void print_start_run();
void print_end_run();
void print_new_best_in_run();
void print_start_problem();
void print_curr_state();
void print_run_stats(FILE *out);
void print_ils_stats();
void print_tunable_parameters();
void setInputDependentParameters();

void update_assignment_count();
void update_lm_count();
void update_ils_count();

void exampleProblemDefition();
void allocateVarsAndPTs(bool name);
void allocateMemoryForDataStructures();
void tearDown();
void first_init();
void read_problem(int argc,char *argv[]);

void runAlgorithm(int **outBestAssignment, double *outLogLikelihood);

void initFromVarsAndProbTables();
void init_run();
void initComputingChangingCachedStructures(bool keepPenalties);

void mb_init();
void random_init();
void sample_value_init_and_init_indices(int greedy);
void setPotentialIndicesScoresAndGoodvars(bool keepPenalties);

double sizeOfVariableSet(int numVars, int* vars);
double sizeOfVariableSet(vector<int> vars);

void anytime_mb();

void greedy_noise();
void gls();
void ils();
void mb_ils_hybrid();
void tabu();

void computeInitValues();
void pertubation(int pertubationStrength, int pertubationType);
void basic_local_search();
void acceptance_criterion();

void output_lms_random ();
void output_lms_guided();


bool flip_var_to(struct instantiation inst, bool flipBack);
struct instantiation best_new_inst(bool best_even_if_not_improving);
int random_new_val(int var);
int sampled_new_val(int var);
void new_ils_base_solution();

void end_run();
void update_if_new_best_in_run();

void handle_interrupt(int sig);

}  // sls4mpe

#endif
#endif
