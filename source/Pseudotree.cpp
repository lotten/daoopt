/*
 * Pseudotree.cpp
 *
 *  Created on: Oct 24, 2008
 *      Author: lars
 */

#include "Pseudotree.h"

#undef DEBUG

int Pseudotree::restrictSubproblem(int i) {

  assert(m_root && i<(int)m_nodes.size() && m_nodes[i]);

  int rootOldDepth = m_nodes[i]->getDepth();

  m_root->setChild(m_nodes[i]);
  m_nodes[i]->setParent(m_root);

  // recompute subproblem variables (recursive)
  m_root->updateSubprobVars();
  m_sizeConditioned = m_root->getSubprobSize()-1; // -1 for bogus

  // update subproblem height, substract -1 for bogus node
  m_heightConditioned = m_root->updateDepth(-1) - 1;

  // compute subproblem width (when conditioning on subproblem context)
  const set<int>& condset = m_nodes[i]->getFullContext();
  stack<PseudotreeNode*> stck;
  stck.push(m_nodes[i]);
  while (stck.size()) {
    PseudotreeNode* n = stck.top();
    stck.pop();
    int x = setminusSize(n->getFullContext(), condset);
    m_widthConditioned = max(m_widthConditioned,x);
    for (vector<PseudotreeNode*>:: const_iterator it=n->getChildren().begin(); it!=n->getChildren().end(); ++it) {
      stck.push(*it);
    }
  }

  return rootOldDepth;

}

void Pseudotree::addFunctionInfo(const vector<Function*>& fns) {
  for (vector<Function*>::const_iterator itF=fns.begin(); itF!=fns.end(); ++itF) {
    const set<int>& scope = (*itF)->getScope();
    vector<int>::const_iterator it = m_elimOrder.begin();
    for (;;++it) {
      if (scope.find(*it)!=scope.end()) {
        m_nodes[*it]->addFunction(*itF);
        break;
      }
    }
  }
}


/* computes an elimination order into 'elim' and returns its tree width */
int Pseudotree::eliminate(Graph G, vector<int>& elim) {

  int width = UNKNOWN;
  int n = G.getStatNodes();

  if (elim.size())
    elim.clear();
  elim.reserve(n);

  vector<double> scores(n);

  const set<int>& nodes = G.getNodes();

  for (set<int>::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
    scores[*it] = G.scoreMinfill(*it);
  }

  // keeps track of the roots of the separate sub pseudo trees
  // that exist throughout the process
  while (G.getStatNodes() != 0) {

    int minNode = UNKNOWN;
    double minScore = numeric_limits<double>::max();
    // keeps track of minimal score nodes
    vector<int> minCand; // minimal score of 1 or higher
    vector<int> simplicial; // simplicial nodes (score 0)

    // find node to eliminate
    for (int i=0; i<n; ++i) {
      if (scores[i] == 0) { // score 0
        simplicial.push_back(i);
      } else if (scores[i] < minScore) { // new, lower score (but greater 0)
        minScore = scores[i];
        minCand.clear();
        minCand.push_back(i);
      } else if (scores[i] == minScore) { // current min. found again
        minCand.push_back(i);
      }
    }

    // eliminate all nodes with score=0 -> no edges will have to be added
    for (vector<int>::iterator it=simplicial.begin(); it!=simplicial.end(); ++it) {
      elim.push_back(*it);
      width = max(width,(int)G.getNeighbors(*it).size());
      G.removeNode(*it);
      scores[*it] = DBL_MAX;
    }

    // anything left to eliminate? If not, we are done!
    if (minScore == numeric_limits<double>::max())
      return width;

    // Pick one of the minimal score nodes (with score >= 1),
    // breaking ties randomly
    minNode = minCand[rand::next(minCand.size())];
    elim.push_back(minNode);

    // remember it's neighbors, to be used later
    const set<int>& neighbors = G.getNeighbors(minNode);

    // update width of implied tree decomposition
    width = max(width, (int) neighbors.size());

    // connect neighbors in primal graph
    G.addClique(neighbors);

    // compute candidates for score update (node's neighbors and their neighbors)
    set<int> updateCand(neighbors);
    for (set<int>::const_iterator it = neighbors.begin(); it != neighbors.end(); ++it) {
      const set<int>& X = G.getNeighbors(*it);
      updateCand.insert(X.begin(),X.end());
    }
    updateCand.erase(minNode);

    // remove node from primal graph
    G.removeNode(minNode);
    scores[minNode] = DBL_MAX; // tag score

    // update scores in primal graph (candidate nodes computed earlier)
    for (set<int>::const_iterator it = updateCand.begin(); it != updateCand.end(); ++it) {
      scores[*it] = G.scoreMinfill(*it); // TODO generalize
    }
  }

  return width;

}

