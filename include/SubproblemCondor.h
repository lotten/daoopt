/*
 * SubproblemCondor.h
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
 *  Created on: Nov 15, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef SUBPROBLEMCONDOR_H_
#define SUBPROBLEMCONDOR_H_

#include "SubproblemHandler.h"

#include <sstream>

//#include "pstream.h"

#ifdef PARALLEL_DYNAMIC

namespace daoopt {

/* a container for information exchange between SubproblemCondor and
 * CondorSubmissionEngine (which in itself wraps around the
 * Subproblem container)  */
struct CondorSubmission {
public:
  size_t threadID;
  size_t batch;
  size_t process;
  Subproblem* subproblem;
public:
  CondorSubmission(Subproblem*, size_t);
};


class SubproblemCondor : public SubproblemHandler {
protected:
  Subproblem*   m_subproblem;
  size_t        m_threadId;
public:
  void operator() ();
protected:
  void removeJob() const;
public:
  SubproblemCondor(SearchSpaceMaster* p, Subproblem* n, size_t threadid);
};


class CondorWaitInstance {
protected:
  size_t m_batch;
  size_t m_proc;
public:
  void operator() ();
  CondorWaitInstance(size_t b, size_t p) :
    m_batch(b), m_proc(p) {}
};


class CondorSubmissionEngine {
protected:
  size_t m_curBatch;
  size_t m_nextProcess;
  SearchSpaceMaster* m_spaceMaster;
protected:
  std::string encodeJob(CondorSubmission*);
  void submitToCondor(const ostringstream& jobstr) const;
public:
  static void mysleep(size_t sec) ;
  void operator() ();
public:
  CondorSubmissionEngine(SearchSpaceMaster* p);
};


/* Inline definitions */

inline CondorSubmission::CondorSubmission(Subproblem* n, size_t id)
  : threadID(id), subproblem(n) {}


inline SubproblemCondor::SubproblemCondor(SearchSpaceMaster* p, Subproblem* n, size_t id)
  : SubproblemHandler(p,n->root), m_subproblem(n), m_threadId(id) {}


/* puts the submission engine to sleep for s seconds */
inline void CondorSubmissionEngine::mysleep(size_t s) {
  boost::xtime xt;
  boost::xtime_get(&xt, boost::TIME_UTC_);
  xt.sec += s;
  boost::thread::sleep(xt);
}

inline CondorSubmissionEngine::CondorSubmissionEngine(SearchSpaceMaster* p)
  : m_curBatch(0), m_nextProcess(0), m_spaceMaster(p) {}

}  // namespace daoopt

#endif /* PARALLEL_DYNAMIC */

#endif /* SUBPROBLEMCONDOR_H_ */
