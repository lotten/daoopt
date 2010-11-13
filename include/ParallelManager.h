/*
 * ParallelManager.h
 *
 *  Created on: Apr 11, 2010
 *      Author: lars
 */

#ifndef PARALLELMANAGER_H_
#define PARALLELMANAGER_H_

#include "_base.h"

#ifdef PARALLEL_MODE

#include "SearchMaster.h"
#include "BoundPropagatorMaster.h"
#include "SearchSpace.h"
#include "Subproblem.h"

//#include "inotify-cxx.h"

class ParallelManager {

protected:
  SearchMaster*          m_search;
  BoundPropagatorMaster* m_prop;
  SearchSpaceMaster*     m_space;

  set<Subproblem*>       m_activeSubproblems;

protected:
  /* */



public:
  /* Constructor */
  ParallelManager(SearchMaster* sm, BoundPropagatorMaster* bp, SearchSpaceMaster* ss);

  /* for threaded execution */
  void operator () ();

  void run();

};


/********************** Inline definitions ****************************/

inline ParallelManager::ParallelManager(SearchMaster* sm, BoundPropagatorMaster* bp, SearchSpaceMaster* ss) :
    m_search(sm), m_prop(bp), m_space(ss)
{
  /* empty */
}

#endif /* PARALLEL_MODE */

#endif /* PARALLELMANAGER_H_ */