/* builds the pseudo tree according to order 'elim' */
void Pseudotree::build(Graph G, const vector<int>& elim, const int cachelimit) {

  const int n = G.getStatNodes();
  assert(n == (int) m_nodes.size());
  assert(n == (int) elim.size());

  if (m_height != UNKNOWN)
    this->reset();

  list<PseudotreeNode*> roots;

  // build actual pseudo tree(s)
  for (vector<int>::const_iterator it = elim.begin(); it!=elim.end(); ++it) {
    const set<int>& N = G.getNeighbors(*it);
    m_width = max(m_width,(int)N.size());
    insertNewNode(*it,N,roots);
    G.addClique(N);
    G.removeNode(*it);
  }

  // compute contexts for adaptive caching
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!=m_nodes.end(); ++it) {
    const set<int>& ctxt = (*it)->getFullContext();
    if (cachelimit == NONE || cachelimit >= (int) ctxt.size()) {
      (*it)->setCacheContext(ctxt);
      continue; // proceed to next node
    }
    int j = cachelimit;
    PseudotreeNode* p = (*it)->getParent();
    while (j--) { // note: cachelimit < ctxt.size()
      while (ctxt.find(p->getVar()) == ctxt.end() )
        p = p->getParent();
      (*it)->addCacheContext(p->getVar());
      p = p->getParent();
    }
    // find reset variable for current node's cache table
    while (ctxt.find(p->getVar()) == ctxt.end() )
      p = p->getParent();
    p->addCacheReset((*it)->getVar());
//    cout << "AC for var. " << (*it)->getVar() << " context " << (*it)->getCacheContext()
//         << " out of " << (*it)->getFullContext() << ", reset at " << p->getVar() << endl;
  }

  // add artificial root node to connect disconnected components
  int bogusIdx = elim.size();
  PseudotreeNode* p = new PseudotreeNode(this,bogusIdx,set<int>());
  //p->addToContext(bogusIdx);
  for (list<PseudotreeNode*>::iterator it=roots.begin(); it!=roots.end(); ++it) {
    p->addChild(*it);
    (*it)->setParent(p);
    ++m_components; // increase component count
  }
  m_nodes.push_back(p);
  m_root = p;

  // remember the elim. order
  m_elimOrder = elim;
  m_elimOrder.push_back(bogusIdx); // add dummy variable as first node in ordering

  // initiate depth/height computation for tree and its nodes (bogus variable
  // gets depth -1), then need to subtract 1 from height for bogus as well
  m_height = m_root->updateDepth(-1) - 1;

  // initiate subproblem variables computation (recursive)
  m_root->updateSubprobVars();
  m_size = m_root->getSubprobSize() -1; // -1 for bogus

#ifdef PARALLEL_MODE
  // compute depth->list<nodes> mapping
  m_levels.resize(m_height+2); // +2 bco. bogus node
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!= m_nodes.end(); ++it) {
    m_levels.at((*it)->getDepth()+1).push_back(*it);
  }
#endif

  return;
//  cout << "Number of disconnected sub pseudo trees: " << roots.size() << endl;

}


