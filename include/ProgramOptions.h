/*
 * ProgramOptions.h
 *
 *  Created on: Nov 15, 2008
 *      Author: lars
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
  int ibound; // bucket elim. i-bound
  int cbound; // cache context size bound
  int cbound_worker; // cache bound for worker processes
  int threads; // max. number of parallel subproblems
  int order_iterations; // no. of randomized order finding iterations
  int cutoff_depth; // fixed cutoff depth for central search
  int memlimit; // memory limit (in MB)
  double cutoff_size; // fixed cutoff subproblem size

  std::string executableName; // name of the executable
  std::string problemName; // name of the problem
  std::string in_problemFile; // problem file path
  std::string in_evidenceFile; // evidence file path
  std::string in_orderingFile; // ordering file path
  std::string in_subproblemFile; // subproblem file path
  std::string out_solutionFile; // file path to write solution to

public:
  ProgramOptions();
};

ProgramOptions parseCommandLine(int argc, char** argv);

inline ProgramOptions::ProgramOptions() :
  nosearch(false), ibound(0), cbound(0), cbound_worker(0), threads(0), order_iterations(0), cutoff_depth(0), memlimit(NONE), cutoff_size(0.0) {}

#endif /* PROGRAMOPTIONS_H_ */
