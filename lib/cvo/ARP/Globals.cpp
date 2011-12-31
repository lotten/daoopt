#include <stdlib.h>
#include <math.h>

#include "Globals.hxx"
#include "Function.hxx"
#include "Problem.hxx"

FILE *ARE::fpLOG = NULL ;
ARE::utils::RecursiveMutex ARE::LOGMutex ;

#define nPrimes 26
char primes[nPrimes] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101} ;

const double ARE::neg_infinity = log10(0.0) ;
double ARE::pos_infinity = 0.0 ;


int ARE::Initialize(void)
{
	pos_infinity = DBL_MAX ;
	pos_infinity *= 10.0 ;
	return 0 ;
}


char PrimeFactoringByTrialDivision(char N, char factors[128])
{
	if (N < 2) 
		return 0 ;
	char limit = sqrt((double) N) ;
	char n = N ;
	char m = 0 ;
	for (int i = 0 ; i < nPrimes ; ) {
		if (primes[i] > limit) 
			break ;
		int r = n % primes[i] ;
		if (0 != r) { ++i ; continue ; }
		factors[m] = primes[i] ;
		n /= factors[m] ;
		++m ;
		if (n <= 1) 
			break ;
		}
	if (n > 1) 
		factors[m++] = n ;
	// array 'factors' contain m prime factors of N
	return m ;
}


int ARE::LoadVarOrderFromFile(
	// IN
	const std::string & fnVO, 
	int N,
	// OUT
	int *VarList, 
	int & Width
	)
{
	Width = -1 ;

	int i ;

	if (NULL != ARE::fpLOG) 
		fprintf(ARE::fpLOG, "\nWill try to load variable ordering from file %s ...", fnVO.c_str()) ;
	FILE *fpVO = fopen(fnVO.c_str(), "rb") ;
	if (NULL == fpVO) {
		if (NULL != ARE::fpLOG) 
			fprintf(ARE::fpLOG, "\nFailed to open variable ordering file ...") ;
		return 1 ;
		}
	else {
		#define VO_FILE_BUF_SIZE 1048576
		char *buf = new char[VO_FILE_BUF_SIZE] ;
		if (NULL == buf) {
			if (NULL != ARE::fpLOG) 
				fprintf(ARE::fpLOG, "\nFailed to allocate buf for variable ordering file data ...") ;
			return 1 ;
			}
		int nRead = fread(buf, 1, VO_FILE_BUF_SIZE, fpVO) ;
		if (nRead >= VO_FILE_BUF_SIZE) {
			if (NULL != ARE::fpLOG) 
				fprintf(ARE::fpLOG, "\nBuf for variable ordering file data is small ...") ;
			delete [] buf ;
			return 1 ;
			}
		if (nRead < N) {
			if (NULL != ARE::fpLOG) 
				fprintf(ARE::fpLOG, "\nData loaded from variable ordering file data is small; nRead=%d ...", nRead) ;
			delete [] buf ;
			return 1 ;
			}
		// first line is comment
		for (i = 0 ; i < nRead ; i++) {
			if ('\n' == buf[i]) 
				break ;
			}
		if (i >= nRead) 
			{ if (NULL != ARE::fpLOG) fprintf(ARE::fpLOG, "\nVariable ordering file data is bad; cannot file end of first line; nRead=%d ...", nRead) ; delete [] buf ; return 1 ; }
		// load nVars
		char *buf_ = buf+i, *buf__ = NULL ;
		int L = nRead - i, l ;
		if (0 != fileload_getnexttoken(buf_, L, buf__, l)) 
			{ if (NULL != ARE::fpLOG) fprintf(ARE::fpLOG, "\nVariable ordering file data is bad; cannot get nVars; nRead=%d ...", nRead) ; delete [] buf ; return 1 ; }
		int nVars = atoi(buf__) ;
		if (nVars != N) 
			{ if (NULL != ARE::fpLOG) fprintf(ARE::fpLOG, "\nVariable ordering file data is bad; nVars != N; nRead=%d ...", nRead) ; delete [] buf ; return 1 ; }
		/*
		// load width
		if (0 != fileload_getnexttoken(buf_, L, buf__, l)) 
			{ if (NULL != ARE::fpLOG) fprintf(ARE::fpLOG, "\nVariable ordering file data is bad; cannot get w; nRead=%d ...", nRead) ; delete [] buf ; return 1 ; }
		Width = atoi(buf__) ;
		if (Width < 0 || Width >= N) 
				{ if (NULL != ARE::fpLOG) fprintf(ARE::fpLOG, "\nVariable ordering file data is bad; w is out of range; w=%d nRead=%d ...", (int) Width, nRead) ; delete [] buf ; Width = -1 ; return 1 ; }
		*/
		for (i = 0 ; i < N ; i++) {
			if (0 != fileload_getnexttoken(buf_, L, buf__, l)) 
				{ if (NULL != ARE::fpLOG) fprintf(ARE::fpLOG, "\nVariable ordering file data is bad; cannot get v; i=%d nRead=%d ...", i, nRead) ; delete [] buf ; return 1 ; }
			VarList[i] = atoi(buf__) ;
			if (VarList[i] < 0 || VarList[i] >= N) 
				{ if (NULL != ARE::fpLOG) fprintf(ARE::fpLOG, "\nVariable ordering file data is bad; v is out of range; v=%d nRead=%d ...", (int) VarList[i], nRead) ; delete [] buf ; return 1 ; }
			}
		delete [] buf ;
		}

	// the variable order is elimination order; we here need BE-order; reverse the list.
	for (i = 0 ; i < N >> 1 ; i++) {
		int u = VarList[i] ;
		int v = VarList[N-i-1] ;
		VarList[i] = v ;
		VarList[N-i-1] = u ;
		}

	return 0 ;
}

