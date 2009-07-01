/*
 * SigHandler.h
 *
 *  Created on: Dec 10, 2008
 *      Author: lars
 */

#ifndef SIGHANDLER_H_
#define SIGHANDLER_H_


//#include "boost/thread.hpp"
#include <csignal>
#include "_base.h"

#ifdef PARALLEL_MODE

// watches for signals like SIGINT and SIGTERM, catches them
// and takes care of processing
class SigHandler {
protected:

  static volatile sig_atomic_t flag;

  // Branch and bound engine
  boost::thread* m_bab;
  // Propagation engine
  boost::thread* m_prop;
  // Condor submission engine
  boost::thread* m_condSub;


public:
  // thread operator
  void operator() ();

  // a static signal handler
  static void handle(int sig);

public:
  SigHandler(boost::thread* bab, boost::thread* prop, boost::thread* condSub) :
    m_bab(bab), m_prop(prop), m_condSub(condSub) {}

};

#endif /* PARALLEL_MODE */

#endif /* SIGHANDLER_H_ */
