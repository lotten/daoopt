/*
 * BranchAndBound.cpp
 *
 *  Created on: Nov 4, 2008
 *      Author: lars
 */

#undef DEBUG

#include "BranchAndBound.h"

#ifdef PARALLEL_MODE
#undef DEBUG
#endif

typedef PseudotreeNode PtNode;


SearchNode* BranchAndBound::nextNode() {
  SearchNode* n = NULL;
#ifdef ANYTIME
  while (m_stacks.size()) {
    MyStack* st = m_stacks.front();
    if (st->getChildren()) {
      m_stacks.push(st);
      m_stackCount = 0;
    } else if (st->empty()) {
      if (st->getParent())
        st->getParent()->delChild();
      delete st;
      m_stackCount = 0;
    } else if (m_stackCount++ == 1000) {
      m_stacks.push(st);
      m_stackCount = 0;
    } else {
      n = st->top();
      st->pop();
      break; // while loop
    }
    m_stacks.pop();
  }
#else
  if (m_stack.size()) {
    n = m_stack.top();
    m_stack.pop();
  }
#endif
  return n;
}


bool BranchAndBound::doExpand(SearchNode* node) {

  assert(node);
  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int depth = ptnode->getDepth();

#ifdef ANYTIME
  MyStack* stack = m_stacks.front();
#endif

  if (node->getType() == NODE_AND) { /*******************************************/

    ++m_nodesAND; // count node
    PAR_ONLY(node->setSubCount(1);)

    if (depth >= 0) { // ignores dummy nodes
      m_nodeProfile.at(depth) += 1; // count node as expanded
    }

#ifdef ANYTIME
    bool splitStack = false;
    if (ptnode->getChildren().size() > 1) splitStack = true;
#endif

    // create new OR children
    for (vector<PtNode*>::const_iterator it=ptnode->getChildren().begin();
        it!=ptnode->getChildren().end(); ++it)
    {
      int vChild = (*it)->getVar();

      SearchNodeOR* n = new SearchNodeOR(node, vChild);

#ifndef NO_HEURISTIC
      // Compute and set heuristic estimate
      heuristicOR(n);
#endif

      node->addChild(n);
#ifdef ANYTIME
      if (!splitStack) {
        stack->push(n);
      } else {
        MyStack* s = new MyStack( stack );
        s->push(n);
        m_stacks.push(s);
        stack->addChild();
      }
#else
      m_stack.push(n);
#endif

#ifdef DEBUG
      ostringstream ss;
      ss << '\t' << n << ": " << *n;
      if (n->isCachable())
        ss << " - Cache: " << (*it)->getCacheContext();
      ss << endl;
      myprint (ss.str());
#endif

#ifndef NO_HEURISTIC
      // store initial lower bound on subproblem (needed for master init.)
      PAR_ONLY( n->setInitialBound(lowerBound(n)) );
#endif

    } // for loop

    if (node->getChildren().size()) {
      /* nothing right now */
    } else { // no children
      node->setLeaf(); // -> terminal node
      node->setValue(ELEM_ONE);
      m_leafProfile.at(depth) += 1; // count leaf node
      PAR_ONLY( node->setSubLeaves(1) );
      return true;
    }

  } else { // NODE_OR /*********************************************************/

    ++m_nodesOR; // count node expansion

#ifndef NO_HEURISTIC
    // actually create new AND children
    double* heur = node->getHeurCache();
#endif

    vector<SearchNode*> newNodes;
    newNodes.reserve(m_problem->getDomainSize(var));

    for (val_t i=m_problem->getDomainSize(var)-1; i>=0; --i) {
#ifdef NO_HEURISTIC
      m_assignment[var] = i;
      // compute label value
      double d = ELEM_ONE;
      const list<Function*>& funs = m_pseudotree->getFunctions(var);
      for (list<Function*>::const_iterator it = funs.begin(); it != funs.end(); ++it)
        d OP_TIMESEQ (*it)->getValue(m_assignment);
      if (d == ELEM_ZERO)
        continue;
      SearchNodeAND* n = new SearchNodeAND(node, i, d);
#else
      // early pruning if heuristic is zero (since it's an upper bound)
      if (heur[2*i+1] == ELEM_ZERO) {
        m_leafProfile.at(depth) += 1;
        PAR_ONLY( node->addSubLeaves(1) );
        continue; // label=0 -> skip
      }
      SearchNodeAND* n = new SearchNodeAND(node, i, heur[2*i+1]); // uses cached label
      // set cached heur. value
      n->setHeur( heur[2*i] );
#endif
      // add node as successor
      node->addChild(n);
      // remember new node
      newNodes.push_back(n);
    }

#ifndef NO_HEURISTIC
    node->clearHeurCache(); // clear heur. cache of OR node
#endif

    if (newNodes.size()) {

#ifndef NO_HEURISTIC
      // sort new nodes by increasing heuristic value
      sort(newNodes.begin(), newNodes.end(), SearchNode::heurLess);
#endif

      // add new child nodes to stack (for MPE, highest heur. value will end up on top)
      for (vector<SearchNode*>::iterator it = newNodes.begin(); it!=newNodes.end(); ++it) {
#ifdef ANYTIME
        stack->push(*it);
#else
        m_stack.push(*it);
#endif
        DIAG( ostringstream ss; ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; myprint(ss.str()); )
      }
    } else { // all child AND nodes were pruned (counted as leafs above)
      node->setLeaf();
      node->setValue(ELEM_ZERO);
      return true;
    }

  } // if-else over node type

  return false; // default false

} // BranchAndBound::doExpand


void BranchAndBound::setInitialBound(double d) const {
  assert(m_space);

  m_space->root->setValue(d);

  return;
/*
  // clear existing dummy nodes (new ones will be created)
  if (m_space->root)
    delete m_space->root;

  // create a new pair of dummy nodes to store the bound into
  PseudotreeNode* ptroot = m_pseudotree->getRoot();
  SearchNode* nodeOR = new SearchNodeOR(NULL, ptroot->getVar() );
  nodeOR->setValue(d);
  SearchNode* nodeAND = new SearchNodeAND(nodeOR, 0, ELEM_ONE);
  nodeOR->addChild(nodeAND);

  // link new search root
  m_space->root = nodeOR;

  // now create pair of dummy nodes (with no value)
  nodeOR = new SearchNodeOR(nodeAND, ptroot->getVar());
  nodeAND->addChild(nodeOR);

  nodeAND = new SearchNodeAND(nodeOR, 0, ELEM_ONE);
  nodeOR->addChild(nodeAND);

  // the last OR node will contain the actual solution in the end,
  // thus linked as subproblemLocal
  m_space->subproblemLocal = nodeOR;

  // reset search stack
  this->resetSearch(nodeAND);
*/
}


#ifndef NO_ASSIGNMENT
void BranchAndBound::setInitialSolution(const vector<val_t>& tuple) const {
  assert(m_space);
  m_space->root->setOptAssig(tuple);
}
#endif


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

#ifdef ANYTIME
  m_rootStack = new MyStack(NULL);
  m_rootStack->push(next);
  m_stacks.push(m_rootStack);
  m_stackCount = 0;
#else
  m_stack.push(next);
#endif

}

