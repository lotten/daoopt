/*
 * SLS4MPE by Frank Hutter, http://www.cs.ubc.ca/labs/beta/Projects/SLS4MPE/
 * Some modifications by Lars Otten
 */
#ifndef _SLS4MPE_VARIABLE_H
#define _SLS4MPE_VARIABLE_H

#include "DEFINES.h"
#ifdef ENABLE_SLS

#include <stdio.h>
#include "sls4mpe/global.h"
#include <set>
using namespace std;

namespace sls4mpe {

class Variable{
public:
	int id;
	Variable();
	~Variable();

//=== Methods.
	void setId(int newID);
	bool initRun(bool justPenalty);
	void setName(char* newName);
	void setDomainSize(int newDomainSize);
	void allocateOcc();
	void setnumVarsInMB(int newnumVarsInMB);
	double score(int value);

	void addVarToMB(int var);
	void removeVarFromMB(int var);
	
	bool flipTo(int newValue, int numFlip, int* good_vars, int* num_good_vars, int caching, double* log_prob, bool justPenalty);
	bool operator< (Variable other) const;

//	Variable* clone();

//=== Attributes.
	char* name;
	int domSize;
	int value;
//	double* scores; // scores[value] = k <=> log_prob would increase by k if var was flipped to value
	double* logProbScores;
	double* penaltyScores;

	int* tabuValues;  // tabuValues[val] = k <=> var has been val until step k 
	                  // and is not allowed to be val again until step k + tl

	bool fixed;

	bool observed;
	bool fakeEvidenceForMB;
	int numVarsInMB;
	double weightOfMB;
	int* mb;    // mb[j] = var2 <=> var2 is the j'th var in this var's markov blanket 
              // the order of vars in the mb is arbitrary
			  // I should really change this to use an STL set sometime ...
	int reservedMemForMB;

	int numOcc;   // variable occurs in numOcc potentials
	int *occ;     // occ[j] = pot <=> var's j'th occurence is in pot.
	int *numInOcc;   // numInOcc[j]  = k <=> var's j'th occurence is at num k in a pot.

	int lastILSValue;
	
//=== Statistics.
	int lastFlip;
	int* numTimesValues;
	int* numTimesValuesInLM;
	int* numTimesValuesAtEndOfRun;
	int numTimesFlipped;

	void outputVals(FILE* out);
	void outputValsInLM(FILE* out);
	void outputValsAtEndOfRun(FILE* out);

};

double inline Variable::score(int value){
  if(algo == ALGO_GLS){
    switch(glsReal){
    case 0:
      return penaltyScores[value];
      break;
    case 1:
      return logProbScores[value] + penaltyScores[value];//scores[value];
      break;
    case 2:
      //printf("Case 2, logProbScores[value] = %lf, penaltyScores[value] = %lf\n", logProbScores[value], penaltyScores[value]);
	    return pow(10.0,logProbScores[value]+log_prob) - pow(10.0,log_prob) + penaltyScores[value];
    }
  }
  return logProbScores[value];
}

}  // sls4mpe

#endif
#endif
