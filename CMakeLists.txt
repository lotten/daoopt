cmake_minimum_required(VERSION 2.6)
project(DAOOPT)

# Included folders for header files
include_directories(
  ${DAOOPT_SOURCE_DIR}/include
  ${DAOOPT_SOURCE_DIR}/lib
)

# Compiler flags
add_definitions(-DLINUX -DNOTHREADS -O3 -Wall)

# Libraries in the lib folder via recursive CMakeLists.txt
add_subdirectory(lib)
set(LIBS Gzstream Sls4mpe BoostProgOpt BoostThread)

# Main daoopt source files
set(FILES
  ./source/BranchAndBoundMaster.cpp
  ./source/Search.cpp
  ./source/BranchAndBound.cpp
  ./source/BestFirst.cpp
  ./source/Function.cpp
  ./source/SLSWrapper.cpp
  ./source/Random.cpp
  ./source/Pseudotree.cpp
  ./source/SearchNode.cpp
  ./source/LimitedDiscrepancy.cpp
  ./source/BoundPropagator.cpp
  ./source/Problem.cpp
  ./source/ProgramOptions.cpp
  ./source/LearningEngine.cpp
  ./source/BoundPropagatorMaster.cpp
  ./source/utils.cpp
  ./source/ParallelManager.cpp
  ./source/Statistics.cpp
  ./source/BranchAndBoundSampler.cpp
  ./source/Main.cpp
  ./source/MiniBucketElim.cpp
  ./source/SearchMaster.cpp
  ./source/BranchAndBoundRotate.cpp
  ./source/hash.cpp
  ./source/MiniBucket.cpp
  ./source/Graph.cpp
  ./source/SubproblemCondor.cpp
  ./source/SigHandler.cpp
)

# Main executable and library dependencies
add_executable(daooptW daoopt.cpp ${FILES})
target_link_libraries(daooptW ${LIBS})
