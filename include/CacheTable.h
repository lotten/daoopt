/*
 * CacheTable.h
 *
 *  Created on: Dec 4, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef CACHETABLE_H_
#define CACHETABLE_H_

#include "_base.h"
#include "utils.h"

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
#ifdef PARALLEL_DYNAMIC
  vector<size_t> m_instCounter;
#endif
  vector< context_hash_map* > m_tables;

public:

#ifndef NO_ASSIGNMENT
  virtual void write(int n, size_t inst, const context_t& ctxt, double v, const vector<val_t>& sol) throw (int);
  virtual pair<double, vector<val_t> > read(int n, size_t inst, const context_t& ctxt) const throw (int);
#else
  virtual void write(int n, size_t inst, const context_t& ctxt, double v) throw (int);
  virtual double read(int n, size_t inst, const context_t& ctxt) const throw (int);
#endif

  virtual void reset(int n);
#ifdef PARALLEL_DYNAMIC
  virtual size_t getInstCounter(int n) const;
#else
  virtual size_t getInstCounter(int n) const { return 0; }
#endif

private:
  int memused() const;

public:
  virtual void printStats() const;

public:
  CacheTable(int size);
  virtual ~CacheTable();
};


class UnCacheTable : public CacheTable  {
public:

#ifndef NO_ASSIGNMENT
  void write(int n, size_t inst, const context_t& ctxt, double v, const vector<val_t>& sol) throw (int) {throw UNKNOWN;}
  pair<double, vector<val_t> > read(int n, size_t inst, const context_t& ctxt) const throw (int) {throw UNKNOWN;}
#else
  void write(int n, size_t inst, const context_t& ctxt, double v) throw (int) {throw UNKNOWN;}
  double read(int n, size_t inst, const context_t& ctxt) const throw (int) {throw UNKNOWN;}
#endif

  void reset(int n) {}
#ifdef PARALLEL_DYNAMIC
  size_t getInstCounter(int n) const { return 0; }
#else
  size_t getInstCounter(int n) const { return 0; }
#endif

public:
  void printStats() const {}

public:
  UnCacheTable() : CacheTable(0) {}
  ~UnCacheTable() {}

};

/* Inline definitions */


/* inserts a value into the respective cache table, throws an int if
 * insert non successful (memory limit or index out of bounds) */
#ifndef NO_ASSIGNMENT
inline void CacheTable::write(int n, size_t inst, const context_t& ctxt, double v, const vector<val_t>& sol) throw (int) {
#else
inline void CacheTable::write(int n, size_t inst, const context_t& ctxt, double v) throw (int) {
#endif
  assert(n < m_size);
#ifdef PARALLEL_DYNAMIC
  // check instance counter
  if (m_instCounter[n] != inst)
    throw UNKNOWN; // mismatch, don't cache
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


/* tries to read a value from a table, throws an int (UNKNOWN) if not found */
#ifndef NO_ASSIGNMENT
inline pair<double, vector<val_t> > CacheTable::read(int n, size_t inst, const context_t& ctxt) const throw (int) {
#else
inline double CacheTable::read(int n, size_t inst, const context_t& ctxt) const throw (int) {
#endif

  assert(n < m_size);
  // does cache table exist?
  if (!m_tables[n])
    throw UNKNOWN;
#ifdef PARALLEL_DYNAMIC
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
#ifdef PARALLEL_DYNAMIC
  // increase instant counter to invalidate late result reports
  ++m_instCounter[n];
#endif
}

#ifdef PARALLEL_DYNAMIC
inline size_t CacheTable::getInstCounter(int n) const {
  assert(n<m_size);
  return m_instCounter[n];
}
#endif

inline CacheTable::CacheTable(int size) :
#ifdef PARALLEL_DYNAMIC
    m_full(false), m_size(size), m_instCounter(size,0), m_tables(size) {
#else
    m_full(false), m_size(size), m_tables(size) {
#endif
  for (int i=0; i<size; ++i)
    m_tables[i] = NULL;
}

inline void CacheTable::printStats() const {
  ostringstream ss;
  ss << "Cache statistics:" ;
  for (vector< context_hash_map * >::const_iterator it=m_tables.begin(); it!=m_tables.end(); ++it) {
    if (*it) ss << " " << (*it)->size();
    else     ss << " .";
  }
  ss << endl;
  myprint(ss.str());
}

inline CacheTable::~CacheTable() {
  for (vector< context_hash_map * >::iterator it=m_tables.begin(); it!=m_tables.end(); ++it)
    if (*it) delete *it;
}

#ifdef WINDOWS
/* TODO? not supported under Windows */
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
