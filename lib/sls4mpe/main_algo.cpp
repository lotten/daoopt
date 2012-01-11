/*
 * SLS4MPE by Frank Hutter, http://www.cs.ubc.ca/labs/beta/Projects/SLS4MPE/
 * Some modifications by Lars Otten
 */
#include "DEFINES.h"
#ifdef ENABLE_SLS

#include <iostream>
#include "sls4mpe/main_algo.h"

#include "sls4mpe/timer.h"
#include "sls4mpe/random_numbers.h"
#include "sls4mpe/my_set.h"
#include "sls4mpe/ProblemReader.h"
#include "sls4mpe/MiniBucketElimination.h"
#include "sls4mpe/Variable.h"

#include "sls4mpe/fheap.h"
using namespace std;

namespace sls4mpe {

#define FORBIDDEN(var,val) (variables[(var)]->fixed || num_flip <= variables[(var)]->tabuValues[(val)]+tl || variables[(var)]->value == (val))

int num_vars;
int num_pots;
int numVarValCombos;

AssignmentManager* assignmentManager;
ProblemReader* pR;
MiniBucketElimination* mbeElim;
ProbabilityTable** probTables;
Variable** variables;
int* mbAssignment;
int* fakeEvidenceForMB;
int numFakeEvidenceForMB;
bool* isgoodvar;
int* initValues;
int externalInitValues[NUM_BP_VARS];
int MAXVARS_IN_FACTOR = 30;

/*********************************************/
/* Main changing data structures			 */
/*********************************************/

int num_run;
long num_flip;
long num_iteration;
int abort_flag;
double log_prob;
double last_log_prob;
long last_steps;
int *last_ils_value;
int num_flipped_since_last_ils_solution;
int *flipped_since_last_ils_solution;
int *value_of_flipped_in_last_ils_solution;
int *vns_pertubation_strength;
int num_vns_pertubation_strength = 5;
bool onlyConvertToBNT = false;

//=== Variables we only need in one function, but which need allocation.

int *best_vars;
int *best_vals;

double *single_goods;
double *sample_probs;


/*****************************************************/
/* Global flags and parameters with default values   */
/*****************************************************/
bool outputBestMPE = true;
int maxRuns = 10;

int maxIterations = BIG;
double maxTime = BIG;
long int maxSteps = BIG;

int caching = CACHING_GOOD_VARS;
int algo = ALGO_GLS; // ALGO_ILS

int tl = 0;
bool tl_rel = false;//true;

int mbPertubation = 0;
double mbInitWeight = 1e5;
double mbInitWeightForHybrid = 1e4;
double maxMBWeight = 1e7;
double psp_base = 1;
double psa_base = 1; // 1;

double preprocessingSizeBound = 1000;
double preprocessingTime;

int glsReal = 1;
double glsPenaltyMultFactor = 10000;
double glsPenaltyIncrement = 1.0;
double glsSmooth = 0.999;
int glsInterval = 200;
int glsAspiration = 0;

double tmult = 1.01;
double tdiv = 2;
double tmin = 0.001;
double e = 2.7182818285;
double tbase = e;
double T = 0.01;

bool verbose = false;

/*
int init_algo = INIT_RANDOM;

//=== Parameters of the pertubation.
int pertubationType = PERTUBATION_RANDOM_VARS_RANDOM_FLIP; // 0:random vars with random outcome
bool vns = false;
bool pertubationFixVars = false;
int pertubation_strength = 4;//1;//5
bool pertubation_rel = false;

//=== Parameters of the acceptance criterion.
int accCriterion = ACC_BETTER;
int restartNumFactor = 10;
double worseningInterval = 5; // was absolute 100 and performed very well.
double accNoise = 0.01;
*/


///*
int init_algo = INIT_MB;

//=== Parameters of the pertubation.
int pertubationType = PERTUBATION_RANDOM_POTS_RANDOM_INDEX; // 0:random vars with random outcome
bool vns = false;
bool pertubationFixVars = true;
int pertubation_strength = 2;
bool pertubation_rel = false;

//=== Parameters of the acceptance criterion.
int accCriterion = ACC_BETTER_RW;//ACC_RW_AFTER_N2;
int restartNumFactor = 5;
double worseningInterval = 5; // was absolute 100 and performed very well.
double accNoise = 0.01;
//*/


int noise = 40;
double cutoff = 10;

int start_iteration_of_current_try;
int save_pertubation_strength = NOVALUE;
int output_to_stdout = 0;
int output_lm = 0;
int output_trajectory = 0;
int output_runstats = 0;
int output_res = 0;
int noout = 0;
bool justStats;

/********************************************************
         Program internal parameters.
 ********************************************************/
int seedThisRun;
int num_good_vars;
int *good_vars; // good_vars[j] = var <=> Flipping var can increase log_prob.
struct fheap *heapOfGoodVars;

int *vars_permuted;
int num_vars_permuted;

int num_pots_flipped;
int *pots_flipped;    // pots_flipped[j] = pot <=> pot has been flipped in current pertubation.

long lastImprovingIteration;
double best_logprob_this_try;
/************************************/
/* Statistics                       */
/************************************/
int inducedWidth; // along the min-degree ordering
double inducedWeight; // along the min-degree ordering
double run_time_so_far;
double bestQualNotAccepted;
int *bestNotAccepted;

double overall_time_so_far;
time_t timestamp_start;

double init_time;
double runInitTime;

FILE *outfile = stdout;
FILE *resfile;
FILE *traj_it_file;
FILE *traj_fl_file;

char network_filename[1000];
char sls_filename[1000];
char res_filename[1000];
char traj_it_filename[1000];
char traj_fl_filename[1000];

/********************************************************
 ========================================================
                          MAIN 
 ========================================================
 ********************************************************/

/********************************************************
           INIT PROBLEM
 Initializes the problem once to begin with.
 ********************************************************/
void first_init(){
	seed = 1; // DON'T SET IT TO 0 !!! Thomas' random number generator will only return zeros then !!!

	//=== Init globals.
	preprocessingTime = 0;
	overall_time_so_far = 0;
	time(&timestamp_start);
	assignmentManager->init();
	num_run = 0;
}

void read_problem(int argc,char *argv[]){
	pR->parse_parameters(argc, argv);
	if(verbose) {
		fprintf(outfile, "begin call\n");
		for (int i=0;i < argc;i++){
			fprintf(outfile, " %s", argv[i]);
		}
		fprintf(outfile, "\nend call\n\n");
	}
	pR->readNetwork();
}

void allocateVarsAndPTs(bool name){
	int var, pot;
	//==== Allocate memory for first dim of 2-dim arrays.
	probTables       = new ProbabilityTable*[num_pots];
	variables        = new Variable*[num_vars];

	//=== Allocate memory for probTables.	
	for(pot=0; pot < num_pots; pot++){
		probTables[pot] = new ProbabilityTable();
	}

	//==== Allocate memory for second dim of 2-dim arrays.
	for(var=0; var < num_vars; var++){
		variables[var] = new Variable();
		if(name) variables[var]->name = new char[MAX_VARNAME_LENGTH];
	}
}

void deallocateVarsAndPTs(bool name) {
  for(int pot=0; pot < num_pots; pot++){
      if (probTables[pot]) delete probTables[pot];
  }
  for(int var=0; var < num_vars; var++){
    if (variables[var]) {
      if (name && variables[var]->name)
        delete[] variables[var]->name;
      delete variables[var];
    }
  }

  delete assignmentManager;
  delete mbeElim;
}

void runAlgorithm(int **outBestAssignment, double *outLogLikelihood){	
	if(preprocessingSizeBound > 0) outputBestMPE = false;

//	assignmentManager = new AssignmentManager();
//	pR = new ProblemReader();
	mbeElim = new MiniBucketElimination();

	first_init();
	initFromVarsAndProbTables();

	print_parameters();
	print_start_problem();

	time_t now; time(&now);
	double elapsed = difftime(now, timestamp_start);
	fprintf(stdout, "Preprocessing for SLS complete: %i seconds\n", int(elapsed));

	if(algo == ALGO_MB){
		assignmentManager->newRun();
		anytime_mb();
		end_run();
	} else {
		while (! abort_flag && num_run < maxRuns) {
			init_run();
			switch(algo) {
				case ALGO_GN:
					if(verbose) printf("Running algorithm G+StS\n");
					greedy_noise();
					break;
				case ALGO_GLS: 
					if(verbose) printf("Running algorithm GLS+\n");
					gls();
					break;
				case ALGO_ILS: 
					if(verbose) printf("Running algorithm ILS\n");
					ils();			
					break;
				case ALGO_HYBRID:
					mb_ils_hybrid();
					break;
				case ALGO_TABU: 
					tabu();			
					break;
			}
//last sol taken to be the best! assignmentManager->recordNewBestSolution();
			end_run();
		}
	}
	print_end_problem();

	int var;
	for(var=0; var<num_vars; var++){
		(*outBestAssignment)[var] = assignmentManager->globalBestAssignments[0][var];
	}
	(*outLogLikelihood) = assignmentManager->overallBestLogProb;
	tearDown();
}


/********************************************************
 ========================================================
                  TRACKING Functionality
 ========================================================
 ********************************************************/

/********************************************************
         Keeping track of best solution and time.
 ********************************************************/

void end_run(){
	print_end_run();
	assignmentManager->endRun();

	run_time_so_far += elapsed_seconds();
	overall_time_so_far += run_time_so_far;
}

void update_if_new_best_in_run(){
	run_time_so_far += elapsed_seconds();

	//=== Updates time only if new best in run.
	if( assignmentManager->updateIfNewBest(log_prob)){
		print_new_best_in_run();
		assignmentManager->runBestTime = run_time_so_far;
	}
}

/********************************************************
 ========================================================
                  ALGORITHM Functionality
 ========================================================
 ********************************************************/

void anytime_mb(){
	double upperBound = DOUBLE_BIG;
	double lowerBound = -DOUBLE_BIG;
	int ib = 1, var;
	if(verbose) printf("DETAILS\n");
	do{
		ib++;
		double thisUpper = mbeElim->solve(ib, DOUBLE_BIG, mbAssignment);
		double thisLower = assignmentManager->get_log_score(mbAssignment);
		if(verbose) fprintf(outfile,"          MB(%d) -> [%lf, %lf]\n", ib, thisLower, thisUpper);

		if(thisLower > lowerBound){
			for(var=0; var<num_vars; var++){
				variables[var]->value = mbAssignment[var];
			}
		}
		log_prob = thisLower;
		update_if_new_best_in_run();
		lowerBound = MAX(lowerBound, thisLower);
		upperBound = MIN(thisUpper, upperBound);
		assignmentManager->optimalLogMPEValue = MIN(assignmentManager->optimalLogMPEValue, thisUpper);
	
		run_time_so_far += elapsed_seconds();
		if(verbose) fprintf(outfile, "time %lf \t cost %lf \t upper %lf\n", run_time_so_far, lowerBound, upperBound);
	} while(upperBound - lowerBound > EPS && run_time_so_far <= maxTime);	
	if(verbose) {
		if(upperBound - lowerBound <= EPS){
			fprintf(outfile, "out of time: false\n");
			fprintf(outfile, "optimal solution cost: %lf\n", lowerBound);
		} else {
			fprintf(outfile, "out of time: true\n");
			fprintf(outfile, "best solution cost: %lf\n", lowerBound);
		}
		fprintf(outfile, "CPU time (search): %lf\n", run_time_so_far);
		if( preprocessingSizeBound == 0 ){
			fprintf(outfile, "Best MB assignment: ");
			assignmentManager->outputCurrentAssignment(outfile);
		}
		if (!noout) printf("Done. Wrote to %s. Exiting.\n", sls_filename);
	}
}

bool hybrid_continue(double realMaxTime){
	return run_time_so_far < realMaxTime && !assignmentManager->foundOptimalThisRun();
}

void runILS(double singleRunTime, double realMaxTime){
	maxTime = MIN(run_time_so_far + singleRunTime, realMaxTime);
	if(verbose) printf("Running ILS for %lf seconds (as long as last MB took or until cutoff time)\n", maxTime-run_time_so_far);
	algo = ALGO_ILS;
	initComputingChangingCachedStructures(true);
	ils();
	maxTime = realMaxTime;
	algo = ALGO_HYBRID;
}

void runGLS(double singleRunTime, double realMaxTime){
	maxTime = MIN(run_time_so_far + singleRunTime, realMaxTime);
	if(verbose) printf("Running GLS for %lf seconds (as long as last MB took or until cutoff time)\n", maxTime-run_time_so_far);
	int realInit = init_algo;
	init_algo = INIT_MB;
	algo = ALGO_GLS;
	initComputingChangingCachedStructures(true);
	gls();
	maxTime = realMaxTime;
	algo = ALGO_HYBRID;
	init_algo = realInit;
}

void mb_ils_hybrid(){
	//=== GLS default params.
	glsReal = 1;
	glsSmooth = 0.999;
	
	//=== ILS default params.
	cutoff=5;
	init_algo = INIT_MB;
	pertubationType = PERTUBATION_RANDOM_POTS_RANDOM_INDEX;
	vns = false;
	pertubationFixVars = true;
	pertubation_strength = 1;
	pertubation_rel = false;
	accCriterion = ACC_BETTER_RW;
	accNoise = 0.003;
	
	//=== Start of Hybrid.
	double upperBound = DOUBLE_BIG;
	double realMaxTime = maxTime;
	double mbRuntime;
//	int realInit = init_algo;
	int var;
	instantiation inst;
	double weightBound;
	for(weightBound = mbInitWeightForHybrid; weightBound <= maxMBWeight && hybrid_continue(realMaxTime); weightBound*=2){
		if(verbose) printf("Running MB with weight bound %g\n", weightBound);
		mbRuntime = run_time_so_far;
		double thisUpper = mbeElim->solve(BIG, weightBound, mbAssignment);
		upperBound = MIN(thisUpper, upperBound);

		//=== Set the upper bound as optimal MPE such that the algo terminates if it finds it.
		if(upperBound < assignmentManager->optimalLogMPEValue){
			assignmentManager->optimalLogMPEValue = upperBound;
		}

		for(var=0; var<num_vars; var++){
			inst.var = var;
			inst.value = mbAssignment[var];
			flip_var_to(inst, false);
		}
		update_if_new_best_in_run();
		mbRuntime = run_time_so_far - mbRuntime;
		if(verbose) printf("MB ==> [%lf, %lf] (in time %lf)   -----    TOTAL: [%lf, %lf]\n",log_prob, thisUpper, mbRuntime, MAX(assignmentManager->runBestLogProb, log_prob), upperBound);
//		printf("best: %lf, upper: %lf\n",assignmentManager->runBestLogProb, upperBound);
		if( upperBound - assignmentManager->runBestLogProb < EPS ){
			if(verbose) fprintf(outfile, "Proofed optimality of logprob %lf with Mini Bucktets.\n", assignmentManager->runBestLogProb);
		} else {
			runILS(mbRuntime, realMaxTime);
			runGLS(mbRuntime, realMaxTime);
		}
	}
	
	if(assignmentManager->foundOptimalThisRun()){
		if(verbose) fprintf(outfile, "MB/ILS/GLS combo proofed optimality of solution quality: %lf\n", assignmentManager->runBestLogProb);
	} else{
		for(; hybrid_continue(realMaxTime); mbRuntime*=2){
			runILS(mbRuntime, realMaxTime);
			runGLS(mbRuntime, realMaxTime);
		}
	}
}

/*****************************************************/
/* MAIN ALGORITHMS                                   */
/*****************************************************/
bool lsContinue(){
	return run_time_so_far < maxTime 
		  && num_flip < maxSteps 
			&& assignmentManager->runBestLogProb+EPS < assignmentManager->optimalLogMPEValue;
}

bool ilsContinue(){
	return lsContinue() && num_iteration < maxIterations;
}

void ils(){
	basic_local_search();
	new_ils_base_solution();
	int pertubation_strength_index = -1;
	while( ilsContinue() ) {
		pertubation_strength_index += 1;
		if(pertubation_strength_index >= num_vns_pertubation_strength) pertubation_strength_index = 0;

		num_iteration++;
//if( num_iteration % 100 == 0) fprintf(stderr, "while in ils, runtime=%lf, numflip=%d, it=%d\n",run_time_so_far, num_flip, num_iteration);

		if( vns ){	
			pertubation_strength = vns_pertubation_strength[pertubation_strength_index];
		}
		pertubation(pertubation_strength, pertubationType);
		
		basic_local_search();
		acceptance_criterion();
//		printf("logprob after acc = %lf\n", log_prob);

		if(output_runstats)	print_ils_stats();
		if(output_runstats)	update_lm_count();
	}
	if(output_runstats)	update_ils_count();
	if( output_lm ) print_curr_state();
}

void randomRestart(){
	if(verbose) fprintf(outfile, "Random restart at time %lf and iteration %ld\n", run_time_so_far, num_iteration);
	computeInitValues(); // random restart.
	instantiation inst;
	for(int var=0; var<num_vars; var++){
		inst.var = var;
		inst.value = initValues[var];
		flip_var_to(inst, false);
	}
//	printf("Restart:\n");
//	assignmentManager->outputCurrentAssignment(outfile);
}

void tabu(){
	instantiation inst;	
	while(lsContinue()) {
		inst = best_new_inst(true);
		if( inst.var == NOVALUE ){
			printf("error, tabu not returning value\n");
			exit(-1);
			//num_flip++; // necessary such that tabu status gets ok.	
		}
		else {
			flip_var_to(inst, false);
			update_if_new_best_in_run();
		}
	}
}

void greedy_noise(){
	double lastCutoffTime = 0;
	double bestThisRestart = -DOUBLE_BIG;
	double bestTimeThisRestart = 0;
	instantiation inst;	
	while(lsContinue()) {
		//==== Determine which new instantiation of a variable to choose.
		if(random_number(&seed)%100<noise){
			inst.var = random_number(&seed) % num_vars;
			inst.value = sampled_new_val(inst.var);
		}	else{
			inst = best_new_inst(false);
			if(inst.var == NOVALUE){
				//=== Local minimum, any flip of a variable to its current value is greedy.
				//=== (This is BAD as we know that we're trapped in the LM and need to leave it,
				//=== but I'm just implementing the algorithm as it's described in Kask's paper)
				inst.var = 0;
				inst.value = variables[inst.var]->value;
				num_flip++;
			}
		}
		flip_var_to(inst, false);
		update_if_new_best_in_run();
		if( log_prob > bestThisRestart ){
			bestThisRestart = log_prob;
			bestTimeThisRestart = run_time_so_far - lastCutoffTime;
		}
		double runTimeThisRestart = run_time_so_far - lastCutoffTime;
		if( runTimeThisRestart > MAX(cutoff * bestTimeThisRestart, 0.1) ){
			if(verbose) fprintf(outfile, "Found %lf in time %lf of this run, now its time %lf in this run. Restarting\n", bestThisRestart, bestTimeThisRestart, runTimeThisRestart);
			randomRestart();
			lastCutoffTime = run_time_so_far;
			bestTimeThisRestart = 0;
			bestThisRestart = -BIG;
			num_iteration++;
		}
	}
}

void increasePenalties(){
	double maxUtility = -1;
	double utility;
	int pot;

	//=== Determine maximal utility.
	for(pot=0; pot<num_pots; pot++){
		utility = probTables[pot]->currUtility();
		if(utility > maxUtility){
			maxUtility = utility;
		}
	}

	//=== Increment penalty of entries with maximal utility.
	for(pot=0; pot<num_pots; pot++){
		if(fabs(probTables[pot]->currUtility() - maxUtility) < EPS){
//				printf("incrementing penalty of table %d, index %d\n", pot, probTables[pot]->index);
			probTables[pot]->incCurrPenalty(glsPenaltyIncrement, caching, good_vars, &num_good_vars );
		}
	}
}

void gls(){
	instantiation inst;
	if(verbose) printf("starting GLS at time %lf\n", run_time_so_far);
	long lmCounter = 0;
	while(lsContinue()) {
		inst = best_new_inst(false);
//		printf("%d->%d\n",inst.var,inst.value);
		if(inst.var == NOVALUE){
//		printf("LM %d at step %d, ll=%lf, increasing penalties\n", lmCounter+1, num_flip, log_prob);
			//=== Local Minimum.
			lmCounter++;
			int pot;
			if(lmCounter % glsInterval == 0){
//				printf("lmCounter=%ld, scaling penalties.\n",lmCounter);
				for(pot=0; pot<num_pots; pot++){
					probTables[pot]->scalePenalties(glsSmooth);
				}
//				if(verbose) printf("...scaling penalties\n");
				for(int var=0; var<num_vars;var++){
					for(int val=0; val<variables[var]->domSize; val++){
						variables[var]->penaltyScores[val] *= glsSmooth;
						//variables[var]->scores[val] *= glsSmooth;
					}
				}
			}
			increasePenalties();

			num_flip++; // such that it doesn't run indefinitely if only given flip bound.
		} else {
			flip_var_to(inst, false);
		}

		update_if_new_best_in_run();
	}
	if(verbose) fprintf(outfile, "Ran %ld iterations, %ld of which were LMs\n", num_flip, lmCounter);
}

/*****************************************************/
/* INITIALIZATIONs OF THE ASSIGNMENT                 */
/*****************************************************/
void init_potIndices(){
	//=== Initialize indices into potentials.
	for(int pot=0; pot<num_pots; pot++){
		probTables[pot]->index = 0;
		for(int i=0; i<probTables[pot]->numPTVars; i++){
			probTables[pot]->index += probTables[pot]->factorOfVar[i]*variables[probTables[pot]->ptVars[i]]->value;
		}
	}
}

void computeInitValues(){
	switch(init_algo){
		case INIT_RANDOM:
			random_init();
			break;
		case INIT_MB:
			mb_init();
			break;
		case INIT_EXTERNAL:
			int var;
			for(var=0; var<num_vars; var++)
				initValues[var] = externalInitValues[var];
			break;
		default:
			if(verbose) printf("... using given values for initialization\n");
			break;
	}
}

void random_init(){
	int var;
	for(var=0; var<num_vars; var++){
		initValues[var] = random_number(&seed)%variables[var]->domSize;
	}
}

void mb_init(){
	int var;
	for(var=0; var<num_vars; var++) variables[var]->fakeEvidenceForMB = false;
	mbeElim->solve(BIG, mbInitWeight, mbAssignment);

	for(var=0; var<num_vars; var++){
		initValues[var] = mbAssignment[var];
	}
}

/*****************************************************/
/* ILS INGREDIENTS                                   */
/*****************************************************/
void basic_local_search(){
//fprintf(stderr, "bls\n");

	double currentBestSolution = log_prob;
	long lastImprovementOnCurrentBest = num_flip;

	instantiation inst;
	while(lsContinue() && (tl == 0 || (num_flip - lastImprovementOnCurrentBest) < 0.1*numVarValCombos)){//num_vars)) {
		inst = best_new_inst(false);
		if( inst.var == NOVALUE ) break; // this is for the case w/o tabu.
		flip_var_to(inst, false);
		if( output_trajectory )
			fprintf(traj_fl_file,"%ld %ld %lf bls\n",num_flip, num_iteration, log_prob);
		update_if_new_best_in_run();
		if(log_prob > currentBestSolution){
			currentBestSolution = log_prob;
			lastImprovementOnCurrentBest = num_flip;
		}
	}
}

int randomNotYetFakeEvidence(){
	//=== Sample var that's not been flipped yet. 
	//=== Efficient method for small strength. 
	//=== Inefficient for strength close to num_vars.
	int var;
	do{ 
		var = random_number(&seed)%num_vars;
	} while( contains(fakeEvidenceForMB, numFakeEvidenceForMB, var) );
	return var;
}

int randomNotYetPermuted(){
	//=== Sample var that's not been flipped yet. 
	//=== Efficient method for small strength. 
	//=== Inefficient for strength close to num_vars.
	int var;
	do{ 
		var = random_number(&seed)%num_vars;
	} while( contains(vars_permuted, num_vars_permuted, var) );
	return var;
}

int randomPotNotYetPermuted(){
	int pot;
	do{
		pot = random_number(&seed)%num_pots;
	} while (contains(pots_flipped, num_pots_flipped, pot));
	return pot;
}

int sampleProbTableFromPotentialProbIncrease(){
	for(int pot=0; pot<num_pots; pot++)	single_goods[pot] = probTables[pot]->getPotentialProbIncrease();
	for(int j=0; j<num_pots_flipped; j++) single_goods[pots_flipped[j]] = -BIG;	

	//pow(10,diff_logs) comes down to the quotient of the optimal prob and the current prob
	//increasing the base yields more greedy behaviour.
	return sample_from_scores(single_goods, num_pots, psp_base);
}

int sampledPotNotYetPermuted(){
	int pot;
	do{
		pot = sampleProbTableFromPotentialProbIncrease();
	} while (contains(pots_flipped, num_pots_flipped, pot));
	return pot;
}

void permuteVarTo(int var, int val){
	instantiation inst;
	inst.var = var;
	inst.value = val;
	flip_var_to(inst, false);
	insert(vars_permuted, &num_vars_permuted, inst.var);
	if( output_trajectory )
		fprintf(traj_fl_file,"permut: %ld %ld %lf T=%lf\n",num_flip, num_iteration, log_prob, T);
}

void permuteVar(int var){
	permuteVarTo(var, random_new_val(var));
}

void permutePotVars(int pot){
	for(int i=0; i<probTables[pot]->numPTVars; i++){
		permuteVar(probTables[pot]->ptVars[i]);
	}
	insert(pots_flipped, &num_pots_flipped, pot);
}

void permutePotVarsToIndex(int pot, int best_ind){
	//=== Compute which variables match with index best_ind.
	int var, value;
	for(int j=probTables[pot]->numPTVars-1; j>=0; j--){
		var = probTables[pot]->ptVars[j];
		if(contains(vars_permuted, num_vars_permuted, var)) continue;
		value = (best_ind/probTables[pot]->factorOfVar[j])%variables[var]->domSize;
		permuteVarTo(var, value);
	}
	insert(pots_flipped, &num_pots_flipped, pot);
}

void pertubation(int strength, int pertub){
//	printf("iteration %d, pert. strength = %d\n", num_iteration, strength);
	int i; //,pot=-1, best_ind=-1; // Lars: unused
	num_vars_permuted = 0;
	num_pots_flipped=0;
	numFakeEvidenceForMB = 0;
	if( num_vars < strength ) {
		if(verbose) fprintf(outfile, "Permutation strength cannot be greater than #(vars)\n");
		exit(-1);
	}

	while( num_vars_permuted < strength ){
		switch(pertub){

			case PERTUBATION_RANDOM_VARS_RANDOM_FLIP:
				permuteVar(randomNotYetPermuted());
				break;

			case PERTUBATION_RANDOM_VARS_SAMPLED_FLIP:
			{
				int var = randomNotYetPermuted();
				permuteVarTo(var, sampled_new_val(var));
				break;
			}

			case PERTUBATION_RANDOM_POTS_RANDOM_INDEX:
				permutePotVars(randomPotNotYetPermuted());
				break;
		
			case PERTUBATION_RANDOM_POTS_SAMPLED_INDEX:
			{
				int pot = randomPotNotYetPermuted();
				permutePotVarsToIndex(pot, probTables[pot]->sampleIndex(psa_base));
				break;
			}

			case PERTUBATION_SAMPLED_POTS_RANDOM_INDEX:
				permutePotVars(sampledPotNotYetPermuted());
				break;

			case PERTUBATION_SAMPLED_POTS_SAMPLED_INDEX:
			{
				int pot = sampledPotNotYetPermuted();
				permutePotVarsToIndex(pot, probTables[pot]->sampleIndex(psa_base));
				break;
			}

			case 6:
			{
				algo = ALGO_GLS;
//				increasePenalties();
				instantiation inst = best_new_inst(false);
				while(inst.var != NOVALUE){
//					printf("flipping %d from %d to %d\n", inst.var, variables[inst.var]->value, inst.value);
					permuteVarTo(inst.var, inst.value); // can be made faster.
					//flip_var_to(inst, false);
					update_if_new_best_in_run();
					inst = best_new_inst(false);
				}
//				printf("pertubation done\n");
//				num_vars_permuted++;
				increasePenalties();
				algo = ALGO_ILS;
				break;
			}
		}
	}
//	printf("whole pertubtation done\n");

	if( mbPertubation > 0 ){
		int var;

//=== Already permuted variables remain permuted and treated as evidence.
		numFakeEvidenceForMB = 0;
		for(i=0; i<num_vars_permuted; i++){
			var = vars_permuted[i];
			variables[var]->fakeEvidenceForMB = true;
			fakeEvidenceForMB[numFakeEvidenceForMB++] = var;
		}

		int rest = num_vars - num_vars_permuted;
		int numMBPert = (int) (ceil(mbPertubation*0.01*rest));

//=== Some more variables (rest-numMBPert) are fixed as evidence.
		for(i=0; i<rest-numMBPert; i++){
			var = randomNotYetFakeEvidence();
			variables[var]->fakeEvidenceForMB = true;
			fakeEvidenceForMB[numFakeEvidenceForMB++] = var;
		}

//					printf("number of fake ev. %d\n", numFakeEvidenceForMB);

		int inducedWidthWithEv;
		double inducedWeightWithEv;

		mbeElim->createOrder(numFakeEvidenceForMB, fakeEvidenceForMB, &inducedWidthWithEv, &inducedWeightWithEv);
//		printf("calling MB with %lf to reinstantiate %d variables\n",maxMBWeight, numMBPert);

//=== The remaining variables (numMBPert many) are determined with Mini Buckets.
		mbeElim->solve(BIG, maxMBWeight, mbAssignment);
//					printf("done\n");

		for(i=0; i<numFakeEvidenceForMB; i++){
			variables[fakeEvidenceForMB[i]]->fakeEvidenceForMB = false;
		}
		numFakeEvidenceForMB = 0;

		instantiation inst;
		for(var=0; var<num_vars; var++){
			inst.var = var;
			inst.value = mbAssignment[var];
			if(variables[inst.var]->fakeEvidenceForMB) assert(inst.value == variables[inst.var]->value);
			flip_var_to(inst, false);
			if( output_trajectory )
				fprintf(traj_fl_file,"%ld %ld %lf p_b MB, T=%lf\n",num_flip, num_iteration, log_prob, T);
		}
//		printf("logprob: %lf\n", log_prob);
		//		printf("... to sol. qual %lf\n",log_prob);
	}

	if( pertubationFixVars ){
		for(i=0; i<num_vars_permuted; i++){
			int var = vars_permuted[i];
			variables[var]->fixed = true;
			//=== Temporarily remove var from good vars if it's good.
			if(isgoodvar[var] && caching==CACHING_GOOD_PQ){
//				printf("removing %d since it's a good var\n", var);
				fh_remove(heapOfGoodVars, var+1);
				isgoodvar[var]=false;
			}
		}
		basic_local_search();
		for(i=0; i<num_vars_permuted; i++){
			int var = vars_permuted[i];
			variables[var]->fixed = false;
			if(caching==CACHING_GOOD_PQ){
				//=== Put var (back) into good vars if it's good.
				int val;
				double bestScoreVal = -DOUBLE_BIG;
				for(val=0; val<variables[var]->domSize; val++){
					if(variables[var]->score(val) > bestScoreVal){
						bestScoreVal = variables[var]->score(val);
					}
				}
				if(bestScoreVal > EPS){
					isgoodvar[var]=true;
					fh_insert(heapOfGoodVars, var+1, -bestScoreVal);
				}
			}
		}
	}
	update_if_new_best_in_run();
}

void new_ils_base_solution(){
	last_log_prob = log_prob;
	last_steps = num_flip;
	bestQualNotAccepted = -DOUBLE_BIG;
	num_flipped_since_last_ils_solution = 0;
	
	//=== Keep track of the variables which have changed in the ILS solution.
	int var;
	for(int i=0; i<num_flipped_since_last_ils_solution; i++){
		var = flipped_since_last_ils_solution[i];
		variables[var]->lastILSValue = variables[var]->value;
	}

	if( output_trajectory ){
		fprintf(traj_it_file, "New ILS base solution: %lf at iteration %ld. Last improving: %ld\n",log_prob, num_iteration, lastImprovingIteration);
		fprintf(traj_fl_file, "New ILS base solution: %lf at iteration %ld. Last improving: %ld\n",log_prob, num_iteration, lastImprovingIteration);
	}
}

void improved_ils(){
  if(log_prob> best_logprob_this_try){
    best_logprob_this_try = log_prob;
    lastImprovingIteration = num_iteration;
  }

	if( output_trajectory ){
		fprintf(traj_fl_file, "accepted improving move at iteration %ld to %lf, T=%lf\n",num_iteration, log_prob, T);
		fprintf(traj_it_file, "accepted improving move at iteration %ld to %lf, T=%lf\n",num_iteration, log_prob, T);
	}
	new_ils_base_solution();
//	T/=tdiv;
}

void flip_back(){
	instantiation inst;

	//=== Flip back to last ils base-solution. 
/*	for(int i=0; i<num_vars; i++){
		inst.var = i;
		inst.value = variables[i]->lastILSValue;
//			if( variables[inst.var]->value != inst.value) 
			flip_var_to(inst, false);
	}*/

	for(int i=0; i<num_flipped_since_last_ils_solution; i++){
		inst.var = flipped_since_last_ils_solution[i];
		inst.value = value_of_flipped_in_last_ils_solution[i];
		flip_var_to(inst, true);
	}
	num_flipped_since_last_ils_solution = 0;
}

void acceptance_criterion(){
/*	int num_iterations_until_restart = 10000;
	double diff_factor = pow(1e7, 1.0/num_iterations_until_restart);

	if(num_iteration % num_iterations_until_restart == 0){
		T = ran01(&seed)*100;
	} else{
		T /= diff_factor; // diff_factor^{num_iterations_until_restart} = 1e7
//		printf("T=%lf\n",T);
	}
*/
	//	printf("last=%lf, now=%lf\n", last_log_prob, log_prob);
	//=== Take improving and sideways moves.
	double diff = (log_prob - last_log_prob); // positive if new one better.
	if( diff > EPS ){ // improving step
		improved_ils();
		return;
	}

	//	bool restartSatisfied = (num_iteration - lastImprovingIteration >= numVarValCombos * restartNumFactor);

	int num_iteration_this_try = num_iteration - start_iteration_of_current_try;
	int last_improving_this_try = lastImprovingIteration - start_iteration_of_current_try;

	bool restart = ( num_iteration_this_try > MAX(numVarValCombos,cutoff * last_improving_this_try) );

	if(restart){
		//=== If too long without improvement over the 
		//=== last iteration, do restart.
	  start_iteration_of_current_try = num_iteration;
	  best_logprob_this_try = -DOUBLE_BIG;
		randomRestart();
		basic_local_search();
		new_ils_base_solution();
		return;
	}

	if( diff > -EPS ){ // sideways step.
		new_ils_base_solution();
		return;
	}

//	printf("iteration: %d, last impr: %d\n", num_iteration, lastImprovingIteration);
	instantiation inst;
	switch(accCriterion){
		case ACC_RW: 
			new_ils_base_solution();
			return;

		case ACC_BETTER:
			flip_back();
			return;

			//		case ACC_RESTART:
			// restart is not satisfied (otherwise, it doesn't get here)
			//			flip_back();
			//			return;

		case ACC_BETTER_RW:
			if( ran01(&seed) < accNoise){
				new_ils_base_solution();
			}	else{
				flip_back();
			}
			return;

		case ACC_BEST_WORSENING:
			int var;
			//=== Update the least worsening step. 
			if(log_prob > bestQualNotAccepted){
				bestQualNotAccepted = log_prob;
				for(var=0; var<num_vars; var++) bestNotAccepted[var] = variables[var]->value;
			}

			//=== If too long without improvement over the last iteration, 
			//=== take the least worsening step found so far.
			if( num_iteration - lastImprovingIteration >= worseningInterval * numVarValCombos ){
				lastImprovingIteration = num_iteration;
				for(var=0; var<num_vars; var++){
					inst.var = var;
					inst.value = bestNotAccepted[var];
					//if( variables[inst.var]->value != inst.value) 
					flip_var_to(inst, false);
				}
				new_ils_base_solution();
			} else {
				flip_back();
			}
			return;
		case ACC_RW_AFTER_N:
			if( num_iteration - lastImprovingIteration >= worseningInterval * numVarValCombos ){
				new_ils_base_solution();
			}	else{
				flip_back();
			}
			return;
		case ACC_RW_AFTER_N2:
			if( num_iteration - lastImprovingIteration >= worseningInterval * numVarValCombos ){
				lastImprovingIteration = num_iteration;
				new_ils_base_solution();
			}	else{
				flip_back();
			}
			return;
		case 7:
			diff = diff * 100 / last_log_prob; // relative difference in percent (positive). 

			double prob = pow(tbase,-diff/T); // diff negative. 
			//printf("prob= %lf\n",prob);
			if( ran01(&seed) < prob ) {
				if(output_trajectory){
					fprintf(traj_fl_file, "accepting worsening move at iteration %ld to %lf, T=%lf\n",num_iteration, log_prob, T);
					fprintf(traj_it_file, "accepting worsening move at iteration %ld to %lf, T=%lf\n",num_iteration, log_prob, T);
				}
				new_ils_base_solution();
			}	else{
//				T = MAX(tmin, T*tmult);
				if(output_trajectory){
					fprintf(traj_fl_file, "not accepting worsening move at iteration %ld to %lf, T=%lf\n",num_iteration, log_prob, T);
					fprintf(traj_it_file, "not accepting worsening move at iteration %ld to %lf, T=%lf\n",num_iteration, log_prob, T);
				}
				flip_back();
			}
			return;

	}

/*
	// The sign of diff is the other way around here than in the ILS paper b/c we're MAXimizing not minimizing.
	double threshold = pow(acceptBase, diff/ T);
	bool accept = (diff > -EPS || ran01(&seed) < threshold); 

	if( output_trajectory ){
		if(accept){
			fprintf(traj_it_file, "%d %lf, T=%lf --- accepted worsening move \n",num_iteration, log_prob, T);
			fprintf(traj_fl_file, "%d %lf, T=%lf --- accepted worsening move \n",num_iteration, log_prob, T);
		}	else {
			fprintf(traj_it_file, "%d %lf, T=%lf --- didn't accept worsening move to %lf \n",num_iteration, last_log_prob, T, log_prob);
			fprintf(traj_fl_file, "%d %lf, T=%lf --- didn't accept worsening move to %lf \n",num_iteration, last_log_prob, T, log_prob);
		}
	}
	//	printf("didn't improve for %d iterations\n", num_iteration - lastImprovingIteration);
*/
}


/********************************************************
 ========================================================
                   CACHING Functionality
 ========================================================
 ********************************************************/

int random_new_val(int var){
	if( variables[var]->domSize == 1 ) return 0;
	int val = variables[var]->value;
	while(val == variables[var]->value){
		val = random_number(&seed)%variables[var]->domSize;
	}
	return val;
}

int sampled_new_val(int var){
	int result = -1;
	double* score = new double[variables[var]->domSize];
	switch(caching){
		case CACHING_INDICES:
		{
			int i, pot;

			for(int value=0; value<variables[var]->domSize; value++){
				score[value] = 0.0;
				for(i=0; i<variables[var]->numOcc; i++){
					pot = variables[var]->occ[i];
					score[value] += probTables[pot]->diffLogProbWithVarFlipped(variables[var]->numInOcc[i],value) + probTables[pot]->diffPenaltyWithVarFlipped(variables[var]->numInOcc[i],value);
				}
			}
		}
		break;
		case CACHING_SCORE: // for both
		case CACHING_GOOD_VARS:
		case CACHING_GOOD_PQ: //all three ;)
		{
			for(int value=0; value<variables[var]->domSize; value++){
				score[value] = variables[var]->score(value);	
			}
		}
		break;
		case CACHING_NONE:
			int initialValue = variables[var]->value;
			for(int value=0; value<variables[var]->domSize; value++){
				variables[var]->value = value;
				score[value] = assignmentManager->get_log_score();
			}
			variables[var]->value = initialValue;
			break;
	}
	score[variables[var]->value] = -BIG; // such that it is not chosen.
	result = sample_from_scores(score, variables[var]->domSize, psa_base);
	delete[] score;
	return result;
}

/********************************************************
   Find best variable instantiation for a one-flip move.
 ********************************************************/

void outputTabu(){
	for(int var=0; var<num_vars; var++){
		if(verbose){
			printf("VAR %d   ",var);
			for(int val=0; val<variables[var]->domSize; val++){
				printf("%d ", variables[var]->tabuValues[val]);
			}
			printf("\n");
		}
	}
	exit(-1);
}

void outputFlipped(){
	for(int var=0; var<num_vars; var++){
		if(verbose) {
			printf("VAR %d   ",var);
			printf("%d ", variables[var]->numTimesFlipped);
			printf("\n");
		}
	}
	exit(-1);
}

struct instantiation best_new_inst(bool best_even_if_not_improving){
	int value, j, var;
	double best_score = -DOUBLE_BIG;
	struct instantiation result;
	result.var = NOVALUE;
	result.value = NOVALUE;
	int num_best=0;
	double aspirationImprovement = assignmentManager->runBestLogProb - log_prob + EPS;
	if(algo == ALGO_GLS && (!glsReal || !glsAspiration)) aspirationImprovement = DOUBLE_BIG; // don't look at scores

