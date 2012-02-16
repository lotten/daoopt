/*
 * CacheTable.h
 *
 *  Copyright (C) 2008-2012 Lars Otten
 *  This file is part of DAOOPT.
 *
 *  DAOOPT is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DAOOPT is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DAOOPT.  If not, see <http://www.gnu.org/licenses/>.
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
typedef hash_map <const context_t, const pair<const double, const vector<val_t> > > context_hash_map;
#else
typedef hash_map <const context_t, const double > context_hash_map;
#endif

class CacheTable {

private:
  bool m_full;
  int m_size;
#ifdef PARALLEL_DYNAMIC
  vector<size_t> m_instCounter;
#endif
  vector< context_hash_map* > m_tables;

protected:
#ifndef NO_ASSIGNMENT
  static const pair<const double, const vector<val_t> > NOT_FOUND;
#else
  static const double NOT_FOUND;
#endif

public:
#ifndef NO_ASSIGNMENT
  virtual bool write(int n, size_t inst, const context_t& ctxt, double v, const vector<val_t>& sol);
  virtual const pair<const double, const vector<val_t> >& read(int n, size_t inst, const context_t& ctxt) const;
#else
  virtual bool write(int n, size_t inst, const context_t& ctxt, double v);
  virtual const double read(int n, size_t inst, const context_t& ctxt) const;
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
  bool write(int n, size_t inst, const context_t& ctxt, double v, const vector<val_t>& sol) { return false; }
  const pair<const double, const vector<val_t> >& read(int n, size_t inst, const context_t& ctxt) const { return NOT_FOUND; }
#else
  bool write(int n, size_t inst, const context_t& ctxt, double v) { return false; }
  const double read(int n, size_t inst, const context_t& ctxt) const { return NOT_FOUND; }
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
inline bool CacheTable::write(int n, size_t inst, const context_t& ctxt, double v, const vector<val_t>& sol) {
#else
inline bool CacheTable::write(int n, size_t inst, const context_t& ctxt, double v) {
#endif
  assert(n < m_size);
#ifdef PARALLEL_DYNAMIC
  // check instance counter
  if (m_instCounter[n] != inst)
    return false; // mismatch, don't cache
#endif

  // NaN is reserved, replace with zero
  if (ISNAN(v))
    v = -ELEM_ZERO;

  // create hash table if needed
  if (!m_tables[n]) {
    m_tables[n] = new context_hash_map;
#if defined HASH_GOOGLE_DENSE || defined HASH_GOOGLE_SPARSE
    m_tables[n]->set_deleted_key(" ");
#endif
#ifdef HASH_GOOGLE_DENSE
    m_tables[n]->set_empty_key("");
#endif
  }
  // this will write only if entry not present yet
#ifndef NO_ASSIGNMENT
  m_tables[n]->insert( context_hash_map::value_type(ctxt, make_pair(v,sol) ) );
#else
  m_tables[n]->insert( context_hash_map::value_type(ctxt, v) );
#endif
  return true;
}


/* tries to read a value from a table, throws an int (UNKNOWN) if not found */
#ifndef NO_ASSIGNMENT
inline const pair<const double, const vector<val_t> >& CacheTable::read(int n, size_t inst, const context_t& ctxt) const {
#else
inline const double CacheTable::read(int n, size_t inst, const context_t& ctxt) const {
#endif

  assert(n < m_size);
  // does cache table exist?
  if (!m_tables[n])
    return NOT_FOUND;
#ifdef PARALLEL_DYNAMIC
  // check instance counter
  if (m_instCounter[n] != inst)
    return NOT_FOUND; // mismatch, abort
#endif
  // look for actual entry
  context_hash_map::const_iterator it = m_tables[n]->find(ctxt);
  if (it == m_tables[n]->end())
    return NOT_FOUND;
  return it->second;
}


inline void CacheTable::reset(int n) {
  assert(n<m_size);
  if (m_tables[n]) {
//    m_tables[n]->clear();
    delete m_tables[n];
    m_tables[n] = NULL;
    m_full = false;
    DIAG(oss ss;  ss << "Reset cache table " << n << endl; myprint(ss.str());)
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
