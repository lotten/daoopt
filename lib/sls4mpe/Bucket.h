/*
 * SLS4MPE by Frank Hutter, http://www.cs.ubc.ca/labs/beta/Projects/SLS4MPE/
 * Some modifications by Lars Otten
 */
#ifndef _SLS4MPE_BUCKET_H_
#define _SLS4MPE_BUCKET_H_

#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/ProbabilityTable.h"
#include "sls4mpe/global.h"
#include <vector>

using namespace std;

namespace sls4mpe {

class Bucket {
public:
	int numPartitions; // numPartitions is the highest number k for partition
	int numSubPTs;
	int num;
	int numBucketVars;
	double bucketWeight;

	vector<int> partition; // partition[ptNum] = k <=> probability table ptNum is in partition k
	vector<int> subPTs; // subPTs[ptNum1]=ptNum2 <=> ptNum1 is a subPT of ptNum
	vector<int> bucketVars;
	vector<int> varsInPartition; // temporary
	vector<ProbabilityTable*> bucketPTs;

	Bucket();
	void defaultInitialization();
	void add(ProbabilityTable* pProbTable);
	void outputBucket();
	ProbabilityTable* processPartition(int partitionNumber);
	int constructPartititons(int ib, double weightBound);
	int getBestAssignmentForBucket(const int* mpeAssignmentSoFar);

	~Bucket();

};

}  // sls4mpe

#endif
#endif
