#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/timer.h"

namespace sls4mpe {

/********************************************************
           IMPORTANT: Windows returns wall clock time,
					            UNIX    returns CPU time.
 ********************************************************/

//=== When timer was initialized or elapsed seconds was called the last time.
double lastTime;

#ifndef NT
	//! Data structure for retrieving net computation time information via library calls from the operating system. 
	static struct rusage res;
#endif

void start_timer(){
	#ifdef NT
		lastTime = clock();
	#else
    getrusage( RUSAGE_SELF, &res );
    lastTime = (double) res.ru_utime.tv_sec +
		   (double) res.ru_stime.tv_sec +
		   (double) res.ru_utime.tv_usec / 1000000.0 +
		   (double) res.ru_stime.tv_usec / 1000000.0;
	#endif
}

double elapsed_seconds(){
	double result = -1, thisTime = -1;
	#ifdef NT
		thisTime = clock();
		result = (thisTime - lastTime) / CLOCKS_PER_SEC;;
	#else
    getrusage( RUSAGE_SELF, &res );
    thisTime =
			(double) res.ru_utime.tv_sec +
      (double) res.ru_stime.tv_sec +
      (double) res.ru_utime.tv_usec / 1000000.0 +
      (double) res.ru_stime.tv_usec / 1000000.0;
		result = thisTime - lastTime;
	#endif

	lastTime = thisTime;
	return result;
}

}  // sls4mpe

#endif
