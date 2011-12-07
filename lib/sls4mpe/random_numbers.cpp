#include "DEFINES.h"
#ifdef ENABLE_SLS

#include "sls4mpe/random_numbers.h"

namespace sls4mpe {

long int seed;

double ran01( long *idum )
/*    
      FUNCTION:       generate a random number uniformly distributed in [0,1]
      INPUT:          pointer to variable containing random number seed
      OUTPUT:         random number uniformly distributed in [0,1]
      (SIDE)EFFECTS:  random number seed is modified (important, has to be done!)
      ORIGIN:         numerical recipes in C
*/
{
  long k;
  double ans;

  k =(*idum)/IQ;
  *idum = IA * (*idum - k * IQ) - IR * k;
  if (*idum < 0 ) *idum += IM;
  ans = AM * (*idum);
  return ans;
}

long int random_number( long *idum )
/*    
      FUNCTION:       generate an integer random number
      INPUT:          pointer to variable containing random number seed
      OUTPUT:         integer random number uniformly distributed in {0,2147483647}
      (SIDE)EFFECTS:  random number seed is modified (important, has to be done!)
      ORIGIN:         numerical recipes in C
*/
{
  long k;

  k =(*idum)/IQ;
  *idum = IA * (*idum - k * IQ) - IR * k;
  if (*idum < 0 ) *idum += IM;
  return *idum;
}

long int random_lh( long *idum, long int low, long int high )
/*    
      FUNCTION:       generates an integer random number
      INPUT:          pointer to variable containing random number seed,
                      borders low and high
      OUTPUT:         integer random number uniformly distributed in [low,high]
      (SIDE)EFFECTS:  random number seed is modified (important, has to be done!)
      ORIGIN:         random number generator from numerical recipes in C
*/
{
  long k;

  k =(*idum)/IQ;
  *idum = IA * (*idum - k * IQ) - IR * k;
  if (*idum < 0 ) *idum += IM;
  
  k = *idum % (high - low + 1);
  
  return (k + low);
}

}  // sls4mpe

#endif
