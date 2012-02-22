/*
 * SearchMaster.cpp
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
 *  Created on: Feb 14, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#undef DEBUG

#include "SearchMaster.h"

#ifdef PARALLEL_DYNAMIC

bool SearchMaster::init() {

  myprint("Init: Beginning initialisation\n");

  bool solvedDuringInit = false;

  /*
   * this tries to find the initial parameters for the cutoff decision:
   * Explores the problem depth-first allowing only a small number of
   * nodes, and takes the largest solves subproblem (lowest depth) to
   * extract parameters
   */
  if (m_space->options->autoCutoff && m_space->options->nodes_init > 0) {
    count_t limit = m_space->options->nodes_init * 100000;
    solvedDuringInit = this->findInitialParams(limit);
    ostringstream ss;
    ss << "Init: explored " << limit << " nodes" << endl;// for cutoff " << cutoff << endl;
    myprint(ss.str());
  }

  if (solvedDuringInit) {
    m_spaceMaster->searchDone = true;
    ostringstream ss;
    ss << "Init: Problem solved during initialization" << endl;
    myprint(ss.str());
    return true; // problem solved
  }

  return false; // default

}


void SearchMaster::operator ()() {

  try {

  while ( !this->isDone() ) {

    { // allowedThreads says how many more subproblems can be processed in parallel
      GETLOCK( m_spaceMaster->mtx_allowedThreads , lk);
      while (!m_spaceMaster->allowedThreads) {
        CONDWAIT(m_spaceMaster->cond_allowedThreads,lk);
      }
      --m_spaceMaster->allowedThreads;
    }
    // new processing thread can now be started
    {
      GETLOCK(m_spaceMaster->mtx_space, lk); // lock the search space

      SearchNode* node = NULL;
      while (true) {
        /*
         * a single loop iteration will take the next node (top of stack, e.g.)
         * and process it. Loop breaks when leaf node is found (due to pruning or
         * parallelization)
         */

        node = this->nextNode();

        if (doProcess(node)) // * initial processing
          break; // node is dead end

        if (doCaching(node)) // * caching
          break; // cached solution found

        if (doPruning(node)) // * pruning
          break; // node was pruned

        if (doSolveLocal(node)) // solve locally?
          break; // node solved locally

        if (doParallelize(node)) // cutoff decision (assigns m_nextSubprob)
          break; // cutoff made

        if (doExpand(node)) // finally, generate child nodes
          break; // no children generated -> leaf node -> propagate
      }

      if ( node->isLeaf() ) { // leaf node, straight to propagation queue
        GETLOCK(m_spaceMaster->mtx_solved, lk2);
        m_spaceMaster->leaves.push(node);
        m_spaceMaster->cond_solved.notify_one();
      } else if ( node->isExtern() ) {

        // Create new process that 'outsources' subproblem solving, collects the
        // results and feeds it back into the search space
        SubproblemCondor subprob(m_spaceMaster, m_nextSubprob, m_nextThreadId);
        {
          GETLOCK(m_spaceMaster->mtx_activeThreads, lk2);
          m_spaceMaster->activeThreads.insert(
              make_pair( m_nextSubprob , new boost::thread(subprob) )  );
        }
        m_nextSubprob = NULL;
        m_nextThreadId += 1;
      }

      // mark search as done if needed (before releasing the search space)
      if ( this->isDone() ) {
        GETLOCK(m_spaceMaster->mtx_searchDone, lk2);
        m_spaceMaster->searchDone = true;
      }

      // only do a limited number of subprob's?
      if (m_spaceMaster->options->maxSubprob != NONE &&
          ((count_t) m_spaceMaster->options->maxSubprob) <= m_nextThreadId) {
        GETLOCK(m_spaceMaster->mtx_searchDone, lk2);
        m_spaceMaster->searchDone = true;
        break; // break the while loop
      }

    } // mtx_space is released
  } // end of while loop

  myprint("\t!!! Search done !!!\n");

  } catch (boost::thread_interrupted i) {
    myprint("\t!!! Search aborted !!!\n");
  }

}


bool SearchMaster::doSolveLocal(SearchNode* node) {

  assert(node);

  if (node->getType() == NODE_AND)
      return false; // ignore AND nodes

  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int width = ptnode->getSubCondWidth();
  int height = ptnode->getSubHeight();

  if (width <= m_space->options->cutoff_width) {
    ostringstream ss;
    ss << "Solving subproblem with root " << var << " locally, h=" << height << " w=" << width << endl;
    myprint(ss.str());
    this->solveLocal(node); // solves subproblem and stores value
    return true;
  }

  return false; // default false

}


