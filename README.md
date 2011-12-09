DAOOPT: Distributed AND/OR Optimization
=======================================

An implementation of sequential and distributed AND/OR Branch and
Bound for MPE (max-product) problems expressed over graphical models
like Bayes and Markov networks.

*By Lars Otten, University of California, Irvine -- see LICENSE.txt
 for licensing details*

Compilation / Usage
-------------------

### Compilation

A recent set of [Boost library](http://www.boost.org) headers is
required to compile the solver. To compile the different solver
variants (see references below for explanation), simply run `make` in
the following folders:

* `Worker` -- Purely sequential AOBB solver.
* `Static` -- Static master mode (also needs worker binaries).
* `Dynamic` -- Dynamic master mode (also needs worker binaries).

In addition, the following two Makefiles are provided

* `Debug` -- Compiles any of the above (through preprocessor defines)
  with debug flags.
* `Windows` -- Windows executable of the sequential solver using MinGW
  compiler, though currently broken.

By default, the *ccache* compiler cache is used by the makefiles. To
disable it, simply run the supplied script `ccache-deactivate.sh` from
within the respective build folder.

Some features (such as computing the optimal tuple vs. only its cost)
can be turned on/off by setting the respective preprocessor defines in
`include/DEFINES.h`.

### Usage

To see the list of command line parameters, run the solver with the
`--help` argument. Problem input should be in [UAI
format](http://graphmod.ics.uci.edu/uai08/FileFormat/), a simple
text-based format to specify graphical model problems. Gzipped input
is supported.

### Sequential execution

Simply compile the `Worker` variant and run it on your problem, no
further setup needed.

### Distributed execution

DAOOPT assumes the Condor grid environment and the machine that the
master executable runs on needs to be able to submit jobs. The file
`daoopt-template.condor` from the working directory will be the basis
for the parallel subproblem submissions, it can be used to customize
Condor options. The sequential solver will be used for the parallel
subproblems, so you will also need to compile that first for the
appropriate architecture: rename the sequential solver to
`daoopt.INTEL` for 32 bit Linux hosts and/or `daoopt.X86_64` for 64
bit Linux hosts; placed in the working directory these will be used
automatically.

Background / References
-----------------------

### AND/OR Branch and Bound

AND/OR Branch and Bound (AOBB) is a search framework developed in
[Rina Dechter's](http://www.ics.uci.edu/~dechter/) research group at
[UC Irvine](http://www.uci.edu/). Relevant publications:

* Rina Dechter and Robert Mateescu. "AND/OR Search Spaces for
  Graphical Models". In Artificial Intelligence, Volume 171 (2-3),
  pages 73-106, 2006.
* Radu Marinescu and Rina Dechter. "AND/OR Branch-and-Bound Search for
  Combinatorial Optimization in Graphical Models." In Artificial
  Intelligence, Volume 173 (16-17), pages 1457-1491, 2009.
* Radu Marinescu and Rina Dechter. "Memory Intensive AND/OR Search
  for Combinatorial Optimization in Graphical Models." In Artificial
  Intelligence, Volume 173 (16-17), pages 1492-1524, 2009.

### Distributed AND/OR Branch and Bound

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

### Acknowledgments

This work has been partially supported by NIH grant 5R01HG004175-03 and
NSF grants IIS-0713118 and IIS-1065618.

Disclaimer
----------

This code was written for research purposes and does therefore not
always adhere to established coding practices and guidelines. View and
use at your own risk. ;-)