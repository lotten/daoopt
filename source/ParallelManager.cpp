/*
 * ParallelManager.cpp
 *
 *  Created on: Apr 11, 2010
 *      Author: lars
 */

#ifdef PARALLEL_MODE

#include "ParallelManager.h"

void ParallelManager::operator ()() {
  try {
    this->run();
  } catch (boost::thread_interrupted ie) {
    // TODO ?
  }
}

void ParallelManager::run() {

  assert(m_search && m_prop);

  while (!m_search->isDone()) {
    m_prop->propagate(m_search->nextLeaf());
  }

}

#endif /* PARALLEL_MODE */