Pseudotree::Pseudotree(const Pseudotree& pt) {

  m_problem = pt.m_problem;
  m_size = pt.m_size;

  m_nodes.reserve(m_size+1);
  m_nodes.resize(m_size+1, NULL);

  m_elimOrder = pt.m_elimOrder;

  // clone PseudotreeNode structure
  stack<PseudotreeNode*> stack;

  PseudotreeNode *ptnNew = NULL, *ptnPar = NULL;
  // start with bogus variable, always has highest index
  m_nodes.at(m_size) = new PseudotreeNode(this,m_size,set<int>());
  m_root = m_nodes.at(m_size);
  stack.push(m_root);

  while (!stack.empty()) {
    ptnPar = stack.top();
    stack.pop();
    int var = ptnPar->getVar();
    const vector<PseudotreeNode*>& childOrg = pt.m_nodes.at(var)->getChildren();
    for (vector<PseudotreeNode*>::const_iterator it=childOrg.begin(); it!=childOrg.end(); ++it) {
       ptnNew = new PseudotreeNode(this, (*it)->getVar(), (*it)->getFullContext());
       m_nodes.at((*it)->getVar()) = ptnNew;
       ptnPar->addChild(ptnNew);
       ptnNew->setParent(ptnPar);
       ptnNew->setCacheContext( (*it)->getCacheContext() );
       ptnNew->setCacheReset( (*it)->getCacheReset() );
       ptnNew->setFunctions( (*it)->getFunctions() );

       stack.push(ptnNew);
    }
  }

  m_height = m_root->updateDepth(-1) - 1;
  m_root->updateSubprobVars();
  m_size = m_root->getSubprobSize() -1; // -1 for bogus

  m_heightConditioned = pt.m_heightConditioned;
  m_width = pt.m_width;
  m_widthConditioned = pt.m_widthConditioned;
  m_components = pt.m_components;
  m_sizeConditioned = pt.m_sizeConditioned;

#ifdef PARALLEL_MODE
  // compute depth->list<nodes> mapping
  m_levels.resize(m_height+2); // +2 bco. bogus node
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!= m_nodes.end(); ++it) {
    if (*it) m_levels.at((*it)->getDepth()+1).push_back(*it);
  }
#endif

}



#ifdef PARALLEL_MODE
/* a-priori computation of several complexity estimates, outputs various
 * results for increasing depth-based cutoff. Returns suggested optimal
 * cutoff depth (bad choice in practice) */
int Pseudotree::computeComplexities(int workers) {

  // compute subproblem complexity parameters of all nodes
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!= m_nodes.end(); ++it)
    (*it)->initSubproblemComplexity();

  bigint c = m_root->getSubsize();

//  cout << "Complexity bound:\t" << mylog10(c) << " (" << c << ")" << endl;

  /*
   * iterate over levels, assuming cutoff, and compute upper bounds as follows:
   *   central - number of nodes explored by master down to this level
   *   tmax - max. subproblem size when conditioning at this level
   *   tsum - sum of all conditioned subproblems
   *   tnum - number of subproblems resulting from conditioning at this level
   *   bound - bounding expression reflecting parallelism over workers
   *   wmax - max. induced width of conditioned subproblems at this level
   */
  bigint central = 0, tmax=0, tsum=0, tnum=0, bound=0, size=0;
  int wmax = 0;
  vector<bigint> bounds; bounds.reserve(m_levels.size());
  for (vector<list<PseudotreeNode*> >::const_iterator it=m_levels.begin();
       it!=m_levels.end(); ++it) {
//    cout << "# " << central << '\t';
    tmax = 0; tsum=0; tnum=0; wmax=0;
    for (list<PseudotreeNode*>::const_iterator itL = it->begin(); itL!=it->end(); ++itL) {
      tmax = max(tmax, (*itL)->getSubsize() );
      tsum += (*itL)->getSubsize() * (*itL)->getNumContexts();
      tnum += (*itL)->getNumContexts();
      wmax = max(wmax, (*itL)->getSubwidth());
    }

    bigfloat ratio = 1;
    if (size != 0) {
      ratio = bigfloat(tsum+central) / size ;
    }
    size = tsum + central;

    bound = central + max( tmax, bigint(tsum / min(tnum,bigint(workers))) );
    bounds.push_back( bound );

//    cout << tnum << '\t' << (tmax) << '\t' << (tsum) << '\t' << (bound) << '\t' << ratio << '\t' << wmax << endl;
    for (list<PseudotreeNode*>::const_iterator itL = it->begin(); itL!=it->end(); ++itL) {
//      cout << " + " << (*itL)->getVar();
      central += (*itL)->getOwnsize();
    }
//    cout << endl;
  }
