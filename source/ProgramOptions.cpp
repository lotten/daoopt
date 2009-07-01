/*
 * ProgramOptions.cpp
 *
 *  Created on: Nov 15, 2008
 *      Author: lars
 */

#include "ProgramOptions.h"

using namespace std;

ProgramOptions parseCommandLine(int ac, char** av) {

  ProgramOptions opt;

  // executable name
  opt.executableName = av[0];

  try{

    po::options_description desc("Valid options");
    desc.add_options()
      ("input-file,f", po::value<string>(), "path to problem file (required)")
      ("evid-file,e", po::value<string>(), "path to evidence file")
      ("ordering,o", po::value<string>(), "read elimination ordering from this file")
      ("subproblem,s", po::value<string>(), "limit search to subproblem specified in file")
      ("sol-file,c", po::value<string>(), "path to output optimal solution to")
      ("ibound,i", po::value<int>()->default_value(10), "i-bound for mini bucket heuristics")
      ("cbound,j", po::value<int>()->default_value(1000), "context size bound for caching")
#ifdef PARALLEL_MODE
      ("cbound-worker,k", po::value<int>()->default_value(1000), "context size bound for caching in worker nodes")
#endif
      ("iterations,t", po::value<int>()->default_value(25), "iterations for finding ordering")
#ifdef PARALLEL_MODE
      ("cutoff-depth,d", po::value<int>()->default_value(-1), "cutoff depth for central search")
      ("cutoff-size,l", po::value<double>()->default_value(-1), "cutoff subproblem size (log10) for central search")
      ("procs,p", po::value<int>()->default_value(5), "max. number of concurrent subproblem processes")
#endif
      ("memlimit,m", po::value<int>()->default_value(-1), "approx. memory limit (in MByte)")
      ("nosearch,n", "performs only preprocessing")
      ("help,h", "produces this help message")
      ;

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      cout << endl << desc << endl;
      exit(0);
    }

    if (!vm.count("input-file")) {
      cout << "No or invalid arguments given, "
	   << "call with '" << av[0] << " --help' for full list."  << endl;
      exit(0);
    }

    opt.in_problemFile = vm["input-file"].as<string>();

    if (vm.count("evid-file"))
      opt.in_evidenceFile = vm["evid-file"].as<string>();

    if (vm.count("ordering"))
      opt.in_orderingFile = vm["ordering"].as<string>();

    if (vm.count("subproblem"))
      opt.in_subproblemFile = vm["subproblem"].as<string>();

    if (vm.count("sol-file"))
      opt.out_solutionFile = vm["sol-file"].as<string>();

    if (vm.count("ibound"))
      opt.ibound = vm["ibound"].as<int>();

    if (vm.count("cbound")) {
      opt.cbound = vm["cbound"].as<int>();
      opt.cbound_worker = vm["cbound"].as<int>();
    }

    if (vm.count("cbound-worker"))
      opt.cbound_worker = vm["cbound-worker"].as<int>();

    if (vm.count("iterations"))
      opt.order_iterations = vm["iterations"].as<int>();

    if (vm.count("cutoff-size"))
      opt.cutoff_size = vm["cutoff-size"].as<double>();

    if (vm.count("cutoff-depth"))
      opt.cutoff_depth = vm["cutoff-depth"].as<int>();

    if (vm.count("procs"))
      opt.threads = vm["procs"].as<int>();

    if (vm.count("memlimit"))
      opt.memlimit = vm["memlimit"].as<int>();

    if(vm.count("nosearch"))
      opt.nosearch = true;

    if (vm.count("subproblem") && !vm.count("ordering")) {
      cerr << "Error: Specifying a subproblem requires reading a fixed ordering from file." << endl;
      exit(1);
    }

    {
      //Extract the problem name
      string& fname = opt.in_problemFile;
      size_t len, start, pos1, pos2;
      static const basic_string <char>::size_type npos = -1;
#if defined(WIN32)
      pos1 = fname.find_last_of("\\");
#elif defined(LINUX)
      pos1 = fname.find_last_of("/");
#endif
      pos2 = fname.find_last_of(".");
      if (pos1 == npos) { len = pos2; start = 0; }
      else { len = (pos2-pos1-1); start = pos1+1; }
      opt.problemName = fname.substr(start, len);
    }

  } catch (exception& e) {
    cerr << "error: " << e.what() << endl;
    exit(1);
  } catch (...) {
    cerr << "Exception of unknown type!" << endl;
    exit(1);
  }

  return opt;

}

