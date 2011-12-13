/*
 * SearchNode.cpp
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
 *  Created on: Oct 9, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "SearchNode.h"


#if false
double SearchNodeOR::getHeur() const {
  return m_heurValue;
  /*
  if (m_children.empty()) return m_heurValue;
  double h = min(m_heurValue,m_nodeValue);
  for (CHILDLIST::const_iterator it=m_children.begin(); it!=m_children.end(); ++it) {
    if ((*it)->getHeur() > h) h = (*it)->getHeur();
  }
  return h;
  */
}
#endif


/* stores the values of the partial solution tree in *bottom-up* order
 * in the argument vector */
void SearchNodeOR::getPST(vector<double>& pst) const {

  const SearchNode* curAND = NULL;
  const SearchNode *curOR = this;

  pst.clear();

  while (curOR->getParent()) {

    curAND = curOR->getParent();
    double label = curAND->getLabel();
    label OP_TIMESEQ curAND->getSubSolved();
    NodeP* children = curAND->getChildren();
    for (size_t i = 0; i < curAND->getChildCountFull(); ++i) {
      if (children[i] && children[i] != curOR)
        label OP_TIMESEQ children[i]->getHeur();
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

/*
ostream& operator << (ostream& os, const CHILDLIST& set) {
  os << '{' ;
  for (CHILDLIST::const_iterator it=set.begin(); it!=set.end();) {
    os << *it;
    if (++it != set.end()) os << ',';
  }
  cout << '}';
  return os;
}
*/

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