//  cout << "# " << central << "\t0\t0\t0\t" << central << "\t1\t0" << endl;

  int curDepth = -1, minDepth=UNKNOWN;
  bigint min = bounds.at(0)+1;
  for (vector<bigint>::iterator it = bounds.begin(); it != bounds.end(); ++it, ++curDepth) {
    if (*it < min) {
      min = *it;
      minDepth = curDepth;
    }
  }

  // TODO: Heuristically add 1 to minimizing level
  minDepth += 1;

  return minDepth;

}
#endif


/* updates a single node's depth, recursively updating the child nodes.
 * return value is max. depth in node's subtree */
int PseudotreeNode::updateDepth(int d) {

  m_depth = d;

  if (m_children.empty()) {
    m_height = 0;
  }
  else {
    int m=0;
    for (vector<PseudotreeNode*>::iterator it=m_children.begin(); it!=m_children.end(); ++it)
      m = max(m, (*it)->updateDepth(m_depth+1));
    m_height = m + 1;
  }

  return m_height;

}


/* recursively updates the set of variables in the current subproblem */
const set<int>& PseudotreeNode::updateSubprobVars() {

  // clear current set
  m_subproblemVars.clear();
  // add self
  m_subproblemVars.insert(m_var);

  // iterate over children and collect their subproblem variables
  for (vector<PseudotreeNode*>::iterator it = m_children.begin(); it!=m_children.end(); ++it) {
    const set<int>& childVars = (*it)->updateSubprobVars();
    for (set<int>::const_iterator itC = childVars.begin(); itC!=childVars.end(); ++itC)
      m_subproblemVars.insert(*itC);
  }

#ifdef DEBUG
  cout << "Subproblem at var. " << m_var << ": " << m_subproblemVars << endl;
#endif

  // return a const reference
  return m_subproblemVars;
}


#ifdef PARALLEL_MODE
/* computes subproblem complexity parameters for a particular pseudo tree node */
void PseudotreeNode::initSubproblemComplexity() {

  const vector<val_t>& domains = m_tree->m_problem->getDomains();

  queue<PseudotreeNode*> Q;
  Q.push(this);

  int w = NONE;
  bigint s = 0;
  PseudotreeNode* node = NULL;

  const set<int>& ctxt = this->getFullContext();

  while (!Q.empty()) {
    node = Q.front();
    Q.pop();

    for (vector<PseudotreeNode*>::const_iterator it=node->getChildren().begin();
         it!=node->getChildren().end(); ++it)
      Q.push(*it);

    // compute parameters for current node
    int w2 = 0;
    bigint s2 = domains.at(node->getVar()); // own var is NOT in context
    const set<int>& fullCtxt = node->getFullContext();
    set<int>::const_iterator itFC = fullCtxt.begin(), itC = ctxt.begin();
    while (itFC != fullCtxt.end() && itC != ctxt.end()) {
      if (*itFC == *itC) {
        ++itFC; ++itC;
      } else if (*itFC < *itC) {
        ++w2; s2 *= domains.at(*itFC); ++itFC;
      } else { // *ita > *itb
        ++itC;
      }
    }
    while (itFC != fullCtxt.end()) {
      ++w2; s2 *= domains.at(*itFC); ++itFC;
    }
    if (w2>w) w=w2; // update max. induced width
    s += s2; // add cluster size to subproblem size

  }

  // compute own cluster size only
  bigint ctxtsize = 1;
  for (set<int>::const_iterator itb = ctxt.begin(); itb!=ctxt.end(); ++itb)
    ctxtsize *= domains.at(*itb); // context variable

  // multiply by own domain size for cluster size
  bigint own = ctxtsize*domains.at(this->getVar());

  m_complexity = new Complexity(w,s,own,ctxtsize);

#ifdef DEBUG
  cout << "Subproblem at node " << m_var << ": w = " << w << ", twb = " << s << endl;
#endif

}
#endif /* PARALLEL_MODE */