bool SearchMaster::doParallelize(SearchNode* node) {

  assert(node);

  ostringstream ss;
  ss << "Considering " << *node;

  if (node->getType() == NODE_AND)
    return false; // don't parallelize on AND nodes

  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int depth = ptnode->getDepth();
  int height = ptnode->getSubHeight();
  int width = ptnode->getSubCondWidth();

  ss << " d=" << depth;
  ss << " w=" << width;

  bigint hwb = ptnode->computeHWB(&m_assignment);
  bigint twb = ptnode->getSubCondBound() ;

  double lb = lowerBound(node);
  double ub = node->getHeur();

  double estimate = NONE;
  double thresh = m_space->options->cutoff_size * 100000;
  double logEst = NONE;
  double logThresh = log10(thresh);

  bool cutoff = false; // solve parallel? (default no)
  bool local = false; // too easy -> solve locally (default no)

  if (m_space->options->autoCutoff) { // && m_nextThreadId >= (size_t) m_space->options->threads) {

    GETLOCK(m_spaceMaster->mtx_stats,lk);
    AvgStatistics* stats = m_spaceMaster->avgStats;

    // compute estimate
    double avgInc = m_spaceMaster->avgStats->getAvgInc();
    double avgBra = m_spaceMaster->avgStats->getAvgBra();

#ifdef USE_LOG
    double estDepth = pow(ub-lb,stats->getAlpha()) / avgInc;
#else
    assert(false); // TODO
#endif

    estDepth *= pow(height,stats->getBeta());
    estDepth += pow(height - stats->getAvgHei(), stats->getGamma());
    estDepth = max(0.0,estDepth);

    ss << " l=" << lb << " u=" << ub << " bra=" << avgBra << " inc=" << avgInc << " h=" << height
       <<  " D: " << estDepth;

    logEst = log10(avgBra)*estDepth;
    estimate = pow(avgBra, estDepth);
    //  double estimate = m_spaceMaster->stats->normalize(rawEst);

    ss << " est: " << ((count_t) estimate) << endl; // TODO handle overflow
    myprint(ss.str());

//    cutoff = (estimate <= m_space->options->cutoff_size * ((double) 100000));
    cutoff = logEst <= logThresh;
    local = logEst <= log10(m_space->options->local_size * 100000);

  } else {
    estimate = 0;
    cutoff = (depth == m_space->options->cutoff_depth);
  }

  /*
  bool cutoff = ( (m_nextThreadId < (size_t) m_space->options->threads)
                 || (!m_space->options->autoCutoff) )
        && (depth == m_space->options->cutoff_depth);
  cutoff = cutoff || (estimate != 0
        && estimate <= m_space->options->cutoff_size * ((double) 100000) ) ;
  */

  // TODO cutoff decision
  //if (ptnode->getDepth() == m_space->options->cutoff_depth) {
//  if (depth == m_space->options->cutoff_depth ||
//      m_hwbCache <= m_space->options->cutoff_size * bigint(100000) )
    //         ptnode->getSubsize()  <= m_space->options->cutoff_size) {
    //    if (ptnode->getSubwidth() == m_space->options->cutoff_depth) {

  if (local) {
    ostringstream ss;
    ss << "Solving subproblem with root " << var << " locally, h=" << height << " w=" << width << endl;
    myprint(ss.str());
    this->solveLocal(node);
    return true;
  } else if (cutoff) {

    stringstream ss;
    ss << "** Parallelising at depth " << depth << "\n" ;
    myprint(ss.str());


    double avgInc = avgIncrement(node);

    double estInc = estimateIncrement(node);

    count_t estN = estimate;

    addSubprobContext(node,ptnode->getFullContextVec()); // set subproblem context to node
//        addPSTlist(node); // to allow pruning in the worker
    node->setExtern();
    node->clearHeurCache(); // clear precomputed heuristic values for AND children

    m_nextSubprob = new Subproblem(node, width, depth, height, m_nextThreadId,
                                lb,ub,avgInc,estInc,estN,hwb,twb);
    return true;
  }

  return false; // default false, don't parallelize

}

#if true
double SearchMaster::avgIncrement(SearchNode* node) const {

  assert(node->getType() == NODE_OR); // only OR nodes

  SearchNode* prevAND = node->getParent(); // AND node
  node = prevAND->getParent(); // OR node

  int c = 0;
  double sum = ELEM_ONE;

  while (node->getParent()) {

    sum OP_TIMESEQ prevAND->getLabel();
    c += 1;

    prevAND = node->getParent(); // next AND
    node = prevAND->getParent(); // next OR
  }

  if (!c) return ELEM_ONE;

#ifdef USE_LOG
  return sum/c;
#else
  return pow(sum, 1.0/c); // c-th root of sum
#endif

}
#endif // false

#if true
double SearchMaster::estimateIncrement(SearchNode* node) const {

  assert(node);
  PseudotreeNode* ptnode = m_pseudotree->getNode(node->getVar());

#ifdef DEBUG
  cout << "Estimating increment for subproblem at " << *node << endl;
#endif

  double est = ELEM_ONE;

  stack<PseudotreeNode*> stck; stck.push(ptnode);
  const vector<int>& ctxt = ptnode->getFullContextVec();

  while (stck.size()) {
    PseudotreeNode* ptn = stck.top();
    stck.pop();
    for ( vector<Function*>::const_iterator it=ptn->getFunctions().begin(); it!=ptn->getFunctions().end(); ++it) {
#ifdef USE_LOG
      double e2 = (*it)->getAverage(ctxt,m_assignment);
//      DIAG (cout << *(*it) << ": " << SCALE_LOG(e2) << " (" << SCALE_NORM(e2) << ")" << endl );
      est OP_TIMESEQ e2;
//      est OP_TIMESEQ ((*it)->getAverage(ctxt,m_assignment) / ptn->getDepth());
#else
      est OP_TIMESEQ pow( (*it)->getAverage(ctxt,m_assignment), 1.0/ptn->getDepth() );
#endif
    }
    for ( vector<PseudotreeNode*>::const_iterator it=ptn->getChildren().begin() ; it!=ptn->getChildren().end(); ++it)
      stck.push(*it);
  }

#ifdef USE_LOG
//  est /= ptnode->getHeight();
#else
//  est = pow(est, 1.0/ptnode->getHeight() );
#endif

//  DIAG (cout << SCALE_LOG(est) << " (" << SCALE_NORM(est) << ")" << endl;);
  est /= ptnode->getSubHeight()+1;
//  DIAG (cout << SCALE_LOG(est) << " (" << SCALE_NORM(est) << ")" << endl;);
  return est;

}
#endif // false


#endif /* PARALLEL_DYNAMIC */
