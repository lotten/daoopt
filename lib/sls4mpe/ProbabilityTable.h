#ifndef _SLS4MPE_PROBABILITYTABLE_H_
#define _SLS4MPE_PROBABILITYTABLE_H_

#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/main_algo.h"
#include "sls4mpe/Variable.h"
#include "sls4mpe/my_set.h"
#include "sls4mpe/fheap.h"

namespace sls4mpe {

class ProbabilityTable{
public:
	ProbabilityTable();
	ProbabilityTable(int newNumVars, int* newVars);
	ProbabilityTable(const ProbabilityTable &old);
	~ProbabilityTable();

	void init(int newNumPTVars);
	void initRun();
	void initFactorsOfVars();
	int getFactorOfGlobalVar(int globalVar);
	void setVar(int localIndex, int var);
	void setNumEntries(int newNumEntries);
	void setEntry(int index, double entry);
	void setFactorsOfVars(int i, int factor);
	void outputVars();
	void outputCPT();
	void outputFactorsOfVars();
	void multiplyBy(ProbabilityTable* smallPT);
	void adaptNewGlobalVars(int numOld, int* old2new);

	double logProbWithVarFlipped(int globalVar, int value);
	double change(int localVar, int value);
	double logProbAt(int questionedIndex);
	double currLogProb();
	double currPenalty();
	double currUtility();
	void incCurrPenalty(double increment, int caching, int* good_vars, int* num_good_vars);
	void scalePenalties(double scaleFactor);

	double diffLogProbWithVarFlipped(int localVar, int value);
	double diffPenaltyWithVarFlipped(int localVar, int value);

	void getMarginal(const int* valsSoFar, int var, double *marginal);
	double getPotentialProbIncrease();

	int sampleIndex(double greedyness);
	
	ProbabilityTable* maximized(int var);
	ProbabilityTable* instantiate(int var, int value);
	ProbabilityTable* instantiated(bool fakeEvidence);
	ProbabilityTable* clone();

	int* global2Local;  //global2Local[global_var] = local_var
	int numPTVars;
	int index;
	int* ptVars;
	int* factorOfVar; // factorOfVar[j] = k <=> the table's j'th variable has factor k
	int numEntries;
	double* logCPT; // logCPT[j] = logprob <=> the table's j'th entry has logprobability 
	                // logprob where j is constructed by the instantiations of vars
                // ptVars[0] (the BN var the table is for) is the highestmost bit, 
	              // then the first parent ptVars[1], then the 2nd, etc.
	double* penalty; // penalty[j] is the penalty for the j'th entry in the cpt.
	double highestLogProb;

private:
	int* factorOfGlobalVar; // factorOfVar[var] = k <=> var has factor k in this table.
	void defaultInitialization();
};






/********************************************************
                     INLINE METHODS
 ********************************************************/


inline double ProbabilityTable::currLogProb(){
	return logCPT[index];
}

inline double ProbabilityTable::currPenalty(){
	return penalty[index];
}

inline double ProbabilityTable::currUtility(){
	return -logCPT[index] / (1.0+ penalty[index]);
}

inline void ProbabilityTable::incCurrPenalty(double increment, int caching, int* good_vars, int* num_good_vars){
	penalty[index] += increment;
	if(caching == CACHING_GOOD_VARS || caching == CACHING_SCORE || caching == CACHING_GOOD_PQ){
		for(int i=0; i<numPTVars; i++){
			int var = ptVars[i];
			double bestValScore = -DOUBLE_BIG;
			for(int val=0; val<variables[var]->domSize; val++){
				if( val == variables[var]->value ) continue;
				// current val is getting worse, so all others are getting better
				variables[var]->penaltyScores[val] += glsPenaltyMultFactor*increment;
				if( variables[var]->score(val) > bestValScore ){
					bestValScore = variables[var]->score(val);
				}
			}
			if(bestValScore > EPS && !isgoodvar[var]){
				if(!(variables[var]->fixed && caching == CACHING_GOOD_PQ)) {
					isgoodvar[var] = true;
				}
				if(caching == CACHING_GOOD_VARS){
					insert(good_vars,num_good_vars,var);
				}
				if(caching == CACHING_GOOD_PQ){
					if(!variables[var]->fixed){
						fh_insert(heapOfGoodVars, var+1, -bestValScore);
					}
				}
			}
		}
	}
}

inline void ProbabilityTable::scalePenalties(double scaleFactor){
	for(int i=0; i<numEntries; i++) penalty[i] *= scaleFactor;
}

inline double ProbabilityTable::logProbWithVarFlipped(int globalVar, int value){
	return logCPT[index + factorOfGlobalVar[globalVar]*(value-variables[globalVar]->value)];
}

inline double ProbabilityTable::diffLogProbWithVarFlipped(int localVar, int value){
	return -logCPT[index] + logCPT[index + factorOfVar[localVar]*(value-variables[ptVars[localVar]]->value)];
}

inline double ProbabilityTable::diffPenaltyWithVarFlipped(int localVar, int value){
	return -glsPenaltyMultFactor*(-penalty[index] + penalty[index + factorOfVar[localVar]*(value-variables[ptVars[localVar]]->value)]);
}

inline double ProbabilityTable::change(int localVar, int value){
	int oldIndex = index;
	index += (value-variables[ptVars[localVar]]->value) * factorOfVar[localVar];
	return logCPT[index] - logCPT[oldIndex];
}

}  // sls4mpe

#endif
#endif
