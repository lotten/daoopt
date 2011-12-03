################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../lib/sls4mpe/AssignmentManager.cpp \
../lib/sls4mpe/Bucket.cpp \
../lib/sls4mpe/MiniBucketElimination.cpp \
../lib/sls4mpe/ProbabilityTable.cpp \
../lib/sls4mpe/ProblemReader.cpp \
../lib/sls4mpe/Variable.cpp \
../lib/sls4mpe/fheap.cpp \
../lib/sls4mpe/main_algo.cpp \
../lib/sls4mpe/my_set.cpp \
../lib/sls4mpe/random_numbers.cpp \
../lib/sls4mpe/timer.cpp 

OBJS += \
./lib/sls4mpe/AssignmentManager.o \
./lib/sls4mpe/Bucket.o \
./lib/sls4mpe/MiniBucketElimination.o \
./lib/sls4mpe/ProbabilityTable.o \
./lib/sls4mpe/ProblemReader.o \
./lib/sls4mpe/Variable.o \
./lib/sls4mpe/fheap.o \
./lib/sls4mpe/main_algo.o \
./lib/sls4mpe/my_set.o \
./lib/sls4mpe/random_numbers.o \
./lib/sls4mpe/timer.o 

CPP_DEPS += \
./lib/sls4mpe/AssignmentManager.d \
./lib/sls4mpe/Bucket.d \
./lib/sls4mpe/MiniBucketElimination.d \
./lib/sls4mpe/ProbabilityTable.d \
./lib/sls4mpe/ProblemReader.d \
./lib/sls4mpe/Variable.d \
./lib/sls4mpe/fheap.d \
./lib/sls4mpe/main_algo.d \
./lib/sls4mpe/my_set.d \
./lib/sls4mpe/random_numbers.d \
./lib/sls4mpe/timer.d 


# Each subdirectory must supply rules for building sources it contributes
lib/sls4mpe/%.o: ../lib/sls4mpe/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache g++ -DLINUX -DPARALLEL_DYNAMIC -UNO_THREADS -I../include -I../lib -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


