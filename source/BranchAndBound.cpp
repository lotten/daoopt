/*
 * BranchAndBound.cpp
 *
 *  Created on: Nov 4, 2008
 *      Author: lars
 */

#include "BranchAndBound.h"

#ifdef PARALLEL_MODE
#undef DEBUG
#endif

#undef DEBUG

typedef PseudotreeNode PtNode;


bool BranchAndBound::doExpand(SearchNode* node) {

  assert(node);
  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int depth = ptnode->getDepth();

  if (node->getType() == NODE_AND) {

    ++m_nodesAND; // count node
    if (depth >= 0) { // ignores dummy nodes
      m_nodeProfile.at(depth) += 1; // count node as expanded
    }

    // create new OR children
    for (vector<PtNode*>::const_iterator it=ptnode->getChildren().begin();
        it!=ptnode->getChildren().end(); ++it)
    {
      int vChild = (*it)->getVar();

      SearchNodeOR* n = new SearchNodeOR(node, vChild);

      // Compute and set heuristic estimate
      heuristicOR(n);

      node->addChild(n);
      m_stack.push(n);
#ifdef DEBUG
      ostringstream ss;
      ss << '\t' << *n;
      if (n->isCachable())
        ss << " - Cache: " << n->getCacheContext();
      ss << endl;
      myprint (ss.str());
#endif

    } // for loop

    if (!node->getChildren().size()) { // no children?
      node->setLeaf(); // -> terminal node
      node->setValue(ELEM_ONE);
      m_leafProfile.at(depth) += 1; // count leaf node
      return true;
    }

  } else { // NODE_OR

    ++m_nodesOR; // count node expansion

    // actually create new AND children
    double* heur = node->getHeurCache();

    vector<SearchNode*> newNodes;
    newNodes.reserve(m_problem->getDomainSize(var));

    for (val_t i=m_problem->getDomainSize(var)-1; i>=0; --i) {

      // can skip node if heuristic is zero (since it's an upper bound)
      if (heur[2*i+1] == ELEM_ZERO) {
        m_leafProfile.at(depth) += 1;
        continue; // label=0 -> skip
      }

      SearchNodeAND* n = new SearchNodeAND(node, i, heur[2*i+1]); // uses cached label

      // set cached heur. value
      n->setHeur( heur[2*i] );
      // add node as successor
      node->addChild(n);
      // remember new node
      newNodes.push_back(n);
    }

    node->clearHeurCache(); // clear heur. cache of OR node

    if (newNodes.size()) {

      // sort new nodes by increasing heuristic value
      sort(newNodes.begin(), newNodes.end(), SearchNode::heurLess);

      // add new child nodes to stack, highest heur. value will end up on top
      for (vector<SearchNode*>::iterator it = newNodes.begin(); it!=newNodes.end(); ++it) {
        m_stack.push(*it);
#ifdef DEBUG
        {
          GETLOCK(mtx_io, lk);
          cout << '\t' << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; // Assig. " << m_assignment << endl;
        }
#endif
      }
    } else { // all child AND nodes were pruned
      node->setLeaf();
      node->setValue(ELEM_ZERO);
      return true;
    }

  } // if-else over node type

  return false; // default false

} // BranchAndBound::doExpand




BranchAndBound::BranchAndBound(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) :
   Search(prob,pt,space,heur) {

  // create first node (dummy variable)
  PseudotreeNode* ptroot = m_pseudotree->getRoot();
  SearchNode* node = new SearchNodeOR(NULL, ptroot->getVar() );
  m_space->root = node;
  // create dummy variable's AND node (domain size 1)
  SearchNode* next = new SearchNodeAND(m_space->root, 0, prob->getGlobalConstant());
  // put global constant into dummy AND node label
  m_space->root->addChild(next);

  m_stack.push(next);

}


