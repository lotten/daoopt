/*
 * SearchMaster.cpp
 *
 *  Created on: Feb 14, 2010
 *      Author: lars
 */

#include "SearchMaster.h"

#ifdef PARALLEL_MODE

void SearchMaster::operator ()() {

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
      while (true) { // loop exited after parallelization or leaf node expansion

        node = this->nextNode();

        if (doProcess(node)) // * initial processing
          break; // node is dead end

        if (doCaching(node)) // * caching
          break; // cached solution found

        if (doPruning(node)) // * pruning
          break; // node was pruned

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
        SubproblemCondor subprob(m_spaceMaster, m_nextSubprob, m_nextThreadId++);
        {
          GETLOCK(m_spaceMaster->mtx_activeThreads, lk2);
          m_spaceMaster->activeThreads.insert(
              make_pair( m_nextSubprob , new boost::thread(subprob) )  );
        }
        m_nextSubprob = NULL;
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

    }
  } // end of while loop

  myprint("\n!Search done!\n\n");

  } catch (boost::thread_interrupted i) {
    myprint("\n!Search aborted!\n");
  }

}


bool SearchMaster::doParallelize(SearchNode* node) {

  assert(node);

  if (node->getType() == NODE_AND)
    return false; // don't parallelize on AND nodes

  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int depth = ptnode->getDepth();

  bigint hwb = ptnode->computeHWB(&m_assignment);
  bigint twb = ptnode->getSubsize() ;

  // TODO cutoff decision
  //if (ptnode->getDepth() == m_space->options->cutoff_depth) {
  if ( depth == m_space->options->cutoff_depth ||
      m_hwbCache <= m_space->options->cutoff_size * bigint(100000) )
    //         ptnode->getSubsize()  <= m_space->options->cutoff_size) {
    //    if (ptnode->getSubwidth() == m_space->options->cutoff_depth) {
  {

    stringstream ss;
    ss << "** Parallelising at depth " << depth << "\n" ;
    myprint(ss.str());

    double lb = lowerBound(node);

    double avgInc = avgIncrement(node);

    addSubprobContext(node,ptnode->getFullContext()); // set subproblem context to node
//        addPSTlist(node); // to allow pruning in the worker
    node->setExtern();
    node->clearHeurCache(); // clear precomputed heuristic values for AND children

    m_nextSubprob = new Subproblem(node, ptnode->getSubwidth(), depth, ptnode->getHeight(),
                                lb,node->getHeur(),avgInc,hwb,twb);
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



double SearchMaster::estimateIncrement(SearchNode*) const {

  return ELEM_NAN;

}



#endif /* PARALLEL_MODE */
