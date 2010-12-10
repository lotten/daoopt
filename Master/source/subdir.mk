################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/BestFirst.cpp \
../source/BoundPropagator.cpp \
../source/BoundPropagatorMaster.cpp \
../source/BranchAndBound.cpp \
../source/BranchAndBoundMaster.cpp \
../source/Function.cpp \
../source/Graph.cpp \
../source/LimitedDiscrepancy.cpp \
../source/MiniBucket.cpp \
../source/MiniBucketElim.cpp \
../source/ParallelManager.cpp \
../source/Problem.cpp \
../source/ProgramOptions.cpp \
../source/Pseudotree.cpp \
../source/Random.cpp \
../source/Search.cpp \
../source/SearchMaster.cpp \
../source/SearchNode.cpp \
../source/SigHandler.cpp \
../source/Statistics.cpp \
../source/SubproblemCondor.cpp \
../source/hash.cpp \
../source/utils.cpp 

OBJS += \
./source/BestFirst.o \
./source/BoundPropagator.o \
./source/BoundPropagatorMaster.o \
./source/BranchAndBound.o \
./source/BranchAndBoundMaster.o \
./source/Function.o \
./source/Graph.o \
./source/LimitedDiscrepancy.o \
./source/MiniBucket.o \
./source/MiniBucketElim.o \
./source/ParallelManager.o \
./source/Problem.o \
./source/ProgramOptions.o \
./source/Pseudotree.o \
./source/Random.o \
./source/Search.o \
./source/SearchMaster.o \
./source/SearchNode.o \
./source/SigHandler.o \
./source/Statistics.o \
./source/SubproblemCondor.o \
./source/hash.o \
./source/utils.o 

CPP_DEPS += \
./source/BestFirst.d \
./source/BoundPropagator.d \
./source/BoundPropagatorMaster.d \
./source/BranchAndBound.d \
./source/BranchAndBoundMaster.d \
./source/Function.d \
./source/Graph.d \
./source/LimitedDiscrepancy.d \
./source/MiniBucket.d \
./source/MiniBucketElim.d \
./source/ParallelManager.d \
./source/Problem.d \
./source/ProgramOptions.d \
./source/Pseudotree.d \
./source/Random.d \
./source/Search.d \
./source/SearchMaster.d \
./source/SearchNode.d \
./source/SigHandler.d \
./source/Statistics.d \
./source/SubproblemCondor.d \
./source/hash.d \
./source/utils.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache g++ -DLINUX -DPARALLEL_DYNAMIC -UNOTHREADS -I../include -I../lib -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


