/*
 * UAI2012.h
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
 *  Created on: Dec 14, 2011
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef UAI2012_H_
#define UAI2012_H_

#include "_base.h"
#include <cstdio>

namespace daoopt {

struct UAI2012 {

  static string filename;  // filename for solution output

  static void outputSolutionValT(const vector<val_t>& assignment, const set<int>& mmap) {
    assert(filename != "");

    // Write solution output to temporary file first.
    string tempname = filename + "-temp";

    ofstream outfile;
    outfile.open(tempname.c_str(), ios::out | ios::trunc);
    string type;
    if (mmap.empty()) {
      // generate the solution file contents first
      outfile << "MPE" << endl;
      // ss << "1" << endl;  // TODO: allow more than one evid. sample
      outfile << assignment.size();
      BOOST_FOREACH(val_t v, assignment) {
        outfile << ' ' << (int) v;
      }
      outfile << endl << flush;
    } else {
      outfile << "MMAP" << endl;
      outfile << mmap.size();
      BOOST_FOREACH(int idx, mmap) {
        outfile << ' ' << idx << ' ' << (int) assignment.at(idx);
      }
      outfile << endl << flush;
    }
    outfile.close();

    // Move temp. file to proper path.
    remove(filename.c_str());
    rename(tempname.c_str(), filename.c_str());
  }

};

}  // namespace daoopt

#endif /* UAI2012_H_ */
