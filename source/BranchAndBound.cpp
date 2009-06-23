/*
 * BranchAndBound.cpp
 *
 *  Created on: Nov 4, 2008
 *      Author: lars
 */

#include "BranchAndBound.h"

#define DISABLE_CACHING
//#define DISABLE_PRUNING


#ifdef USE_THREADS
#undef DEBUG
#endif

#undef DEBUG

typedef PseudotreeNode PtNode;

#ifdef USE_THREADS
void BranchAndBound::operator ()() {

  try { while (!m_stack.empty()) {


    { // allowedThreads says how many more subproblems can be processed in parallel
      GETLOCK( m_space->mtx_allowedThreads , lk);
      while (!m_space->allowedThreads) {
        CONDWAIT(m_space->cond_allowedThreads,lk);
      }
      --m_space->allowedThreads;
    }
    // new processing thread can now be started
    {
      GETLOCK(m_space->mtx_space, lk); // lock the search space

      while (!m_nextLeaf) {
        expandNext();
      }

      if (m_nextLeaf->isLeaf()) {
        // create new process for leaf node processing (=subproblem is solved already)
        // TODO move node straight to solved queue?
        SubproblemStraight subprob(m_space,m_nextLeaf);
        {
          GETLOCK(m_space->mtx_activeThreads, lk2);
          m_space->activeThreads.insert( // store process info in shared table
              make_pair( m_nextLeaf , new boost::thread(subprob) )  );
        }
      } else {
        // Create new process that 'outsources' subproblem solving, collects the
        // results and feeds it back into the searhc space
        bigint sz = m_pseudotree->getNode(m_nextLeaf->getVar())->getSubsize();
        SubproblemCondor subprob(m_space,m_nextLeaf,m_nextThreadId++,sz);
        {
          GETLOCK(m_space->mtx_activeThreads, lk2);
          m_space->activeThreads.insert(
              make_pair( m_nextLeaf , new boost::thread(subprob) )  );
        }
      }
      m_nextLeaf = NULL; // reset next leaf pointer

      // mark search as done if needed (before releasing the search space)
      if (m_stack.empty()) {
        GETLOCK(m_space->mtx_searchDone, lk2);
        m_space->searchDone = true;
      }

    }
  } // end of while loop

  myprint("\n!BAB done!\n\n");

  } catch (boost::thread_interrupted i) {
    myprint("\n!BAB aborted!\n");
  }


}
#endif


bool BranchAndBound::canBePruned(SearchNode* n) const {

  SearchNode* curAND;// = n;
  SearchNode* curOR;// = n->getParent(); // root of current partial solution subtree
  double curPSTVal;// = curAND->getHeur(); // includes label

  if (n->getType() == NODE_AND) {
    curAND = n;
    curOR = n->getParent();
    curPSTVal = curAND->getHeur();
  } else { // NODE_OR
    curAND = NULL;
    curOR = n;
    curPSTVal = curOR->getHeur();
  }

#ifdef DEBUG
  {
    GETLOCK(mtx_io, lk);
    cout << "\tcanBePruned(" << *n << ")"
         << " h=" << n->getHeur() << endl;
  }
#endif

  list<SearchNode*> notOptOR; // marks nodes to tag as possibily not optimal

  // up to root node, if we have to
  while (true) {
#ifdef DEBUG
    {
      GETLOCK(mtx_io, lk);
      cout << "\tPST root: " << *curOR
      << " pst=" << curPSTVal << " v=" << curOR->getValue() << endl;
    }
#endif

/*
#ifdef USE_LOG
    if (curOR->getValue() == -INFINITY)
#else
    if (curOR->getValue() == 0.0)
#endif
      return false; // OR node has value 0, no pruning possible
*/

//    if ( fpLt(curPSTVal, curOR->getValue()) ) {
    if ( curPSTVal <= curOR->getValue() ) {
      for (list<SearchNode*>::iterator it=notOptOR.begin(); it!=notOptOR.end(); ++it)
        (*it)->setNotOpt(); // mark as possibly not optimal
      return true; // pruning is possible!
    }
//    else
//      return false;

    // check if moving up in search space is possible
    if (! curOR->getParent())
      return false;

    notOptOR.push_back(curOR);

    // climb up, update values
    curAND = curOR->getParent();
#ifdef USE_LOG
    curPSTVal += curAND->getLabel();
#else
    curPSTVal *= curAND->getLabel();
#endif

    const CHILDLIST& children = curAND->getChildren();

    // incorporate new sibling OR nodes
    for (CHILDLIST::const_iterator it=children.begin(); it!=children.end(); ++it) {
      if (*it == curOR) continue; // skip previous or node
#ifdef USE_LOG
      else curPSTVal += (*it)->getHeur();
#else
      else curPSTVal *= (*it)->getHeur();
#endif
    }

    curOR = curAND->getParent();
  }

  // default
  return false;

}


