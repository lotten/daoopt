
#include <cmath>
#include <new>
//#include <malloc.h>

#include "sls4mpe/ProbabilityTable.h"
#include "sls4mpe/global.h"
#include "sls4mpe/Variable.h"
#include "sls4mpe/my_set.h"

namespace sls4mpe {

void ProbabilityTable::defaultInitialization(){
	highestLogProb = -BIG;	
	ptVars = NULL;
	factorOfVar = NULL;
	logCPT = NULL;
	penalty = NULL;
}

ProbabilityTable::ProbabilityTable(){
	defaultInitialization();
}

ProbabilityTable::ProbabilityTable(const ProbabilityTable &old){
	defaultInitialization();

	setNumEntries((&old)->numEntries);
//	printf("cloning: %d\n",numPTVars);
	init((&old)->numPTVars);
	numPTVars = (&old)->numPTVars;
	memcpy(ptVars, (&old)->ptVars, sizeof(int)* ((&old)->numPTVars));
	memcpy(factorOfVar, (&old)->factorOfVar, sizeof(int)* ((&old)->numPTVars));
	highestLogProb = (&old)->highestLogProb;
	index = (&old)->index;
	
	if((&old)->logCPT == NULL) logCPT = NULL;
	else memcpy(logCPT, (&old)->logCPT, sizeof(double)* ((&old)->numEntries));

	if((&old)->penalty == NULL) penalty  = NULL;
	else memcpy(penalty, (&old)->penalty , sizeof(double)* ((&old)->numEntries));
}

ProbabilityTable::ProbabilityTable(int newNumVars, int* newVars){
	defaultInitialization();
		
	int i;
	//=== Allocate and initialize variables.
	init(newNumVars);

	//=== Set variables and their indices.
	for(i=0; i<newNumVars; i++) setVar(i,newVars[i]);
	initFactorsOfVars();

	//=== Compute numEntries and initialize logCPT.
	numEntries = 1;
	for(i=0; i<newNumVars; i++){
		numEntries *= variables[newVars[i]]->domSize;
	}

	setNumEntries(numEntries);
	for(i=0; i<numEntries; i++){
		logCPT[i] = 0;
		penalty[i] = 0;
	}
}

int ProbabilityTable::getFactorOfGlobalVar(int globalVar){
	//=== Get factor of marginalVar by simple linear search.
	int i;
	int factorOfGlobalVar = -1;
	for(i=0; i<numPTVars; i++){
		if(ptVars[i] == globalVar){
			factorOfGlobalVar = factorOfVar[i];
			break;
		}
	}
	assert(factorOfGlobalVar != -1);
	return factorOfGlobalVar;
}

void ProbabilityTable::init(int newNumPTVars){
	assert(newNumPTVars >= 0);
	numPTVars = newNumPTVars;
	ptVars = new int[numPTVars];
	assert(ptVars);
	factorOfVar = new int[numPTVars];
	assert(factorOfVar);
}

void ProbabilityTable::setVar(int localIndex, int globalVar){
	ptVars[localIndex] = globalVar;
}

//=== Initialize factors for the variables.
void ProbabilityTable::initFactorsOfVars(){
	long factor = 1;
	for(int i=numPTVars-1; i>=0; i--){
		setFactorsOfVars(i, factor);
		factor *= variables[ptVars[i]]->domSize;
	}
}

void ProbabilityTable::setFactorsOfVars(int i, int factor){
	factorOfVar[i] = factor;
}

void ProbabilityTable::setNumEntries(int newNumEntries){
//	printf("setNumEntries(%d)\n",newNumEntries);
	numEntries = newNumEntries;

	logCPT = new (std::nothrow) double[numEntries];
//	printf("%x\n", logCPT);
	if(logCPT == 0){
		printf("Out of memory - died.\n");
		exit(-1);
	}	
	penalty = new (std::nothrow) double[numEntries];
//	penalty = (double*) malloc( sizeof(double) * numEntries );
	if(penalty == 0){
		printf("Out of memory - died.\n");
		exit(-1);
	}
//	printf("setNumEntries went through\n");
/*
	if(numEntries > 10000000){
		printf("Can't process factor of size %d (>10000000). Out of memory\n", numEntries);
		exit(-1);
	}
*/
}

void ProbabilityTable::setEntry(int index, double entry){
	if( entry != 0 )
		logCPT[index] = log10(entry);
	else 
		logCPT[index] = log_zero;
	highestLogProb = MAX(logCPT[index], highestLogProb);
}

void ProbabilityTable::initRun(){
	for(int i=0; i<numEntries; i++) penalty[i] = 0;
}

int ProbabilityTable::sampleIndex(double greedyness){
	return sample_from_scores(logCPT, numEntries, greedyness);
}

void ProbabilityTable::outputVars()	{
	output(numPTVars, ptVars);
}

void ProbabilityTable::outputCPT() {
	output(numEntries, logCPT);
}

void ProbabilityTable::outputFactorsOfVars(){
	output(numPTVars, factorOfVar);
}

double ProbabilityTable::getPotentialProbIncrease(){
	return highestLogProb - logCPT[index];
}

/*******************************************************
 From here on:
 METHODS FOR MINIBUCKETS.
 *******************************************************/


void ProbabilityTable::multiplyBy(ProbabilityTable* smallPT){
	int i,j;
//=== Gather vars which are here, but not in small.
	int numOnlyHereVars = 0;
	int* onlyHereVars = new int[numPTVars]; // only an upper bound. //from now on local indices!!
	for(i=0; i<numPTVars; i++){
		for(j=0; j<smallPT->numPTVars; j++){
			if(ptVars[i] == smallPT->ptVars[j]) break;
		}
		if(j==smallPT->numPTVars) onlyHereVars[numOnlyHereVars++] = i;
	}
	assert(numOnlyHereVars == numPTVars - smallPT->numPTVars);

//=== Gather indices for all combinations of onlyHereVars
	int numAdditionalIndices = 1;
	for(i=0;i<numOnlyHereVars; i++)	numAdditionalIndices *= variables[ptVars[onlyHereVars[i]]]->domSize;
	int* additionalIndices = new int[numAdditionalIndices];

	int* valuesOfOnlyHereVars = new int[numOnlyHereVars];
	for(i=0;i<numOnlyHereVars; i++) valuesOfOnlyHereVars[i] = 0;

	//=== Binary counter to get all combinations of onlyHereVars into additionalIndices.
	for(i=0;i<numAdditionalIndices; i++){
		additionalIndices[i]=0;
		int carry = 1;
		for(j=numOnlyHereVars-1; j>=0; j--){
			additionalIndices[i] += valuesOfOnlyHereVars[j]*factorOfVar[onlyHereVars[j]];
			if(carry) valuesOfOnlyHereVars[j]++;
			if(valuesOfOnlyHereVars[j] == variables[ptVars[onlyHereVars[j]]]->domSize) // leave carry==1
				valuesOfOnlyHereVars[j]=0;
			else carry = 0;
		}
	}

//=== For all entries of smallPT, get indices in this PT 
//===	and multiply them and all combinations of the additional indices by the 
//=== entry in smallPT.
	int* valuesOfSmallPTVars = new int[smallPT->numPTVars];
	for(i=0;i<smallPT->numPTVars; i++)	valuesOfSmallPTVars[i] = 0;

	for(i=0;i<smallPT->numEntries; i++){
//		printf("i=%d\n",i);
		int hereIndex=0;
		int carry = 1;
		for(j=smallPT->numPTVars-1; j>=0; j--){
			hereIndex += valuesOfSmallPTVars[j]*getFactorOfGlobalVar(smallPT->ptVars[j]);
			if(carry) valuesOfSmallPTVars[j]++;
			if(valuesOfSmallPTVars[j] == variables[smallPT->ptVars[j]]->domSize) // leave carry==1
				valuesOfSmallPTVars[j]=0;
			else carry=0;
		}
		//=== Actually do the multiplication.
		for(int k=0;k<numAdditionalIndices; k++){
//			printf("k=%d, hereIndex=%d, additionalIndex=%d, index=%d\n",k, hereIndex, additionalIndices[k], hereIndex+additionalIndices[k]);
			logCPT[hereIndex+additionalIndices[k]] += smallPT->logCPT[i];
		}
	}
	highestLogProb = -DOUBLE_BIG;
	for(i=0; i<numEntries; i++) highestLogProb = MAX(highestLogProb, logCPT[i]);

	delete[] valuesOfSmallPTVars;
	delete[] valuesOfOnlyHereVars;
	delete[] additionalIndices ;
	delete[] onlyHereVars;
}

void ProbabilityTable::getMarginal(const int* valsSoFar, int marginalVar, double *marginal){
	int factorOfMarginalVar = getFactorOfGlobalVar(marginalVar);
	//=== Get entris for all values of marginalVar, given all the other vars.
	assert(valsSoFar[marginalVar] == -1); // This is the one we want to assign.
	assert(contains(ptVars, numPTVars, marginalVar));
	int othersIndex = 0;
	for(int i=0; i<numPTVars; i++){
		int var = ptVars[i];
		if(var == marginalVar) continue;
		assert(valsSoFar[var] != -1);
		othersIndex += factorOfVar[i]*valsSoFar[var];
	}
	for(int val=0; val<variables[marginalVar]->domSize; val++){
		marginal[val] = logCPT[othersIndex + val*factorOfMarginalVar];
	}
/* //=== In retrospect really helpful debug information! Helped me.
	printf("Single Marginals: ");
	output(variables[marginalVar]->domSize, marginal);
	printf("\n");
*/
}

ProbabilityTable* ProbabilityTable::maximized(int var){
//	outputVars();
//	printf(" - maximizing out %d\n", var);
	int factorOfMaxVar = getFactorOfGlobalVar(var);
	int domSize = variables[var]->domSize;
	ProbabilityTable* result = new ProbabilityTable;
	result->init(numPTVars-1);
	result->setNumEntries(numEntries/domSize);

//=== Input the new vars and factors. The last vars has the lowest factor.
	//=== Down to the variable that's maximized out.
	int i;
	for(i=numPTVars-1; i>=0; i--){
		if(ptVars[i] == var) break;
		result->setVar(i-1,ptVars[i]);
		result->setFactorsOfVars(i-1,factorOfVar[i]);
	}
	//=== Down from the variable that's maximized out.
	for(; i>=0; i--){
		if(ptVars[i] == var) continue;
		result->setVar(i,ptVars[i]);
		result->setFactorsOfVars(i,factorOfVar[i]/domSize);
	}

//=== Extract the new cpt from the current one by using the correct indices.
	for(i=0;i<result->numEntries;i++) result->logCPT[i] = -BIG;
	int resultIndex=0;
	for(i=0;i<numEntries;i+=domSize*factorOfMaxVar){ // steps of size domsize*factor. After this, all bits lower or equal to var are the same again.
		for(int k=0; k<factorOfMaxVar; k++){ // all combinations of all the bits lower than var
			for(int j=0; j<domSize; j++){ // all values of var
//				result->cpt[resultIndex] += cpt[i+k+j*factorOfMaxVar]; // for marginalize w/o normalize
				result->logCPT[resultIndex] = MAX(result->logCPT[resultIndex], logCPT[i+k+j*factorOfMaxVar]); // for marginalize w/o normalize
			}
			resultIndex++; // entries for all values of var with the rest of the vars remaining the same are added to one entry.
		}
	}
	assert(resultIndex == result->numEntries);
	return result;
}

ProbabilityTable* ProbabilityTable::instantiate(int var, int value){
	//=== Exactly the same as maximized, but not the maximal value
	//=== for the var is taken, but the value given as a parameter. TODO: refactor !
/*	printf("instantiate %d to %d in \n",var,value);
	output(numPTVars, ptVars);
	printf("\n");*/
	int factorOfInstVar = getFactorOfGlobalVar(var);
	int i;

	int domSize = variables[var]->domSize;
	ProbabilityTable* result = new ProbabilityTable;
	result->init(numPTVars-1);
	result->setNumEntries(numEntries/domSize);

//=== Input the new vars and factors. The last vars has the lowest factor.
	//=== Down to the variable that's maximized out.
	for(i=numPTVars-1; i>=0; i--){
		if(ptVars[i] == var) break;
		result->setVar(i-1,ptVars[i]);
		result->setFactorsOfVars(i-1,factorOfVar[i]);
	}
	//=== Down from the variable that's maximized out.
	for(; i>=0; i--){
		if(ptVars[i] == var) continue;
		result->setVar(i,ptVars[i]);
		result->setFactorsOfVars(i,factorOfVar[i]/domSize);
	}

//=== Extract the new cpt from the current one by using the correct indices.
	for(i=0;i<result->numEntries;i++) result->logCPT[i] = -BIG;
	int resultIndex=0;
	for(i=0;i<numEntries;i+=domSize*factorOfInstVar){ // steps of size domsize*factor. After this, all bits lower or equal to var are the same again.
		for(int k=0; k<factorOfInstVar; k++){ // all combinations of all the bits lower than var
//			printf("i=%d, k=%d, value=%d, index=%d\n",i,k,value, i+k+value*factorOfInstVar);
			result->logCPT[resultIndex] = logCPT[i+k+value*factorOfInstVar];
			result->highestLogProb = MAX(result->highestLogProb, result->logCPT[resultIndex]);
			resultIndex++; // entries for all values of var with the rest of the vars remaining the same are added to one entry.
		}
	}
	assert(resultIndex == result->numEntries);
/*	output(result->numEntries, result->logCPT);
	printf("\n");*/

	return result;
}

ProbabilityTable* ProbabilityTable::instantiated(bool fakeEvidence){
	vector<int> newVars;
	int* factorsOfNewVars = new int[numPTVars];
	int* onlyHereVars = new int[numPTVars]; //local indices as well.
	int numOnlyHereVars = 0, i;

	for(i=0; i<numPTVars; i++){
		if(variables[ptVars[i]]->observed || (fakeEvidence && variables[ptVars[i]]->fakeEvidenceForMB)){
			onlyHereVars[numOnlyHereVars++] = i;
		} else {
			factorsOfNewVars[newVars.size()] = factorOfVar[i];
			newVars.push_back(ptVars[i]);
		}
	}

//Hack since the constructor is not yet set up for vectors.
	int* tmp = new int[newVars.size()];
	int tmpSize=0;
	addAllToFrom(tmp, &tmpSize, newVars);
	ProbabilityTable* result = new ProbabilityTable(tmpSize, tmp);
	delete[] tmp;

	//=== Binary counter to get all correct entries from here into the new PT.
	int evidenceVarAdditionalIndex = 0;
	for(i=0; i<numOnlyHereVars; i++){
		evidenceVarAdditionalIndex += factorOfVar[onlyHereVars[i]]*variables[ptVars[onlyHereVars[i]]]->value;
	}
	
	int* valuesOfNewVars = new int[newVars.size()];
	for(i=0;i<newVars.size(); i++) valuesOfNewVars[i] = 0;
//assert(numNewVars == numPTVars);
//assert(result->numEntries == numEntries);

	for(i=0; i<result->numEntries; i++){
		int carry = 1;
		int index = evidenceVarAdditionalIndex;
//		printf("--0: %d\n",index);
		for(int j=newVars.size()-1; j>=0; j--){
			index += valuesOfNewVars[j]*factorsOfNewVars[j];
//			printf("--1: %d, %d\n",j, index);
			if(carry){
				valuesOfNewVars[j]++;
				if(valuesOfNewVars[j] == variables[newVars[j]]->domSize) // leave carry==1
					valuesOfNewVars[j]=0;
				else carry = 0;
			}
		}
//		printf("%d: %d\n",i, index);
		result->logCPT[i] = logCPT[index];
		result->highestLogProb = MAX(result->highestLogProb, result->logCPT[i]);
	}

/*	output(numEntries, logCPT);
	printf("\n");
	output(result->numEntries, result->logCPT);
	printf("\n");*/

//	for(i=0; i<result->numEntries; i++){assert(result->logCPT[i] == logCPT[i]);}
/*	output(result->numEntries, result->logCPT);
	printf("\n");*/
	delete[] valuesOfNewVars;
	delete[] factorsOfNewVars;
	delete[] onlyHereVars;
	return result;
}


ProbabilityTable* ProbabilityTable::clone(){
	ProbabilityTable* result = new ProbabilityTable();
	result->setNumEntries(numEntries);
//	printf("cloning: %d\n",numPTVars);
	result->init(numPTVars);
	result->numPTVars = numPTVars;
	memcpy(result->ptVars, ptVars, sizeof(int)*numPTVars);
	memcpy(result->factorOfVar, factorOfVar, sizeof(int)*numPTVars);
	result->highestLogProb = highestLogProb;
	result->index = index;
	
	if(logCPT == NULL) result->logCPT = NULL;
	else memcpy(result->logCPT, logCPT, sizeof(double)*numEntries);

	if(penalty == NULL) result->penalty  = NULL;
	else memcpy(result->penalty, penalty , sizeof(double)*numEntries);
	
	return result;
}


ProbabilityTable::~ProbabilityTable(){
	delete[] ptVars;
	delete[] factorOfVar; 
	
	if (logCPT != NULL) delete[] logCPT;
	if (penalty != NULL) delete[] penalty;
//	if (logCPT != NULL) free(logCPT);
//	if (penalty != NULL) free(penalty);
}

}  // sls4mpe
