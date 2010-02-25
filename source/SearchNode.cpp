/*
 * SearchNode.cpp
 *
 *  Created on: Oct 9, 2008
 *      Author: lars
 */

#include "SearchNode.h"

//size_t SearchNode::noNodes = 0;

//string SearchNodeAND::emptyString = "";

// Stream output operator
ostream& operator << (ostream& os, const SearchNode& n) {
  if (n.getType() == NODE_AND) {
    return os << "AND node: " << n.getVar() << "->" << (int) n.getVal();
  } else { // OR node
    return os << "OR node: " << n.getVar();
  }
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
