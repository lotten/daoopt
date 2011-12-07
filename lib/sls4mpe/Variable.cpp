#include "DEFINES.h"
#ifdef ENABLE_SLS

#include <string.h>
#include "sls4mpe/Variable.h"
#include "sls4mpe/my_set.h"
#include "sls4mpe/ProbabilityTable.h"
#include "sls4mpe/AssignmentManager.h"
#include <set>
#include "sls4mpe/fheap.h"

namespace sls4mpe {

void Variable::setName(char* newName){
	name = new char[strlen(newName)+1];
	strcpy(name, newName);
}

Variable::Variable(){
	mb = new int[INITIAL_MBSET_SIZE];  // this is an upper bound.
	reservedMemForMB = INITIAL_MBSET_SIZE;
	weightOfMB = 1;
	numVarsInMB = 0;
	fixed = false;
	observed = false;
	fakeEvidenceForMB = false;
}

void Variable::setId(int newID){
	id = newID;
}

void Variable::setDomainSize(int newDomainSize){
	domSize = newDomainSize;
	numTimesValues = new int[domSize];
	numTimesValuesInLM = new int[domSize];
	numTimesValuesAtEndOfRun = new int[domSize];
	tabuValues = new int[domSize];
	//scores = new double[domSize];
	logProbScores = new double[domSize];
	penaltyScores = new double[domSize];
	for(int val=0; val<domSize; val++) numTimesValuesAtEndOfRun[val] = 0;
}

bool Variable::flipTo(int newValue, int numFlip, int* good_vars, int* num_good_vars, int caching, double* log_prob, bool justPenalty){
	if (value == newValue){
		return false;
//		printf("Error. Trying to flip variable %d to value %d which it already has\nExiting.\n", inst.var, variables[inst.var]->value);		
//		assert(false);
	}

	int old_pot_index, j, val, num, var2;
	//	fprintf(stderr, "flip %d to %d\n",inst.var,inst.value);

	if (caching == CACHING_INDICES){
		int pot;
		for(j=0; j<numOcc; j++){
			pot = occ[j];
			(*log_prob) += probTables[pot]->change(numInOcc[j],newValue);		
		}
	}

//=== For CACHING_SCORE and CACHING_GOOD_VARS and CACHING_GOOD_PQ, cache the score.
	if (caching==CACHING_SCORE || caching==CACHING_GOOD_VARS || caching==CACHING_GOOD_PQ){

		for(j=0; j<numOcc; j++){
	//=== For each potential var occurs in, do lots of computation for the caching.
			int pot = occ[j];
			//=== Save old pot_index.
			old_pot_index = probTables[pot]->index;

			//double diff_score = probTables[pot]->diffLogProbWithVarFlipped(numInOcc[j], newValue) + probTables[pot]->diffPenaltyWithVarFlipped(numInOcc[j], newValue);
			double diff_logProbScore = probTables[pot]->diffLogProbWithVarFlipped(numInOcc[j], newValue);
			double diff_penaltyScore = probTables[pot]->diffPenaltyWithVarFlipped(numInOcc[j], newValue);

			(*log_prob) += probTables[pot]->change(numInOcc[j], newValue);
			int index = probTables[pot]->index;
			
	//=== Update the scores of all var-value pairs for vars in pot.
			//=== Change scores for the flipped var. (Only reflecting the change from this prob_table)
			for(val=0; val<domSize; val++){
				logProbScores[val] -= diff_logProbScore;
				penaltyScores[val] -= diff_penaltyScore;
				//scores[val] -= diff_score;
			}
			//=== To counter bad problems with numerical inexactness,
			//=== set the score of the new var and the new val to 0.
			//=== It is 0 anyways, but the caching is subject to numerical instabilities.
			//=== When it is slightly positive, just above EPS, we're caught in 
			//=== an endless loop!
			penaltyScores[newValue] = 0;
			logProbScores[newValue] = 0;
			//scores[newValue] = 0;
			
			for(num=0; num<probTables[pot]->numPTVars; num++){
				if( num == numInOcc[j] ) continue; // var itself is a special case and handled above.
				var2 = probTables[pot]->ptVars[num];
				int var2_factor = probTables[pot]->factorOfVar[num];

				//== Compute change in index when var2 is set to val=0.
				int var2indexIncrease = -var2_factor * variables[var2]->value;

				//=== Compute changes in the scores of the other variables var2 in the potential when flipping var.
				for(val=0; val<variables[var2]->domSize; val++){
					//== Here: variables[var2]->scores[val] = k <=> before var was flipped, if I flip var2 to val it buys us k
					// (actually, scores subsumes this cached value from all prob_tables)
					
					variables[var2]->penaltyScores[val] -= (-glsPenaltyMultFactor*probTables[pot]->penalty[old_pot_index + var2indexIncrease]);
					variables[var2]->penaltyScores[val] += (-glsPenaltyMultFactor*probTables[pot]->penalty[index + var2indexIncrease]);

					variables[var2]->logProbScores[val] -= probTables[pot]->logCPT[old_pot_index + var2indexIncrease];
					variables[var2]->logProbScores[val] += probTables[pot]->logCPT[index + var2indexIncrease];

/*
					if(justPenalty){
						variables[var2]->scores[val] -= (-glsPenaltyMultFactor*probTables[pot]->penalty[old_pot_index + var2indexIncrease]);
						variables[var2]->scores[val] += (-glsPenaltyMultFactor*probTables[pot]->penalty[index + var2indexIncrease]);
						if( glsReal ){
							variables[var2]->scores[val] -= probTables[pot]->logCPT[old_pot_index + var2indexIncrease];
							variables[var2]->scores[val] += probTables[pot]->logCPT[index + var2indexIncrease];
						}
					} else{
						variables[var2]->scores[val] -= probTables[pot]->logCPT[old_pot_index + var2indexIncrease];
						variables[var2]->scores[val] += probTables[pot]->logCPT[index + var2indexIncrease];
					}
*/
					//variables[var2]->scores[val] -= diff_score;
					variables[var2]->logProbScores[val] -= diff_logProbScore;
					variables[var2]->penaltyScores[val] -= diff_penaltyScore;
					
					//== Here: variables[var2]->scores[val] = k <=> after var was flipped, if I flip var2 to val it buys us k
					// (actually, the difference between above and here is just that var has been
					// flipped in this prob_table. The outer loop is iterating the affected prob_tables)

					//=== Increment var2indexIncrease for the next val.
					var2indexIncrease += var2_factor;
				}
			}
		}

//=== For CACHING_GOOD_VARS and CACHING_GOOD_PQ, cache the good vars.
		if(caching==CACHING_GOOD_VARS || caching==CACHING_GOOD_PQ){
			//=== For all vars in Markov blanket of var, update whether they can lead to an improvement.
				// for slightly faster implementation, it may be possible to prevent having this 2nd loop
				// by changing the loop above to the same iteration order.
			//=== var itself is also in its Markov blanket here.
			for(j=0; j<numVarsInMB; j++){
				var2 = mb[j];
//				printf("mb[%d]=%d\n", j, var2);

				bool is_good_var = false;
				double bestValScore = -DOUBLE_BIG;
				for(val=0; val<variables[var2]->domSize; val++){
					if(variables[var2]->score(val) > bestValScore){
						bestValScore = variables[var2]->score(val);
					}
				}
				if( bestValScore > EPS){
					is_good_var = true;
				}

				if( is_good_var && isgoodvar[var2] && caching==CACHING_GOOD_PQ){
					//=== Update the key.
					fh_remove(heapOfGoodVars, var2+1);
					fh_insert(heapOfGoodVars, var2+1, -bestValScore);
				}
				if( is_good_var && !isgoodvar[var2] ){ // new good var.
					if(!(caching==CACHING_GOOD_PQ && variables[var2]->fixed)){
						isgoodvar[var2] = true;
					}
					if(caching==CACHING_GOOD_VARS){
	//					assert(!contains(good_vars, (*num_good_vars), var2));
						good_vars[(*num_good_vars)++] = var2;
						//insert(good_vars,num_good_vars,var2);
	//					assert(contains(good_vars, (*num_good_vars), var2));
	//					assert(!contains2(good_vars, (*num_good_vars), var2));
	//					printf("good var: %d\n", var2);
					} else { // caching PQ
						if(!variables[var2]->fixed){
							fh_insert(heapOfGoodVars, var2+1, -bestValScore);
						}
					}
				} else {
					if(!is_good_var && isgoodvar[var2]){ // was good, not good anymore.
						isgoodvar[var2] = false;
						if( caching==CACHING_GOOD_VARS ){
	//						assert(contains(good_vars, (*num_good_vars), var2));
							remove(good_vars,num_good_vars,var2);
	//						assert(!contains(good_vars, (*num_good_vars), var2));
		//					printf("is no good var: %d\n", var2);
						} else { // caching PQ
//							printf("4: %d\n", heapOfGoodVars->n);
							fh_remove(heapOfGoodVars, var2+1);
						}
					}
				}
			}
		} // caching score or good vars
	} // caching good vars


	numTimesValues[value] += numFlip - lastFlip;
	tabuValues[value] = numFlip;
	value = newValue;
	if(caching==CACHING_NONE){
		(*log_prob) = assignmentManager.get_log_score();
/*		// Updating the time is necessary in this case since even one 
		// basic local search can be super-slow and we might want to stop it.
		// For the other cases, we don't do this to prevent updating the
		// time too often, thus slowing down the algorithm.
		run_time_so_far += elapsed_seconds(); */
	}

	lastFlip = numFlip;
	numTimesFlipped++;
	return true;
}

void Variable::addVarToMB(int var){
	if(!contains(mb, numVarsInMB, var)){
		weightOfMB *= variables[var]->domSize;
		insertAndExtend(var, &mb, &numVarsInMB, &reservedMemForMB);
	}
//	else mb[numVarsInMB++] = var;
//	printf("end of addVar\n");
}

void Variable::allocateOcc(){
	numInOcc = new int[numOcc];
	occ = new int[numOcc];
}

void Variable::removeVarFromMB(int var){
	if(contains(mb, numVarsInMB, var)){
		remove(mb,&numVarsInMB, var);
		weightOfMB /= variables[var]->domSize;
	}
}

bool Variable::initRun(bool justPenalty){
	bool is_a_good_var = false;
	for(int val=0; val<domSize; val++){
		numTimesValues[val] = 0;
		numTimesValuesInLM[val] = 0;
		tabuValues[val] = -BIG;

		//=== Init scores.
		//scores[val] = 0.0;
		logProbScores[val] = 0.0;
		penaltyScores[val] = 0.0;

		for(int j=0; j<numOcc; j++){
			int pot = occ[j];
			logProbScores[val] += probTables[pot]->diffLogProbWithVarFlipped(numInOcc[j],val);
			penaltyScores[val] += probTables[pot]->diffPenaltyWithVarFlipped(numInOcc[j],val);
//			scores[val] += probTables[pot]->diffLogProbWithVarFlipped(numInOcc[j],val) + probTables[pot]->diffPenaltyWithVarFlipped(numInOcc[j],val);
		}
		if(score(val) > EPS) is_a_good_var = true;
	}
	numTimesFlipped = 0;
	lastFlip = 0;
	return is_a_good_var;
}

void Variable::outputVals(FILE* out){
	fprintf(out, "%s", name);
	for(int val=0; val<domSize; val++){
		fprintf(out, " %d", numTimesValues[val]);
	}
	fprintf(out, "\n");
}

void Variable::outputValsInLM(FILE* out){
	fprintf(out, "%s", name);
	for(int val=0; val<domSize; val++){
		fprintf(out, " %d", numTimesValuesInLM[val]);
	}
	fprintf(out, "\n");
}

void Variable::outputValsAtEndOfRun(FILE* out){
	fprintf(out, "%s", name);
	for(int val=0; val<domSize; val++){
		fprintf(out, " %d", numTimesValuesAtEndOfRun[val]);
	}
	fprintf(out, "\n");
}

/*
 Comparison operator compares the weight of the
 Markov Blanket for Mini Buckets with min-weight heuristic.
 */
bool Variable::operator< (const Variable other) const{
	return weightOfMB < other.weightOfMB;
}

/* broken for some reason ... probably missing an instantiation or misspecifying one.
Variable* Variable::clone(){
	Variable* result = new Variable();
	result->setDomainSize(domSize);
	result->setName(name);
	result->numOcc = numOcc;
	result->allocateOcc();
	result->fakeEvidenceForMB = fakeEvidenceForMB;
	result->fixed = fixed;
	result->id = id;
	result->lastFlip = lastFlip;
	result->lastILSValue = lastILSValue;
	memcpy(result->logProbScores, logProbScores, sizeof(double)*domSize);
	memcpy(result->mb, mb, sizeof(int)*numVarsInMB);
	memcpy(result->numInOcc, numInOcc, sizeof(int)*numOcc);
	result->numTimesFlipped = numTimesFlipped;
	memcpy(result->numTimesValues, numTimesValues, sizeof(int)*domSize);
	memcpy(result->numTimesValuesAtEndOfRun, numTimesValuesAtEndOfRun, sizeof(int)*domSize);
	memcpy(result->numTimesValuesInLM, numTimesValuesInLM, sizeof(int)*domSize);
	result->numVarsInMB = numVarsInMB;
	result->observed = observed;
	memcpy(result->occ, occ, sizeof(int)*numOcc);
	memcpy(result->penaltyScores, penaltyScores, sizeof(double)*domSize);
	memcpy(result->tabuValues, tabuValues, sizeof(int)*domSize);
	result->value = value;
	result->weightOfMB = weightOfMB;

	return result;
}*/

Variable::~Variable(){
	delete[] mb;
	delete[] logProbScores;
	delete[] tabuValues;
	delete[] occ;
	delete[] numInOcc;
	delete[] numTimesValues;
	delete[] numTimesValuesInLM;
	delete[] numTimesValuesAtEndOfRun;
}

}  // sls4mpe

#endif