double BranchAndBound::heuristicOR(SearchNode* n) {

  int v = n->getVar();
  double d;
  double* dv = new double[m_problem->getDomainSize(v)*2];
#ifdef USE_LOG
  double h = -INFINITY; // the new OR nodes h value
#else
  double h = 0.0; // the new OR nodes h value
#endif
  for (val_t i=0;i<m_problem->getDomainSize(v);++i) {
    m_assignment[v] = i;
    // compute heuristic value
    dv[2*i] = m_heuristic->getHeur(v,m_assignment);
    // precompute label value

#ifdef USE_LOG
    d = 0.0; // = log(1)
#else
    d = 1.0;
#endif
    const list<Function*>& funs = m_pseudotree->getFunctions(v);
    for (list<Function*>::const_iterator it = funs.begin(); it != funs.end(); ++it)
    {
#ifdef USE_LOG
      d += (*it)->getValue(m_assignment);
#else
      d *= (*it)->getValue(m_assignment);
#endif
    }
    dv[2*i+1] = d; // label
#ifdef USE_LOG
    dv[2*i] += d; // heuristic
#else
    dv[2*i] *= d; // heuristic
#endif

    if (dv[2*i] > h) h = dv[2*i]; // keep max. for OR node heuristic
  }

  n->setHeur(h);
  n->setHeurCache(dv);

/*
#ifdef DEBUG
  {
    GETLOCK(mtx_io,lk);
    cout << "   Precomputed " << m_problem->getDomainSize(v) << " values" << endl;
  }
#endif
*/

  return h;
}


