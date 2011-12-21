/*
 * BranchAndBoundSampler.cpp
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
 *  Created on: Oct 30, 2011
 *      Author: lars
 */

#undef DEBUG

#include "BranchAndBoundSampler.h"

bool BranchAndBoundSampler::doExpand(SearchNode* n) {
  assert(n);
  m_expand.clear();

  if (n->getType() == NODE_AND) {  // AND node

    if (m_expand.size() == 0 && generateChildrenAND(n, m_expand)) {
      return true; // no children
    }

#ifdef DEBUG
    oss ss;
    for (vector<SearchNode*>::iterator it = m_expand.begin(); it != m_expand.end(); ++it)
      ss << '\t' << *it << ": " << *(*it) << endl;
    myprint (ss.str());
#endif

    for (vector<SearchNode*>::iterator it=m_expand.begin(); it!=m_expand.end(); ++it)
      m_stack.push(*it);

  } else {  // OR node

    if (m_expand.size() == 0 && generateChildrenOR(n, m_expand)) {
      return true; // no children
    }

    int pick = NONE;
    if (n->getDepth() <= m_space->options->sampleDepth)
      pick = rand::next(m_expand.size());

    for (int i = 0; i < (int) m_expand.size(); ++i) {
      if (i != pick) {
        m_stack.push(m_expand[i]);
        DIAG(oss ss; ss <<'\t'<< m_expand[i] <<": "<< *(m_expand[i]) <<" (l="<< m_expand[i]->getLabel() <<")"<< endl; myprint(ss.str()); )
      }
    }
    if (pick != NONE) {
      m_stack.push(m_expand[pick]);
      DIAG(oss ss; ss <<'\t'<< m_expand[pick] <<": "<< *(m_expand[pick]) <<" (l="<< m_expand[pick]->getLabel() <<")"<< endl; myprint(ss.str()); )
    }

    DIAG (oss ss; ss << "\tGenerated " << n->getChildCountFull() <<  " child AND nodes" << endl; myprint(ss.str()); )

  } // if over node type

  return false; // default false
}

