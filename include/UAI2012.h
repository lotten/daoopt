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

struct UAI2012 {

  static string filename;  // filename for solution output

  static void outputSolutionInt(const vector<int>& assignment) {
    assert(filename != "");

    // generate the solution file contents first
    ostringstream ss;
    ss <<  "MPE" << endl << "1" << endl;  // TODO: allow more than one evid. sample
    ss << assignment.size();
    for (vector<int>::const_iterator it = assignment.begin();
         it != assignment.end(); ++it) {
      ss << " " << (*it);
    }
    ss << endl;

    // now write them to file
    ofstream outfile;
    outfile.open(filename.c_str(), ios::out | ios::trunc);
    outfile << ss.str();
    outfile.close();
  }

  static void outputSolutionValT(const vector<val_t>& assignment) {
    vector<int> assigInt(assignment.size());
    for (int i=0; i<assignment.size(); ++i) {
      assigInt[i] = (int) assignment[i];
    }
    outputSolutionInt(assigInt);
  }

};


#endif /* UAI2012_H_ */
