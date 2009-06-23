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
