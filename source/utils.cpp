/*
 * utils.cpp
 *
 *  Created on: Oct 10, 2008
 *      Author: lars
 */

#include "utils.h"


#ifdef USE_THREADS
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

