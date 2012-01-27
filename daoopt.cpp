/*
 * daoopt.cpp
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
 *  Created on: Oct 13, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "Main.h"

/* define to enable diagnostic output of memory stats */
//#define MEMDEBUG

int main(int argc, char** argv) {

  Main main;

  if (!main.start())
    exit(0);
  if (!main.parseOptions(argc, argv))
    exit(0);
  if (!main.outputInfo())
    exit(0);
  if (!main.loadProblem())
    exit(0);
  if (!main.preprocessHeuristic())
    exit(0);
  if (!main.runSLS())
    exit(0);
  if (!main.findOrLoadOrdering())
    exit(0);
  if (!main.initDataStructs())
    exit(0);
  if (!main.compileHeuristic())
    exit(0);
  if (!main.runLDS())
    exit(0);
  if (!main.finishPreproc())
    exit(0);
  if (!main.runSearch())
    exit(0);
  if (!main.outputStats())
    exit(0);

  return 0;

}
