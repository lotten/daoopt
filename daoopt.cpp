/*
 * daoopt.cpp
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
  if (!main.findOrLoadOrdering())
    exit(0);
  if (!main.initSearchSpace())
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
