#ifndef _SLS4MPE_MINIBUCKETELIMINATION_H_
#define _SLS4MPE_MINIBUCKETELIMINATION_H_

#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/Bucket.h"
#include "sls4mpe/Variable.h"

namespace sls4mpe {

class MiniBucketElimination{
public:
	void initOnce();
	void initRun(bool newOrder);
	void createBucketPT(ProbabilityTable* pProbTable);
	void outputBuckets();
	double solve(int ib, double weight, int* mbAssignment);
	void preprocess(double weightBoundToStopExecution, int *outNumVars, int** leftVarIndices, Variable ***outVars, int *outNumPots, ProbabilityTable ***outPots);
	int inducedWidth;
	double maxTakenWeight;

	int iBound;
	double weightBound;

	int numOfBuckets;
	Bucket* buckets;
	int* order;
	int* optimalVars;
	Variable **mbVariables;

	void createOrder(int numFakeEvidenceVars, int* fakeEvidenceVars, int* outInducedWidth, double* outInducedWeight);
private:
	double process();
};

}  // sls4mpe

#endif
#endif
