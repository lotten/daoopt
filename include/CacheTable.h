/*
 * CacheTable.h
 *
 *  Created on: Dec 4, 2008
 *      Author: lars
 */

#ifndef CACHETABLE_H_
#define CACHETABLE_H_

#include "_base.h"
#include <vector>
#include <string>
#include <malloc.h>

//typedef hash_map <context_t, double> context_hash_map;
#ifndef NO_ASSIGNMENT
typedef hash_map <context_t, pair<double,vector<val_t> > > context_hash_map;
#else
typedef hash_map <context_t, double > context_hash_map;
#endif

class CacheTable {

private:
  bool m_full;
  int m_size;
  int m_memlimit;
/*
  // Cache the previous request
  int m_cached_n;
  std::string m_cached_ctxt;
  hash_map<std::string, double>::iterator m_cached_it;
*/
#ifdef PARALLEL_MODE
  vector<size_t> m_instCounter;
#endif
  vector< context_hash_map* > m_tables;

public:

#ifndef NO_ASSIGNMENT
  void write(int n, size_t inst, const context_t& ctxt, double v, const vector<val_t>& sol) throw (int);
  pair<double, vector<val_t> > read(int n, size_t inst, const context_t& ctxt) const throw (int);
#else
  void write(int n, size_t inst, const context_t& ctxt, double v) throw (int);
  double read(int n, size_t inst, const context_t& ctxt) const throw (int);
#endif

  void reset(int n);
#ifdef PARALLEL_MODE
  size_t getInstCounter(int n) const;
#else
  size_t getInstCounter(int n) const { return 0; }
#endif

private:
  int memused() const;

public:
  CacheTable(int size, int memlimit = NONE);
  ~CacheTable();
};

// Inline definitions

// inserts a value into the respective cache table
// throws an int if insert non successful (memory limit or index out of bounds)
#ifndef NO_ASSIGNMENT
inline void CacheTable::write(int n, size_t inst, const context_t& ctxt, double v, const vector<val_t>& sol) throw (int) {
#else
inline void CacheTable::write(int n, size_t inst, const context_t& ctxt, double v) throw (int) {
#endif
  assert(n < m_size);
#ifdef PARALLEL_MODE
  // check instance counter
  if (m_instCounter[n] != inst)
    throw UNKNOWN; // mismatch, don't cache
#endif

#ifdef CACHE_MEMLIMIT
  // check if mem limit hit
  if (m_memlimit != NONE && (m_full || memused() > m_memlimit)) {
    m_full = true;
    throw NONE;
  }
#endif

  // create hash table if needed
  if (!m_tables[n]) {
    m_tables[n] = new context_hash_map;
#if defined HASH_GOOGLE_DENSE | defined HASH_GOOGLE_SPARSE
    m_tables[n]->set_deleted_key(" ");
#endif
#ifdef HASH_GOOGLE_DENSE
    m_tables[n]->set_empty_key("");
#endif
  }
  // this will write only if entry not present yet
#ifndef NO_ASSIGNMENT
  m_tables[n]->insert( make_pair(ctxt, make_pair(v,sol) ) );
#else
  m_tables[n]->insert( make_pair(ctxt, v) );
#endif
}


// throws an int (UNKNOWN) if not found

#ifndef NO_ASSIGNMENT
inline pair<double, vector<val_t> > CacheTable::read(int n, size_t inst, const context_t& ctxt) const throw (int) {
#else
inline double CacheTable::read(int n, size_t inst, const context_t& ctxt) const throw (int) {
#endif

  assert(n < m_size);
  // does cache table exist?
  if (!m_tables[n])
    throw UNKNOWN;
#ifdef PARALLEL_MODE
  // check instance counter
  if (m_instCounter[n] != inst)
    throw UNKNOWN; // mismatch, abort
#endif
  // look for actual entry
  context_hash_map::const_iterator it = m_tables[n]->find(ctxt);
  if (it == m_tables[n]->end())
    throw UNKNOWN;
  return it->second;
}


inline void CacheTable::reset(int n) {
  assert(n<m_size);
  if (m_tables[n]) {
//    m_tables[n]->clear();
    delete m_tables[n];
    m_tables[n] = NULL;
    m_full = false;
//    cout << "Reset cache table " << n << endl;
  }
#ifdef PARALLEL_MODE
  // increase instant counter to invalidate late result reports
  ++m_instCounter[n];
#endif
}

#ifdef PARALLEL_MODE
inline size_t CacheTable::getInstCounter(int n) const {
  assert(n<m_size);
  return m_instCounter[n];
}
#endif

inline CacheTable::CacheTable(int size, int memlimit) :
#ifdef PARALLEL_MODE
    m_full(false), m_size(size), m_memlimit(NONE), m_instCounter(size,0), m_tables(size) {
#else
    m_full(false), m_size(size), m_memlimit(memlimit), m_tables(size) {
#endif
  for (int i=0; i<size; ++i)
    m_tables[i] = NULL;
}

inline CacheTable::~CacheTable() {
  for (vector< context_hash_map * >::iterator it=m_tables.begin(); it!=m_tables.end(); ++it) {
    if (*it) delete *it;
  }
}

#ifdef WINDOWS
// not supported under Windows // TODO
inline int CacheTable::memused() const {
  return -1;
}
#else
inline int CacheTable::memused() const {

  // read mem stats
  struct mallinfo info;
  info = mallinfo();

  int m= info.hblkhd + info.uordblks;
  m /= 1024*1024 ; // to MByte

  return m;
}
#endif

#endif /* CACHETABLE_H_ */
