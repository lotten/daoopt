#ifndef _SLS4MPE_TIMER_H_
#define _SLS4MPE_TIMER_H_

#define WIN32 1
#ifdef WIN32
	#define NT 1 
#endif

#ifdef NT
	#include <time.h>
#else
	#include <sys/resource.h>
#endif

#ifndef CLK_TCK
	#define CLK_TCK 60
#endif

namespace sls4mpe {

void start_timer();
double elapsed_seconds();

}  // sls4mpe

#endif  // _TIMER_H_
