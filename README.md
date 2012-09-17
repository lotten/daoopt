DAOOPT: Distributed AND/OR Optimization
=======================================

An implementation of sequential and distributed AND/OR Branch-and-Bound
and its Breadth-Rotating AND/OR Branch-and-Bound enhancement for
combinatorial optimization problems expressed as MPE (max-product)
queries over graphical models like Bayes and Markov networks.

Also implements the following:

* full context-based caching.
* mini-buckets for heuristic generation.
* minfill algorithm to find variable orderings.
* limited discrepancy search to quickly find initial solution.
* stochastic local search to quickly find initial solution (via GLS+
  code by Frank Hutter).

*By Lars Otten, University of California, Irvine. Main AOBB source
 code under GPL, included libraries vary -- see LICENSE.txt for
 details*

Compilation
-----------

A recent set of [Boost library](http://www.boost.org) headers is
required to compile the solver, either in the system-wide include path
or copied/symlinked into `./lib/boost` locally (confirmed to work is
version 1.47.0).

### CMake

The easiest and most universal way of compilation is provided through
the included CMake files. Create a `build` subfolder and from within
it run `cmake ..`. Afterwards `make all` starts compilation, while
`make edit_cache` allows to choose between release and debug compiler
flags, toggle static linking, and select one of the solver variants
(see references below); the default choice is the release-optimized,
dynamically linked, sequential solver.

### Included makefiles

Alternatively, makefiles for the different solver variants are
provided directly in the following folders:

* `Worker` -- Purely sequential AOBB solver.
* `Static` -- Static master mode (also needs worker binaries).
* `Dynamic` -- Dynamic master mode (also needs worker binaries).

In addition, the following two Makefiles are provided

* `Debug` -- Compiles any of the above (through preprocessor defines)
  with debug flags.
* `Windows` -- Windows executable of the sequential solver using MinGW
  compiler (not tested in a while and probably broken).

By default, the *ccache* compiler cache is used by the makefiles. To
disable it, simply run the supplied script `ccache-deactivate.sh` from
within the respective build folder.

Some features (such as computing the optimal tuple vs. only its cost)
can be turned on/off by setting the respective preprocessor defines in
`include/DEFINES.h`.

Usage
-----

To see the list of command line parameters, run the solver with the
`--help` argument. Problem input should be in [UAI
format](http://graphmod.ics.uci.edu/uai08/FileFormat/), a simple
text-based format to specify graphical model problems. Gzipped input
is supported.

### Sequential execution

Simply compile the `Worker` variant and run it on your problem, no
further setup needed.

### Distributed execution

DAOOPT assumes the Condor grid environment, the machine that the
master executable runs on needs to be able to submit jobs. The file
`daoopt-template.condor` from the working directory will be the basis
for the parallel subproblem submissions, it can be used to customize
Condor options. The sequential solver will be used for the parallel
subproblems, so you will also need to compile that first (probably
statically linked) for the appropriate architecture: rename the
sequential solver to `daoopt.INTEL` for 32 bit Linux hosts and/or
`daoopt.X86_64` for 64 bit Linux hosts; placed in the working
directory these will be used automatically.

Background / References
-----------------------

### AND/OR Branch-and-Bound

AND/OR Branch-and-Bound (AOBB) is a search framework developed in
[Rina Dechter's](http://www.ics.uci.edu/~dechter/) research group at
[UC Irvine](http://www.uci.edu/), most recently enhanced through
Breadth-Rotating AND/OR Branch-and-Bound (BRAOBB). Relevant
publications:

* Rina Dechter and Robert Mateescu. "AND/OR Search Spaces for
  Graphical Models". In Artificial Intelligence, Volume 171 (2-3),
  pages 73-106, 2006.
* Radu Marinescu and Rina Dechter. "AND/OR Branch-and-Bound Search for
  Combinatorial Optimization in Graphical Models." In Artificial
  Intelligence, Volume 173 (16-17), pages 1457-1491, 2009.
* Radu Marinescu and Rina Dechter. "Memory Intensive AND/OR Search
  for Combinatorial Optimization in Graphical Models." In Artificial
  Intelligence, Volume 173 (16-17), pages 1492-1524, 2009.
* Lars Otten and Rina Dechter. "Anytime AND/OR Depth-first Search for
  Combinatorial Optimization." In AI Communications, Volume 25 (3), pages
  211-227, 2012.

### Distributed AND/OR Branch-and-Bound

A recent expansion of the AOBB framework to allow using the parallel
resources of a computational grid; it is still the focus of ongoing
research. Some relevant publications:

* Lars Otten and Rina Dechter. "Towards Parallel Search for
  Optimization in Graphical Models". In Proceedings of ISAIM'10, Fort
  Lauderdale, FL, USA, January 2010.
* Lars Otten and Rina Dechter. "Load Balancing for Parallel Branch and
  Bound". In SofT'10 Workshop and CRAGS'10 Workshop, at CP'10,
  St. Andrews, Scotland, September 2010.
* Lars Otten and Rina Dechter. "Learning Subproblem Complexities in
  Distributed Branch and Bound". In DISCML'11 Workshop, at NIPS'11,
  Granada, Spain, December 2011.
* Lars Otten and Rina Dechter. "A Case Study in Complexity Estimation:
  Towards Parallel Branch-and-Bound over Graphical Models." In Proceedings
  of UAI'12, Catalina Island, CA, U.S.A, August 2012.

### Acknowledgments

This work has been partially supported by NIH grant 5R01HG004175-03 and
NSF grants IIS-0713118 and IIS-1065618.

Disclaimer
----------

This code was written for research purposes and does therefore not
strictly adhere to established coding practices and guidelines. View
and use at your own risk! ;-)

