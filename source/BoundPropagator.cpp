/*
 * BoundsPropagator.cpp
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
 *  Created on: Nov 7, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#undef DEBUG

#include "BoundPropagator.h"


SearchNode* BoundPropagator::propagate(SearchNode* n, bool reportSolution, SearchNode* upperLimit) {

  // these two pointers move upward in the search space, always one level
  // apart s.t. cur is the parent node of prev
  SearchNode* cur = n->getParent(), *prev=n;

  // Keeps track of the highest node to be deleted during cleanup,
  // where .second will be deleted as a child of .first
  pair<SearchNode*,SearchNode*> highestDelete(NULL,NULL);

  // 'prop' signals whether we are still propagating values in this call
  bool prop = true;
  // 'del' signals whether we are still deleting nodes in this call
  bool del = (upperLimit!=n) ? true : false;

#ifdef PARALLEL_STATIC
  count_t subCount = 0;
#endif

#ifdef PARALLEL_DYNAMIC
  // keeps track of the size of the deleted subspace and its number of leaf nodes
  count_t subCount = 0;
  count_t subLeaves = 0;
  count_t subLeafD = 0;

  m_subCountCache = 0;
  m_subLeavesCache = 0;
  m_subLeafDCache = 0;
#endif

  // going all the way to the root, if we have to
  do {
    DIAG( ostringstream ss; ss << "PROP  " << prev <<": " << *prev << " + " << cur << ": " << *cur << endl; myprint(ss.str()); )

    if (cur->getType() == NODE_AND) {
      // ===========================================================================

      // propagate and update node values
      if (prop) {
        // optimal solution to previously solved and deleted child OR nodes
        double d = cur->getSubSolved();
        // current best solution to yet-unsolved OR child nodes
        NodeP* children = cur->getChildren();
        for (size_t i = 0; i < cur->getChildCountFull(); ++i) {
          if (children[i])
            d OP_TIMESEQ children[i]->getValue();
        }

        // store into value (thus includes cost of subSolved)
        cur->setValue(d);

        if ( ISNAN(d) ) {// || d == ELEM_ZERO ) { // not all OR children solved yet, propagation stops here
          prop = false;
#ifndef NO_ASSIGNMENT
          propagateTuple(n,cur); // save (partial) opt. subproblem solution at current AND node
#endif
        }

      }

      // clean up fully propagated nodes (i.e. no children or only one (=prev))
      if (del) {
        if (prev->getChildCountAct() <= 1) {

#ifndef NO_CACHING
          // prev is OR node, try to cache
          if (m_doCaching && prev->isCachable() && !prev->isNotOpt() ) {
            try {
#ifndef NO_ASSIGNMENT
              m_space->cache->write(prev->getVar(), prev->getCacheInst(), prev->getCacheContext(), prev->getValue(), prev->getOptAssig() );
#else
              m_space->cache->write(prev->getVar(), prev->getCacheInst(), prev->getCacheContext(), prev->getValue() );
#endif
#ifdef DEBUG
              {
                ostringstream ss;
                ss << "-Cached " << *prev << " with value " << prev->getValue()
#ifndef NO_ASSIGNMENT
                << " and opt. solution " << prev->getOptAssig()
#endif
                << endl;
                myprint(ss.str());
              }
#endif
            } catch (...) { /* wrong cache instance counter */ }
          }
#endif
          highestDelete = make_pair(cur,prev);
#ifdef PARALLEL_STATIC
          subCount += prev->getSubCount();
          m_subCountCache = subCount;
          m_subStatsCache.update(prev, m_space->pseudotree->getNode(prev->getVar()), subCount);
#endif
#ifdef PARALLEL_DYNAMIC
          subCount += prev->getSubCount();
          subLeaves += prev->getSubLeaves();
          subLeafD += prev->getSubLeafD();

          // cache parameters of deleted subproblem
          m_subRootvarCache = prev->getVar();
          m_subCountCache = subCount;
          m_subLeavesCache = subLeaves;
          m_subLeafDCache = subLeafD;
          // cache lower/upper bound of OR to be deleted (for master init.)
          m_subBoundsCache = make_pair(highestDelete.second->getInitialBound(), highestDelete.second->getHeur());

          // climb up one level
          subLeafD += subLeaves;
