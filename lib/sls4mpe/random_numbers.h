#ifndef _SLS4MPE_RANDOM_NUMBERS_H_
#define _SLS4MPE_RANDIN_NUMBERS_H

#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define MASK 123459876

namespace sls4mpe {

extern long seed;

double ran01 ( long *idum );

long int random_number ( long *idum );

long int random_lh( long *idum, long int low, long int high );

}  // sls4mpe

#endif
