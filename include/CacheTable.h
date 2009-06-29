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
typedef hash_map <context_t, pair<double,vector<val_t> > > context_hash_map;

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
#ifdef USE_THREADS
  vector<size_t> m_instCounter;
#endif
  vector< context_hash_map* > m_tables;

public:
  void write(int n, size_t inst, const context_t& ctxt, double v, vector<val_t> sol) throw (int);
//  bool hasEntry(int n, const std::string& ctxt);
  pair<double, vector<val_t> > read(int n, size_t inst, const context_t& ctxt) const throw (int);
  void reset(int n);
#ifdef USE_THREADS
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
inline void CacheTable::write(int n, size_t inst, const context_t& ctxt, double v, vector<val_t> sol) throw (int) {
  assert(n < m_size);
#ifdef USE_THREADS
  // check instance counter
  if (m_instCounter[n] != inst)
    throw UNKNOWN; // mismatch, don't cache
#endif

  // check if mem limit hit
  if (m_memlimit != NONE && (m_full || memused() > m_memlimit)) {
    m_full = true;
    throw NONE;
  }

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
  m_tables[n]->insert(make_pair(ctxt, make_pair(v,sol) ) );
}

// throws an int (UNKNOWN) if not found
inline pair<double, vector<val_t> > CacheTable::read(int n, size_t inst, const context_t& ctxt) const throw (int) {
  assert(n < m_size);
  // does cache table exist?
  if (!m_tables[n])
    throw UNKNOWN;
#ifdef USE_THREADS
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
#ifdef USE_THREADS
  // increase instant counter to invalidate late result reports
  ++m_instCounter[n];
#endif
}

#ifdef USE_THREADS
inline size_t CacheTable::getInstCounter(int n) const {
  assert(n<m_size);
  return m_instCounter[n];
}
#endif

inline CacheTable::CacheTable(int size, int memlimit) :
#ifdef USE_THREADS
    m_size(size), m_instCounter(size,0), m_tables(size) {
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


inline int CacheTable::memused() const {

  // read mem stats
  struct mallinfo info;
  info = mallinfo();

  int m= info.hblkhd + info.uordblks;
  m /= 1024*1024 ; // to MByte

  return m;
}


#endif /* CACHETABLE_H_ */
