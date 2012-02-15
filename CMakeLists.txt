# CMake file for DAOOPT solver
# by Lars Otten, 2012

cmake_minimum_required(VERSION 2.6)
project(DAOOPT)

# To select the two master modes
option(DAOOPT_WORKER "Force worker binary" OFF)
option(DAOOPT_MASTER_STATIC "Force static master binary" OFF)
option(DAOOPT_MASTER_DYNAMIC "Force dynamic master binary" OFF)

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

# Optional forced version: static over dynamic master, worker over both
if(DAOOPT_WORKER)
  add_definitions(-DNOTHREADS)
else()
  if(DAOOPT_MASTER_STATIC)
    add_definitions(-DPARALLEL_STATIC)
  endif()
  if(DAOOPT_MASTER_DYNAMIC AND NOT DAOOPT_MASTER_STATIC)
    add_definitions(-DPARALLEL_DYNAMIC)
    find_package(Threads)  # includes pthread
  endif()
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
  ./source/BestFirst.cpp
  ./source/BoundPropagator.cpp
  ./source/BoundPropagatorMaster.cpp
  ./source/BranchAndBound.cpp
  ./source/BranchAndBoundMaster.cpp
  ./source/BranchAndBoundRotate.cpp
  ./source/BranchAndBoundSampler.cpp
  ./source/Function.cpp
  ./source/Graph.cpp
  ./source/hash.cpp
  ./source/LearningEngine.cpp
  ./source/LimitedDiscrepancy.cpp
  ./source/Main.cpp
  ./source/MiniBucket.cpp
  ./source/MiniBucketElim.cpp
  ./source/ParallelManager.cpp
  ./source/Problem.cpp
  ./source/ProgramOptions.cpp
  ./source/Pseudotree.cpp
  ./source/Random.cpp
  ./source/Search.cpp
  ./source/SearchMaster.cpp
  ./source/SearchNode.cpp
  ./source/SigHandler.cpp
  ./source/SLSWrapper.cpp
  ./source/Statistics.cpp
  ./source/SubproblemCondor.cpp
  ./source/SubprobStats.cpp
  ./source/utils.cpp
)

# Main executable and library dependencies
add_executable(daoopt daoopt.cpp ${FILES})
target_link_libraries(daoopt ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
