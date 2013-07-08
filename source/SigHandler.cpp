/*
 * SigHandler.cpp
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

#include "SigHandler.h"

#ifdef PARALLEL_DYNAMIC

namespace daoopt {

volatile sig_atomic_t SigHandler::flag = 1;

void SigHandler::handle(int sig) {
  if (sig == SIGINT || sig == SIGTERM || sig == SIGUSR1)
    SigHandler::flag = 0;
  // re-register signal handler
  signal(sig,SigHandler::handle);
}

void SigHandler::operator ()() {

  signal(SIGINT,SigHandler::handle);
  signal(SIGTERM,SigHandler::handle);
  signal(SIGUSR1,SigHandler::handle);

  sigset_t emptymask;
  sigemptyset( & emptymask ); // block no signals

  while (SigHandler::flag) {
    sigsuspend( &emptymask );
  }
  {
    GETLOCK(mtx_io,lk);
    printf("\n======== Terminating ========\n");
  }


  // interrupt threads
  if (m_prop) {
    m_prop->interrupt();
    m_prop->join();
  }
  if (m_bab) {
    m_bab->interrupt();
    m_bab->join();
  }
  if (m_condSub) {
    m_condSub->interrupt();
    m_condSub->join();
  }

}

}  // namespace daoopt

#endif /* PARALLEL_DYNAMIC */