	switch(caching){
		case CACHING_GOOD_PQ:
		{
			if((num_flip+1) % 1000000 == 0){
			  setPotentialIndicesScoresAndGoodvars(true); // to avoid numerical problems due to errors adding up
			  log_prob = assignmentManager->get_log_score();
			}
			if(heapOfGoodVars->n == 0){
				return result; // heap empty => no improving step. should't have to do
			}
			var = fh_inspect(heapOfGoodVars)-1;
			double bestScoreVal = -DOUBLE_BIG;
			int bestVal = -1;
			for(value=0; value<variables[var]->domSize; value++){
				if(variables[var]->score(value) > bestScoreVal){
					bestScoreVal = variables[var]->score(value);
					bestVal = value;
				}
			}
			assert(bestVal != -1);
			best_vars[num_best] = var;
			best_vals[num_best++] = bestVal;
			best_score = bestScoreVal;
		}
		break;
		
		case CACHING_GOOD_VARS:
			if(num_flip % 100000 == 0){
			  setPotentialIndicesScoresAndGoodvars(true);
			  log_prob = assignmentManager->get_log_score();
			}

		{
			if(num_good_vars==0) return result;
			num_best=0;

//			output(num_good_vars, good_vars);
//			printf("\n");


/*
			//=== Determine best score.
			for(j=0; j<num_good_vars; j++){
				var = good_vars[j];
				for(value=0; value<variables[var]->domSize; value++){
					if(FORBIDDEN(var, value)){
						if(variables[var]->score(value) <= aspirationImprovement) continue;
						fprintf(outfile, "Aspiration step to logprob %lf possible\n", log_prob + variables[var]->logProbScores[value]);
					}

					best_score = MAX(best_score, variables[var]->score(value));
				}
			}
			
			//=== Check for variables achieving best score.
			for(var=0; var<num_vars; var++){
				if(!isgoodvar[var]) continue;
				for(value=0; value<variables[var]->domSize; value++){
					if(FORBIDDEN(var, value)){
						if(variables[var]->score(value) <= aspirationImprovement) continue;
					}
					if(variables[var]->score(value) > best_score - EPS){ 
						best_vars[num_best] = var;
						best_vals[num_best++] = value;
					}
				}
			}
*/

			//=== Since num_good_vars > 0, there is at least one good variable, but it might be forbidden.
			bool aspirationStepPossible = false;
			int realAlgo = algo;
			for(j=0; j<num_good_vars; j++){
				var = good_vars[j];
				for(value=0; value<variables[var]->domSize; value++){
					if(FORBIDDEN(var, value)){
//						if(variables[var]->value != value) exit(-1);
						if(variables[var]->logProbScores[value] <= aspirationImprovement) continue;
						if(verbose) fprintf(outfile, "Aspiration step to logprob %lf possible by flipping %d to %d\n", log_prob + variables[var]->logProbScores[value], var, value);
					}
					if(algo == ALGO_GLS && variables[var]->logProbScores[value] > aspirationImprovement && !aspirationStepPossible){
						  aspirationStepPossible = true;
						  num_best = 0; // We want to do one greedy step and forget all previous non-improving possibilities.
						  best_score = -DOUBLE_BIG;//such that we definitely take this one.
						  algo = ALGO_ILS; // such that score(value) yields greedy score.
						  if(verbose) printf("Will take aspiration step to logprob %lf (by flipping %d to %d) or higher.\n", log_prob + variables[var]->logProbScores[value], var, value);
					}
					double EPS2 = EPS; //1e-4 actually yields somewhat better results here, 
					if(variables[var]->score(value) > best_score - EPS2){

///*
						if(variables[var]->score(value) > best_score + EPS2){
							num_best = 0;
							best_score = variables[var]->score(value);
						}
//*/
//						best_score = MAX(best_score, variables[var]->score(value));
						best_vars[num_best] = var;
						best_vals[num_best++] = value;
					}
				}
			}
			algo = realAlgo;

			
/*
			//=== Since num_good_vars > 0, there is at least one good variable, but it might be forbidden.
			for(j=0; j<num_good_vars; j++){
				var = good_vars[j];
				for(value=0; value<variables[var]->domSize; value++){
					if(FORBIDDEN(var, value)){
						if(variables[var]->logProbScores[value] <= aspirationImprovement) continue;
						fprintf(outfile, "Aspiration step to score %lf possible\n", log_prob + variables[var]->logProbScores[value]);
					}
					if(variables[var]->score(value) > best_score - EPS){
						if(variables[var]->score(value) > best_score + EPS){
							best_score = MAX(best_score, variables[var]->score(value));
							num_best = 0;
						}
						best_vars[num_best] = var;
						best_vals[num_best++] = value;
					}
				}
			}
*/
		}
		break;

		case CACHING_SCORE:
		{
//			if(num_flip >= 5) outputTabu();
//			if(num_flip == 10000) outputFlipped();
//			printf("num_flip: %d\n", num_flip);
			for(var=0; var<num_vars; var++){
				for(value=0; value<variables[var]->domSize; value++){
					if(FORBIDDEN(var, value)){
						if(variables[var]->logProbScores[value] <= aspirationImprovement) continue;
						if(verbose) fprintf(outfile, "Aspiration step to score %lf possible by flipping %d to %d\n", log_prob + variables[var]->logProbScores[value], var, value);
					}

					if( variables[var]->score(value) > best_score - EPS){ 
						if( variables[var]->score(value) > best_score + EPS){ 
							best_score = variables[var]->score(value);
							num_best=0;
						}
						best_vars[num_best] = var;
						best_vals[num_best++] = value;
					}
				}
			}
		}
		break;
	
		case CACHING_INDICES:
		{
			int i, pot;
			double score;

			for(var=0; var<num_vars; var++){
				for(value=0; value<variables[var]->domSize; value++){

					//=== Compute score of flipping var to value.
					score = 0;
					for(i=0; i<variables[var]->numOcc; i++){
						pot = variables[var]->occ[i];
						if(algo==ALGO_GLS){
							score += probTables[pot]->diffPenaltyWithVarFlipped(variables[var]->numInOcc[i], value);
						}
						if(algo != ALGO_GLS || (algo==ALGO_GLS && glsReal)){
							score += probTables[pot]->diffLogProbWithVarFlipped(variables[var]->numInOcc[i], value);
						}
					}

					if(FORBIDDEN(var, value)){
						if(score <= aspirationImprovement) continue;
						if(verbose) fprintf(outfile, "Aspiration step to score %lf possible\n", log_prob + score);
					}

					if( score > best_score - EPS){ 
						if( score > best_score + EPS){ 
							best_score = score;
							num_best=0;
						}
						best_vars[num_best] = var;
						best_vals[num_best++] = value;
					}
				}
			}
		}
		break;

		case CACHING_NONE:
		{
			if(glsReal){
				printf("Sorry, I didn't bother to implement GLS+ for this stupid caching.");
				exit(-1);
			}
			int initial_value;
			double score;

			for(var=0; var<num_vars; var++){
				initial_value = variables[var]->value;
				for(value=0; value<variables[var]->domSize; value++){
					variables[var]->value = value;
					score = assignmentManager->get_log_score();
					if(FORBIDDEN(var, value)){
						if(score <= assignmentManager->runBestLogProb + EPS)	continue;
						if(verbose) fprintf(outfile, "Aspiration step to score %lf possible\n", score);
					}
					if( score > best_score - EPS ){ 
						if( score > best_score + EPS){ 
							best_score = score;
							num_best=0;
						}
						best_vars[num_best] = var;
						best_vals[num_best++] = value;
					}
				}
				variables[var]->value = initial_value;
			}
			best_score -= assignmentManager->get_log_score(); // Such that it is > EPS in case of an improvement.
		}
		break;
	}

