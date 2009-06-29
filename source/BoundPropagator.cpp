/*
 * BoundsPropagator.cpp
 *
 *  Created on: Nov 7, 2008
 *      Author: lars
 */

#include "BoundPropagator.h"

#undef DEBUG

#ifdef USE_THREADS
void BoundPropagator::operator() () {

  bool allDone = false;
  size_t noPropagated = 0;

  try { while (!allDone) {

    SearchNode* n = 0;
    bool hasMore = false;
    noPropagated = 0;
    boost::thread* tp = NULL;

    do {
      {
        GETLOCK(m_space->mtx_solved, lk);

        while( m_space->solved.empty() ) {
          CONDWAIT(m_space->cond_solved, lk);
        }

        // new subproblem to propagate
        n = m_space->solved.front();
        m_space->solved.pop();

      }

      { // clean up processing thread
        GETLOCK(m_space->mtx_activeThreads, lk);
        map< SearchNode*, boost::thread* >::iterator it = m_space->activeThreads.find(n);
        if (it!=m_space->activeThreads.end()) {
          tp = m_space->activeThreads.find(n)->second;
          m_space->activeThreads.erase(n);
          tp->join();
          delete tp;
        }
      }


      { // actual propagation
        GETLOCK(m_space->mtx_space, lk);
        propagate(n);
        ++noPropagated;
      }

      { //check if there's more to propagate
        GETLOCK(m_space->mtx_solved, lk);
        hasMore = !m_space->solved.empty();
      }

    } while (hasMore);



    { // TODO: Make sure race condition can't happen
      GETLOCK(m_space->mtx_searchDone,lk);
      if (m_space->searchDone) { // search process done?
        GETLOCK(m_space->mtx_activeThreads,lk2);
        if (m_space->activeThreads.empty()) { // no more processing threads
          allDone = true;
        }
      } else { // search not done, notify
        { // notify the search that the buffer is open again
          GETLOCK(m_space->mtx_allowedThreads,lk);
          m_space->allowedThreads += noPropagated;
          m_space->cond_allowedThreads.notify_one();
        }
      }
    }

  } // overall while(!allDone) loop

  myprint("\n!PROP done!\n\n");

  } catch (boost::thread_interrupted i) {
    myprint("\n!PROP aborted!\n");
  }

}

#endif


void BoundPropagator::propagate(SearchNode* n) {

  // these two pointers move upward in the search space, always one level
  // apart s.t. cur is the parent node of prev
  SearchNode* cur = n->getParent(), *prev=n;

  // 'prop' signals whether we are still propagating values in this call
  // 'del' signals whether we are still deleting nodes in this call
  bool prop = true, del = true;

  // going all the way to the root, if we have to
  do {
#ifdef DEBUG
    {
      GETLOCK(mtx_io, lk);
      cout << "PROP  " << *prev << " + " << *cur << endl;
    }
#endif

    if (cur->getType() == NODE_AND) {
      // ===========================================================================

      // clean up fully propagated nodes (i.e. no children)
      if (prev->getChildren().empty()) {
#ifdef USE_LOG
          cur->setLabel(cur->getLabel()+prev->getValue());
#else
          cur->setLabel(cur->getLabel()*prev->getValue());
#endif

#ifndef NO_CACHING
          // prev is OR node, try to cache
          if ( prev->isCachable() && !prev->isNotOpt() ) {
            try {
              m_space->cache->write(prev->getVar(), prev->getCacheInst(), prev->getCacheContext(), prev->getValue(), prev->getOptAssig() );
#ifdef DEBUG
              {
                GETLOCK(mtx_io, lk);
                cout << "-Cached " << *prev << " with opt. solution " << prev->getOptAssig() << endl;
              }
#endif
            } catch (...) {}
          }
#endif
          mergePrevAssignment(prev, cur); // merge opt. solutions
          cur->eraseChild(prev); // finally, delete
      } else {
        del = false;
      }

      if (prop) {
#ifdef USE_LOG
        double d = 0.0;
#else
        double d = 1.0;
#endif
        for (CHILDLIST::const_iterator it = cur->getChildren().begin();
            it != cur->getChildren().end(); ++it)
        {
#ifdef USE_LOG
          d += (*it)->getValue();
#else
          d *= (*it)->getValue();
#endif
        }

        cur->setValue(d);

      }

      if (prop && !del)
        mergePrevAssignment(prev, cur);

      // ===========================================================================
    } else { // cur is OR node
      // ===========================================================================

#ifdef USE_LOG
      double v = prev->getValue() + prev->getLabel();
#else
      double v = prev->getValue() * prev->getLabel();
#endif

      if (prop) {
        if (v > cur->getValue()) {
          cur->setValue(v); // update max. value
          mergePrevAssignment(prev, cur); // update opt. subproblem assignment
        } else {
          prop = false; // no more value propagation upwards in this call
        }
      }

      if (prev->getChildren().empty()) { // prev has no children?
        cur->eraseChild(prev); // remove the node
      } else {
        del = false;
      }

      // Stop at root node
      if (cur == m_space->subproblem) break;

      // ===========================================================================
    }

    // move pointers up in search space
    if (prop || del) {
      prev = cur;
      cur = cur->getParent();
    } else {
      break;
    }

  } while (cur); // until cur==NULL, i.e. parent of root

#ifdef DEBUG
  myprint("! Prop done.\n");
//  cout << "! Prop done." << endl;
#endif

}