void BranchAndBound::expandNext() {

//  myprint(".");

  SearchNode* node = m_stack.top();
  m_stack.pop();

  int var = node->getVar();
  // get the node's pseudo tree equivalent
  PtNode* ptnode = m_pseudotree->getNode(var);

  if (node->getType() == NODE_AND) { //===================== AND NODE =============================

    val_t val = node->getVal();

    // keep track of assignment
    m_assignment[var] = val;

#ifdef DEBUG
    {
      GETLOCK(mtx_io, lk);
      cout << *node << " (l=" << node->getLabel() << ") Assig. " << m_assignment << endl;
    }
#endif

    // dead end (label = 0)?
#ifdef USE_LOG
    if (node->getLabel() == -INFINITY) {
#else
    if (node->getLabel() == 0.0) {
#endif
      node->setLeaf(); // mark as leaf
      m_nextLeaf = node;
      return; // and abort
    }

#ifndef DISABLE_PRUNING
    // check heuristic for pruning
    if (canBePruned(node)) {
#ifdef DEBUG
    {
      GETLOCK(mtx_io, lk);
      cout << "\t !pruning " << endl;
    }
#endif
      node->setLeaf();
      node->setPruned();
#ifdef USE_LOG
      node->setValue(-INFINITY);
#else
      node->setValue(0.0);
#endif
      m_nextLeaf = node;
      return;
    }
#endif


#ifndef NO_CACHING
    // Reset adaptive cache tables if required
    {
      const list<int>& resetList = ptnode->getCacheResetList();
      for (list<int>::const_iterator it=resetList.begin(); it!=resetList.end(); ++it)
        m_space->cache->reset(*it);
    }
#endif


    // create new OR children
    for (vector<PtNode*>::const_iterator it=ptnode->getChildren().begin();
        it!=ptnode->getChildren().end(); ++it)
    {
      int v = (*it)->getVar();
      SearchNodeOR* n = new SearchNodeOR(node, v);

      // Compute and set heuristic estimate
      heuristicOR(n);

      node->addChild(n);
      m_stack.push(n);
#ifdef DEBUG
      {
        GETLOCK(mtx_io,lk);
        cout << '\t' << *n;
        if (n->isCachable())
          cout << " - Cache: " << n->getCacheContext();
        cout << endl;
      }
#endif
    }

    if (!node->getChildren().size()) { // no children?
      m_nextLeaf = node;
      node->setLeaf(); // -> terminal node
    }

    ++m_nodesAND;

  } else { // if (node->getType() == NODE_OR) { //====================== OR NODE =======================

#ifdef DEBUG
    {
      GETLOCK(mtx_io,lk);
      cout << *node << endl;
    }
#endif


#ifndef NO_CACHING
    // Caching for this node
    if (ptnode->getFullContext().size() <= ptnode->getParent()->getFullContext().size()) {
      addCacheContext(node,ptnode->getCacheContext());
#ifdef DEBUG
      {
        GETLOCK(mtx_io, lk);
        cout << "    Context set: " << node->getCacheContext() << endl;
      }
#endif
      // try to get value from cache
      try {
        // will throw int(UNKNOWN) if not found
        double v = m_space->cache->read(var, node->getCacheInst(), node->getCacheContext());
        node->setValue( v );
        node->setLeaf();
        m_nextLeaf = node;
        node->clearHeurCache(); // clear precomputed heur./labels
        return;
      } catch (...) { // cache lookup failed
        node->setCachable(); // mark for caching later
      }
    }
#endif


#ifndef DISABLE_PRUNING
    // Pruning
    if (canBePruned(node)) {
#ifdef DEBUG
    {
      GETLOCK(mtx_io, lk);
      cout << "\t !pruning " << endl;
    }
#endif
      node->setLeaf();
      node->setPruned();
#ifdef USE_LOG
      node->setValue(-INFINITY);
#else
      node->setValue(0.0);
#endif
      m_nextLeaf = node;
      node->clearHeurCache();
      return;
    }
#endif


#ifdef USE_THREADS
    // TODO currently outsource at fixed depth
//    if (ptnode->getDepth() == m_space->options->cutoff_depth) {
    if ( ptnode->getDepth() == m_space->options->cutoff_depth ||
        mylog10( ptnode->getSubsize() ) <= m_space->options->cutoff_size) {
//    if (ptnode->getSubwidth() == m_space->options->cutoff_depth) {

      {
        GETLOCK(mtx_io, lkio);
        cout << "** Parallelising at depth " << ptnode->getDepth() << endl;
      }

      addSubprobContext(node,ptnode->getFullContext()); // set subproblem context
      addPSTlist(node); // to allow pruning in the worker
      node->setExtern();
      node->clearHeurCache(); // clear from cache
      m_nextLeaf = node;
      return;
    }
#endif

    /////////////////////////////////////
    // actually create new AND children
    double* heur = node->getHeurCache();

    for (val_t i=m_problem->getDomainSize(var)-1; i>=0; --i) {
      SearchNodeAND* n = new SearchNodeAND(node, i);

      // get cached heur. value
      n->setHeur( heur[2*i] );
      // get cached label value
      n->setLabel( heur[2*i+1] );

      // add node as successor
      node->addChild(n);
//      m_stack.push(n);
#ifdef DEBUG
      {
        GETLOCK(mtx_io,lk);
        cout << '\t' << *n << " (l=" << n->getLabel() << ") Assig. " << m_assignment << endl;
      }
#endif

    }

    // add new child nodes to stack (CHILDLIST implies order)
    for (CHILDLIST::const_iterator it=node->getChildren().begin(); it!=node->getChildren().end(); ++it)
      m_stack.push(*it);

    node->clearHeurCache(); // clear heur. cache of OR node

    ++m_nodesOR;

  } //=========================================================== END =============================

}

#ifndef NO_CACHING
void BranchAndBound::addCacheContext(SearchNode* node, const set<int>& ctxt) const {

#if true
  context_t* sig = new context_t();
  sig->reserve(ctxt.size());
  for (set<int>::const_iterator itC=ctxt.begin(); itC!=ctxt.end(); ++itC) {
    sig->push_back(m_assignment[*itC]);
  }
#else
  size_t intsz = sizeof(int);
  size_t ln = ctxt.size() * intsz;

  context_t* sig = new context_t(ln,'\0');

  size_t idx=0;
  int v = UNKNOWN;

  for (set<int>::const_iterator itC=ctxt.begin(); itC!=ctxt.end(); ++itC) {
    v = m_assignment[*itC];
    memcpy(& sig->at(idx), &v, intsz);
    idx += intsz;
  }
#endif

  node->setCacheContext(sig);

#if false

  ostringstream ss(ios::binary | ios::out);
//  ss << node->getVar();
  int v = UNKNOWN;
  for (set<int>::const_iterator itC=ctxt.begin(); itC!=ctxt.end(); ++itC) {
    v = m_assignment[*itC];
    BINWRITE( ss , v );
//    ss << ' ' << (*itC) << ' ' << v;
  }
  // Set search node's cache context and instance counter
  node->setCacheContext(ss.str());
#endif

  node->setCacheInst( m_space->cache->getInstCounter(node->getVar()) );

}
#endif


#ifdef USE_THREADS
void BranchAndBound::addSubprobContext(SearchNode* node, const set<int>& ctxt) const {

  context_t* sig = new context_t();
  sig->reserve(ctxt.size());
  for (set<int>::const_iterator itC=ctxt.begin(); itC!=ctxt.end(); ++itC) {
    sig->push_back(m_assignment[*itC]);
  }

#if false
  size_t intsz = sizeof(int);
  size_t ln = ctxt.size() * intsz;

  context_t* sig = new context_t(ln,'\0');

  size_t idx = 0;
  int v = UNKNOWN;

  for (set<int>::const_iterator itC=ctxt.begin(); itC!=ctxt.end(); ++itC) {
    v = m_assignment[*itC];
    memcpy(& sig->at(idx), &v, intsz);
    idx += intsz;
  }
#endif

  node->setSubprobContext(sig);

#if false
  ostringstream ss(ios::binary | ios::out); // binary write mode
  int v = node->getVar();
  BINWRITE( ss, v );
  v = ctxt.size();
  BINWRITE( ss, v );
  for (set<int>::const_iterator itC=ctxt.begin(); itC!=ctxt.end(); ++itC) {
    v = m_assignment[*itC]; BINWRITE( ss, v );
  }
  node->setSubprobContext(ss.str());
#endif
}
#endif


#ifdef USE_THREADS
void BranchAndBound::addPSTlist(SearchNode* node) const {

  // assume OR node
  assert(node->getType() == NODE_OR);

  SearchNode* curAND = NULL;
  SearchNode* curOR = node;

  double d = 0.0;

  while (true) {

    curAND = curOR->getParent();

    d = curAND->getLabel();

    // incorporate new sibling OR nodes
    const CHILDLIST& children = curAND->getChildren();
    for (CHILDLIST::const_iterator it=children.begin(); it!=children.end(); ++it) {
      if (*it != curOR) // skip previous or node
#ifdef USE_LOG
        d += (*it)->getHeur();
#else
        d *= (*it)->getHeur();
#endif
    }

    curOR = curAND->getParent();

    // add PST tuple to list as (OR value , AND label + children heur.)
    node->addPSTVal( make_pair(curOR->getValue(),d) );

    // search tree root reached?
    if (!curOR->getParent()) break;
  }

}
#endif


#ifndef USE_THREADS
SearchNode* BranchAndBound::nextLeaf() {

//  assert(!m_stack.empty());

  while (!m_nextLeaf) { // next leaf node will be set by expandNext()
    expandNext();
  }

//  myprint("L");

  SearchNode* n = m_nextLeaf;
  m_nextLeaf = NULL;

  return n;
}
#endif


void BranchAndBound::restrictSubproblem(const string& file) {
  assert(!file.empty());

  {
    ifstream inTemp(file.c_str());
    inTemp.close();

    if (inTemp.fail()) {
      cerr << "Error opening subproblem specification " << file << '.' << endl;
      exit(1);
    }
  }

//  string line;
  igzstream fs(file.c_str(), ios::in | ios::binary);

  int rootVar = UNKNOWN;

  //////////////////////////////////////
  // FIRST: read context of subproblem

  // root variable of subproblem
  BINREAD(fs, rootVar);
  if (rootVar<0 || rootVar>= m_problem->getN()) {
    cerr << "Error reading subproblem specification, variable index " << rootVar << " out of range" << endl;
    exit(1);
  }

  // re-adjust the root of the pseudo tree
  m_pseudotree->restrictSubproblem(rootVar);

  int x = UNKNOWN;
  // context size
  BINREAD(fs, x);
  const set<int>& context = m_pseudotree->getNode(rootVar)->getFullContext();
  if (x != (int) context.size()) {
    cerr << "Error reading subproblem specification, context size doesn't match." << endl;
    exit(1);
  }
  val_t y = UNKNOWN;
  for (set<int>::const_iterator it = context.begin(); it!=context.end(); ++it) {
    BINREAD(fs, y);
    if (y<0 || y>=m_problem->getDomainSize(*it)) {
      cerr << "Error reading subproblem specification, variable value " << (int)y << " not in domain." << endl;
      exit(1);
    }
    m_assignment[*it] = y;
  }

  ///////////////////////////////////////////////////////////////////////////
  // SECOND: read information about partial solution tree in parent problem

  // read encoded PST values for advanced pruning,
  // builds dummy node structure with proper values/labels top-down.
  double d1, d2;

  SearchNode* next = m_stack.top(); // dummy AND node on top of stack
  SearchNode* node = NULL;

  int pstSize = UNKNOWN;
  BINREAD(fs,pstSize);

  while (pstSize--) {
    BINREAD(fs, d1);
    BINREAD(fs, d2);

    // add dummy OR node with proper value
    node = next;
    next = new SearchNodeOR(node,node->getVar());
    next->setValue(d1);
//    cout << " Added dummy OR node with value " << d1 << endl;
    node->addChild(next);

    node = next;
    next = new SearchNodeAND(node,0);
    next->setLabel(d2);
//    cout << " Added dummy AND node with label " << d2 << endl;
    node->addChild(next);

  }

  // create another dummy node as a buffer for the subproblem value
  // since the previous dummy nodes might have non-empty labels from
  // from the parent problem)
  node = next;
  next = new SearchNodeOR(node,node->getVar());
  node->addChild(next);
  m_space->subproblem = next;
  node = next;
  next = new SearchNodeAND(node,0);
  node->addChild(next);

  // remove old dummy node from stack
  m_stack.pop();
  // replace with new
  m_stack.push(next);


  fs.close();

}


// Constructor
BranchAndBound::BranchAndBound(Problem* prob, Pseudotree* pt, SearchSpace* space, Heuristic* heur) :
    Search(prob,pt,space,heur), m_nodesOR(0), m_nodesAND(0), m_nextThreadId(0), m_nextLeaf(NULL) {
  // Init context cache table
#ifndef NO_CACHING
  cout << "Initialising cache system." << endl;
  space->cache = new CacheTable(prob->getN(), m_space->options->memlimit);
#endif

  // create first node (dummy variable)
  PseudotreeNode* ptroot = m_pseudotree->getRoot();
  SearchNode* node = new SearchNodeOR(NULL, ptroot->getVar() );
  m_space->root = node;
  // create dummy variable's AND node (domain size 1)
  SearchNode* next = new SearchNodeAND(m_space->root,0);
/*
#ifdef USE_LOG
  node->setLabel(0.0);
#else
  node->setLabel(1.0);
#endif
*/
  m_space->root->addChild(next);

  /*
  for (int i=100; i; --i) {
    node = next;
    next = new SearchNodeOR(node, ptroot->getVar() );
    node->addChild(next);

    node = next;
    next = new SearchNodeAND(node,0);
    node->addChild(next);
  }
  */

  m_stack.push(next);
  // initialize the assignment vector
  m_assignment.resize(m_problem->getN(),NONE);
}


