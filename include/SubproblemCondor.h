/*
 * SubproblemCondor.h
 *
 *  Created on: Nov 15, 2008
 *      Author: lars
 */

#ifndef SUBPROBLEMCONDOR_H_
#define SUBPROBLEMCONDOR_H_

#include "SubproblemHandler.h"

#include <sstream>

//#include "pstream.h"

#ifdef PARALLEL_MODE

// a container for information exchange between SubproblemCondor and
// CondorSubmissionEngine
struct CondorSubmission {
public:
  size_t threadID;
  size_t batch;
  size_t process;
  SearchNode* problem;
  bigint estimate;
public:
  CondorSubmission(SearchNode*,size_t,bigint);
};


class SubproblemCondor : public SubproblemHandler {
protected:
  size_t m_threadId;      // the thread id
  bigint m_estimate;     // the complexity estimate
public:
  void operator() ();
protected:
  void removeJob() const;
public:
  SubproblemCondor(SearchSpace* p, SearchNode* n, size_t threadid, bigint sz);
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
  SearchSpace* m_space;
protected:
  std::string encodeJob(CondorSubmission*);
  void submitToCondor(const ostringstream& jobstr) const;
public:
  static void mysleep(size_t sec) ;
  void operator() ();
public:
  CondorSubmissionEngine(SearchSpace* p);
};


// Inline definitions

inline CondorSubmission::CondorSubmission(SearchNode* n, size_t id, bigint sz)
  : threadID(id), problem(n), estimate(sz) {}


inline SubproblemCondor::SubproblemCondor(SearchSpace* p, SearchNode* n, size_t id, bigint sz)
  : SubproblemHandler(p,n), m_threadId(id), m_estimate(sz) {}


// puts the submission engine to sleep for s seconds
inline void CondorSubmissionEngine::mysleep(size_t s) {
  boost::xtime xt;
  boost::xtime_get(&xt, boost::TIME_UTC);
  xt.sec += s;
  boost::thread::sleep(xt);
}

inline CondorSubmissionEngine::CondorSubmissionEngine(SearchSpace* p)
  : m_curBatch(0), m_nextProcess(0), m_space(p) {}


#endif /* USE_THREADHS */


#endif /* SUBPROBLEMCONDOR_H_ */
