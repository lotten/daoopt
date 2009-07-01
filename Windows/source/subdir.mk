################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/BoundPropagator.cpp \
../source/BranchAndBound.cpp \
../source/Function.cpp \
../source/Graph.cpp \
../source/MiniBucketElim.cpp \
../source/Problem.cpp \
../source/ProgramOptions.cpp \
../source/Pseudotree.cpp \
../source/Random.cpp \
../source/SearchNode.cpp \
../source/SigHandler.cpp \
../source/SubproblemCondor.cpp \
../source/hash.cpp \
../source/utils.cpp 

OBJS += \
./source/BoundPropagator.o \
./source/BranchAndBound.o \
./source/Function.o \
./source/Graph.o \
./source/MiniBucketElim.o \
./source/Problem.o \
./source/ProgramOptions.o \
./source/Pseudotree.o \
./source/Random.o \
./source/SearchNode.o \
./source/SigHandler.o \
./source/SubproblemCondor.o \
./source/hash.o \
./source/utils.o 

CPP_DEPS += \
./source/BoundPropagator.d \
./source/BranchAndBound.d \
./source/Function.d \
./source/Graph.d \
./source/MiniBucketElim.d \
./source/Problem.d \
./source/ProgramOptions.d \
./source/Pseudotree.d \
./source/Random.d \
./source/SearchNode.d \
./source/SigHandler.d \
./source/SubproblemCondor.d \
./source/hash.d \
./source/utils.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DWINDOWS -DNOTHREADS -I../include -I../lib -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