#ifdef PARALLEL_MODE
/* compute upper bound on subproblem size, assuming conditioning on 'cond' */
bigint PseudotreeNode::computeSubCompDet(const set<int>& cond, const vector<val_t>* assig) const {

  set<int> cluster(this->getFullContext()); // get context
  cluster.insert(m_var); // add own variable to get cluster
  const vector<val_t>& doms = m_tree->m_problem->getDomains();

//  DIAG( cout << "Covering cluster " << cluster << " conditioned on " << cond << endl );

  // compute locally relevant set of variables, i.e. context minus instantiated vars in 'cond'
  set<int> vars;
  set_difference(cluster.begin(), cluster.end(), cond.begin(), cond.end(), inserter(vars,vars.begin()) );

  // collect all relevant functions by going up in the PTree
  list<Function*> funcs( this->getFunctions() );
  for (PseudotreeNode* node = this->getParent(); node; node = node->getParent() )
    funcs.insert( funcs.end(), node->getFunctions().begin(), node->getFunctions().end() );

#ifdef DEBUG
  cout << " Functions : " ;
  for (list<Function*>::iterator itFu=funcs.begin(); itFu!= funcs.end(); ++itFu)
    cout << (*(*itFu)) << ", " ;
  cout << endl;
#endif

  // TODO filter functions by scope

  set<int> uncovered(vars); // everything uncovered initially
  list<Function*> cover;
  while (!uncovered.empty()) {
    bigfloat ratio = 1, ctemp = 1;
    Function* cand = NULL;
    list<Function*>::iterator candIt = funcs.begin();

//    DIAG(cout << "  Uncovered: " << uncovered << endl);

    for (list<Function*>::iterator itF = funcs.begin(); itF!= funcs.end(); ++itF) {
      bigfloat rtemp = (*itF)->gainRatio(uncovered, cluster, cond, assig);
//      DIAG( cout << "   Ratio for " << (*(*itF)) <<": " << rtemp << endl );
      if (rtemp != UNKNOWN && rtemp < ratio)
        ratio = rtemp, cand = *itF, candIt = itF;
    }

    if (!cand) break; // covering is not complete but can't be improved

    // add function to covering and remove its scope from uncovered set
    cover.push_back(cand);
    funcs.erase(candIt);
#ifdef DEBUG
    cout << "  Adding " << (*cand) << endl;
#endif
    const set<int>& cscope = cand->getScope();
    for (set<int>::const_iterator itSc = cscope.begin(); itSc!=cscope.end(); ++itSc)
      uncovered.erase(*itSc);

  }

  // compute upper bound based on clustering
  bigint bound = 1;
  for (set<int>::iterator itU=uncovered.begin(); itU!=uncovered.end(); ++itU)
    bound *= doms.at(*itU); // domain sizes of uncovered variables
  for (list<Function*>::iterator itC=cover.begin(); itC!=cover.end(); ++itC)
    bound *= (*itC)->getTightness(cluster,cond,assig); // projected function tightness

#ifdef DEBUG
  cout << "@ " << bound << endl;
#endif

  // add bounds from child nodes
  for (vector<PseudotreeNode*>::const_iterator itCh=m_children.begin(); itCh!=m_children.end(); ++itCh)
    bound += (*itCh)->computeSubCompDet(cond, assig);

  return bound;

}
#endif /* PARALLEL_MODE */


void Pseudotree::insertNewNode(const int i, const set<int>& N, list<PseudotreeNode*>& roots) {
  // create new node in pseudo tree
  PseudotreeNode* p = new PseudotreeNode(this,i,N);
//  p->addContext(i);
  m_nodes[i] = p;

  // incorporate new pseudo tree node
  list<PseudotreeNode*>::iterator it = roots.begin();
  while (it != roots.end()) {
    const set<int>& context = (*it)->getFullContext();
    if (context.find(i) != context.end()) {
      p->addChild(*it); // add child to current node
      (*it)->setParent(p); // set parent of previous node
      it = roots.erase(it); // remove previous node from roots list
    } else {
      ++it;
    }
  }
  roots.push_back(p); // add current node to roots list

}

