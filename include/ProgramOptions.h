/*
 * ProgramOptions.h
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
 *  Created on: Nov 15, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef PROGRAMOPTIONS_H_
#define PROGRAMOPTIONS_H_

#include "boost/program_options.hpp"
#include "_base.h"

namespace po = boost::program_options;

#include <string>
#include <iostream>

struct ProgramOptions {
public:
  bool nosearch; // abort before starting the actual search
  bool nocaching; // disable caching
  bool autoCutoff; // enable automatic cutoff
  bool autoIter; // enable adaptive ordering limit
  bool orSearch; // use OR search (builds pseudo tree as chain)
  bool par_preOnly; // static parallel: preprocessing only (generate subproblems)
  bool par_postOnly; // static parallel: postprocessing only (read solution files)
  bool rotate; // enables breadth-rotating AOBB
  int ibound; // bucket elim. i-bound
  int cbound; // cache context size bound
  int cbound_worker; // cache bound for worker processes
  int threads; // max. number of parallel subproblems
  int order_iterations; // no. of randomized order finding iterations
  int order_timelimit; // no. of seconds to look for variable ordering
  int order_tolerance; // allowed range of deviation from suggested optimal minfill heuristic
  int cutoff_depth; // fixed cutoff depth for central search
  int cutoff_width; // fixed width for central cutoff
  int nodes_init; // number of nodes for local initialization (times 10^6)
  int memlimit; // memory limit (in MB)
  int cutoff_size; // fixed cutoff subproblem size (times 10^6)
  int local_size; // lower bound for problem size to be solved locally (times 10^6)
  int maxSubprob; // only generate this many subproblems, then abort (for testing)
  int lds;  // run initial LDS with this limit (-1: enabled)
  int seed; // the seed for the random number generator
  int rotateLimit; // how many nodes to expand per subproblem stack before rotating
  int subprobOrder; // subproblem ordering, integers defined in _base.h
  int sampleDepth; // max. depth for randomness in sampler (will follow heuristic otherwise)
  int sampleScheme; // sampling scheme (TBD)
  int sampleRepeat; // how many times to repeat the sample size sequence
  int maxWidthAbort; // upper bound for induced width, abort if above this
  int slsIter; // number of SLS iterations for initial lower bound
  int slsTime; // time per SLS iteration (in seconds)
  int aobbLookahead;  // max. number of nodes for parallel static AOBB subproblem lookahead

  double initialBound; // initial lower bound

  std::string executableName; // name of the executable
  std::string problemName; // name of the problem
  std::string runTag; // string tag of this particular run
  std::string sampleSizes; // Sequence of sizes for subproblem samples (for prediction),
                           // comma-separated list of doubles.
  std::string in_problemFile; // problem file path
  std::string in_evidenceFile; // evidence file path
  std::string in_orderingFile; // ordering file path
  std::string in_minibucketFile; // minibucket file path
  std::string in_subproblemFile; // subproblem file path
  std::string in_boundFile; // file with initial lower bound (from SLS, e.g.)
  std::string out_solutionFile; // file path to write solution to
  std::string out_reducedFile; // file to save reduced network to
  std::string out_pstFile; // file to output pseudo tree description to (for plotting)

public:
  ProgramOptions();
};

ProgramOptions* parseCommandLine(int argc, char** argv);

inline ProgramOptions::ProgramOptions() :
		      nosearch(false), nocaching(false), autoCutoff(false), autoIter(false), orSearch(false),
		      par_preOnly(false), par_postOnly(false), rotate(false),
		      ibound(0), cbound(0), cbound_worker(0),
		      threads(0), order_iterations(0), order_timelimit(0), order_tolerance(0),
		      cutoff_depth(NONE), cutoff_width(NONE),
		      nodes_init(NONE), memlimit(NONE),
		      cutoff_size(NONE), local_size(NONE), maxSubprob(NONE),
		      lds(NONE), seed(NONE), rotateLimit(0), subprobOrder(NONE),
		      sampleDepth(NONE), sampleScheme(NONE), sampleRepeat(NONE),
		      maxWidthAbort(NONE), slsIter(0), slsTime(5),
		      aobbLookahead(0),
		      initialBound(ELEM_NAN) {}

#endif /* PROGRAMOPTIONS_H_ */
