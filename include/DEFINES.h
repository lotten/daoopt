/*
 * DEFINES.h
 *
 *  Copyright (C) 2008-2012 Lars Otten
 *  This file is part of DAOOPT.
 *
 *  DAOOPT is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DAOOPT is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DAOOPT.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  Created on: Apr 9, 2009
 *      Author: Lars Otten <lotten@ics.uci.edu>
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

//#define LIKELIHOOD

/*****************************************************************/

/******************************************************************
 * define PARALLEL_DYNAMIC if multithreading should be used (for central
 * process), used only if NOTHREADS hasn't been set externally
 * NOTE: these are usually set through the makefile
 */

#ifndef PARALLEL_DYNAMIC
//#define PARALLEL_DYNAMIC
#endif

#ifndef PARALLEL_STATIC
#define PARALLEL_STATIC
#endif

/*****************************************************************/

/*
 * define ANYTIME_DEPTH to enable "subproblem dive" with plain AOBB: performs
 * dive guided by mini bucket for every new (sub)subproblem, will ideally
 * yield better anytime performance.
 */

//#define ANYTIME_DEPTH

/*****************************************************************/


/*****************************************************************/

/*
 * undefine ENABLE_SLS to disable the SLS component during preprocessing.
 */
#define ENABLE_SLS

/*****************************************************************/

/*****************************************************************
 * define NO_HEURISTIC to disable Mini Buckets and thus pruning
 */

//#define NO_HEURISTIC

/*****************************************************************/

namespace daoopt {
/******************************************************************
 * alias for val_t, the data type for variable values: larger types
 * allow higher max. domain size but use more memory. Set to exactly
 * one type
 */

/* signed char = 8 bit, max. domain size is 127 */
//typedef signed char val_t;

/* short = 16 bit, max. domain size is 32,767 */
typedef short val_t;

/* int = 32 bit, max. domain size is 2,147,483,647 */
//typedef int val_t;

}  // namespace daoopt
/*****************************************************************/


/* disable threading, tuple computation, and pruning for likelihood computation */
#ifdef LIKELIHOOD
  #define NO_ASSIGNMENT
  #define NO_HEURISTIC
  #ifndef NOTHREADS
    #define NOTHREADS
  #endif
  #ifdef ANYTIME_DEPTH
    #undef ANYTIME_DEPTH
  #endif
#endif

/* disable parallelism if NOTHREADS is defined */
#ifdef NOTHREADS
  #undef PARALLEL_DYNAMIC
  #undef PARALLEL_STATIC
#endif

/* only allow one parallel mode */
#ifdef PARALLEL_DYNAMIC
#undef PARALLEL_STATIC
#endif

#endif /* DEFS_H_ */
