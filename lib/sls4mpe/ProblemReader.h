#ifndef _SLS4MPE_PROBLEMREADER_H_
#define _SLS4MPE_PROBLEMREADER_H_

#include "DEFINES.h"
#ifdef ENABLE_SLS

#include <cstdio>
#include "sls4mpe/global.h"
#include "sls4mpe/Variable.h"
#include "sls4mpe/ProbabilityTable.h"

namespace sls4mpe {

class ProblemReader{
public:
	void readNetwork();
	void outputNetAsBNT();
	void readUAI(char* filename);
	void readSimpleBN(FILE* bn_file);
	void readSATCNF(FILE* bn_file, char* line);
	void readFG(FILE* bn_file, char* line);
	void parse_parameters(int argc,char *argv[]);
private:
	void scanone(int argc, char *argv[], int i, int *varptr);
	void scandouble(int argc, char *argv[], int i, double *varptr);
	void scanlongint(int argc, char *argv[], int i, long int *varptr);
	void print_help();
};

}  // sls4mpe

#endif
#endif
