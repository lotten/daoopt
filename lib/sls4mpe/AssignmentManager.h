#ifndef _SLS4MPE_ASSIGNMENTMANAGER_H_
#define _SLS4MPE_ASSIGNMENTMANAGER_H_

#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/global.h"
#include "sls4mpe/my_set.h"
#include "sls4mpe/ProbabilityTable.h"
#include "sls4mpe/fheap.h"

namespace sls4mpe {

class AssignmentManager {
public:
	bool outputLM;
	int numRun;
	double overallBestLogProb;
	double runBestLogProb;

	bool M_MPE;
	int M;
	double *runBestLogProbs;
	struct fheap *fh_BestMLogProbs;

	double bestTime;
	double runBestTime;
	double optimalLogMPEValue;
	double worstSolutionCostFound;
	double avgSolutionCostFound;

	int numRunsWithOptimumFound;
	int bestRun;
	ProbabilityTable** localProbTables;
	int* tmpAssignment;

	int numVars;
	int** runBestAssignments;
	int** globalBestAssignments;
	//globalBestAssignments[i][var]=value <=> in the i'th 
								                //best explanation found at all, variable var is set to val

	AssignmentManager(){}
	bool newGoodAssignment();

	double getMthBestSolQual(){
		if(fh_BestMLogProbs->n > 0){
			return runBestLogProbs[fh_inspect(fh_BestMLogProbs)-1];
		} else return -1;
	}

	void init(){
		if(!M_MPE) M=1;
		fh_BestMLogProbs = fh_alloc(M);
		overallBestLogProb = -DOUBLE_BIG;
		runBestLogProbs = new double[M];
		runBestAssignments = new int*[M];
		globalBestAssignments = new int*[M];
		numRun = 0;
		numRunsWithOptimumFound = 0;
		worstSolutionCostFound = DOUBLE_BIG;
		avgSolutionCostFound = 0;
		bestTime = NOVALUE;
	}

	void setProbTables(ProbabilityTable** probTables){
		localProbTables = probTables;	
	} 

	void setNumberOfVariables(int newNumberOfVariables, bool newOutputLM){
		numVars = newNumberOfVariables;
		outputLM = newOutputLM;
		for(int i=0; i<M; i++){
			runBestAssignments[i] = new int[newNumberOfVariables];
			globalBestAssignments[i] = new int[newNumberOfVariables];
		}
		tmpAssignment = new int[newNumberOfVariables];
	}

	double get_log_score();
	double get_log_score(int* values);
	void endRun();
	bool updateIfNewBest(double log_prob);
	void outputRunLMs(FILE* outfile);
	void outputGlobalLMs(FILE* outfile);
	void outputAssignment(FILE* outfile, int* assignment);
	void outputCurrentAssignment(FILE* outfile);

	void newRun(){
		numRun++;
		runBestLogProb = -DOUBLE_BIG;
	}
	
	void recordNewBestSolution();

	bool foundOptimalThisRun(){
		return runBestLogProb > optimalLogMPEValue - EPS;
	}

	void outputResult(FILE* outfile){
		fprintf(outfile, "%lf %lf\n", runBestLogProb, runBestTime);
	}

	void copyAssignment(int* newValues){
		for(int var=0; var<numVars; var++) newValues[var] = variables[var]->value;	
	}

	bool sameAssignment(const int* values){
		for(int var=0; var<numVars; var++){
			if( values[var] != variables[var]->value ) return false;	
		}
		return true;
	}

	int hammingDistFromLastLM(){
		int result = 0;
		for(int var=0; var<numVars; var++){
			if(variables[var]->value != variables[var]->lastILSValue) result++;
		}
		return result;
	}

	bool differentFromLastILSSolution(){
		return hammingDistFromLastLM() != 0;
	}
};

}  // sls4mpe

#endif
#endif
