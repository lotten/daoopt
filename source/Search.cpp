/*
 * Search.cpp
 *
 *  Created on: Sep 22, 2009
 *      Author: lars
 */

#include "Search.h"
#include "ProgramOptions.h"



Search::Search(Problem* prob, Pseudotree* pt, SearchSpace* s, Heuristic* h) :
  m_nodesOR(0), m_nodesAND(0), m_nextThreadId(0), m_problem(prob),
  m_pseudotree(pt), m_space(s), m_heuristic(h), m_nextLeaf(NULL) {

#ifndef NO_CACHING
  // Init context cache table
  cout << "Initialising cache system." << endl;
  m_space->cache = new CacheTable(prob->getN(), m_space->options->memlimit);
#endif

  // initialize the local assignment vector for BaB
  m_assignment.resize(m_problem->getN(),NONE);
}



#ifdef PARALLEL_MODE
void Search::operator ()() {

  try { while ( this->hasMoreNodes() ) {


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
      if (! this->hasMoreNodes() ) {
        GETLOCK(m_space->mtx_searchDone, lk2);
        m_space->searchDone = true;
      }

    }
  } // end of while loop

  myprint("\n!Search done!\n\n");

  } catch (boost::thread_interrupted i) {
    myprint("\n!Search aborted!\n");
  }


}
#endif


#ifndef PARALLEL_MODE
SearchNode* Search::nextLeaf() {

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



bool Search::canBePruned(SearchNode* n) const {

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
    if (curOR->getValue() == ELEM_ONE)
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
    curPSTVal OP_TIMESEQ curAND->getLabel();

    const CHILDLIST& children = curAND->getChildren();

    // incorporate new sibling OR nodes
    for (CHILDLIST::const_iterator it=children.begin(); it!=children.end(); ++it) {
      if (*it == curOR) continue; // skip previous or node
      else curPSTVal OP_TIMESEQ (*it)->getHeur();
    }

    curOR = curAND->getParent();
  }

  // default
  return false;

}



double Search::heuristicOR(SearchNode* n) {

  int v = n->getVar();
  double d;
  double* dv = new double[m_problem->getDomainSize(v)*2];
  double h = ELEM_ZERO; // the new OR nodes h value
  for (val_t i=0;i<m_problem->getDomainSize(v);++i) {
    m_assignment[v] = i;
    // compute heuristic value
    dv[2*i] = m_heuristic->getHeur(v,m_assignment);
    // precompute label value

    d = ELEM_ONE;
    const list<Function*>& funs = m_pseudotree->getFunctions(v);
    for (list<Function*>::const_iterator it = funs.begin(); it != funs.end(); ++it)
    {
      d OP_TIMESEQ (*it)->getValue(m_assignment);
    }
    dv[2*i+1] = d; // label
    dv[2*i] OP_TIMESEQ d; // heuristic

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


#ifndef NO_CACHING
void Search::addCacheContext(SearchNode* node, const set<int>& ctxt) const {

#if true
  context_t sig;
  sig.reserve(ctxt.size());
  for (set<int>::const_iterator itC=ctxt.begin(); itC!=ctxt.end(); ++itC) {
    sig.push_back(m_assignment[*itC]);
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


#ifdef PARALLEL_MODE
void Search::addSubprobContext(SearchNode* node, const set<int>& ctxt) const {

  context_t sig;
  sig.reserve(ctxt.size());
  for (set<int>::const_iterator itC=ctxt.begin(); itC!=ctxt.end(); ++itC) {
    sig.push_back(m_assignment[*itC]);
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


#ifdef PARALLEL_MODE
void Search::addPSTlist(SearchNode* node) const {

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
        d OP_TIMESEQ (*it)->getHeur();
    }

    curOR = curAND->getParent();

    // add PST tuple to list as (OR value , AND label + children heur.)
    node->addPSTVal( make_pair(curOR->getValue(),d) );

    // search tree root reached?
    if (!curOR->getParent()) break;
  }

}
#endif




void Search::restrictSubproblem(const string& file) {
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

  SearchNode* next = this->firstNode(); // dummy AND node on top of stack
  SearchNode* node = NULL;

  int pstSize = UNKNOWN;
  BINREAD(fs,pstSize);

  while (pstSize--) {
    BINREAD(fs, d1);
    BINREAD(fs, d2);

    // add dummy OR node with proper value
    node = next;
    next = new SearchNodeOR(node, node->getVar()) ;
    next->setValue(d1);
//    cout << " Added dummy OR node with value " << d1 << endl;
    node->addChild(next);

    node = next;
    next = new SearchNodeAND(node, 0) ;
    next->setLabel(d2);
//    cout << " Added dummy AND node with label " << d2 << endl;
    node->addChild(next);

  }

  fs.close();

  // create another dummy node as a buffer for the subproblem value
  // since the previous dummy nodes might have non-empty labels from
  // the parent problem)
  node = next;
  next = new SearchNodeOR(node, node->getVar());
  node->addChild(next);
  m_space->subproblem = next;
  node = next;
  next = new SearchNodeAND(node, 0);
  node->addChild(next);

  // remove old dummy node from queue/stack/etc. and replace with new one
  this->resetSearch(next);

}


