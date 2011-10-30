/*
 * SigHandler.cpp
 *
 *  Created on: Dec 10, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "SigHandler.h"


#ifdef PARALLEL_DYNAMIC

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

#endif /* PARALLEL_DYNAMIC */
