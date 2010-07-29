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
      ("evid-file,e", po::value<string>(), "path to optional evidence file")
      ("ordering,o", po::value<string>(), "read elimination ordering from this file (first to last)")
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
      ("cutoff-width,w", po::value<int>()->default_value(-1), "cutoff width for central search")
      ("cutoff-size,l", po::value<int>()->default_value(-1), "subproblem size cutoff for central search (* 10^6)")
      ("local-size,u", po::value<int>()->default_value(-1), "minimum subproblem size (* 10^6)")
      ("init-nodes,x", po::value<int>()->default_value(-1), "number of nodes (*10^6) for local initialization")
      ("noauto,a", "don't determine cutoff automatically")
      ("procs,p", po::value<int>()->default_value(5), "max. number of concurrent subproblem processes")
      ("max-sub", po::value<int>()->default_value(-1), "only generate the first few subproblems (for testing)")
#endif
      ("bound-file,b", po::value<string>(), "file with initial lower bound on solution cost")
      ("initial-bound", po::value<double>(), "initial lower bound on solution cost" )
      ("lds,a",po::value<int>()->default_value(-1), "run initial LDS search with given limit (-1: disabled)")
      ("memlimit,m", po::value<int>()->default_value(-1), "approx. memory limit for mini buckets (in MByte)")
#ifdef ANYTIME_BREADTH
      ("stacklimit,z", po::value<int>()->default_value(1000), "nodes per subproblem stack rotation (0: disabled)")
#endif
      ("seed", po::value<int>(), "seed for random number generator (uses time() otherwise)")
      ("nosearch,n", "perform preprocessing, output stats, and exit")
      ("reduce,r", po::value<string>(), "outputs the reduced network to this file (removes evidence and unary variables)")
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

    if (vm.count("cutoff-depth"))
      opt.cutoff_depth = vm["cutoff-depth"].as<int>();

    if (vm.count("cutoff-width"))
      opt.cutoff_width = vm["cutoff-width"].as<int>();

    if (vm.count("cutoff-size"))
      opt.cutoff_size = vm["cutoff-size"].as<int>();

    if (vm.count("local-size"))
      opt.local_size = vm["local-size"].as<int>();

    if (vm.count("init-nodes"))
      opt.nodes_init = vm["init-nodes"].as<int>();

    if (vm.count("noauto"))
      opt.autoCutoff = false;
    else
      opt.autoCutoff = true;

    if (vm.count("procs"))
      opt.threads = vm["procs"].as<int>();

    if (vm.count("max-sub"))
      opt.maxSubprob = vm["max-sub"].as<int>();

    if (vm.count("bound-file"))
      opt.in_boundFile = vm["bound-file"].as<string>();

    if (vm.count("initial-bound"))
      opt.initialBound = vm["initial-bound"].as<double>();

    if (vm.count("lds"))
      opt.lds = vm["lds"].as<int>();

    if (vm.count("memlimit"))
      opt.memlimit = vm["memlimit"].as<int>();

    if(vm.count("nosearch"))
      opt.nosearch = true;
    else
      opt.nosearch = false;

#ifdef ANYTIME_BREADTH
    if (vm.count("stacklimit"))
      opt.stackLimit = vm["stacklimit"].as<int>();
#endif

    if (vm.count("seed"))
      opt.seed = vm["seed"].as<int>();

    if (vm.count("reduce"))
      opt.out_reducedFile = vm["reduce"].as<string>();

    if (vm.count("subproblem") && !vm.count("ordering")) {
      cerr << "Error: Specifying a subproblem requires reading a fixed ordering from file." << endl;
      exit(1);
    }

    {
      //Extract the problem name
      string& fname = opt.in_problemFile;
      size_t len, start, pos1, pos2;
#if defined(WIN32)
      pos1 = fname.find_last_of("\\");
#elif defined(LINUX)
      pos1 = fname.find_last_of("/");
#endif
      pos2 = fname.find_last_of(".");
      if (pos1 == string::npos) { len = pos2; start = 0; }
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

