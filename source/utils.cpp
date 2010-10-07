/*
 * utils.cpp
 *
 *  Created on: Oct 10, 2008
 *      Author: lars
 */

#include "utils.h"

extern time_t time_start;

void myprint(std::string s) {
  time_t now; time(&now);
  double T = difftime(now,time_start);
  {
    GETLOCK(mtx_io, lk);
    std::cout << '[' << T << "] " << s << std::flush;
  }
}

void myerror(std::string s) {
  time_t now; time(&now);
  double T = difftime(now,time_start);
  {
    GETLOCK(mtx_io, lk);
    std::cerr << '[' << T << "] " << s << std::flush;
  }
}

#ifdef PARALLEL_MODE
 #ifdef USE_GMP
double mylog10(bigint a) {
  double l = 0;
  while (!a.fits_sint_p()) {
    a /= 10;
    ++l;
  }
  l += log10(a.get_si());
  return l;
}
 #endif
#endif

