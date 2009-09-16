/*
 * BoundsPropagator.cpp
 *
 *  Created on: Nov 7, 2008
 *      Author: lars
 */

#include "BoundPropagator.h"

#undef DEBUG

#ifdef PARALLEL_MODE
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

  // Keeps track of the highest node to be deleted during cleanup,
  // where .second will be deleted as a child of .first
  pair<SearchNode*,SearchNode*> highestDelete(NULL,NULL);

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

      if (prop) {
        double d = ELEM_ONE;
        for (CHILDLIST::const_iterator it = cur->getChildren().begin();
            it != cur->getChildren().end(); ++it)
        {
          d OP_TIMESEQ (*it)->getValue();
        }

        cur->setValue(d);

        if ( ISNAN(d) ) {
          prop = false;
#ifndef NO_ASSIGNMENT
          propagateTuple(n,cur); // save (partial) opt. subproblem solution at current AND node
#endif
        }

      }

      // clean up fully propagated nodes (i.e. no children)
      if (del) {
        if (prev->getChildren().size() <= 1) {

#ifndef NO_CACHING
          // prev is OR node, try to cache
          if ( prev->isCachable() && !prev->isNotOpt() ) {
            try {
#ifndef NO_ASSIGNMENT
              m_space->cache->write(prev->getVar(), prev->getCacheInst(), prev->getCacheContext(), prev->getValue(), prev->getOptAssig() );
#else
              m_space->cache->write(prev->getVar(), prev->getCacheInst(), prev->getCacheContext(), prev->getValue() );
#endif
#ifdef DEBUG
              {
                GETLOCK(mtx_io, lk);
                cout << "-Cached " << *prev
#ifndef NO_ASSIGNMENT
                << " with opt. solution " << prev->getOptAssig()
#endif
                << endl;
              }
#endif
            } catch (...) { /* wrong cache instance counter */ }
          }
#endif
          highestDelete = make_pair(cur,prev);
#ifdef DEBUG
          cout << " deleting" << endl;
#endif
        } else {
          del = false;
        }

      }

      // ===========================================================================
    } else { // cur is OR node
      // ===========================================================================

      double v = prev->getValue() OP_TIMES prev->getLabel();

      if (prop) {
        if ( ISNAN( cur->getValue() ) || v > cur->getValue()) {
          cur->setValue(v); // update max. value
        } else {
          prop = false; // no more value propagation upwards in this call
        }
      }

      if (del) {
        if (prev->getChildren().size() <= 1) { // prev has no or one children?
          highestDelete = make_pair(cur,prev);
#ifdef DEBUG
          cout << " deleting" << endl;
#endif
        } else {
          del = false;
        }
      }

#ifndef NO_ASSIGNMENT
      // save opt. tuple, will be needed for caching later
      if ( prop && cur->isCachable() && !cur->isNotOpt() ) {
#ifdef DEBUG
        myprint("< Cachable OR node found\n");
#endif
        propagateTuple(n, cur);
      }
#endif

      // Stop at subproblem root node (if defined)
      if (cur == m_space->subproblem) {
#ifdef DEBUG
        myprint("PROP reached ROOT\n");
#endif
#ifndef NO_ASSIGNMENT
        if (prop)
          propagateTuple(n,cur);
#endif
        break;
      }

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

#ifndef NO_ASSIGNMENT
  // propagated up to root node, update tuple as well
  if (prop && !cur)
    propagateTuple(n,prev);
#endif

  // Prepare clean up
  if(highestDelete.first->getType() == NODE_AND)
    highestDelete.first->setLabel(
          highestDelete.first->getLabel() OP_TIMES highestDelete.second->getValue()
        );
  // clean up, remove unnecessary nodes from memory
  highestDelete.first->eraseChild(highestDelete.second);

#ifdef DEBUG
  myprint("! Prop done.\n");
#endif

}

#ifndef NO_ASSIGNMENT
/*
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
*/

// collects the joint assignment from 'start' upwards until 'end' and
// records it into 'end' for later use
void BoundPropagator::propagateTuple(SearchNode* start, SearchNode* end) {

  assert(start && end);

#ifdef DEBUG
  cout << "< REC opt. assignment from " << *start << " to " << *end << endl;
#endif

  int endVar = end->getVar();
  const set<int>& endSubprob = m_space->pseudotree->getNode(endVar)->getSubprobVars();

  // will keep track of the merged assignment
  map<int,val_t> assig;

  int curVar = UNKNOWN, curVal = UNKNOWN;

  for (SearchNode* cur=start; cur!=end; cur=cur->getParent()) {

    curVar = cur->getVar();

    if (cur->getType() == NODE_AND) {
      curVal = cur->getVal();
      if (curVal!=UNKNOWN)
        assig.insert(make_pair(curVar,curVal));
    }

    if (cur->getOptAssig().size()) {
      // check previously saved partial assignment
      const set<int>& curSubprob = m_space->pseudotree->getNode(cur->getVar())->getSubprobVars();
      set<int>::const_iterator itVar = curSubprob.begin();
      vector<val_t>::const_iterator itVal = cur->getOptAssig().begin();

      for(; itVar!= curSubprob.end(); ++itVar, ++itVal ) {
        if (*itVal != UNKNOWN)
          assig.insert(make_pair(*itVar, *itVal));
      }

      // clear optimal assignment of AND node, since now propagated upwards
      if (cur->getType() == NODE_AND)
        cur->clearOptAssig(); // TODO ?
    }

  } // end for

  // now record assignment into end node
  // TODO dynamically allocate (resize) subprob vector (not upfront)
  end->getOptAssig().resize(endSubprob.size(), UNKNOWN);
  set<int>::const_iterator itVar = endSubprob.begin();
  vector<val_t>::iterator itVal = end->getOptAssig().begin();

  map<int,val_t>::const_iterator itAssig = assig.begin();

  while( itAssig != assig.end() /*&& itVar != endSubprob.end()*/ ) {
    if (*itVar == itAssig->first) {
      // record assignment
      *itVal = itAssig->second;
      // forward pointers
      ++itAssig;
    }
    ++itVar, ++itVal;
  }

#ifdef DEBUG
  cout << "< Tuple for "<< *end << " after recording: " << (end->getOptAssig()) << endl;
#endif

}

#endif // NO_ASSIGNMENT
