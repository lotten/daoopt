/*
 * defs.h
 *
 *  Created on: Apr 9, 2009
 *      Author: lars
 */

#ifndef DEFS_H_
#define DEFS_H_

/******************************************************************
 * define NO_ASSIGNMENT to turn *off* MPE tuple computation, which
 * will speed up computation time
 */

//#define NO_ASSIGNMENT

/*****************************************************************/

/******************************************************************
 * define LIKELIHOOD to compute likelihood instead of MPE
 * (HIGHLY EXPERIMENTAL)
 */

#define LIKELIHOOD

/*****************************************************************/

/******************************************************************
 * define PARALLEL_MODE if multithreading should be used (for central
 * process), used only if NOTHREADS hasn't been set externally
 */

#ifndef PARALLEL_MODE
#define PARALLEL_MODE
#endif

/*****************************************************************/

/******************************************************************
 * alias for val_t, the data type for variable values: larger types
 * allow higher max. domain size but use more memory. Set to exactly
 * one type
 */

/* signed char = 8 bit, max. domain size is 127 */
typedef signed char val_t;

/* short = 16 bit, max. domain size is 32,767 */
//typedef short val_t;

/* int = 32 bit, max. domain size is 2,147,483,647 */
//typedef int val_t;

/*****************************************************************/


/* disable threading, tuple computation, and pruning for likelihood computation */
#ifdef LIKELIHOOD
  #define NO_ASSIGNMENT
  #define NO_HEURISTIC
  #ifndef NOTHREADS
    #define NOTHREADS
  #endif
#endif

/* disable parallelism if NOTHREADS is defined */
#ifdef NOTHREADS
  #undef PARALLEL_MODE
#endif


#endif /* DEFS_H_ */
