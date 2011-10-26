/*
 * BranchAndBound.cpp
 *
 *  Created on: Nov 4, 2008
 *      Author: lars
 */

#undef DEBUG

#include "BranchAndBound.h"

#if defined(PARALLEL_DYNAMIC) || defined(PARALLEL_STATIC)
#undef DEBUG
#endif

typedef PseudotreeNode PtNode;


SearchNode* BranchAndBound::nextNode() {
  SearchNode* n = NULL;
#ifdef ANYTIME_BREADTH
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
    } else if (m_stackLimit && m_stackCount++ == m_stackLimit) {
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

 #ifdef ANYTIME_DEPTH
  if (m_stackDive.size()) {
    n = m_stackDive.top();
    m_stackDive.pop();
  }
 #endif

  if (!n && m_stack.size()) {
    n = m_stack.top();
    m_stack.pop();
  }
#endif
  return n;
}


bool BranchAndBound::doExpand(SearchNode* n) {

  assert(n);
  vector<SearchNode*> chi;

#ifdef ANYTIME_BREADTH
  MyStack* stack = m_stacks.front();
#endif

  if (n->getType() == NODE_AND) { /////////////////////////////////////

    if (generateChildrenAND(n,chi))
      return true; // no children

#ifdef DEBUG
    ostringstream ss;
    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it)
      ss << '\t' << *it << ": " << *(*it) << endl;
    myprint (ss.str());
#endif


#if defined(ANYTIME_BREADTH)
    if (chi.size() == 1) { // no decomposition
      stack->push(chi.at(0));
    } else { // decomposition, split stacks
      // reverse iterator needed since new stacks are put in queue (and not depth-first stack)
      for (vector<SearchNode*>::reverse_iterator it=chi.rbegin(); it!=chi.rend(); ++it) {
        MyStack* s = new MyStack(stack);
        stack->addChild();
        s->push(*it);
        m_stacks.push(s);
      }
    }
#elif defined(ANYTIME_DEPTH)
    // reverse iterator needed since dive step reverses subproblem order
    for (vector<SearchNode*>::reverse_iterator it=chi.rbegin(); it!=chi.rend(); ++it)
      m_stackDive.push(*it);
#else
    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it)
      m_stack.push(*it);
#endif

  } else { // NODE_OR /////////////////////////////////////////////////

    if (generateChildrenOR(n,chi))
      return true; // no children

    for (vector<SearchNode*>::iterator it=chi.begin(); it!=chi.end(); ++it) {
#if defined(ANYTIME_BREADTH)
      stack->push(*it);
#else
      m_stack.push(*it);
#endif
      DIAG( ostringstream ss; ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; myprint(ss.str()); )
    } // for loop
    DIAG (ostringstream ss; ss << "\tGenerated " << n->getChildren().size() <<  " child AND nodes" << endl; myprint(ss.str()); )

#ifdef ANYTIME_DEPTH
    // pull last node from normal stack for dive
    m_stackDive.push(m_stack.top());
    m_stack.pop();
#endif

  } // if over node type //////////////////////////////////////////////

  return false; // default false

}


#if false
bool BranchAndBound::doExpand(SearchNode* node) {

  assert(node);
  int var = node->getVar();
  PseudotreeNode* ptnode = m_pseudotree->getNode(var);
  int depth = ptnode->getDepth();

#ifdef ANYTIME_BREADTH
  MyStack* stack = m_stacks.front();
#endif

  if (node->getType() == NODE_AND) { /*******************************************/

    ++m_space->nodesAND; // count node
    PAR_ONLY(node->setSubCount(1);)

    if (depth >= 0) { // ignores dummy nodes
      m_nodeProfile.at(depth) += 1; // count node as expanded
    }

#ifdef ANYTIME_BREADTH
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
#if defined(ANYTIME_BREADTH)
      if (!splitStack) {
        stack->push(n);
      } else {
        MyStack* s = new MyStack( stack );
        s->push(n);
        m_stacks.push(s);
        stack->addChild();
      }
#elif defined(ANYTIME_DEPTH)
      m_stackDive.push(n);
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

    ++m_space->nodesOR; // count node expansion

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
#if defined(ANYTIME_BREADTH)
        stack->push(*it);
#else
        m_stack.push(*it);
#endif
        DIAG( ostringstream ss; ss << '\t' << *it << ": " << *(*it) << " (l=" << (*it)->getLabel() << ")" << endl; myprint(ss.str()); )
      }
#ifdef ANYTIME_DEPTH
      // pull last node from stack for dive
      m_stackDive.push(m_stack.top());
      m_stack.pop();
#endif


    } else { // all child AND nodes were pruned (counted as leafs above)
      node->setLeaf();
      node->setValue(ELEM_ZERO);
      return true;
    }

  } // if-else over node type

  return false; // default false

} // BranchAndBound::doExpand
#endif

/*
void BranchAndBound::setInitialSolution(double d
#ifndef NO_ASSIGNMENT
    ,const vector<val_t>& tuple
#endif
  ) const {
  assert(m_space);
  m_space->root->setValue(d);
#ifdef NO_ASSIGNMENT
  m_problem->updateSolution(this->getCurOptValue(), make_pair(0,0), true);
#else
  m_space->root->setOptAssig(tuple);
  m_problem->updateSolution(this->getCurOptValue(), this->getCurOptTuple(), make_pair(0,0), true);
#endif
}
*/

BranchAndBound::BranchAndBound(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) :
   Search(prob,pt,space,heur) {

#ifndef NO_CACHING
  // Init context cache table
  if (!m_space->cache)
    m_space->cache = new CacheTable(prob->getN());
#endif

#ifdef ANYTIME_BREADTH
  m_stackLimit = m_space->options->stackLimit;  // TODO
#endif

  SearchNode* first = this->initSearch();
  if (first) {
#ifdef ANYTIME_BREADTH
    m_rootStack = new MyStack(NULL);
    m_rootStack->push(first);
    m_stacks.push(m_rootStack);
    m_stackCount = 0;
#else
    m_stack.push(first);
#endif
  }
}

