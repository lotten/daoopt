/*
 * utils.cpp
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
 *  Created on: Oct 10, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "utils.h"

namespace daoopt {

extern time_t _time_start;  // from Main.cpp

void myprint(std::string s) {
  time_t now; time(&now);
  double T = difftime(now, _time_start);
  {
    GETLOCK(mtx_io, lk);
    std::cout << '[' << (int)T << "] " << s << std::flush;
  }
}

void myerror(std::string s) {
  time_t now; time(&now);
  double T = difftime(now, _time_start);
  {
    GETLOCK(mtx_io, lk);
    std::cerr << '[' << (int)T << "] " << s << std::flush;
  }
}

void err_txt(std::string s) {
  GETLOCK(mtx_io, lk);
  std::cerr << "Error: " << s << endl;
  ofstream ferr("err.txt", ios_base::out | ios_base::app);
  ferr << "DAOOPT: " << s << endl;
  ferr.close();
}


/***************************************/
/*        some cout functions          */

ostream& operator <<(ostream& os, const vector<int>& s) {
  os << '[';
  for (vector<int>::const_iterator it = s.begin(); it != s.end(); ) {
    os << *it;
    if (++it != s.end()) os << ',';
  }
  os << ']';
  return os;
}

ostream& operator <<(ostream& os, const vector<unsigned int>& s) {
  os << '[';
  for (vector<unsigned int>::const_iterator it = s.begin(); it != s.end(); ) {
    os << *it;
    if (++it != s.end()) os << ',';
  }
  os << ']';
  return os;
}

ostream& operator <<(ostream& os, const vector<signed short>& s) {
  os << '[';
  for (vector<signed short>::const_iterator it = s.begin(); it != s.end(); ) {
    os << (int) *it;
    if (++it != s.end()) os << ',';
  }
  os << ']';
  return os;
}

ostream& operator <<(ostream& os, const vector<signed char>& s) {
  os << '[';
  for (vector<signed char>::const_iterator it = s.begin(); it != s.end(); ) {
    os << (int) *it;
    if (++it != s.end()) os << ',';
  }
  os << ']';
  return os;
}

ostream& operator <<(ostream& os, const vector<int*>& s) {
  os << '[';
  for (vector<int*>::const_iterator it = s.begin(); it != s.end(); ) {
    os << **it;
    if (++it != s.end()) os << ',';
  }
  os << ']';
  return os;
}

ostream& operator <<(ostream& os, const set<int>& s) {
  os << '{';
  for (set<int>::const_iterator it = s.begin(); it != s.end(); ) {
    os << *it;
    if (++it != s.end()) os << ',';
  }
  os << '}';
  return os;
}

ostream& operator <<(ostream& os, const set<unsigned int>& s) {
  os << '{';
  for (set<unsigned int>::const_iterator it = s.begin(); it != s.end(); ) {
    os << *it;
    if (++it != s.end()) os << ',';
  }
  os << '}';
  return os;
}

ostream& operator <<(ostream& os, const vector<double>& s) {
  os << '[';
  for (vector<double>::const_iterator it = s.begin(); it != s.end(); ) {
    os << *it;
    if (++it != s.end()) os << ',';
  }
  os << ']';
  return os;
}

string str_replace(string& s, const string& x, const string& y) {
  string res = s;
  size_t i=res.find(x);
  while ( i != string::npos) {
    res = res.replace(i, x.size(), y);
    i = res.find(x, i+y.size());
  }
  return res;
}

#ifdef PARALLEL_DYNAMIC
 #ifdef USE_GMP
double mylog10(bigint a) {
  double l = 0;
  while (!a.fits_sint_p()) {
    a /= 10;
    ++l;
  }
  l += log10(a.get_si());
  return l;
}
 #endif
#endif

}  // namespace daoopt
