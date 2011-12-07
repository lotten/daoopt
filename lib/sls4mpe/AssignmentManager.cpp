/*
 * SLS4MPE by Frank Hutter, http://www.cs.ubc.ca/labs/beta/Projects/SLS4MPE/
 * Some modifications by Lars Otten
 */
#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/AssignmentManager.h"

namespace sls4mpe {

extern time_t timestamp_start;

void AssignmentManager::outputCurrentAssignment(FILE* outfile){
	copyAssignment(tmpAssignment);
	outputAssignment(outfile, tmpAssignment);
}

void AssignmentManager::outputAssignment(FILE* outfile, int* assignment){
	fprintf(outfile, "assignment %lf[", get_log_score(assignment));
	for(int i=0; i<numVars-1; i++){
		fprintf(outfile, "%d ", assignment[i]);
	}
	fprintf(outfile, "%d]\n", assignment[numVars-1]);
}

void AssignmentManager::endRun(){
	if(runBestLogProb >= optimalLogMPEValue - EPS){
		numRunsWithOptimumFound++;
	}
	if( runBestLogProb < worstSolutionCostFound ) worstSolutionCostFound = runBestLogProb;
	avgSolutionCostFound += runBestLogProb;

	//=== Gather best solution.
	if(runBestLogProb > overallBestLogProb - EPS){  
		if(runBestLogProb > overallBestLogProb + EPS){  // new better.
			overallBestLogProb = runBestLogProb;
			bestTime = runBestTime;
			bestRun = numRun;
			if(M_MPE){ // for M-MPE, the best assignment is not necessarily the first one in the array.
				double bestLogProb = -DOUBLE_BIG;
				int bestIndex = -1;
				for(int i=0; i<M; i++){
					if(runBestLogProbs[i] > bestLogProb){
						bestLogProb = runBestLogProbs[i];
						bestIndex = i;
					}
				}
				assert(bestIndex > -1);	
				copy_from_to(runBestAssignments[bestIndex], globalBestAssignments[0], numVars);
			} else {
				copy_from_to(runBestAssignments[0], globalBestAssignments[0], numVars);
			}
		}
	}
}

void AssignmentManager::recordNewBestSolution(){
	copyAssignment(runBestAssignments[0]);
}

bool AssignmentManager::newGoodAssignment(){
	for(int ass=0; ass < fh_BestMLogProbs->n; ass++){
		bool same = true;
		for(int var=0; var<numVars; var++){
			if(runBestAssignments[ass][var] != variables[var]->value) {
				same = false;
				break;
			}
		}
		if( same ) return false;
	}
	return true;
}

//#include <iostream>
bool AssignmentManager::updateIfNewBest(double log_prob){
//	printf("updateIfNewBest(%lf)\n", log_prob);
	bool gotBetter = false;
	//	printf("LOG %.5lf\n",log_prob);

	if(log_prob > runBestLogProb + EPS){  // new better.
		gotBetter = true;
		runBestLogProb = log_prob;
		if(!M_MPE){
			copyAssignment(runBestAssignments[0]);
		}

		bool debug = false;
		if(debug){
			assert(fabs(get_log_score() - get_log_score(runBestAssignments[0]))<EPS);
			if(fabs(get_log_score(runBestAssignments[0]) - runBestLogProb) > EPS){
				fprintf(stdout, "Log probability %lf of best run assignment does not match %lf\n", get_log_score(runBestAssignments[0]), runBestLogProb);
				fprintf(stderr, "Log probability %lf of best run assignment does not match %lf\n", get_log_score(runBestAssignments[0]),  runBestLogProb);
				assert(false);
			}
		}
	}

	if(gotBetter && log_prob > overallBestLogProb + EPS){  // new global best.
	  time_t now; time(&now);
	  double elapsed = difftime(now, timestamp_start);
	  fprintf(stdout, "[%i] u -1 -1 %g", (int) elapsed, log_prob);
#ifndef NO_ASSIGNMENT
	  for(int var=0; var<numVars; var++)
	    fprintf(stdout, " %i", variables[var]->value);
#endif
	  fprintf(stdout, "\n");
	  //std::cout << "[" << int(elapsed) << "] u -1 -1 " << log_prob << std::endl;
	}

	if(M_MPE){ // additional work.
		int numSolutionsInHeap = fh_BestMLogProbs->n;
		if( numSolutionsInHeap < M && newGoodAssignment()){ // put in until M.
			printf("Putting in %dth assignment with prob %lf\n", numSolutionsInHeap, log_prob);
			copyAssignment(runBestAssignments[numSolutionsInHeap]);
			runBestLogProbs[numSolutionsInHeap] = log_prob;
			fh_insert(fh_BestMLogProbs, numSolutionsInHeap+1, log_prob);
			gotBetter = true;
		} else {
//			printf("log_prob=%lf, index = %d, %lf\n", log_prob, fh_inspect(fh_BestMLogProbs), runBestLogProbs[fh_inspect(fh_BestMLogProbs)] + EPS);
//			printf("newGood %d\n", newGoodAssignment()?1:0);
			if( (log_prob > runBestLogProbs[fh_inspect(fh_BestMLogProbs)-1] + EPS) 
				&& newGoodAssignment() ){ 	 // replace worst if strictly larger

				int index = fh_delete_min(fh_BestMLogProbs);
				printf("Removed index %d with prob %lf\n", index, runBestLogProbs[index-1]);
				
				copyAssignment(runBestAssignments[index-1]);
				printf("Put in index %d with prob %lf\n", index, log_prob);
				fh_insert(fh_BestMLogProbs, index, log_prob);
				runBestLogProbs[index-1] = log_prob;
				gotBetter = true;
			}
		}
	} 

//	printf("done with updateIfNewBest\n");
	return gotBetter;
}

void AssignmentManager::outputRunLMs(FILE* outfile){
	fprintf(outfile, "\nOptimal MPE found: %s\n\n", foundOptimalThisRun() ? "yes" : "no"); 
	if(M_MPE){
		int i;
		/* build heap for sorting solutions in output. */
		int* order = new int[M];
		
		struct fheap* tmp;
		tmp = fh_alloc(M);
		for(i=0; i<fh_BestMLogProbs->n; i++){
			fh_insert(tmp, i+1, -runBestLogProbs[i]);
		}

		for(i=0; i<fh_BestMLogProbs->n; i++){
			order[i] = fh_delete_min(tmp)-1;
		}
	//	fh_free(tmp); crashes !!

		fprintf(outfile, "  Best M Scores:\n");
		for(i=0; i<fh_BestMLogProbs->n; i++){
			fprintf(outfile, "%.6lf ", runBestLogProbs[order[i]]);
		}
		fprintf(outfile, "\n");
		delete[] order;
	} else {
		if(outputBestMPE) outputAssignment(outfile, runBestAssignments[0]);
			if(fabs(get_log_score(runBestAssignments[0]) - runBestLogProb) > EPS){
			fprintf(outfile, "Log probability %lf of best run assignment does not match %lf\n", get_log_score(runBestAssignments[0]), runBestLogProb);
			fprintf(stderr, "Log probability %lf of best run assignment does not match %lf\n", get_log_score(runBestAssignments[0]), runBestLogProb);
			assert(false);
		}
	}
}

double AssignmentManager::get_log_score(int* values){
	double result = 0;
	int pot,j;

//==== Compute temporary potential indices and 
//==== return the sum of the log_probs at these indices.
	int tmp_pot_ind;
	long factor;
	for(pot=0; pot<num_pots; pot++){
		tmp_pot_ind = 0;
		factor = 1;
		for(j=localProbTables[pot]->numPTVars-1; j>=0; j--){
			tmp_pot_ind += factor*values[localProbTables[pot]->ptVars[j]];
			factor *= variables[localProbTables[pot]->ptVars[j]]->domSize;
		}
		result += localProbTables[pot]->logCPT[tmp_pot_ind];
		localProbTables[pot]->index = tmp_pot_ind; // this is done to be able to use pot_index in the pertubation!
	}
	return result;
}

void AssignmentManager::outputGlobalLMs(FILE* outfile){
	fprintf(outfile, "\n================== STATS ================\n");
	fprintf(outfile, "Worst solution quality found: %lf\n", worstSolutionCostFound);
	fprintf(outfile, "Average solution quality found: %lf\n", avgSolutionCostFound / numRun);
	fprintf(outfile, "Best solution quality found: %lf\n", overallBestLogProb);
	fprintf(outfile, "%d/%d runs found optimal MPE.\n", numRunsWithOptimumFound, numRun); 
	fprintf(outfile, "=========================================\n\n");

	if(fabs(get_log_score(globalBestAssignments[0]) - overallBestLogProb) > EPS){
		fprintf(outfile, "Log probability %lf of overall best assignment does not match %lf\n", get_log_score(globalBestAssignments[0]), overallBestLogProb);
		fprintf(stderr, "Log probability %lf of overall best assignment does not match %lf\n", get_log_score(globalBestAssignments[0]), overallBestLogProb);
		assert(false);
	}
	
	if(outputBestMPE) outputAssignment(outfile, globalBestAssignments[0]);
}

double AssignmentManager::get_log_score(){
	for(int var=0; var<numVars; var++) tmpAssignment[var] = variables[var]->value;
	return get_log_score(tmpAssignment);
}

}  // sls4mpe

#endif
