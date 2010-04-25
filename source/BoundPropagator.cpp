/*
 * BoundsPropagator.cpp
 *
 *  Created on: Nov 7, 2008
 *      Author: lars
 */

#undef DEBUG

#include "BoundPropagator.h"


SearchNode* BoundPropagator::propagate(SearchNode* n) {

  // these two pointers move upward in the search space, always one level
  // apart s.t. cur is the parent node of prev
  SearchNode* cur = n->getParent(), *prev=n;

  // Keeps track of the highest node to be deleted during cleanup,
  // where .second will be deleted as a child of .first
  pair<SearchNode*,SearchNode*> highestDelete(NULL,NULL);

  // 'prop' signals whether we are still propagating values in this call
  // 'del' signals whether we are still deleting nodes in this call
  bool prop = true, del = true;

#ifdef PARALLEL_MODE
  // keeps track of the size of the deleted subspace and its number of leaf nodes
  count_t subCount = 0;
  count_t subLeaves = 0;
  count_t subLeafD = 0;
#endif

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

      // propagate and update node values
      if (prop) {
        // optimal solution to previously solved and deleted child OR nodes
        double d = cur->getSubSolved();
        // current best solution to yet-unsolved OR child nodes
        for (CHILDLIST::const_iterator it = cur->getChildren().begin();
            it != cur->getChildren().end(); ++it)
        {
          d OP_TIMESEQ (*it)->getValue();
        }

        // store into value (thus includes cost of subSolved)
        cur->setValue(d);

        if ( ISNAN(d) ) {
          prop = false;
#ifndef NO_ASSIGNMENT
          propagateTuple(n,cur); // save (partial) opt. subproblem solution at current AND node
#endif
        }

      }

      // clean up fully propagated nodes (i.e. no children or only one (=prev))
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
#ifdef PARALLEL_MODE
          subCount += prev->getSubCount();
          subLeaves += prev->getSubLeaves();
          subLeafD += prev->getSubLeafD();

          // cache parameters of deleted subproblem
          m_subRootvarCache = prev->getVar();
          m_subCountCache = subCount;
          m_subLeavesCache = subLeaves;
          m_subLeafDCache = subLeafD;
          // cache lower/upper bound of OR to be deleted (for master init.)
          m_subBoundsCache = make_pair(highestDelete.second->getInitialBound(), highestDelete.second->getHeurOrg());

          // climb up one level
          subLeafD += subLeaves;
#endif
          DIAG(myprint(" deleting AND\n") );
        } else {
          del = false;
        }

      }

      // ===========================================================================
    } else { // cur is OR node
      // ===========================================================================

      double v = prev->getValue() OP_TIMES prev->getLabel(); // getValue includes subSolved

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
#ifdef PARALLEL_MODE
          subCount += prev->getSubCount();
          subLeaves += prev->getSubLeaves();
          subLeafD += prev->getSubLeafD();
#endif
          DIAG(myprint(" deleting OR\n"));
        } else {
          del = false;
        }
      }

#ifndef NO_ASSIGNMENT
      // save opt. tuple, will be needed for caching later
      if ( prop && cur->isCachable() && !cur->isNotOpt() ) {
        DIAG(myprint("< Cachable OR node found\n"));
        propagateTuple(n, cur);
      }
#endif

      // Stop at subproblem root node (if defined)
      if (cur == m_space->subproblemLocal) {
        if (del) highestDelete = make_pair(cur,prev);
        DIAG(myprint("PROP reached ROOT\n"));
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

  } while (cur); // until cur==NULL, i.e. 'parent' of root

#ifndef NO_ASSIGNMENT
  // propagated up to root node, update tuple as well
  if (prop && !cur)
    propagateTuple(n,prev);
#endif

  if(highestDelete.first->getType() == NODE_AND) {
    // Store value of OR node to be deleted into AND parent
    highestDelete.first->addSubSolved(highestDelete.second->getValue());
  }
#ifdef PARALLEL_MODE
  // store size of deleted subproblem in parent node
  highestDelete.first->addSubCount(subCount);
  highestDelete.first->addSubLeaves(subLeaves);
  highestDelete.first->addSubLeafD(subLeafD);
#endif
  // finally clean up, delete subproblem with unnecessary nodes from memory
  highestDelete.first->eraseChild(highestDelete.second);

  DIAG(myprint("! Prop done.\n"));

  return highestDelete.first;

}

#ifndef NO_ASSIGNMENT

/* collects the joint assignment from 'start' upwards until 'end' and
 * records it into 'end' for later use */
void BoundPropagator::propagateTuple(SearchNode* start, SearchNode* end) {

  assert(start && end);

  DIAG(cout << "< REC opt. assignment from " << *start << " to " << *end << endl);

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

  DIAG(cout << "< Tuple for "<< *end << " after recording: " << (end->getOptAssig()) << endl);

}

#endif // NO_ASSIGNMENT
