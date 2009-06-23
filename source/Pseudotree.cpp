/*
 * Pseudotree.cpp
 *
 *  Created on: Oct 24, 2008
 *      Author: lars
 */

#include "Pseudotree.h"


void Pseudotree::restrictSubproblem(int i) {
  assert(m_root && i<(int)m_nodes.size() && m_nodes[i]);

  m_root->setChild(m_nodes[i]);
  m_nodes[i]->setParent(m_root);

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


// computes an elimination order into 'elim' and returns its tree width
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

// builds the pseudo tree according to order 'elim'
void Pseudotree::build(Graph G, const vector<int>& elim, const int cachelimit) {

  const int n = G.getStatNodes();
  assert(n == (int) m_nodes.size());
  assert(n == (int) elim.size());

  list<PseudotreeNode*> roots;

  // build actual pseudo tree(s)
  for (vector<int>::const_iterator it = elim.begin(); it!=elim.end(); ++it) {
    const set<int>& N = G.getNeighbors(*it);
    m_width = max(m_width,(int)N.size());
    insertNewNode(*it,N,roots);
    G.addClique(N);
    G.removeNode(*it);
  }

#if true
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
#endif

  // add artificial root node to connect disconnected components
  int bogusIdx = elim.size();
  PseudotreeNode* p = new PseudotreeNode(bogusIdx,set<int>());
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
  m_elimOrder.push_back(bogusIdx); // add dummy variable

  // initiate depth computation for tree and its nodes
  // (bogus variable gets depth -1)
  m_depth = m_root->updateDepth(-1);

  // initiate subproblem variables computation (recursive)
  m_root->updateSubprobVars();

  // compute depth->list<nodes> mapping
  m_levels.resize(m_depth+2);
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!= m_nodes.end(); ++it) {
    m_levels.at((*it)->getDepth()+1).push_back(*it);
  }

  return;

//  cout << "Number of disconnected sub pseudo trees: " << roots.size() << endl;

}


#ifdef USE_THREADS
void Pseudotree::computeComplexities(const Problem& problem) {

  // compute subproblem complexity parameters of all nodes
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!= m_nodes.end(); ++it)
    (*it)->initSubproblemComplexity(problem.getDomains());

  bigint c = m_root->getSubsize();

  cout << "Complexity bound:\t" << mylog10(c) << " (" << c << ")" << endl;

  // iterate over levels and compute estimated bound on parallelized search space
  bigint cutset = 0, tmax=0, tsum=0, tnum=0;
  for (vector<list<PseudotreeNode*> >::const_iterator it=m_levels.begin();
       it!=m_levels.end(); ++it) {
    cout << "# " << cutset << '\t';
    tmax = 0; tsum=0; tnum=0;
    for (list<PseudotreeNode*>::const_iterator itL = it->begin(); itL!=it->end(); ++itL) {
      tmax = max(tmax, (*itL)->getSubsize() );
      tsum += (*itL)->getSubsize() * (*itL)->getNumContexts();
      tnum += (*itL)->getNumContexts();
    }
    cout << tnum << '\t' << (tmax) << '\t' << (tsum) << endl;
    for (list<PseudotreeNode*>::const_iterator itL = it->begin(); itL!=it->end(); ++itL) {
//      cout << " + " << (*itL)->getVar();
      cutset += (*itL)->getOwnsize();
    }
//    cout << endl;
  }
  cout << "# " << cutset << "\t0\t0\t0" << endl;

}
#endif


// updates a single node's depth, recursively updating the child nodes.
// return value is max. depth in node's subtree
int PseudotreeNode::updateDepth(int d) {

  m_depth = d;

  if (m_children.empty())
    return m_depth;
  else {
    int m = 0;
    for (vector<PseudotreeNode*>::iterator it=m_children.begin(); it!=m_children.end(); ++it)
      m = max(m, (*it)->updateDepth(m_depth+1) );
    return m;
  }

}


// recursively updates the set of variables in the current subproblem
const set<int>& PseudotreeNode::updateSubprobVars() {
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

#ifdef USE_THREADS
// computes the subproblem complexity parameters for this particular node
void PseudotreeNode::initSubproblemComplexity(const vector<val_t>& domains) {

  queue<PseudotreeNode*> Q;
  Q.push(this);

  int w = NONE;
  bigint s = 0;
  PseudotreeNode* node = NULL;

  const set<int>& B = this->m_context;

  while (!Q.empty()) {
    node = Q.front();
    Q.pop();

    for (vector<PseudotreeNode*>::const_iterator it=node->getChildren().begin();
         it!=node->getChildren().end(); ++it)
      Q.push(*it);

    // compute parameters for current node
    int w2 = 0;
    bigint s2 = domains.at(node->getVar());
    const set<int>& A = node->getFullContext();
    set<int>::const_iterator ita = A.begin(), itb = B.begin();
    while (ita != A.end() && itb != B.end()) {
      if (*ita == *itb) {
        ++ita; ++itb;
      } else if (*ita < *itb) {
        ++w2; s2 *= domains.at(*ita); ++ita;
      } else { // *ita > *itb
        ++itb;
      }
    }
    while (ita != A.end()) {
      ++w2; s2 *= domains.at(*ita); ++ita;
    }
    if (w2>w) w=w2; // update max. induced width
    s += s2; // add cluster size to subproblem size

  }

  // compute own cluster size only
  bigint csize = 1;
  for (set<int>::const_iterator itb = B.begin(); itb!=B.end(); ++itb)
    csize *= domains.at(*itb);

  bigint own = csize*domains.at(this->getVar());

  m_complexity = new Complexity(w,s,own,csize);

#ifdef DEBUG
  cout << "Subproblem at node " << m_var << ": w = " << w << ", twb = " << s << endl;
#endif

}
#endif /* USE_THREADS */

/*
// computes the induced width of the subproblem represented by this ptnode,
// assuming conditioning on set A
Complexity PseudotreeNode::computeSubCompRec(const set<int>& C) const {

  // compute own parameters by going over own context and conditioning set

  set<int>::iterator ita = m_context.begin(), itb = C.begin();
  int w = 0; bigint s = ;
  while (ita != a.end() && itb != b.end()) {
    if (*ita == *itb) {
      ++ita; ++itb;
    } else if (*ita < *itb) {
      ++w; ++ita;
    } else { // *ita > *itb
      ++itb;
    }
  }
  while (ita != a.end()) {
    ++w; ++ita;
  }



  int w = setminusSize(m_context, A);
  for (vector<PseudotreeNode*>::const_iterator it=m_children.begin(); it!=m_children.end(); ++it)
    w = max(w, (*it)->computeSubCompRec(A) );
  return w;
}
*/

void Pseudotree::insertNewNode(const int& i, const set<int>& N, list<PseudotreeNode*>& roots) {
  // create new node in pseudo tree
  PseudotreeNode* p = new PseudotreeNode(i,N);
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

