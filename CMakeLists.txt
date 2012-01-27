# CMake file for DAOOPT solver
# by Lars Otten, 2012

cmake_minimum_required(VERSION 2.6)
project(DAOOPT)

# To select the two master modes
option(DAOOPT_MASTER_STATIC "Build static master binary" OFF)
option(DAOOPT_MASTER_DYNAMIC "Build dynamic master binary" OFF)

# To enable static linking of the final daoopt binary
option(DAOOPT_LINK_STATIC "Link binary statically" OFF)

# General Compiler flags
add_definitions(-Wall)

if(WIN32)
  add_definitions(-DWINDOWS)
else()
  add_definitions(-DLINUX)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -O0 -g3")
set(CMAKE_C_FLAGS_DEBUG "-DDEBUG -O0 -g3")

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELEASE "-O3")

if(DAOOPT_LINK_STATIC)
  set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

# Master binary: static has precedence over dynamic mode
if(DAOOPT_MASTER_STATIC)
  add_definitions(-DPARALLEL_STATIC)
endif()
if(DAOOPT_MASTER_DYNAMIC AND NOT DAOOPT_MASTER_STATIC)
  add_definitions(-DPARALLEL_DYNAMIC)
  find_package(Threads)  # includes pthread
endif()
if(NOT DAOOPT_MASTER_DYNAMIC AND NOT DAOOPT_MASTER_STATIC)
  add_definitions(-DNOTHREADS)
endif()

# Included folders for header files
include_directories(
  ${DAOOPT_SOURCE_DIR}/include
  ${DAOOPT_SOURCE_DIR}/lib
)

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
target_link_libraries(daooptW ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
