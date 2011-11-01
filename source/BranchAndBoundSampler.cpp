/*
 * BranchAndBoundSampler.cpp
 *
 *  Created on: Oct 30, 2011
 *      Author: lars
 */

#include "BranchAndBoundSampler.h"


bool BranchAndBoundSampler::doExpand(SearchNode* n) {

  assert(n);
  vector<SearchNode*> chi;

  if (n->getType() == NODE_AND) {  // AND node

    if (chi.size() == 0 && generateChildrenAND(n,chi)) {
      return true; // no children
    }

#ifdef DEBUG
    oss ss;
    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it)
      ss << '\t' << *it << ": " << *(*it) << endl;
    myprint (ss.str());
#endif

    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it)
      m_stack.push(*it);

  } else {  // OR node

    if (chi.size() == 0 && generateChildrenOR(n,chi)) {
      return true; // no children
    }

    int pick = NONE;
    if (m_space->options->sampleDepth <= n->getDepth())
      pick = rand::next(chi.size());

    for (int i = 0; i < chi.size(); ++i) {
      if (i != pick) {
        m_stack.push(chi[i]);
        DIAG(oss ss; ss <<'\t'<< chi[i] <<": "<< *(chi[i]) <<" (l="<< chi[i]->getLabel() <<")"<< endl; myprint(ss.str()); )
      }
    }
    if (pick != NONE) {
      m_stack.push(chi[pick]);
      DIAG(oss ss; ss <<'\t'<< chi[pick] <<": "<< *(chi[pick]) <<" (l="<< chi[pick]->getLabel() <<")"<< endl; myprint(ss.str()); )
    }

    DIAG (oss ss; ss << "\tGenerated " << n->getChildCountFull() <<  " child AND nodes" << endl; myprint(ss.str()); )

  } // if over node type

  return false; // default false
}

