/*
 * SLS4MPE by Frank Hutter, http://www.cs.ubc.ca/labs/beta/Projects/SLS4MPE/
 * Some modifications by Lars Otten
 */
#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/Bucket.h"
#include "sls4mpe/my_set.h"
#include "sls4mpe/Variable.h"
#include <vector>
using namespace std;

namespace sls4mpe {

void Bucket::defaultInitialization(){
	numBucketVars = 0;
	bucketWeight = 1;
	numSubPTs = 0;
	numPartitions = 0;

	subPTs.clear();
	bucketVars.clear();
	varsInPartition.clear();
	partition.clear();
	bucketPTs.clear();
}

Bucket::Bucket(){}

void Bucket::add(ProbabilityTable* pProbTable){
	bucketPTs.push_back(pProbTable);
	partition.push_back(-1);
	subPTs.push_back(-1);

	for(int i=0; i<pProbTable->numPTVars; i++){
		if( !contains(bucketVars, pProbTable->ptVars[i])){
			bucketVars.push_back(pProbTable->ptVars[i]);
			bucketWeight *= variables[pProbTable->ptVars[i]]->domSize;
		}
	}
}

void Bucket::outputBucket(){
	printf("Bucket[%d]: ", num);
	for(size_t i=0; i<bucketPTs.size(); i++){
		bucketPTs[i]->outputVars();
	}
	printf("\n");
}

ProbabilityTable* Bucket::processPartition(int partitionNumber){
	size_t i;
	//	printf("In process, numBucketVars = %d\n", numBucketVars);
	varsInPartition.clear();
	int numMatchingPTs = 0;
	assert(bucketPTs.size());

	for(i=0; i<bucketPTs.size(); i++){
		if(partition[i] == partitionNumber){
			addAllToFrom(&varsInPartition, bucketPTs[i]->ptVars, bucketPTs[i]->numPTVars);
			if(subPTs[i] == -1) numMatchingPTs++;
		}
	}
//Hack since the constructor is not yet set up for vectors.
	int* tmp = new int[varsInPartition.size()];
	int tmpSize=0;
	addAllToFrom(tmp, &tmpSize, varsInPartition);
	ProbabilityTable* result = new ProbabilityTable(tmpSize, tmp);
	delete[] tmp;

/* 
	printf("Processing bucket[%d] with %d PTs, weight %d and %d vars: ",num, bucketPTs.size(), bucketWeight, numBucketVars );
	output(numBucketVars, bucketVars);
	printf("\n");
*/
	for(i=0; i<bucketPTs.size(); i++){
		if(partition[i] == partitionNumber){
			result->multiplyBy(bucketPTs[i]);
		}
	}
//	printf("result.numEntries = %d\n",result->numEntries);
/*	printf("numMatchingPTs = %d\n",numMatchingPTs);
	assert(numMatchingPTs >= 1);*/
	return result;
}

int Bucket::getBestAssignmentForBucket(const int* mpeAssignmentSoFar){
/*
	printf("\n  Best so far: ");
	output(num_vars, mpeAssignmentSoFar);
	printf("\n");
*/
	if(variables[num]->fakeEvidenceForMB) return variables[num]->value;
	int domSize = variables[num]->domSize;
//	printf("Bucket %i, domain %i\n", num, domSize);
	double* wholeMarginal = new double[domSize];
	double* singleMarginal = new double[domSize];
	int i,j;
	for(i=0; i<domSize; i++) wholeMarginal[i]=0;
	for(i=0; i<bucketPTs.size(); i++){
		if(bucketPTs[i]->numPTVars != 0){ // PTs w/o vars are constants.
			bucketPTs[i]->getMarginal(mpeAssignmentSoFar, num, singleMarginal);
			for(j=0; j<domSize; j++) wholeMarginal[j] += singleMarginal[j];
		}
	}
//	printf("  Whole Marginals: ");
//	output(domSize, wholeMarginal);
	double best = -BIG;
	int result = -1;
	for(int val=0; val<domSize; val++) {
		if(wholeMarginal[val] > best){
			best = wholeMarginal[val];
			result = val;
		}
	}
	assert(result != -1);
	delete[] wholeMarginal;
	delete[] singleMarginal;
//	printf("\n  Best index: %d\n",result);

	return result;
}

int Bucket::constructPartititons(int ib, double weightBound){
	size_t i;
//=== Identify subsumptions.
	bool iSubsumesJ, jSubsumesI;
	for(i=0; i<bucketPTs.size(); i++){
		for(size_t j=i+1; j<bucketPTs.size(); j++){
			jSubsumesI = subset(bucketPTs[i]->numPTVars, bucketPTs[i]->ptVars, bucketPTs[j]->numPTVars, bucketPTs[j]->ptVars);
			iSubsumesJ = subset(bucketPTs[j]->numPTVars, bucketPTs[j]->ptVars, bucketPTs[i]->numPTVars, bucketPTs[i]->ptVars);
			if(jSubsumesI) subPTs[i] = j; // also if they are equal
			else{
				if(iSubsumesJ) subPTs[j] = i;
			}
		}
	}

//	outputBucket();
/*	
	printf("subPTs: ");
	output(bucketPTs.size(), subPTs);
	printf("\n");
*/
//	printf("numPartitions=%d\n",numPartitions);

//=== Construct partitions.
	varsInPartition.clear();
//	printf("constructing partition for bucket %d\n",num);
	for(i=0; i<bucketPTs.size(); i++){
		if(subPTs[i] == -1){ // not subsumed
			addAllToFrom(&varsInPartition, bucketPTs[i]->ptVars, bucketPTs[i]->numPTVars);
			if (varsInPartition.size() > ib || sizeOfVariableSet(varsInPartition) > weightBound){
			//=== Start next partition.
			//=== (unless there are only the new vars in the partition. In that case,
			//=== it was empty before. But at least one PT has to be in the partition.)
				if(varsInPartition.size() != bucketPTs[i]->numPTVars){
					numPartitions++; 
				}
				varsInPartition.clear();
				addAllToFrom(&varsInPartition, bucketPTs[i]->ptVars, bucketPTs[i]->numPTVars);
			}
			partition[i] = numPartitions;
		}
	}

	numPartitions++;
	
//	outputBucket();

//=== Iteratively assign partitions to subsumed PTs until no more subsumed ones left.
	bool changed = true;
	while(changed){
		changed = false;
		for(i=0; i<bucketPTs.size(); i++){
			if(partition[i] == -1){ // still unassigned
				if(partition[subPTs[i]] == -1) continue;
				partition[i] = partition[subPTs[i]];
				changed = true;
			}
		}
	}

	for(i=0; i<bucketPTs.size(); i++) assert(partition[i] != -1);

/*
	printf("partitions: ");
	output(bucketPTs.size(), partition);
	printf("\n");
*/
	return numPartitions;

}

}  // sls4mpe

#endif
