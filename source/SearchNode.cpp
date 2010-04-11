/*
 * SearchNode.cpp
 *
 *  Created on: Oct 9, 2008
 *      Author: lars
 */

#include "SearchNode.h"


double SearchNodeOR::getHeur() const {
  return m_heurValue;
  if (m_children.empty()) return m_heurValue;
  double h = min(m_heurValue,m_nodeValue);
  for (CHILDLIST::const_iterator it=m_children.begin(); it!=m_children.end(); ++it) {
    if ((*it)->getHeur() > h) h = (*it)->getHeur();
  }
  return h;
}


void SearchNodeOR::getPST(vector<double>& pst) const {

  const SearchNode* curAND = NULL;
  const SearchNode *curOR = this;

  pst.clear();

  while (curOR->getParent()) {

    curAND = curOR->getParent();
    double label = curAND->getLabel();
    label OP_TIMESEQ curAND->getSubSolved();
    const CHILDLIST& children = curAND->getChildren();
    for (CHILDLIST::const_iterator it=children.begin(); it!=children.end(); ++it) {
      if (*it != curOR) // skip previous or node
        label OP_TIMESEQ  (*it)->getHeur();
    }
    pst.push_back(label);

    curOR = curAND->getParent();
    pst.push_back(curOR->getValue());

  }

}


// Some output operators and functions
ostream& operator << (ostream& os, const SearchNode& n) {
  if (n.getType() == NODE_AND) {
    return os << "AND " << n.getVar() << "->" << (int) n.getVal();
  } else { // OR node
    return os << "OR " << n.getVar();
  }
}


ostream& operator << (ostream& os, const CHILDLIST& set) {
  os << '{' ;
  for (CHILDLIST::const_iterator it=set.begin(); it!=set.end();) {
    os << *it;
    if (++it != set.end()) os << ',';
  }
  cout << '}';
  return os;
}


string SearchNode::toString(const SearchNode* n) {
  assert(n);
  ostringstream ss;
  if (n->getType() == NODE_AND) {
    ss << "AND node: " << n->getVar() << "->" << (int) n->getVal();
  } else { // OR node
    ss << "OR node: " << n->getVar();
  }
  return ss.str();
}

