/*
 * SearchMaster.cpp
 *
 *  Created on: Feb 14, 2010
 *      Author: lars
 */

#include "SearchMaster.h"

#ifdef PARALLEL_MODE

#undef DEBUG

void SearchMaster::operator ()() {

  bool solvedDuringInit = false;

  /*
   * this tries to find the initial parameters for the cutoff desicion:
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

  try { while ( this->hasMoreNodes() ) {

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
      if (! this->hasMoreNodes() ) {
        GETLOCK(m_spaceMaster->mtx_searchDone, lk2);
        m_spaceMaster->searchDone = true;
      }

      if (m_spaceMaster->options->maxSubprob != NONE &&
          ((count_t) m_spaceMaster->options->maxSubprob) <= m_nextThreadId) {
        GETLOCK(m_spaceMaster->mtx_searchDone, lk2);
        m_spaceMaster->searchDone = true;
        break; // break the while loop
      }

    } // mtx_space is released
  } // end of while loop

  myprint("\n!Search done!\n\n");

  } catch (boost::thread_interrupted i) {
    myprint("\n!Search aborted!\n");
  }

}


bool SearchMaster::doSolveLocal(SearchNode* node) {

  assert(node);

  if (node->getType() == NODE_AND)
      return false; // ignore AND nodes

  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int width = ptnode->getSubwidth();

  if (width <= m_space->options->cutoff_width) {
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
  int height = ptnode->getHeight();
  int width = ptnode->getSubwidth();

  ss << " d=" << depth;
  ss << " w=" << width;

  bigint hwb = ptnode->computeHWB(&m_assignment);
  bigint twb = ptnode->getSubsize() ;

  double lb = lowerBound(node);
  double ub = node->getHeur();

  double estimate = NONE;
  bool cutoff = false; // default no parallelization

  if (m_space->options->autoCutoff) { // && m_nextThreadId >= (size_t) m_space->options->threads) {

    GETLOCK(m_spaceMaster->mtx_stats,lk);
    Statistics* stats = m_spaceMaster->stats;

    // compute estimate
    double avgInc = m_spaceMaster->stats->getAvgInc();
    double avgBra = m_spaceMaster->stats->getAvgBra();

#ifdef USE_LOG
    double estDepth = pow(ub-lb,stats->getAlpha()) / avgInc;
#else
    assert(false); // TODO
#endif

    estDepth *= pow(height,stats->getBeta());
    estDepth += pow(height - stats->getAvgHei(), stats->getGamma());

    ss << " l=" << lb << " u=" << ub << " bra=" << avgBra << " inc=" << avgInc << " h=" << height
       <<  " D: " << estDepth;

    estimate = pow(avgBra, estDepth);
    //  double estimate = m_spaceMaster->stats->normalize(rawEst);

    ss << " est: " << ((count_t) estimate) << endl;
    myprint(ss.str());

    cutoff = (estimate <= m_space->options->cutoff_size * ((double) 100000));

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
  if (cutoff)
  {

    stringstream ss;
    ss << "** Parallelising at depth " << depth << "\n" ;
    myprint(ss.str());


    double avgInc = avgIncrement(node);

    double estInc = estimateIncrement(node);

    count_t estN = estimate;

    addSubprobContext(node,ptnode->getFullContext()); // set subproblem context to node
//        addPSTlist(node); // to allow pruning in the worker
    node->setExtern();
    node->clearHeurCache(); // clear precomputed heuristic values for AND children

    m_nextSubprob = new Subproblem(node, width, depth, height, m_nextThreadId,
                                lb,ub,avgInc,estInc,estN,hwb,twb);
    return true;
  }

  return false; // default false, don't parallelize

}


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



double SearchMaster::estimateIncrement(SearchNode* node) const {

  assert(node);
  PseudotreeNode* ptnode = m_pseudotree->getNode(node->getVar());

#ifdef DEBUG
  cout << "Estimating increment for subproblem at " << *node << endl;
#endif

  double est = ELEM_ONE;

  stack<PseudotreeNode*> stck; stck.push(ptnode);
  const set<int>& ctxt = ptnode->getFullContext();

  while (stck.size()) {
    PseudotreeNode* ptn = stck.top();
    stck.pop();
    for ( list<Function*>::const_iterator it=ptn->getFunctions().begin(); it!=ptn->getFunctions().end(); ++it) {
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
  est /= ptnode->getHeight()+1;
//  DIAG (cout << SCALE_LOG(est) << " (" << SCALE_NORM(est) << ")" << endl;);
  return est;

}



#endif /* PARALLEL_MODE */