	if((best_even_if_not_improving || best_score > EPS) && num_best > 0){
		if(num_best == 0) {
			printf("Error: no best flip available.\n");
			exit(-1);
		}
		int randnum = random_number(&seed)%num_best;
		result.var = best_vars[randnum];
		result.value = best_vals[randnum];
	} else{
//		printf("Best: %lf\n", best_score);
	}
// fprintf(stderr, "Score of flipping %d from %d to %d: %e, score[%d][%d]=%e\n", result.var, variables[result.var]->value, result.value, best_score, result.var, result.value, variables[result.var]->score(result.value));
//	if( result.var != NOVALUE ) 	fprintf(stderr, "Previous value: %d\n", variables[result.var]->value);

	return result;
}


/********************************************************
           ACTUALLY FLIP VARIABLE
 ********************************************************/
bool flip_var_to(struct instantiation inst, bool flipBack){
	if(algo==ALGO_ILS && num_iteration > 0) {
		if(!flipBack && !contains(flipped_since_last_ils_solution, num_flipped_since_last_ils_solution, inst.var)){
			flipped_since_last_ils_solution[num_flipped_since_last_ils_solution] = inst.var;
			value_of_flipped_in_last_ils_solution[num_flipped_since_last_ils_solution++] = variables[inst.var]->value;
		}
	}
//	printf("flipping %d to %d\n", inst.var, inst.value);
	bool reallyHadToFlip = variables[inst.var]->flipTo(inst.value, num_flip, good_vars, &num_good_vars, caching, &log_prob, algo==ALGO_GLS);
	if( reallyHadToFlip && !flipBack) {
		num_flip++;
		return true;
	}
	return false;
}

/********************************************************
 ========================================================
                     INIT Functionality
 ========================================================
 ********************************************************/



/********************************************************
           INITIALIZE_FROM_FROM_VARS_AND_PROBABILITIES
 From the variables and probTabes, initialize data structures.
 ********************************************************/
void initializeAllDataStructuresFromVarsAndPots(){
	int j,var,pot;

	for(pot=0; pot<num_pots; pot++){
		probTables[pot]->initFactorsOfVars(); //redo with new vars.
	}

	assignmentManager->setProbTables(probTables);
	assignmentManager->setNumberOfVariables(num_vars,output_lm?true:false);

	//==== Determine #variable-value combinations.
	numVarValCombos = 0;
	for(var=0; var<num_vars; var++){
		numVarValCombos += variables[var]->domSize;
	}

	//==== Determine in which potentials a variable is contained.
	for(var=0; var<num_vars; var++){
		variables[var]->numOcc = 0;
	}

	//==== Iterate over potentials, determine num of var occurences for memory allocation.
	for(pot=0; pot<num_pots; pot++){
		for(j=0; j<probTables[pot]->numPTVars; j++){
			var = probTables[pot]->ptVars[j];
			variables[var]->numOcc++;
		}
	}

	//==== Allocate memory.  (TODO: this is dirty, make clean.)
	for(var=0; var<num_vars; var++){
		variables[var]->allocateOcc();
		variables[var]->numOcc = 0;
	}

	//==== Now collect variable occurences. Not possible before b/c of memory allocation.
	for(pot=0; pot<num_pots; pot++){
		for(j=0; j<probTables[pot]->numPTVars; j++){
			var = probTables[pot]->ptVars[j];
			variables[var]->occ[variables[var]->numOcc] = pot; // pot's jth var is in pot.
			variables[var]->numInOcc[variables[var]->numOcc++] = j; // It's the jth one there.
		}
	}

//==== Determine Markov blanket for all variables.
	int k,var2;
	//=== Reset width and weight before (o/w, from preprocessing, weight is DOUBLE_BIG)
	for (var=0; var<num_vars; var++){
		variables[var]->numVarsInMB = 0;
		variables[var]->weightOfMB = 1;
	}
	for (var=0; var<num_vars; var++){
		//=== Go through the occurences.
		for (j=0; j<variables[var]->numOcc; j++){
			pot = variables[var]->occ[j];
			//=== Check all neighbours in this potential.
			for(k=0; k<probTables[pot]->numPTVars; k++){
				var2 = probTables[pot]->ptVars[k];
				variables[var]->addVarToMB(var2);
			}
		}
	}
}


/********************************************************
  Removes the observed variables, so we don't waste time 
  flipping them away from their observed value.
********************************************************/
void removeObservedVariables(){
	int i, pot, var;

//=== Reduce probability tables to non-observed variables.
	for(pot=0; pot<num_pots; pot++){
		ProbabilityTable* tmp = probTables[pot]->instantiated(false);
		delete probTables[pot];
		probTables[pot] = tmp;
	}

//=== Compute new indices of variables (when observed ones are gone).
	int* nonObservedVarIndeces = new int[num_vars];
	int numNonObserved=0;

	for(var=0; var<num_vars; var++){
		if(variables[var]->observed){
			delete variables[var];
		} else {
			nonObservedVarIndeces[numNonObserved++] = var;
		}
	}

//=== Do the change in the variable indices.
	int* old2new = new int[num_vars];
	for(i=0; i<numNonObserved; i++){
		variables[i] = variables[nonObservedVarIndeces[i]];
		old2new[nonObservedVarIndeces[i]] = i;
	}
	num_vars = numNonObserved;

//=== Change the variables in the probTables and recompute their factors.
	for(pot=0; pot<num_pots; pot++){
		for(int j=0; j<probTables[pot]->numPTVars; j++){
			probTables[pot]->ptVars[j] = old2new[probTables[pot]->ptVars[j]];
		}
	}
	
	delete[] nonObservedVarIndeces;
	delete[] old2new;
}

/********************************************************
  Preprocessig step dealing with structured parts of the net. 
********************************************************/
void preprocessStructuredParts(){
	int var,j,pot;

	start_timer();
	//=== Prepare Mini-Buckets elimination.
	mbeElim->initOnce(); // MB
	if(verbose) printf("... creating Mini-Buckets elimination order.\n");
	mbeElim->createOrder(0, fakeEvidenceForMB, &inducedWidth, &inducedWeight); // empty evidence array.
	printStats();
	if( justStats ) exit(0);

	if(preprocessingSizeBound > 0){
		int numVars, numPots;
		Variable** vars;
		ProbabilityTable** pots;
		int* leftVarIndices;
		mbeElim->preprocess(preprocessingSizeBound, &numVars, &leftVarIndices, &vars, &numPots, &pots);
		if (numVars == 0){
			assert(pots[0]->numEntries==1);
			assert(pots[0]->numPTVars==0);
			if(verbose) printf("Optimal MPE found in preprocessing: %lf\n", pots[0]->logCPT[0]);
			preprocessingTime = elapsed_seconds();
			if(verbose) printf("Used bound: %lf, required time: %lf\n\n", preprocessingSizeBound, preprocessingTime);
			assignmentManager->optimalLogMPEValue = pots[0]->logCPT[0];
			//		exit(1);
		}

	//=== Prepare mapping of old variables in potentials to new reduced subset.
		int* oldToNewVars = new int[num_vars];
		for(var=0; var<num_vars;var++){
			oldToNewVars[var] = -1;
		}
		for(var=0; var<numVars;var++){
			oldToNewVars[leftVarIndices[var]] = var; //leftVarIndices is mapping newToOldVars
	//		printf("%d -> %d, weight=%lf\n", leftVarIndices[i], i, vars[i]->weightOfMB);
		}

		delete[] variables;
		delete[] probTables;

		num_pots = numPots;
		probTables = pots;
		if(numVars==0) assert(probTables[0]->numPTVars==0);

		num_vars = numVars;
		variables = vars;

	//=== Adapt the indices in the Markov Blanket to the new reduced subset of variables.
		for(var=0; var<num_vars;var++){
			for(j=0; j<variables[var]->numVarsInMB; j++){
				variables[var]->mb[j] = oldToNewVars[variables[var]->mb[j]];
			}
		}

	//=== Adapt the indices in the potentials to the new reduced subset of variables.
		for(pot=0; pot<num_pots; pot++){
			for(j=0; j<probTables[pot]->numPTVars; j++){
				probTables[pot]->ptVars[j] = oldToNewVars[probTables[pot]->ptVars[j]];
			}
		}
		delete[] oldToNewVars;

	//=== Let the variables know their new id.
		for(var=0; var<num_vars;var++){
			variables[var]->setId(var);
		}

		initializeAllDataStructuresFromVarsAndPots();
	}
	preprocessingTime += elapsed_seconds();
	if(verbose) printf("Preprocessing time for bound %lf: %lf\n", preprocessingSizeBound, preprocessingTime);

	delete mbeElim;
	mbeElim = new MiniBucketElimination();

	mbeElim->initOnce(); // MB
	if(verbose) printf("... creating Mini-Buckets elimination order for reduced network.\n");
	mbeElim->createOrder(0, fakeEvidenceForMB, &inducedWidth, &inducedWeight); // empty evidence array.
}

/*************************************************************
   Allocate memory for 1-dim arrays indexed by vars or pots.
*************************************************************/
void allocateMemoryForDataStructures(bool deleteFirst){

  if (deleteFirst) tearDown();

	isgoodvar              = new bool[num_vars];
	good_vars              = new int[num_vars];
	vars_permuted          = new int[num_vars];
	fakeEvidenceForMB      = new int[num_vars];
	mbAssignment           = new int[num_vars];
	initValues             = new int[num_vars];

	bestNotAccepted        = new int[num_vars];
	last_ils_value         = new int[num_vars];

	heapOfGoodVars = fh_alloc(num_vars+1);

	flipped_since_last_ils_solution = new int[num_vars];
	value_of_flipped_in_last_ils_solution = new int[num_vars];
 
	single_goods           = new double[num_pots];
	sample_probs           = new double[num_pots];

	// For the following, num_vars is just an upper bound.
	pots_flipped    = new int[num_pots];
}

/*************************************************************
   Free memory again.
*************************************************************/
void tearDown(){
  delete[] isgoodvar;
	delete[] good_vars;
	delete[] vars_permuted;
	delete[] fakeEvidenceForMB;
	delete[] mbAssignment;
	delete[] initValues;

	delete[] bestNotAccepted;
	delete[] last_ils_value;

	fh_free(heapOfGoodVars);

	delete[] flipped_since_last_ils_solution;
	delete[] value_of_flipped_in_last_ils_solution;
 
	delete[] single_goods;
	delete[] sample_probs;

	// For the following, num_pots is just an upper bound.
	delete[] pots_flipped;
}

/********************************************************
           INIT DATASTRUCTURES
 Initializes the network using num_vars, num_pots, vars, and probTables.
 Allocates appropriate memory for all the datastructures
 and all of their initialization that's independent of
 the initial variable assignment.
 ********************************************************/
void initFromVarsAndProbTables(){
	int var, pot;

	//=== Initialize factors of the variables in the potentials. 
	//(this has to be done before the reduction to non-observed vars)
	for(pot=0; pot<num_pots; pot++) probTables[pot]->initFactorsOfVars();

	//=== In the current implementation, I also need to do
	// this initialization twice, before the reduction of observed variables,
	// after the reduction, and after the preprocessing.
	allocateMemoryForDataStructures(false);
	initializeAllDataStructuresFromVarsAndPots();

	if(verbose) printf("... removeObservedVariables\n");
	removeObservedVariables();
	for(var=0; var<num_vars; var++){
		variables[var]->setId(var);
		assert(variables[var]->observed == false);
	}

	if(verbose) printf("... allocate memory for data structures\n");
	allocateMemoryForDataStructures(true);

	if(verbose) printf("... initializeAllDataStructuresFromVarsAndPots\n");
	initializeAllDataStructuresFromVarsAndPots();
	
	if(verbose) printf("... preprocessStructuredParts\n");
	preprocessStructuredParts();

	if(verbose) printf("... initializeAllDataStructuresFromVarsAndPots\n");
	initializeAllDataStructuresFromVarsAndPots();
	
/********************************************************
  Initialize data structures that depend on number of vars.
********************************************************/
	best_vars = new int[numVarValCombos];
	best_vals = new int[numVarValCombos];

	setInputDependentParameters();
	init_time = elapsed_seconds();
}

void printStats(){
	if(verbose) {
		fprintf(outfile, "========================================\n        BASIC INSTANCE STATS\n========================================\n");
		if (network_filename) fprintf(outfile, "Instance: %s\n", network_filename);
		else fprintf(outfile, "Dealing with manually defined instance.\n");
		fprintf(outfile, "Number of free variables: %d\n", num_vars);
		fprintf(outfile, "Number of factors: %d\n", num_pots);
		fprintf(outfile, "Number of variable-value pairs: %d\n", numVarValCombos);
		fprintf(outfile, "Avg. domain size: %.2lf\n", numVarValCombos / (num_vars+0.0));
		fprintf(outfile, "Induced width: %d\n", inducedWidth);
		fprintf(outfile, "Induced weight: %e\n", inducedWeight);
		fprintf(outfile, "========================================\n");
	}
}

/********************************************************
           INIT RUN
 Does everything that is necessary 
 at the beginning of a new run.
 ********************************************************/

void setPotentialIndicesScoresAndGoodvars(bool keepPenalties){
//	printf("in setPotentialIndicesScoresAndGoodvars\n");
	int var;
	init_potIndices();
	log_prob = assignmentManager->get_log_score();

	//=== Initialize good_vars.
	num_good_vars = 0;
	fh_free(heapOfGoodVars);
	heapOfGoodVars = fh_alloc(num_vars+1);

	for(var=0; var<num_vars; var++){
		bool good = variables[var]->initRun(algo==ALGO_GLS);
		int val;
		double bestScoreVal = -DOUBLE_BIG;
		for(val=0; val<variables[var]->domSize; val++){
			if(variables[var]->score(val) > bestScoreVal){
				bestScoreVal = variables[var]->score(val);
			}
		}
		if(good) {
			good_vars[num_good_vars++] = var; // fast insert (know it's not in yet)
			if(!(caching==CACHING_GOOD_PQ && variables[var]->fixed)){
				isgoodvar[var] = true;
				fh_insert(heapOfGoodVars, var+1, -bestScoreVal);
			}
		} else {
				isgoodvar[var] = false;
		} 
	}
	update_if_new_best_in_run();

	if(!keepPenalties){
		for(int pot=0; pot<num_pots; pot++){
			probTables[pot]->initRun();
		}
	}
//	printf("end of setPotentialIndicesScoresAndGoodvars\n");
}

void initComputingChangingCachedStructures(bool keepPenalties){
	int var;
	if(verbose) printf("...init changing data structures\n");
	//==== Initialization of values.
	computeInitValues();
	runInitTime = elapsed_seconds();
	run_time_so_far = preprocessingTime + runInitTime;
	for(var=0; var<num_vars; var++) variables[var]->value = initValues[var];
	if(verbose) printf("...init caching\n");
	setPotentialIndicesScoresAndGoodvars(keepPenalties);
}

void init_run(){
	start_timer();
	num_run++;
	assignmentManager->newRun();
	seedThisRun = seed;
	print_start_run();

	num_flip = 0;
	num_iteration = 0;
	lastImprovingIteration = 0;
	best_logprob_this_try = -DOUBLE_BIG;
	start_iteration_of_current_try = 0;
	num_flipped_since_last_ils_solution = 0;

	initComputingChangingCachedStructures(false);
	
	abort_flag = FALSE;
}

/********************************************************
 ========================================================
                     IO Functionality
 ========================================================
 ********************************************************/
	
/********************************************************
           PARAMETER HANDLING
 ********************************************************/

/********************************************************
  Set input-dependent parameters.
********************************************************/
void setInputDependentParameters(){
	int i;
	save_pertubation_strength = pertubation_strength;

	double pTmp = pertubation_strength;
	if (pertubation_rel) pTmp = pertubation_strength*0.01*num_vars;

	if(vns){
		vns_pertubation_strength = new int[num_vns_pertubation_strength];
		double strength = pTmp / pow(2.0,(num_vns_pertubation_strength-1.0)/2);
		if(verbose) fprintf(outfile, "pertubation strengths:\n");
		for(i=0; i<num_vns_pertubation_strength; i++){
			vns_pertubation_strength[i] = (int) ceil(strength);
			vns_pertubation_strength[i] = MIN(vns_pertubation_strength[i], num_vars);
			if(verbose) fprintf(outfile, "%d ", vns_pertubation_strength[i]);
			strength *= 2;
		}
		if(verbose) fprintf(outfile, "\n");
	}	else{
		pertubation_strength = (int) ceil(pTmp);
		pertubation_strength = MIN(pertubation_strength, num_vars);
		if(verbose) fprintf(outfile, "effective pertubation strength = %d\n", pertubation_strength);
	}
//		pertubation_strength = (int) ceil(pertubation_strength*(pow(num_vars, 0.3)));
//		pertubation_strength = (int) ceil(20 + pertubation_strength*(0.01*num_vars));
//		if( pertubation_strength >= num_vars / (2.0) ) pertubation_strength = (int) ceil(num_vars/2.0);
		
	if(tl_rel && tl > 0){
		tl = (int) ceil((numVarValCombos-num_vars)*0.01*tl); // percent of the var,vals it could choose
		if(verbose) fprintf(outfile, "effective tabu length = %d\n", tl);
		if(tl > numVarValCombos){
			if(verbose) fprintf(outfile, "tl is greater than there are variable,value - pairs! Exiting.\n");
			exit(-1);
		}
	}

	//=== If neither time nor steps are specified, run for 5 seconds.
	if( maxTime == BIG && maxSteps == BIG && maxIterations == BIG ){
		if(algo == ALGO_MB){
			maxTime = BIG;
		} else{
			maxTime = 5;
		}
	}
}

void print_tunable_parameters(){
	if(verbose){
		printf("-a{0,1,2,3}[%d]\n",algo);
		//printf("-c{0,1,2,3}\n");
	//	printf("-tl{0,5}\n");
		printf("-b{0,1}[%d]\n",init_algo);
		printf("-c{0,1,2,3}[%d]\n",caching);


	//=== GLS params.
	//	printf("-glsInc{0.1, 1, 10}[%lf]\n", glsPenaltyIncrement);
		printf("-glsSmooth{0.7, 0.8, 0.9, 0.99, 0.999, 1.00}[%lf]\n", glsSmooth);
		printf("-glsInterval{50, 200, 1000, 10000, 1000000000000}[%d]\n", glsInterval );
		printf("-glsPenMult{100,1000,10000,100000}[%lf]\n", glsPenaltyMultFactor );
		printf("-glsReal{0, 1}[%d]\n", glsReal );
		printf("-glsAspiration{0,1}[%d]\n",glsAspiration);

	//=== G+StS params.
		printf("-n{5,10,20,30,40,50}[%d]\n", noise);
		printf("-cf{1.5,2,5,10,100}[%lf]\n", cutoff);

	//=== ILS params.
		//=== Pertubation
		printf("-pert{0, 2}[%d]\n",pertubationType);
		printf("-pfix{0,1}[%d]\n", pertubationFixVars);
		printf("-vns{0,1}[%d]\n", vns?1:0);
		printf("-mbp{0,30,60,80,90}[%d]\n", mbPertubation);
	//	printf("-psab{1, 2, 10}[%lf]\n", psa_base); // 1: random, 10:quite greedy
	//	printf("-pspb{1, 2, 10}[%lf]\n", psp_base);

		printf("-p{1,2,3,4}[%d]\n",pertubation_strength);
		printf("-prel{0,1}[%d]\n",pertubation_rel);

		//=== Acceptance criterion.
		printf("-acc{%d,%d,%d,7}[%d]\n",ACC_BETTER, ACC_RW, ACC_BETTER_RW, accCriterion);
	//	printf("-rint{100,300,1000,3000,10000}[%d]\n",restartInterval);
		printf("-wint{0.1, 0.25, 0.5, 1, 2.5}[%lf]\n",worseningInterval);
		printf("-an{0.003, 0.01,0.03}[%lf]\n",accNoise);
		printf("-rf{1, 2, 4}[%d]\n", restartNumFactor);

	//	printf("-tmult{1.003, 1.01, 1.03}[%lf]\n", tmult);
	//	printf("-tdiv{1.5, 2, 4, 8}[%lf]\n", tdiv);
	//	printf("-tmin{0.0001, 0.0003, 0.001, 0.003}[%lf]\n", tmin);
		printf("-tbase{1.1, 1.5, 2, 4, 6, 8}[%lf]\n", tbase);
		printf("-T{0.01, 0.03, 0.1}[%lf]\n", T);

	//	printf("-tl{0,4,8,12,16,20,24}\n");
	//	printf("-mw{10000,100000,1000000}\n");

		printf("Conditionals\n");
	//	printf("mw|b in {3}\n");

	//=== ILS algorithm.
	//	printf("tl|a in {%d}\n", ALGO_ILS);
		
		//=== Acceptance criterion.
		printf("acc|a in {%d}\n", ALGO_ILS);
		printf("rf|a in {%d}\n", ALGO_ILS);
		printf("an|a in {%d}\n", ALGO_ILS);
		printf("wint|a in {%d}\n", ALGO_ILS);
	//	printf("tmult|a in {%d}\n", ALGO_ILS);
	//	printf("tdiv|a in {%d}\n", ALGO_ILS);
	//	printf("tmin|a in {%d}\n", ALGO_ILS);
		printf("tbase|a in {%d}\n", ALGO_ILS);
		printf("T|a in {%d}\n", ALGO_ILS);

		printf("rf|acc in {%d}\n", ACC_RESTART);
		printf("an|acc in {%d}\n", ACC_BETTER_RW);
		printf("wint|acc in {%d,%d,%d}\n", ACC_BEST_WORSENING, ACC_RW_AFTER_N, ACC_RW_AFTER_N2);
	//	printf("tmult|acc in {7}\n");
	//	printf("tdiv|acc in {7}\n");
	//	printf("tmin|acc in {7}\n");
		printf("tbase|acc in {7}\n");
		printf("T|acc in {7}\n");

		//=== Pertubtation.
		printf("p|a in {2}\n");
		printf("prel|a in {2}\n");
		printf("vns|a in {2}\n");
		printf("mbp|a in {2}\n");
		
		printf("pfix|a in {2}\n");
		printf("pert|a in {2}\n");
	//	printf("pspb|a in {2}\n");
	//	printf("psab|a in {2}\n");

	//	printf("pspb|pert in {%d, %d}\n", PERTUBATION_SAMPLED_POTS_RANDOM_INDEX, PERTUBATION_SAMPLED_POTS_SAMPLED_INDEX);
	//	printf("psab|pert in {%d, %d, %d}\n", PERTUBATION_RANDOM_POTS_SAMPLED_INDEX, PERTUBATION_SAMPLED_POTS_SAMPLED_INDEX, PERTUBATION_RANDOM_VARS_SAMPLED_FLIP);

	//=== G+StS algorithm.
		printf("n|a in {0}\n");
		printf("cf|a in {0,2}\n");

	//=== GLS algorithm.
		printf("glsInc|a in {1}\n");
		printf("glsSmooth|a in {1}\n");
		printf("glsInterval|a in {1}\n");
		printf("glsPenMult|a in {1}\n");
		printf("glsReal|a in {1}\n");
		printf("glsAspiration|a in {1}\n");
	}
}

/********************************************************
         PRINTING OUTPUT
 ********************************************************/
void print_curr_state(){
	int var;
	if(verbose) {
		fprintf(outfile, "next lm\n");
		for(var=0; var <num_vars-1; var++){
			fprintf(outfile, "%3d ", variables[var]->value);
		}
		fprintf(outfile, "%3d %lf %ld %ld %lf\n", variables[var]->value, log_prob, num_flip, num_iteration, run_time_so_far);
	}
}

void print_parameters(){
	//  fprintf(outfile, "SLS for MPE \n");
	if(verbose) {
		fprintf(outfile, "begin parameters\n");
		fprintf(outfile, "  preprocessingBound %lf\n", preprocessingSizeBound);	
		fprintf(outfile, "  =====================\n");	
		fprintf(outfile, "  optimalLogMPE %lf\n", assignmentManager->optimalLogMPEValue);
		fprintf(outfile, "  maxRuns %d\n", maxRuns);
		fprintf(outfile, "  seed %ld\n", seed);
		fprintf(outfile, "  maxTime %lf\n", maxTime);
		fprintf(outfile, "  maxSteps %ld\n", maxSteps);
		fprintf(outfile, "  maxIterations %d\n", maxIterations);
		fprintf(outfile, "  caching %d\n", caching);
		fprintf(outfile, "  init %d\n", init_algo);
		switch(algo){
			case ALGO_GN:
				fprintf(outfile, "  algo G+StS (%d)\n", algo);
				fprintf(outfile, "  G+StS cutoff %lf\n", cutoff);
				fprintf(outfile, "  G+StS noise %d\n", noise);
				break;
			case ALGO_GLS:
				fprintf(outfile, "  algo GLS (%d)\n", algo);
				fprintf(outfile, "   GLS increment%lf\n", glsPenaltyIncrement);
				fprintf(outfile, "   GLS smoothing factor%lf\n", glsSmooth);
				fprintf(outfile, "   GLS scaling interval%d\n", glsInterval);
				fprintf(outfile, "   Real GLS %d\n", glsReal);
				fprintf(outfile, "   GLS penalty multiplication factor %lf\n", glsPenaltyMultFactor);
				break;
			case ALGO_ILS:
				fprintf(outfile, "  pertubation type %d\n", pertubationType);
				fprintf(outfile, "   pertubationStrength %d\n", pertubation_strength);	
				fprintf(outfile, "   fixing perturbed vars %d\n", pertubationFixVars?1:0);
				fprintf(outfile, "   variable neighbourhood search %d\n", vns?1:0);
	//			fprintf(outfile, "  tabu length %d\n", tl);
	//			fprintf(outfile, "  maxMBWeight for MiniBuckets %lf\n", maxMBWeight);
				fprintf(outfile, "   original param pertubation strength %d, relative %d\n", save_pertubation_strength, pertubation_rel);
				fprintf(outfile, "  acceptance criterion %d\n", accCriterion);
				if(accCriterion == ACC_RESTART)
					fprintf(outfile, "   restart factor %d\n", restartNumFactor);
				if(accCriterion == ACC_BEST_WORSENING || accCriterion == ACC_RW_AFTER_N || accCriterion == ACC_RW_AFTER_N2)
					fprintf(outfile, "   interval %lf\n", worseningInterval);
				if(accCriterion == ACC_BETTER_RW)
					fprintf(outfile, "   acceptance noise %lf\n", accNoise);
				if(accCriterion == 7)
					fprintf(outfile, "   LSMC base %lf\n", tbase);
					fprintf(outfile, "   LSMC temperature %lf\n", T);
				break;
		}
		fprintf(outfile, "  EPS = %g\n", EPS);
		fprintf(outfile, "end parameters\n\n");
	}
}

void print_start_problem(){
	if(verbose) {
		if(network_filename) fprintf(outfile, "Begin problem %s\n\n", network_filename);
		else fprintf(outfile, "Begin manually defined problem\n");
	}
}

void print_start_run(){
//  fprintf(outfile, "\n===============================================\n");
//  fprintf(outfile, "Run %d/%d\n", num_run, maxRuns);
//  fprintf(outfile, "===============================================\n");
	if(verbose) fprintf(outfile, "begin try %d\nseed: %ld\n",num_run, seed);
}

void print_new_best_in_run(){
//	if(verbose) fprintf(outfile, "  best %12.6lf    %dth %12.6lf    flip %8d    it %8d    time %5.2lf    p.str %d   #goodV %d \n", assignmentManager->runBestLogProb, assignmentManager->M, assignmentManager->getMthBestSolQual(), num_flip, num_iteration, run_time_so_far, pertubation_strength, (caching==CACHING_GOOD_PQ)? heapOfGoodVars->n : num_good_vars);
}

void print_end_run(){
	if(verbose) {
		fprintf(outfile, "end try %d\n\n",num_run);

		fprintf(outfile, "begin solution %d\n",num_run);
		fprintf(outfile, "  best %lf\n", assignmentManager->runBestLogProb);
		fprintf(outfile, "  time %lf\n", run_time_so_far);
		fprintf(outfile, "  iteration %ld\n", num_iteration);
		fprintf(outfile, "  flip %ld\n", num_flip);
		fprintf(outfile, "  seed %d\n", seedThisRun);	
		assignmentManager->outputRunLMs(outfile);
		fprintf(outfile, "end solution %d\n\n",num_run);

		fprintf(outfile, "begin solutiondata %d\n",num_run);
		fprintf(outfile, "end solutiondata %d\n\n",num_run);

		fprintf(outfile, "begin further_infos%d\n",num_run);
		fprintf(outfile, "  Preprocessing time (same each run): %lf\n",preprocessingTime);
		fprintf(outfile, "  Init time for run: %lf\n",runInitTime);
		fprintf(outfile, "  Total time for run: %lf\n",run_time_so_far);
		fprintf(outfile, "  Total #(steps) and #(iterations) this run: %ld, %ld\n",num_flip, num_iteration);
		fprintf(outfile, "end further_infos%d\n\n",num_run);

		if( output_res ) assignmentManager->outputResult(resfile);
	}
}

void print_end_problem(){
	if(verbose) {
		if(network_filename) fprintf(outfile, "end problem %s\n\n", network_filename);
		else fprintf(outfile, "End manually defined problem\n\n");

		fprintf(outfile, "begin further_global_infos 1\n");
		fprintf(outfile, "	Instance contains %d variables\n", num_vars);
		fprintf(outfile, "	Read in and init took %lf seconds\n", init_time);
		assignmentManager->outputGlobalLMs(outfile);
		if(output_runstats) print_run_stats(outfile);

		fprintf(outfile, "  Best found log probability: %lf, prob: %e\n", assignmentManager->overallBestLogProb, pow(10.0,assignmentManager->overallBestLogProb));
		fprintf(outfile, "  Experiment took %lf seconds of CPU time, best solution was found after %lf seconds of run %d.\n", overall_time_so_far, assignmentManager->bestTime, assignmentManager->bestRun);
		fprintf(outfile, "end further_global_infos 1\n\n");
		
		fprintf(outfile, "begin system 1\n");
		fprintf(outfile, "end system 1\n\n");
		if (!noout) printf("Done. Wrote to %s. Exiting.\n", sls_filename);
	}
}

void print_assignment(FILE *out){
	int var;
	if(verbose) {
		fprintf(out, "Assignment: ");
		for(var = 0; var <num_vars; var++){
			fprintf(out, "%d ", variables[var]->value);
		}
		fprintf(out, "Log prob: %lf, prob: %e\n", log_prob, pow(10.0,log_prob));
	}
}

void print_run_stats(FILE *out){
	int var, value;
	if(verbose) {
		fprintf(out, "Begin run_stats\n");
		fprintf(out, "Assignment counts:\n");
		for(var=0; var<num_vars; var++){
			fprintf(out, "%d", var);	
			for(value=0; value<variables[var]->domSize; value++){
				fprintf(out, "  %d", variables[var]->numTimesValues[value]);
			}
			fprintf(out, "\n");
		}

		fprintf(out, "Local minima counts:\n");
		for(var=0; var<num_vars; var++)	variables[var]->outputValsInLM(out);

		fprintf(out, "Counts at ends of run:\n");
		for(var=0; var<num_vars; var++) variables[var]->outputValsAtEndOfRun(out);
		fprintf(out, "End run_stats\n");
	}
}

void print_ils_stats(){
	if(verbose) fprintf(outfile, "ils %ld: %d %lf %ld\n", num_iteration, assignmentManager->hammingDistFromLastLM(), log_prob-last_log_prob, num_flip-last_steps);
}

void update_lm_count(){
	for(int var=0; var<num_vars; var++) variables[var]->numTimesValuesInLM[variables[var]->value]++;
}

void update_ils_count(){
	for(int var=0; var<num_vars; var++) variables[var]->numTimesValuesAtEndOfRun[variables[var]->value]++;
}
/********************************************************
         Few functions that don't fit anywhere else.
 ********************************************************/

void handle_interrupt(int sig)
{
	if(verbose) fprintf(outfile, "Handle interrupt %d\n",sig);
  if (abort_flag) exit(-1);
  abort_flag = TRUE;
}

double sizeOfVariableSet(int numVars, int* vars){
	double result = 1;
	for(int i=0; i<numVars; i++) result *= variables[vars[i]]->domSize;
	return result;
}

double sizeOfVariableSet(vector<int> vars){
	double result = 1;
	for(size_t i=0; i<vars.size(); i++) result *= variables[vars[i]]->domSize;
	return result;
}

}  // sls4mpe

#endif
