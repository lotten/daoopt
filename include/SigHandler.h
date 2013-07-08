/*
 * SigHandler.h
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
 *  Created on: Dec 10, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef SIGHANDLER_H_
#define SIGHANDLER_H_


//#include "boost/thread.hpp"
#include <csignal>
#include "_base.h"

namespace daoopt {

#ifdef PARALLEL_DYNAMIC

/* watches for signals like SIGINT and SIGTERM, catches them
 * and takes care of processing */
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

#endif /* PARALLEL_DYNAMIC */

}  // namespace daoopt

#endif /* SIGHANDLER_H_ */
