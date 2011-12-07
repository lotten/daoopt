/*
 * SLS4MPE by Frank Hutter, http://www.cs.ubc.ca/labs/beta/Projects/SLS4MPE/
 * Some modifications by Lars Otten
 */
#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/MiniBucketElimination.h"
#include "sls4mpe/random_numbers.h"
#include "sls4mpe/my_set.h"
#include <stdlib.h>
#include "sls4mpe/fheap.h"

namespace sls4mpe {

void MiniBucketElimination::initOnce(){
	int i;
	numOfBuckets = num_vars;
	buckets = new Bucket[numOfBuckets];
	order = new int[num_vars];
	for(i=0; i<numOfBuckets; i++) buckets[i].num=i;
	mbVariables = new Variable*[num_vars];
	optimalVars = new int[num_vars];
	for(i=0; i<num_vars; i++) mbVariables[i] = new Variable();
}

void MiniBucketElimination::createOrder(int numFakeEvidenceVars, int* fakeEvidenceVars, int* outInducedWidth, double* outInducedWeight){
	int var, i;
//=== Compute order offline in a randomized way.
	//=== Reset temporary structure used to compute order.
	for(var=0; var<num_vars; var++){
		mbVariables[var]->reservedMemForMB = variables[var]->reservedMemForMB;
		if(mbVariables[var]->reservedMemForMB != INITIAL_MBSET_SIZE){
			// set got extended, need to reallocate memory.
			delete[] mbVariables[var]->mb;
			mbVariables[var]->mb = new int[variables[var]->reservedMemForMB];
		}
		copy_from_to(variables[var]->mb, mbVariables[var]->mb, variables[var]->numVarsInMB);
		mbVariables[var]->weightOfMB = variables[var]->weightOfMB;
		assert(mbVariables[var]->weightOfMB);
		mbVariables[var]->numVarsInMB = variables[var]->numVarsInMB;
		mbVariables[var]->domSize = variables[var]->domSize;
		assert(mbVariables[var]->domSize);
		mbVariables[var]->fakeEvidenceForMB = variables[var]->fakeEvidenceForMB;
		mbVariables[var]->value = variables[var]->value;
		mbVariables[var]->id = variables[var]->id;
	}

//double* weights = new double[num_vars];

	//=== Fill heap with variables.

//    BinaryHeap<Variable> variableHeap( num_vars );
//	for(var=0; var<num_vars; var++){
//		Variable x = *mbVariables[var];
//		variableHeap.insert(x);
//		Variable *v = mbVariables[var];
//		printf("variable %d has weight %lf\n", v->id, v->weightOfMB);
//	}
//	for(var=0; var<10; var++){
//		Variable v = variableHeap.findMin();
//		variableHeap.deleteMin();
//		printf("variable %d has weight %lf\n", v.id, v.weightOfMB);
//	}
//	exit(-1);


//=== Compute order with min weight heuristic.
	(*outInducedWidth) = -BIG;
	(*outInducedWeight) = -BIG;

	//=== Fill heap with variables and their initial weights as keys.
	struct fheap* heap = fh_alloc(num_vars+1);
	for(var=0; var<num_vars; var++){
		fh_insert(heap, var+1, mbVariables[var]->weightOfMB);
//		printf("%lf ", mbVariables[var]->weightOfMB);
	}

	for(i=num_vars-1; i>=0; i--){
		int elim = fh_delete_min(heap)-1;
//		printf("eliminating %d\n",elim);

		int j,k;
		//=== Connect neighbours of eliminated variable.
		for(j=0; j<mbVariables[elim]->numVarsInMB; j++){
			int var = mbVariables[elim]->mb[j];
			for(k=j+1; k<mbVariables[elim]->numVarsInMB; k++){
				int var2 = mbVariables[elim]->mb[k];
//				printf("  ... adding1 %d to MB of %d\n", var2, var);
				mbVariables[var]->addVarToMB(var2);
//				printf("  ... adding2 %d to MB of %d\n", var, var2);
				mbVariables[var2]->addVarToMB(var);
			}
		}

		//=== Remove eliminated variable from neighbour lists.
//		output(mbVariables[elim]->numVarsInMB, mbVariables[elim]->mb);
//		printf(" (before)\n");
		for(j=0; j<mbVariables[elim]->numVarsInMB; j++){
			int var = mbVariables[elim]->mb[j];
			// not to remove var is really important since the loop
			// doesn't complete otherwise !!
			if(var == elim) continue; // do not remove this! learned it the hard way !!
//			printf("Removing %d from %d. => From %lf ", elim, var, mbVariables[var]->weightOfMB);
			mbVariables[var]->removeVarFromMB(elim);
//			printf("down to %lf\n", mbVariables[var]->weightOfMB);
		}

//		output(mbVariables[elim]->numVarsInMB, mbVariables[elim]->mb);
//		printf(" (after)\n");

  //=== Update weights of the neighbours.
		//=== Delete all of var's neighbours from heap.
		for(j=0; j<mbVariables[elim]->numVarsInMB; j++){
			int var = mbVariables[elim]->mb[j];
			if(var == elim) continue; // do not remove this !!
//			printf("decreasing %d\n", var);
//			fh_decrease_key(heap, var+1, -DOUBLE_BIG);
//			printf("deleting min %d\n", var);
//			assert(fh_delete_min(heap)==var+1);
//			printf("removing %d\n", var);
			fh_remove(heap, var+1);
		}
		//=== Put them back in with updated key.
		//Ideally, we would want an "updateKey" operation (can in- or decrease).
		for(j=0; j<mbVariables[elim]->numVarsInMB; j++){
			int var = mbVariables[elim]->mb[j];
			if(var == elim) continue; // do not remove this !!
//			printf("putting back in %d\n", var);
			fh_insert(heap, var+1, mbVariables[var]->weightOfMB);
		}

		//=== Remember position of variable in the ordering.
		order[i] = elim;
//		weights[i] = mbVariables[elim]->weightOfMB;
		if(mbVariables[elim]->weightOfMB > (*outInducedWeight)) 
			(*outInducedWeight) = mbVariables[elim]->weightOfMB;
		if(mbVariables[elim]->numVarsInMB > (*outInducedWidth))
			(*outInducedWidth) = mbVariables[elim]->numVarsInMB;
	}
	fh_free(heap);

	/* Straight forward version running in N^2
//	Not using fakeEvidence anymore!
//	int numFakeEvidenceVarsProcessed=0;
	double minWeight = DOUBLE_BIG;
	for(i=num_vars-1; i>=0; i--){
		int elim;
//		if(numFakeEvidenceVarsProcessed >= numFakeEvidenceVars){
			//=== Regularly look for best variable to eliminate.
			int minDegree, numOptimalVars;
			numOptimalVars = 0;
			minDegree = BIG;
			minWeight = DOUBLE_BIG;
			//=== Determine next variable to eliminate.
			for(int var=0; var<num_vars; var++){
				if(mbVariables[var]->weightOfMB <= minWeight+0.1 ){
					if(mbVariables[var]->weightOfMB < minWeight-0.1 ) numOptimalVars = 0;
					minWeight = mbVariables[var]->weightOfMB;
					optimalVars[numOptimalVars++] = var;
				}
			}
			assert(minWeight < DOUBLE_BIG);
			//elim = optimalVars[random_number(&seed)%numOptimalVars]; //deterministic for debugging. TODO: randomize again
			elim = optimalVars[0];
//			printf("elim %d\n", elim);

			//=== Eliminate variable.
			order[i] = elim;
			weights[i] = mbVariables[elim]->weightOfMB;
		//		printf("Planning to eliminate variable %d with degree %d and weight %d\n", elim, mbVariables[elim].numVarsInMB, mbVariables[elim]->weightOfMB);
			if(minWeight > (*outInducedWeight)) (*outInducedWeight) = minWeight;
			if(mbVariables[elim]->numVarsInMB > (*outInducedWidth))
				(*outInducedWidth) = mbVariables[elim]->numVarsInMB;
			
			//=== Connect neighbours of eliminated variable.
			for(int j=0; j<mbVariables[elim]->numVarsInMB; j++){
				int var = mbVariables[elim]->mb[j];
				for(int k=j+1; k<mbVariables[elim]->numVarsInMB; k++){
					int var2 = mbVariables[elim]->mb[k];
//					printf("adding1 %d to MB of %d\n", var2, var);
					mbVariables[var]->addVarToMB(var2);
//					printf("adding2 %d to MB of %d\n", var, var2);
					mbVariables[var2]->addVarToMB(var);
				}
			}
//			printf("done connecting neighbours\n");
//		} else {
//			elim = fakeEvidenceVars[numFakeEvidenceVarsProcessed++];
//			order[i] = elim;
//		}

		//=== Remove eliminated variable from neighbour lists.
		while(mbVariables[elim]->numVarsInMB > 0){
			int var = mbVariables[elim]->mb[0];
			mbVariables[elim]->removeVarFromMB(var);
			mbVariables[var]->removeVarFromMB(elim);
		}

		mbVariables[elim]->weightOfMB = DOUBLE_BIG; // such that it's not chosen again.
//		mbVariables[elim]->numVarsInMB = BIG; // such that it's not chosen again.
	}
	*/
//	output(num_vars, weights);
//	output(num_vars, order);
//	exit(-1);

  //	printf("created order\n");
}


/*void MiniBucketElimination::createOrder(int numFakeEvidenceVars, int* fakeEvidenceVars, int* outInducedWidth, double* outInducedWeight){
//=== Compute order offline in a randomized way.
	//=== Reset temporary structure used to compute order.
	for(int var=0; var<num_vars; var++){
		copy_from_to(variables[var]->mb, mbVariables[var]->mb, variables[var]->numVarsInMB);
		mbVariables[var]->weightOfMB = variables[var]->weightOfMB;
		mbVariables[var]->numVarsInMB = variables[var]->numVarsInMB;
		mbVariables[var]->domSize = variables[var]->domSize;
		mbVariables[var]->fakeEvidenceForMB = variables[var]->fakeEvidenceForMB;
		mbVariables[var]->value = variables[var]->value;
	}

	//=== Compute order with min degree heuristic.
	(*outInducedWidth) = -BIG;
	(*outInducedWeight) = -BIG;

	int minDegree, numOptimalVars,i;
	
	int numFakeEvidenceVarsProcessed=0;
	for(i=num_vars-1; i>=0; i--){
		int elim;
		if(numFakeEvidenceVarsProcessed >= numFakeEvidenceVars){
			//=== Regularly look for best variable to eliminate.
			numOptimalVars = 0;
			minDegree = BIG;
			//=== Determine next variable to eliminate.
			for(int var=0; var<num_vars; var++){
				if(mbVariables[var]->numVarsInMB <= minDegree ){
					if(mbVariables[var]->numVarsInMB < minDegree ) numOptimalVars = 0;
					minDegree = mbVariables[var]->numVarsInMB;
					optimalVars[numOptimalVars++] = var;
				}
			}
			//elim = optimalVars[random_number(&seed)%numOptimalVars]; //deterministic for debugging. TODO: randomize again
			elim = optimalVars[0];

			//=== Eliminate variable.
			order[i] = elim;
		//		printf("Planning to eliminate variable %d with degree %d and weight %d\n", elim, mbVariables[elim].numVarsInMB, mbVariables[elim]->weightOfMB);
			if(minDegree > (*outInducedWidth)) (*outInducedWidth) = minDegree;
			if(mbVariables[elim]->weightOfMB > (*outInducedWeight))
				(*outInducedWeight) = mbVariables[elim]->weightOfMB;
			
			//=== Connect neighbours of eliminated variable.
			for(int j=0; j<mbVariables[elim]->numVarsInMB; j++){
				int var = mbVariables[elim]->mb[j];
				for(int k=j+1; k<mbVariables[elim]->numVarsInMB; k++){
					int var2 = mbVariables[elim]->mb[k];
					mbVariables[var]->addVarToMB(var2);
					mbVariables[var2]->addVarToMB(var);
				}
			}
		} else {
			elim = fakeEvidenceVars[numFakeEvidenceVarsProcessed++];
			order[i] = elim;
		}

		//=== Remove eliminated variable from neighbour lists.
		while(mbVariables[elim]->numVarsInMB > 0){
			int var = mbVariables[elim]->mb[0];
			mbVariables[elim]->removeVarFromMB(var);
			mbVariables[var]->removeVarFromMB(elim);
		}

		mbVariables[elim]->numVarsInMB = BIG; // such that it's not chosen again.
	}
//	printf("created order\n");
}
*/

void MiniBucketElimination::initRun(bool newOrder){
	int bucketNum, pot;
	//=== Reset buckets.
	for(bucketNum=0; bucketNum<numOfBuckets; bucketNum++) buckets[bucketNum].defaultInitialization();

	//=== Create a new randomized order not taking into account approximations.
// not randomized for now, thus we don't need to redo work.	if( newOrder ) createOrder(numFakeEvidenceVars, fakeEvidenceVars); 

	//=== Assign PTs to their bucket.
	for(pot=0; pot<num_pots; pot++){
//		printf("MiniBucketElimination::initRun - pot %d\n", pot);
		createBucketPT(probTables[pot]->clone()); // creates a new copy.
//TODO:check that this works.		createBucketPT(probTables[pot]->instantiated(true)); // creates a new copy.
	}
/* DEBUG: get result brute force for really small networks.
	printf("%d potentials\n", num_pots);
	for(bucketNum=0; bucketNum<numOfBuckets; bucketNum++) 
	buckets[bucketNum].outputBucket();

	int num = 0;
	int* vars = new int[num_vars];
	for(pot=0; pot<num_pots; pot++){
		addAllToFrom(vars, &num, probTables[pot]->ptVars, probTables[pot]->numPTVars);
	}

	ProbabilityTable *result;
	result = new ProbabilityTable(num, vars);
	for(pot=0; pot<num_pots; pot++){
		result->multiplyBy(probTables[pot]);
	}
	printf("%g\n", result->highestLogProb);

	exit(-1);
	*/
}

void MiniBucketElimination::createBucketPT(ProbabilityTable* pProbTable){
	//=== Determine the bucket for this PT. The PT must be destroyed
	//=== when it is not used anymore. It's just a copy of the original one.
/*
	printf("vars in probtable: ");
	pProbTable->outputVars();
	printf("\n");
*/
	int matchingBucketNum = -1;
	for(int i=numOfBuckets-1; i>=0; i--){
		int bucketNum = order[i];
//		buckets[bucketNum].outputBucket();

		for(int j=0; j<pProbTable->numPTVars; j++){
			if( bucketNum == pProbTable->ptVars[j] ){  // the table contains this bucket variable.
				matchingBucketNum = bucketNum;
				break;
			}
		}
		if( matchingBucketNum != -1 ) break;
	}
	if((pProbTable->numPTVars) == 0){
//		buckets[order[0]].add(pProbTable->clone());
		buckets[order[0]].add(pProbTable);
		return;
	}

	assert(matchingBucketNum != -1); // must have gotten a match.
	//=== Add the table to the bucket.
//	buckets[matchingBucketNum].add(pProbTable->clone());
	buckets[matchingBucketNum].add(pProbTable);
}

void MiniBucketElimination::outputBuckets(){
	for(int i=0; i<num_vars; i++){
		int bucketNum = order[i];
		buckets[bucketNum].outputBucket();
	}
}

double MiniBucketElimination::process(){
//=== Processing all buckets according to the order.
	int i,j;
	double result;
	ProbabilityTable* pTable;
	maxTakenWeight = 0;
	inducedWidth = 0;
	for(i=numOfBuckets-1; i>=0; i--){
//		printf("Mini-Buckets::processing - bucket %d\n", i);
		int bucketNum = order[i];

		if( variables[bucketNum]->fakeEvidenceForMB ){
//			printf("fake ev %d=%d\n",bucketNum, variables[bucketNum]->value);
			if(i!=0) 	assert(buckets[bucketNum].bucketPTs.size() == 0);
			for(j=0; j<buckets[bucketNum].bucketPTs.size(); j++){
				//pTable = buckets[bucketNum].bucketPTs[j]->instantiated(false);
				pTable = buckets[bucketNum].bucketPTs[j];
				if( i==0 ){
					assert( pTable->numEntries == 1 );
					result = pTable->logCPT[0];
					delete pTable;
					return result;
				} else {
					createBucketPT(pTable);
				}

//				pTable = buckets[bucketNum].bucketPTs[j]->instantiate(bucketNum, mbVariables[bucketNum]->value);
			}
		} else {
			int numPartitions ;
			//=== Processing all MiniBuckets of Bucket bucketNum
			numPartitions = buckets[bucketNum].constructPartititons(iBound, weightBound);
			for(int j=0; j<numPartitions; j++){
				ProbabilityTable* probTable = buckets[bucketNum].processPartition(j);
/*				printf("prpa again\n");
				ProbabilityTable* p2 = buckets[bucketNum].processPartition(j);
				printf("prpa again done\n");
				probTable->outputVars();
				printf("\n");
				p2->outputVars();
				printf("\n\n");
*/
				maxTakenWeight = MAX(maxTakenWeight, probTable->numEntries);
				inducedWidth = MAX(inducedWidth, probTable->numPTVars);

//				printf("max1\n");
				pTable = probTable->maximized(bucketNum);
				delete probTable;
				if( i==0 ){
					assert( buckets[bucketNum].numPartitions == 1 );
					assert( pTable->numEntries == 1 );
					result = pTable->logCPT[0];
					delete pTable;
					return result;
				} else {
					createBucketPT(pTable);
				}
			}
		}
//		printf("size of bucket %d: %d\n", bucketNum, probTable->numEntries);
//		printf("Product for bucket %d:\n", bucketNum);
//		probTable->outputCPT();
//		printf("\n");
	}
	assert(false);
	return -1;
}

double MiniBucketElimination::solve(int ib, double weight, int* mbAssignment){
	if(numOfBuckets==0) return probTables[0]->logCPT[0]; // only potential, only value

	iBound = ib;
	weightBound = weight;
//==== Initialize the real computation.
	initRun(false);

//==== Do the real computation.
	double upperbound = process();
//	printf("Ran MB getting upper bound %g: %d variables of fake evidence, approximation with ind. width %d, max. size: %g\n", upperbound, numFakeEvidenceVars, inducedWidth, maxTakenWeight);

//==== Retrieve assignment.
	int i;
	for(i=0; i<num_vars; i++) mbAssignment[i] = -1;

	for(i=0; i<numOfBuckets; i++){
		int bucketNum = order[i];
		mbAssignment[bucketNum] = buckets[bucketNum].getBestAssignmentForBucket(mbAssignment);
	}
	return upperbound;
}

//==== Preprocessing step for RB-SLS. Do varelim until potentials grow
//==== larger than weightBoundToStopExecution
//==== Returns the optimal solution (log-prob) iff the problem could solved to completion
//==== with the allowed weightBound. Otherwise, it returns +1.
void MiniBucketElimination::preprocess(double weightBoundToStopExecution, int *outNumVars, int** leftVarIndices, Variable ***outVars, int *outNumPots, ProbabilityTable ***outPots){
	initRun(false);

	//=== Processing all buckets according to the order.
	int i,j,k;
	ProbabilityTable* pTable;
	maxTakenWeight = 0;
	inducedWidth = 0;
	for(i=numOfBuckets-1; i>=0; i--){
//		printf("Mini-buckets::preprocess - bucket %d\n", i);

		int bucketNum = order[i];
//		printf("Removing var %d (# %d/%d)\n",bucketNum, numOfBuckets-i,numOfBuckets);

		//=== Create one big partition (just for code reusage)
		int numPartitions = buckets[bucketNum].constructPartititons(BIG, DOUBLE_BIG);
		assert(numPartitions == 1);
//		printf("bucketNum = %d, numOfBuckets=%d\n", bucketNum, numOfBuckets);

		//=== If the table would grow too big, stop the preprocessing.
		if(buckets[bucketNum].bucketWeight > weightBoundToStopExecution)
			break;

//		printf("Resulting size: %lf\n",buckets[bucketNum].bucketWeight);
		ProbabilityTable* probTable = buckets[bucketNum].processPartition(0);

		maxTakenWeight = MAX(maxTakenWeight, probTable->numEntries);
		inducedWidth = MAX(inducedWidth, probTable->numPTVars);

		pTable = probTable->maximized(bucketNum);
		delete probTable;
		if( i==0 ){
			assert( buckets[bucketNum].numPartitions == 1 );
			assert( pTable->numPTVars == 0 );
			assert( pTable->numEntries == 1 );
			(*outNumVars)=0;
			//(*leftVarIndices) need not be specified.
			//(*outVars) need not be specified.
			(*outNumPots)=1;
			ProbabilityTable** pt = new ProbabilityTable*[1];
			pt[0] = pTable;
			(*outPots) = pt;
//			double optMPE = pTable->logCPT[0];
//			delete pTable;
//			printf("Removed all variables in the preprocessing - solved optimally.\nWidth & weight processed: (%d,%lf)\n", inducedWidth, maxTakenWeight);
			return;
		} else {
			createBucketPT(pTable);
		}
	}
	(*outNumVars)=i+1;
	int* varIndices = new int[i+1];
	Variable** vars = new Variable*[i+1];
	int numPots = 0;
	for(j=0; j<i+1; j++){
		varIndices[j] = order[j];
		vars[j] = variables[order[j]];
		numPots += buckets[order[j]].bucketPTs.size();
//		printf("weight of var %d: %lf\n", j, vars[j]->weightOfMB);
	}
	(*leftVarIndices) = varIndices;
	(*outVars) = vars;

	(*outNumPots) = numPots;
	ProbabilityTable** pt = new ProbabilityTable*[numPots];

	numPots=0;
	for(j=0; j<i+1; j++){
		for(k=0; k<buckets[order[j]].bucketPTs.size(); k++){
			pt[numPots++] = buckets[order[j]].bucketPTs[k];
		}
	}
	(*outPots) = pt;
	if(verbose) printf("Removed %d/%d variables in the preprocessing\n%d vars and %d factors remaining.\n\nWidth & weight processed: (%d,%lf)\n\n", (numOfBuckets-1)-i, numOfBuckets, i+1, numPots, inducedWidth, maxTakenWeight);
}

}  // sls4mpe

#endif
