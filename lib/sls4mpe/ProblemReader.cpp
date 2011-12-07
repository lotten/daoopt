#include "DEFINES.h"
#ifdef ENABLE_SLS

#include <iostream>
#include "sls4mpe/ProblemReader.h"
#include "sls4mpe/global.h"
#include "sls4mpe/random_numbers.h"
#include "sls4mpe/AssignmentManager.h"

#include "gzstream.h"

namespace sls4mpe {

char bntFileAndFunctionName[1000];
int inputType = 0; // bn in .simple format

void ProblemReader::readNetwork(){

    // check for UAI input?
    if (strstr(network_filename,".uai")) {
      readUAI(network_filename);
    } else {

      FILE     *bn_file;
      char     line[LINE_LEN], nextc;
      bn_file = fopen(network_filename, "r");
      assert(bn_file != NULL);

//=== Skip the comments.
      do{
        fgets(line, LINE_LEN, bn_file);
        sscanf(line, "%c", &nextc );
      } while ( nextc == 'c' );

      switch(inputType){
      case 0:
        readSimpleBN(bn_file);
        break;
      case 1:
        readSATCNF(bn_file, line);
        break;
      case 2:
        readFG(bn_file, line);
        break;
      }
    }

    if(onlyConvertToBNT){
		outputNetAsBNT();
		exit(0);
	}
}

void ProblemReader::readSimpleBN(FILE* bn_file){
	int i,j,pot, var, value, var_num;
	char line[LINE_LEN];

//==== Read num_vars (which equals num_pots).
	fgets(line, LINE_LEN, bn_file);
	if( !sscanf(line, "%i", &num_vars )){
		fprintf(stderr, "Problem has to start with #variables.\n");
		fprintf(stderr, "WRONG INPUT FORMAT, ABORT !!\n");
		exit(-1);
	}
	num_pots = num_vars;

	allocateVarsAndPTs(true);

	char nextc;
//==== Read variables.
	for(var=0; var<num_vars; var++){
		fgets(line, LINE_LEN, bn_file);
		int num_parents, domSize;
		char varname[MAX_VARNAME_LENGTH];
		if( sscanf(line, "%c %d %d %d %s", &nextc, &var_num, &domSize, &num_parents, variables[var]->name ) < 5){
			fprintf(stderr, "Reading variable line failed:\n%s\nIt has to contain a char, three integers and a string.\n", line );
			fprintf(stderr, "WRONG INPUT FORMAT, ABORT !!\n");
			variables[var]->setName(varname);
			exit(-1);
		}
		variables[var]->setDomainSize(domSize);
		probTables[var]->init(num_parents+1); // here, we assume each var has a pot.
		assert( nextc == 'v' );	
		assert( var_num == var );
	}

//==== Read probability tables.
	for(pot=0; pot<num_pots; pot++){
		//==== Read variable.
		fgets(line, LINE_LEN, bn_file);
		int chars_read, tmp;
		sscanf(line, "%c %d %n", &nextc, &var_num, &chars_read);
//		printf("line = %s\n", line);
		assert( nextc == 'p' );

		//==== Read parents of variable.
		probTables[pot]->setVar(0,pot);
		probTables[pot]->ptVars[0] = pot;
		for(j=1; j<probTables[pot]->numPTVars; j++){
			int tmpVar;
			sscanf(line+chars_read, "%d %n", &tmpVar, &tmp);
			probTables[pot]->setVar(j, tmpVar);
			chars_read += tmp;
		}
		
		//==== Compute number of entries.
		int numEntries = 1;
		for(i=0; i<probTables[pot]->numPTVars; i++){
			numEntries *= variables[probTables[pot]->ptVars[i]]->domSize;
		}
		probTables[pot]->setNumEntries(numEntries);

	//==== Read probability table.
		chars_read = 0;
		for(j=0; j<numEntries; j++){
			double entry;
			fscanf(bn_file, "%lf %n", &entry, &tmp);
			probTables[pot]->setEntry(j,entry);

			chars_read += tmp;
		}
	}

//=== Read observed variables.
	fgets(line, LINE_LEN, bn_file); // obs
	while( fscanf(bn_file, "%d %d ", &var, &value)==2) {
		variables[var]->observed = true;
		variables[var]->value = value;
//		fprintf(outfile, "observed %d=%d\n", var, value);
	}

}

void ProblemReader::readSATCNF(FILE* bn_file, char* line){
	int var, pot;

//=== Read num_vars and num_clauses (probTables)
	if( !sscanf(line, "p cnf %i %i", &num_vars, &num_pots )){
		fprintf(stderr, "Problem has to start with 'p cnf <#vars> <#clauses>'.\n");
		fprintf(stderr, "WRONG INPUT FORMAT, ABORT !!\n");
		exit(-1);
	}

//=== Allocate space and register domainsizes 2.
	allocateVarsAndPTs(false);
	for(var=0; var < num_vars; var++){
		variables[var]->setDomainSize(2);
	}

//=== Process all clauses.
	for(pot=0; pot<num_pots; pot++){
		int chars_read=0, tmp, tmp2;
		const int MAX_VARS_IN_POT = 30;
		int vars[MAX_VARS_IN_POT], i;
		
	//=== Read the clause and remember the variables.
		fgets(line, LINE_LEN, bn_file);
//		printf("line = %s\n", line);
		for(i=0; sscanf(line+chars_read, "%d %n", &tmp, &tmp2) == 1; i++){
			chars_read+=tmp2;
			vars[i] = tmp;
//			printf("%d\n",vars[i]);
		}
//		printf("vars[i-1] = %d\n", vars[i-1]);
		assert(vars[i-1] == 0);
		int num_vars_in_pot = i-1;

	//=== Set the probTable's length and variables.
		probTables[pot]->init(num_vars_in_pot);
		for(i=0; i<num_vars_in_pot; i++){
			if(i>MAX_VARS_IN_POT){
				printf("Sorry, can't handle clauses with more than %d variables.\n", MAX_VARS_IN_POT);
				printf("The factor for each clause is exponential in its number of variables. EXITING");
				exit(-1);
			}
			probTables[pot]->setVar(i,abs(vars[i])-1); // I start counting vars at 0.
		}

	//=== Set the probTable's size and initialize its entries.
		probTables[pot]->setNumEntries( (int) floor(pow(2.0, num_vars_in_pot)));
		for(i=0; i<probTables[pot]->numEntries; i++) probTables[pot]->setEntry(i,1);

	//=== Figure out the unsatisfied index and set that one to 0.
		probTables[pot]->initFactorsOfVars(); // factors need to be set for the following.

		int onlyBadIndex = 0;
//		printf("potential %d:\n",pot);
		for(i=0; i<num_vars_in_pot; i++){
			// when variable is negated the clause needs it true to be false.
			if(vars[i] < 0) onlyBadIndex += probTables[pot]->factorOfVar[i]*1;
//			printf("  var %d : %d\n", i, probTables[pot]->factorOfVar[i]*1);
		}
		probTables[pot]->setEntry(onlyBadIndex,0);
	}
}


void ProblemReader::readUAI(char* filename) {

  igzstream in(filename);
  assert(in);

  // Problem type indicator
  string s;
  in >> s;
  if (! (s=="MARKOV" || s=="BAYES")) {
    fprintf(stderr, "Problem type not supported.\n");
    exit(-1);
  }

  // read num_vars
  in >> num_vars;

  // read domain sizes and store locally for now
  vector<int> doms(num_vars,-1);
  for (int i=0; i<num_vars; ++i)
    in >> doms[i];

  // read num_pots
  in >> num_pots;

  // now that num_vars and num_pots is read, we can allocate the memory
  allocateVarsAndPTs(false);

  // set domain sizes
  for (int i=0; i<num_vars; ++i)
    variables[i]->setDomainSize(doms[i]);

  // read and set function scopes
  vector<vector<int> > scopes(num_pots);
  vector<int> tabSizes(num_pots,-1);
  for (int i=0; i<num_pots; ++i) {
    int var, z, num_vars_in_pot;
    in >> num_vars_in_pot;
    z = num_vars_in_pot;
    while (z--) {
      in >> var;
      scopes[i].push_back(var);
    }
    // allocate and init function table
    probTables[i]->init(scopes[i].size());
    int numEntries = 1;
    for(int j=0; j<num_vars_in_pot; ++j){
            probTables[i]->setVar(j, scopes[i][j]);
            numEntries *= doms[scopes[i][j]]; //variables[scopes[i][j]]->domSize;
    }
    tabSizes[i] = numEntries;
    probTables[i]->setNumEntries(numEntries);
  }

  // read function tables
  double d;
  for (int i=0; i<num_pots; ++i) {
    int numEntries;
    in >> numEntries;
    assert(numEntries == tabSizes[i]);

    for (int j=0; j<numEntries; ++j) {
      in >> d;
      probTables[i]->setEntry(j,d);
    }
  }

}


void ProblemReader::readFG(FILE* bn_file, char* line){
	int var, pot, var_num;
	char nextc;

//=== Read num_vars and num_factors (probTables)
	if( !sscanf(line, "%i %i", &num_vars, &num_pots )){
		fprintf(stderr, "Problem has to start with '<#vars> <#factors>'.\n");
		fprintf(stderr, "WRONG INPUT FORMAT, ABORT !!\n");
		exit(-1);
	}
	allocateVarsAndPTs(false);

//==== Read variables.
	for(var=0; var<num_vars; var++){
		fgets(line, LINE_LEN, bn_file);
		int domSize;
		if( sscanf(line, "%c %d %d", &nextc, &var_num, &domSize) < 3){
			fprintf(stderr, "Reading variable line failed:\n%s\nIt has to contain a V, the running variable number and its domain size.\n", line );
			fprintf(stderr, "WRONG INPUT FORMAT, ABORT !!\n");
			exit(-1);
		}
		assert( nextc == 'V' );	
		assert( var_num == var );
		variables[var]->setDomainSize(domSize);
	}

//=== Read factors.
	for(pot=0; pot<num_pots; pot++){
		int chars_read=0, tmp, tmp2;
		const int MAX_VARS_IN_POT = 30;
		int vars[MAX_VARS_IN_POT], i;
		
	//=== Read the factor's variables.
		fgets(line, LINE_LEN, bn_file);
//		printf("line = %s\n", line);
		assert(sscanf(line+chars_read, "%c %n", &nextc, &tmp2)==1);
		assert(nextc == 'F');
		chars_read+=tmp2;
		for(i=0; sscanf(line+chars_read, "%d %n", &tmp, &tmp2) == 1; i++){
			chars_read+=tmp2;
			vars[i] = tmp;
		}
		int num_vars_in_pot = i;
		probTables[pot]->init(num_vars_in_pot);

	//=== Set the probTable's size and initialize its entries.
		int numEntries = 1;
		for(i=0; i<num_vars_in_pot; i++){
			probTables[pot]->setVar(i, vars[i]);
			numEntries *= variables[vars[i]]->domSize;
		}
		probTables[pot]->setNumEntries(numEntries);

	//=== Set multiplicative factors for the following.
		probTables[pot]->initFactorsOfVars(); 

	//=== Read the factor's entries.
		int numEntry;
		double entry;
		chars_read = 0;

		//=== Read entry numEntry.
		for(numEntry=0; numEntry<numEntries; numEntry++){
			fgets(line, LINE_LEN, bn_file);
			chars_read = 0;
			for(i=0; sscanf(line+chars_read, "%d %n", &tmp, &tmp2) == 1; i++){
				chars_read+=tmp2;
				vars[i] = tmp; // this is now the VALUE of the var.
	//			printf("%d\n",vars[i]);
			}
//			printf("%d, %d\n", i, num_vars_in_pot);
			assert(i==num_vars_in_pot);
			assert(sscanf(line+chars_read, "%c %lf", &nextc, &entry) == 2);
			assert(nextc == ':');
//			printf("entry = %lf\n", entry);
	
		//=== Figure out the entry's index and set it.
			int index = 0;
			for(i=0; i<num_vars_in_pot; i++){
//				printf("factor %d\n", probTables[pot]->factorOfVar[i]);
				index += vars[i] * probTables[pot]->factorOfVar[i];
			}
//			printf("index %d\n", index);
			probTables[pot]->setEntry(index, pow(10.0, entry));
/*			for(i=0; i<num_vars_in_pot; i++){
				printf("%d ", vars[i]);
			}
			printf(" -> index %d, entry %d\n", index, entry);*/
		}
	}

/*
	//=== Read the best solution BP comes up with.
	fgets(line, LINE_LEN, bn_file);
	assert(sscanf(line, "%c", &nextc)==1);
	assert(nextc == 'E');
	for(var=0; var<num_vars; var++){
		fgets(line, LINE_LEN, bn_file);
		assert(sscanf(line, "%d", &externalInitValues[var])==1);
		externalInitValues[var]--;
	}
*/
}

void ProblemReader::outputNetAsBNT(){
	printf("function bnet = %s()\n", bntFileAndFunctionName);
	printf("%% Converted from Bayes net repository by Frank Hutter <hutter@cs.ubc.ca> on Nov 29, 2004\n\n");
	int var,pot,j;
	printf("names = cell(1,%d);\n", num_vars);
	printf("sizes = zeros(1,%d);\n", num_vars);
	for(var=0; var<num_vars; var++){
		printf("names{%d} = '%s'; ", var+1, variables[var]->name);
		printf("sizes(%d) = %d;\n", var+1, variables[var]->domSize);
	}
	printf("\n");

	printf("dag = zeros(%d);\n", num_vars);
	for(pot=0; pot<num_pots; pot++){
		for(j=1; j<probTables[pot]->numPTVars; j++){
			printf("dag(%d, %d) = 1;\n", probTables[pot]->ptVars[j]+1, probTables[pot]->ptVars[0]+1);
		}
	}
	printf("\n");

	printf("bnet = mk_bnet(dag, sizes, 'names', [names]);\n");

	for(var=0;var<num_vars;var++){
		printf("bnet.CPD{%d} = tabular_CPD(bnet, %d, [", var+1, var+1);
		for(j=0; j<probTables[var]->numEntries; j++){
			printf("%lf ", pow(10.0, probTables[var]->logCPT[j]));
		}
		printf("]);\n");
	}

//	for(var=0; var<num_vars; var++){
//		if(variables[var]->observed) printf("");
	exit(1);
}


/******************************************************************************** 
  DEAL WITH INPUT PARAMETERS
*********************************************************************************/

void ProblemReader::print_help(){
//	cerr	<< "Call:" << endl
//				<< "\t sls\t [-h|--help]" << endl
//				<< "\t\t [-i|--input]" << endl;
}


void ProblemReader::scanone(int argc, char *argv[], int i, int *varptr){
  if (i>=argc || sscanf(argv[i],"%d",varptr)!=1){
		fprintf( stderr, "Bad argument %s\n", i<argc ? argv[i] : argv[argc-1]);
		exit(-1);
  }
}

void ProblemReader::scanlongint(int argc, char *argv[], int i, long int *varptr){
  if (i>=argc || sscanf(argv[i],"%ld",varptr)!=1){
		fprintf( stderr, "Bad argument %s\n", i<argc ? argv[i] : argv[argc-1]);
		exit(-1);
  }
}

void ProblemReader::scandouble(int argc, char *argv[], int i, double *varptr){
  if (i>=argc || sscanf(argv[i],"%lf",varptr)!=1){
		fprintf( stderr, "Bad argument %s\n", i<argc ? argv[i] : argv[argc-1]);
		exit(-1);
  }
}

void ProblemReader::parse_parameters(int argc,char *argv[]){
  int i,tmp;
  assignmentManager.M_MPE = false;
  assignmentManager.optimalLogMPEValue = DOUBLE_BIG;

  network_filename[0] = '\0';
	sls_filename[0] = '\0';
  for (i=1;i < argc;i++){
		if (strcmp(argv[i],"-i") == 0 || strcmp(argv[i],"--input") == 0){
			i++;
			strncpy( network_filename, argv[i], strlen( argv[i] ) );
		} 
		else if (strcmp(argv[i],"-o") == 0 || strcmp(argv[i],"--output") == 0){
			i++;
			strncpy( sls_filename, argv[i], strlen( argv[i] ) );
		}
		else if (strcmp(argv[i],"--inputType") == 0)
		  scanone(argc,argv,++i,&inputType);	
		else if (strcmp(argv[i],"-x") == 0 || strcmp(argv[i],"--maxRuns") == 0)
		  scanone(argc,argv,++i,&maxRuns);	
	    else if (strcmp(argv[i],"-s") == 0 || strcmp(argv[i],"--seed")==0)
		  scanlongint(argc,argv,++i,&seed);
		else if (strcmp(argv[i],"-t") == 0 || strcmp(argv[i],"--maxTime") == 0)	
		  scandouble(argc,argv,++i,&maxTime);
		else if (strcmp(argv[i],"-z") == 0 || strcmp(argv[i],"--maxSteps") == 0)	
		  scanlongint(argc,argv,++i,&maxSteps);
		else if (strcmp(argv[i],"-it") == 0 || strcmp(argv[i],"--maxIterations") == 0)	
		  scanone(argc,argv,++i,&maxIterations);
		else if (strcmp(argv[i],"-a") == 0 || strcmp(argv[i],"--algo") == 0)	
		  scanone(argc,argv,++i,&algo);
		else if (strcmp(argv[i],"-c") == 0 || strcmp(argv[i],"--caching") == 0)
			scanone(argc,argv,++i,&caching);
		else if (strcmp(argv[i],"-b") == 0 || strcmp(argv[i],"--init") == 0)
		  scanone(argc,argv,++i,&init_algo);
		else if (strcmp(argv[i],"-pert") == 0)
		  scanone(argc,argv,++i,&pertubationType);
		else if (strcmp(argv[i],"-tmult") == 0)
		  scandouble(argc,argv,++i,&tmult);
		else if (strcmp(argv[i],"-tdiv") == 0)
		  scandouble(argc,argv,++i,&tdiv);
		else if (strcmp(argv[i],"-tmin") == 0)
		  scandouble(argc,argv,++i,&tmin);
		else if (strcmp(argv[i],"-tbase") == 0)
		  scandouble(argc,argv,++i,&tbase);
		else if (strcmp(argv[i],"-T") == 0)
		  scandouble(argc,argv,++i,&T);
		else if (strcmp(argv[i],"-mmpe") == 0){
			assignmentManager.M_MPE = true;
			scanone(argc,argv,++i,&assignmentManager.M);
		}

		else if (strcmp(argv[i],"-onlyConvertToBNT") == 0){
			onlyConvertToBNT = true;		
			sscanf(argv[++i],"%s", bntFileAndFunctionName);
		}

		else if (strcmp(argv[i],"-nvns") == 0)
		  scanone(argc,argv,++i,&num_vns_pertubation_strength);
		else if (strcmp(argv[i],"-mbp") == 0){
		  scanone(argc,argv,++i,&mbPertubation );
		}
		else if (strcmp(argv[i],"-vns") == 0){
		  scanone(argc,argv,++i,&tmp);
			vns = (tmp!=0);
		}
		else if (strcmp(argv[i],"-rf") == 0)
		  scanone(argc,argv,++i,&restartNumFactor);
		else if (strcmp(argv[i],"-pfix") == 0){
		  scanone(argc,argv,++i,&tmp);
			pertubationFixVars = (tmp!=0);
		}
		else if (strcmp(argv[i],"-pspb") == 0)
		  scandouble(argc,argv,++i,&psp_base);
		else if (strcmp(argv[i],"-psab") == 0)
		  scandouble(argc,argv,++i,&psa_base);
		else if (strcmp(argv[i],"-p") == 0 || strcmp(argv[i],"--pertubationStrength") == 0)	{
		  scanone(argc,argv,++i,&pertubation_strength);
		}
		else if (strcmp(argv[i],"-tl") == 0)	
		  scanone(argc,argv,++i,&tl);
		else if (strcmp(argv[i],"-prel") == 0 || strcmp(argv[i],"--pertubationStrengthRelative") == 0){	
		  scanone(argc,argv,++i,&tmp);
			pertubation_rel = (tmp!=0);
		}

//=== Acceptance criterion.
		else if (strcmp(argv[i],"-acc") == 0 || strcmp(argv[i],"--acceptCrit") == 0)	
		  scanone(argc,argv,++i,&accCriterion);
//		else if (strcmp(argv[i],"-rint") == 0 || strcmp(argv[i],"--restartInterval") == 0)	
//		  scanone(argc,argv,++i,&restartInterval);
		else if (strcmp(argv[i],"-wint") == 0 || strcmp(argv[i],"--worseningInterval") == 0)	
		  scandouble(argc,argv,++i,&worseningInterval);
		else if (strcmp(argv[i],"-an") == 0 || strcmp(argv[i],"--acceptNoise") == 0)	
		  scandouble(argc,argv,++i,&accNoise);

		else if (strcmp(argv[i],"-pB") == 0 || strcmp(argv[i],"--prepBound") == 0)	
		  scandouble(argc,argv,++i,&preprocessingSizeBound);
		
		else if (strcmp(argv[i],"-mw") == 0 || strcmp(argv[i],"--maxMBWeight") == 0)	
		  scandouble(argc,argv,++i,&maxMBWeight);
		else if (strcmp(argv[i],"-glsInc") == 0)	
		  scandouble(argc,argv,++i,&glsPenaltyIncrement);
		else if (strcmp(argv[i],"-glsSmooth") == 0)	
		  scandouble(argc,argv,++i,&glsSmooth);
		else if (strcmp(argv[i],"-glsInterval") == 0)	
		  scanone(argc,argv,++i,&glsInterval);
		else if (strcmp(argv[i],"-glsPenMult") == 0)	
		  scandouble(argc,argv,++i,&glsPenaltyMultFactor);
		else if (strcmp(argv[i],"-glsReal") == 0)
		  scanone(argc,argv,++i,&glsReal);
		else if (strcmp(argv[i],"-glsAspiration") == 0)
		  scanone(argc,argv,++i,&glsAspiration);
		else if (strcmp(argv[i],"-noout") == 0)	
		  noout = 1;
		else if (strcmp(argv[i],"-res") == 0){
		  output_res = TRUE;
			i++;
			strncpy( res_filename, argv[i], strlen( argv[i] ) );
		}
		else if (strcmp(argv[i],"-opt") == 0 || strcmp(argv[i],"--optimalLogMPE") == 0)	
		  scandouble(argc,argv,++i,&assignmentManager.optimalLogMPEValue);
		else if (strcmp(argv[i],"-n") == 0 || strcmp(argv[i],"--noise") == 0)	
		  scanone(argc,argv,++i,&noise);
		else if (strcmp(argv[i],"-cf") == 0 || strcmp(argv[i],"--cutoff") == 0)	
		  scandouble(argc,argv,++i,&cutoff);
		else if (strcmp(argv[i],"-stdout") == 0)
		  output_to_stdout=1;
		else if (strcmp(argv[i],"-lm") == 0){
		  output_lm = TRUE;
			maxRuns = 20;
		}
		else if (strcmp(argv[i],"-stats") == 0)
			justStats = true;
		else if (strcmp(argv[i],"-traj") == 0){
		  output_trajectory = 1;
//			maxRuns = 1;
		}
		else if (strcmp(argv[i],"-runstats") == 0){
		  output_runstats = 1;
			maxIterations = 100;
		}
		else if (strcmp(argv[i],"-params") == 0){
		  print_tunable_parameters();
			exit(0);
		}
		else if (strcmp(argv[i],"--help")==0 || strcmp(argv[i],"-h")==0 ){
			print_help();
			exit(0);
		} else{
			if(i==argc-1 && strcmp(argv[i],"&") == 0) continue;
			fprintf(stderr, "=========================\nBad argument %s\n", argv[i]);	
		  fprintf(stderr, "USAGE:\n\n");
			print_help();
		  exit(-1);
		}
  }

	if(argc <= 1){
		fprintf(stderr, "USAGE:\n\n");
		print_help();
		exit(-1);
	}

  if(network_filename[0] == '\0') {
		fprintf(stderr, "You must specifiy a BN input file in .simple format using -i <network_filename>\n");
		exit(-1);
  } 
	int j,k;
	for(j=strlen( network_filename )-1; network_filename[j] != '.'; j--); // find last . in network_filename
	if(sls_filename[0] == '\0') {
		for(i=0; i<=j; i++){
			sls_filename[i] = network_filename[i];
		}
		k=j;
		sls_filename[++k] = 's';
		sls_filename[++k] = 'l';
		sls_filename[++k] = 's';
		sls_filename[++k] = '\0';
		outfile = stdout;
	} else {
		outfile = fopen(sls_filename, "w");
	}
		
	for(j=strlen( sls_filename )-1; sls_filename[j] != '.'; j--); // find last . in network_filename
	for(i=0; i<=j; i++){
		traj_it_filename[i]  = sls_filename[i];
		traj_fl_filename[i]  = sls_filename[i];
	}

	k=j;
	traj_it_filename[++k] = 't';
	traj_it_filename[++k] = 'r';
	traj_it_filename[++k] = 'a';
	traj_it_filename[++k] = 'j';
	traj_it_filename[++k] = 'i';
	traj_it_filename[++k] = 't';
	traj_it_filename[++k] = '\0';

	k=j;
	traj_fl_filename[++k] = 't';
	traj_fl_filename[++k] = 'r';
	traj_fl_filename[++k] = 'a';
	traj_fl_filename[++k] = 'j';
	traj_fl_filename[++k] = 'f';
	traj_fl_filename[++k] = 'l';
	traj_fl_filename[++k] = '\0';

	if( output_trajectory ) traj_it_file  = fopen(traj_it_filename, "w");
	if( output_trajectory ) traj_fl_file  = fopen(traj_fl_filename, "w");
	if( output_res ) resfile = fopen(res_filename, "w");

/*	if(algo == ALGO_GLS && (caching == CACHING_GOOD_VARS || caching == CACHING_SCORE)){
		printf("caching scheme %d not supported for GLS. Too complicated. Using scheme 2\n", caching);
		//exit(-1);
		caching = CACHING_INDICES;
	}*/
	if(algo == ALGO_TABU && caching == CACHING_GOOD_VARS){
		printf("Cannot use caching scheme CACHING_GOOD_VARS with tabu search.\n");
		printf("There, we also need to take worsening steps.\n");
		exit(-1);
	}
}

}  // sls4mpe

#endif