#endif
          DIAG(myprint(" marking OR for deletion\n") );
        } else {
          del = false;
#ifdef LIKELIHOOD
          prop = false;
#endif
        }

      }

      // ===========================================================================
    } else { // cur is OR node
      // ===========================================================================

      if (prop) {
        double d = prev->getValue() OP_TIMES prev->getLabel(); // getValue includes subSolved
#ifdef LIKELIHOOD
        if ( ISNAN( cur->getValue() ) || cur->getValue() == ELEM_ZERO )
          cur->setValue(d);
        else
          cur->setValue( OP_PLUS(d,cur->getValue()) );
#else
        if ( ISNAN( cur->getValue() ) || d > cur->getValue()) {
          cur->setValue(d); // update max. value
        } else {
          prop = false; // no more value propagation upwards in this call
        }
#endif /* LIKELIHOOD */
      }

      if (del) {
        if (prev->getChildCountAct() <= 1) { // prev has no or one children?
          highestDelete = make_pair(cur,prev);
#ifdef PARALLEL_STATIC
          subCount += prev->getSubCount();
#endif
#ifdef PARALLEL_DYNAMIC
          subCount += prev->getSubCount();
          subLeaves += prev->getSubLeaves();
          subLeafD += prev->getSubLeafD();
#endif
          DIAG(myprint(" marking AND for deletion\n"));
        } else {
          del = false;
#ifdef LIKELIHOOD
          prop = false;
#endif
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
        if (prop) {
#ifndef NO_ASSIGNMENT
          propagateTuple(n,cur);
          if (reportSolution)
            m_problem->updateSolution(cur->getValue(), cur->getOptAssig(), & m_space->stats, true);
#else
          if (reportSolution)
            m_problem->updateSolution(cur->getValue(), & m_space->stats, true);
#endif
      }
      break;
      }

      // ===========================================================================
    }

    // don't delete anything higher than upperLimit
    if (upperLimit == cur)
      del = false;

    // move pointers up in search space
    if (prop || del) {
      prev = cur;
      cur = cur->getParent();
    } else {
      break;
    }

  } while (cur); // until cur==NULL, i.e. 'parent' of root

  // propagated up to root node, update tuple as well
  if (prop && !cur) {
#ifndef NO_ASSIGNMENT
    propagateTuple(n,prev);
    if (reportSolution)
      m_problem->updateSolution(prev->getValue(), prev->getOptAssig(), & m_space->stats , true);
#else
    if (reportSolution)
      m_problem->updateSolution(prev->getValue(), & m_space->stats, true);
#endif
  }

  if (highestDelete.first) {
    SearchNode* parent = highestDelete.first;
    SearchNode* child = highestDelete.second;
    if(parent->getType() == NODE_AND) {
      // Store value of OR node to be deleted into AND parent
      parent->addSubSolved(child->getValue());
    }
#ifdef PARALLEL_STATIC
    parent->addSubCount(subCount);
#endif
#ifdef PARALLEL_DYNAMIC
    // store size of deleted subproblem in parent node
    parent->addSubCount(subCount);
    parent->addSubLeaves(subLeaves);
    parent->addSubLeafD(subLeafD);
#endif
    // finally clean up, delete subproblem with unnecessary nodes from memory
    parent->eraseChild(child);
  }

  DIAG(myprint("! Prop done.\n"));

  return highestDelete.first;

}

#ifndef NO_ASSIGNMENT

/* collects the joint assignment from 'start' upwards until 'end' and
 * records it into 'end' for later use */
void BoundPropagator::propagateTuple(SearchNode* start, SearchNode* end) {
  assert(start && end);
  DIAG(ostringstream ss; ss << "< REC opt. assignment from " << *start << " to " << *end << endl; myprint(ss.str());)

  int endVar = end->getVar();
  const set<int>& endSubprob = m_space->pseudotree->getNode(endVar)->getSubprobVars();

  // get variable map for end node
  vector<int> endVarMap = m_space->pseudotree->getNode(endVar)->getSubprobVarMap();
  // allocate assignment in end node
  vector<val_t>& assig = end->getOptAssig();
  assig.resize(endSubprob.size(), UNKNOWN);

  int curVar = UNKNOWN, curVal = UNKNOWN;
  for (SearchNode* cur=start; cur!=end; cur=cur->getParent()) {
    curVar = cur->getVar();

    if (cur->getType() == NODE_AND) {
      curVal = cur->getVal();
      if (curVal!=UNKNOWN)
        assig.at(endVarMap.at(curVar)) = curVal;
    }

    if (cur->getOptAssig().size()) {
      // check previously saved partial assignment
      const set<int>& curSubprob = m_space->pseudotree->getNode(cur->getVar())->getSubprobVars();
      set<int>::const_iterator itVar = curSubprob.begin();
      vector<val_t>::const_iterator itVal = cur->getOptAssig().begin();

      for(; itVar!= curSubprob.end(); ++itVar, ++itVal ) {
        if (*itVal != UNKNOWN)
          assig[endVarMap[*itVar]] = *itVal;
      }

      // clear optimal assignment of AND node, since now propagated upwards
      if (cur->getType() == NODE_AND)
        cur->clearOptAssig(); // TODO correct ?
    }
  } // end for

  DIAG(ostringstream ss; ss << "< Tuple for "<< *end << " after recording: " << (end->getOptAssig()) << endl; myprint(ss.str());)
}

#endif // NO_ASSIGNMENT
