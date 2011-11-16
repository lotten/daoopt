/*
 * ProgramOptions.cpp
 *
 *  Created on: Nov 15, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "ProgramOptions.h"

using namespace std;

ProgramOptions* parseCommandLine(int ac, char** av) {

  ProgramOptions* opt = new ProgramOptions;

  // executable name
  opt->executableName = av[0];

  try{

    po::options_description desc("Valid options");
    desc.add_options()
      ("input-file,f", po::value<string>(), "path to problem file (required)")
      ("evid-file,e", po::value<string>(), "path to optional evidence file")
      ("ordering,o", po::value<string>(), "read elimination ordering from this file (first to last)")
      ("adaptive", "enable adaptive ordering scheme")
      ("minibucket", po::value<string>(), "path to read/store mini bucket heuristic")
      ("subproblem,s", po::value<string>(), "limit search to subproblem specified in file")
      ("suborder,r",po::value<int>()->default_value(0), "subproblem order (0:width-inc 1:width-dec 2:heur-inc 3:heur-dec)")
      ("sol-file,c", po::value<string>(), "path to output optimal solution to")
      ("ibound,i", po::value<int>()->default_value(10), "i-bound for mini bucket heuristics")
      ("cbound,j", po::value<int>()->default_value(1000), "context size bound for caching")
#if defined PARALLEL_DYNAMIC or defined PARALLEL_STATIC
      ("cbound-worker,k", po::value<int>()->default_value(1000), "context size bound for caching in worker nodes")
#else
      ("rotate,y", "use breadth-rotating AOBB")
      ("rotatelimit,z", po::value<int>()->default_value(1000), "nodes per subproblem stack rotation (0: disabled)")
#endif
      ("iterations,t", po::value<int>()->default_value(25), "iterations for finding ordering")
      ("max-width", po::value<int>(), "max. induced width to process, abort otherwise")
#if defined PARALLEL_DYNAMIC or defined PARALLEL_STATIC
      ("cutoff-depth,d", po::value<int>()->default_value(-1), "cutoff depth for central search")
      ("cutoff-width,w", po::value<int>()->default_value(-1), "cutoff width for central search")
      ("cutoff-size,l", po::value<int>()->default_value(-1), "subproblem size cutoff for central search (* 10^5)")
      ("local-size,u", po::value<int>()->default_value(-1), "minimum subproblem size (* 10^5)")
      ("init-nodes,x", po::value<int>()->default_value(-1), "number of nodes (*10^5) for local initialization")
      ("noauto", "don't determine cutoff automatically")
      ("procs,p", po::value<int>()->default_value(-1), "max. number of concurrent subproblem processes")
      ("max-sub", po::value<int>()->default_value(-1), "only generate the first few subproblems (for testing)")
      ("tag", po::value<string>(), "tag of the parallel run (to differentiate filenames etc.)")
#endif
#ifdef PARALLEL_STATIC
      ("pre", "perform preprocessing and generate subproblems only")
      ("post", "read previously solved subproblems and compile solution")
      ("sampledepth", po::value<int>()->default_value(10), "Randomness branching depth for initial sampling")
      ("samplesizes", po::value<string>(), "Sequence of sample sizes for complexity prediction (in 10^5 nodes)")
      ("samplerepeat", po::value<int>()->default_value(1), "Number of sample sequence repeats")
#endif
      ("bound-file,b", po::value<string>(), "file with initial lower bound on solution cost")
      ("initial-bound", po::value<double>(), "initial lower bound on solution cost" )
      ("lds,a",po::value<int>()->default_value(-1), "run initial LDS search with given limit (-1: disabled)")
      ("memlimit,m", po::value<int>()->default_value(-1), "approx. memory limit for mini buckets (in MByte)")
      ("seed", po::value<int>(), "seed for random number generator, time() otherwise")
      ("or", "use OR search (build pseudo tree as chain)")
      ("nosearch,n", "perform preprocessing, output stats, and exit")
#if not (defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC)
      ("reduce", po::value<string>(), "path to output the reduced network to (removes evidence and unary variables)")
#endif
      ("pst-file", po::value<string>(), "path to output the pseudo tree to, for plotting")
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
      return NULL;
    }

    opt->in_problemFile = vm["input-file"].as<string>();

    if (vm.count("evid-file"))
      opt->in_evidenceFile = vm["evid-file"].as<string>();

    if (vm.count("ordering"))
      opt->in_orderingFile = vm["ordering"].as<string>();

    if (vm.count("adaptive"))
      opt->autoIter = true;
    else
      opt->autoIter = false;

    if (vm.count("subproblem"))
      opt->in_subproblemFile = vm["subproblem"].as<string>();

    if (vm.count("sol-file"))
      opt->out_solutionFile = vm["sol-file"].as<string>();

    if (vm.count("minibucket"))
      opt->in_minibucketFile = vm["minibucket"].as<string>();

    if (vm.count("suborder")) {
      opt->subprobOrder = vm["suborder"].as<int>();
      if (opt->subprobOrder < 0 || opt->subprobOrder > 3) {
        cout << endl << desc << endl;
        exit(0);
      }
    }

    if (vm.count("ibound"))
      opt->ibound = vm["ibound"].as<int>();

    if (vm.count("cbound")) {
      opt->cbound = vm["cbound"].as<int>();
      opt->cbound_worker = vm["cbound"].as<int>();
    }

    if (vm.count("cbound-worker"))
      opt->cbound_worker = vm["cbound-worker"].as<int>();

    if (vm.count("iterations"))
      opt->order_iterations = vm["iterations"].as<int>();

    if (vm.count("max-width"))
      opt->maxWidthAbort = vm["max-width"].as<int>();

    if (vm.count("cutoff-depth"))
      opt->cutoff_depth = vm["cutoff-depth"].as<int>();

    if (vm.count("cutoff-width"))
      opt->cutoff_width = vm["cutoff-width"].as<int>();

    if (vm.count("cutoff-size"))
      opt->cutoff_size = vm["cutoff-size"].as<int>();

    if (vm.count("local-size"))
      opt->local_size = vm["local-size"].as<int>();

    if (vm.count("init-nodes"))
      opt->nodes_init = vm["init-nodes"].as<int>();

    if (vm.count("noauto"))
      opt->autoCutoff = false;
    else
      opt->autoCutoff = true;

    if (vm.count("procs"))
      opt->threads = vm["procs"].as<int>();

    if (vm.count("max-sub"))
      opt->maxSubprob = vm["max-sub"].as<int>();

    if (vm.count("bound-file"))
      opt->in_boundFile = vm["bound-file"].as<string>();

    if (vm.count("initial-bound"))
      opt->initialBound = vm["initial-bound"].as<double>();

    if (vm.count("lds"))
      opt->lds = vm["lds"].as<int>();

    if (vm.count("memlimit"))
      opt->memlimit = vm["memlimit"].as<int>();

    if (vm.count("or"))
      opt->orSearch = true;
    else
      opt->orSearch = false;

    if(vm.count("nosearch"))
      opt->nosearch = true;
    else
      opt->nosearch = false;

    if (vm.count("rotate"))
      opt->rotate = true;
    if (vm.count("rotatelimit"))
      opt->rotateLimit = vm["rotatelimit"].as<int>();

    if (vm.count("seed"))
      opt->seed = vm["seed"].as<int>();

    if (vm.count("tag"))
      opt->runTag = vm["tag"].as<string>();

    if (vm.count("pre"))
      opt->par_preOnly = true;
    else if (vm.count("post"))
      opt->par_postOnly = true;

    if (vm.count("sampledepth"))
      opt->sampleDepth = vm["sampledepth"].as<int>();
    if (vm.count("samplesizes"))
      opt->sampleSizes = vm["samplesizes"].as<string>();
    if (vm.count("samplerepeat"))
      opt->sampleRepeat = vm["samplerepeat"].as<int>();

    if (vm.count("reduce"))
      opt->out_reducedFile = vm["reduce"].as<string>();

    if (vm.count("pst-file"))
      opt->out_pstFile = vm["pst-file"].as<string>();

    if (vm.count("subproblem") && !vm.count("ordering")) {
      cerr << "Error: Specifying a subproblem requires reading a fixed ordering from file." << endl;
      exit(1);
    }

    {
      //Extract the problem name
      string& fname = opt->in_problemFile;
      size_t len, start, pos1, pos2;
#if defined(WIN32)
      pos1 = fname.find_last_of("\\");
#elif defined(LINUX)
      pos1 = fname.find_last_of("/");
#endif
      pos2 = fname.find_last_of(".");
      if (pos1 == string::npos) { len = pos2; start = 0; }
      else { len = (pos2-pos1-1); start = pos1+1; }
      opt->problemName = fname.substr(start, len);
    }

  } catch (exception& e) {
    cerr << "error: " << e.what() << endl;
    return NULL;
  } catch (...) {
    cerr << "Exception of unknown type!" << endl;
    exit(1);
  }

  return opt;

}

