/*
 * defs.h
 *
 *  Created on: Apr 9, 2009
 *      Author: lars
 */

#ifndef DEFS_H_
#define DEFS_H_

#ifndef NOTHREADS
// define if multithreading should be used (for central process)
// only if NOTHREADS hasn't been set externally
//#define USE_THREADS
#endif

// data type for variable values (determines max. domain size)
typedef signed char val_t;
//typedef short val_t;
//typedef int val_t;


#endif /* DEFS_H_ */