void BoundPropagator::mergePrevAssignment(SearchNode* prev, SearchNode* cur) {
  assert(prev);
  assert(cur);

#ifdef DEBUG
  GETLOCK(mtx_io, lk); // will block during full function body
  cout << "MERGE " << *prev << " + " << *cur << endl;
#endif

  int curVar = cur->getVar();
  const set<int>& curSubprob = m_space->pseudotree->getNode(curVar)->getSubprobVars();

  if (cur->getType() == NODE_AND) { // current is AND node, child OR

    int prevVar = prev->getVar();
    const set<int>& prevSubprob = m_space->pseudotree->getNode(prevVar)->getSubprobVars();

    set<int>::const_iterator itCurVar = curSubprob.begin();
    set<int>::const_iterator itPrevVar = prevSubprob.begin();

    vector<val_t>::iterator itCurVal = cur->getOptAssig().begin();
    vector<val_t>::iterator itPrevVal = prev->getOptAssig().begin();

    // iterate over current and previous optimal assignment, updating respective
    // parts of current with values from previous
    while (itCurVar!=curSubprob.end() || itPrevVar!=prevSubprob.end()) {

      if (*itCurVar == curVar) {
        *itCurVal = cur->getVal();
        ++itCurVar; ++itCurVal;
        continue;
      } else if (itPrevVar == prevSubprob.end()) {
        ++itCurVar; ++itCurVal;
        continue;
      }
      if (*itCurVar == *itPrevVar) {
        *itCurVal = *itPrevVal;
        ++itCurVar; ++itCurVal; ++itPrevVar; ++itPrevVal;
      } else if (*itCurVar < *itPrevVar) {
        ++itCurVar; ++itCurVal;
      } else { // *itCurVar > *itPrevVar
        ++itPrevVar; ++itPrevVal;
      }

    }

#ifdef DEBUG
    itCurVar = curSubprob.begin();
    itCurVal = cur->getOptAssig().begin();
    for (;itCurVar != curSubprob.end(); ++itCurVar, ++itCurVal )
      cout << " " << *itCurVar << "=" << (int) *itCurVal << flush;
    cout << endl;
#endif

  } else { // cur->getType() == NODE_OR

    // this means subprobVars of current and previous are identical
    // just need to copy values 1 by 1
    copy(prev->getOptAssig().begin(), prev->getOptAssig().end(), cur->getOptAssig().begin());

#ifdef DEBUG
    set<int>::const_iterator itCurVar = curSubprob.begin();
    vector<val_t>::iterator itCurVal = cur->getOptAssig().begin();
    for (;itCurVar != curSubprob.end(); ++itCurVar, ++itCurVal )
      cout << " " << *itCurVar << "=" << (int) *itCurVal << flush;
    cout << endl;
#endif

  }

}

