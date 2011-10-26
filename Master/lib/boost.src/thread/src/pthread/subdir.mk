################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../lib/boost.src/thread/src/pthread/once.cpp \
../lib/boost.src/thread/src/pthread/thread.cpp 

OBJS += \
./lib/boost.src/thread/src/pthread/once.o \
./lib/boost.src/thread/src/pthread/thread.o 

CPP_DEPS += \
./lib/boost.src/thread/src/pthread/once.d \
./lib/boost.src/thread/src/pthread/thread.d 


# Each subdirectory must supply rules for building sources it contributes
lib/boost.src/thread/src/pthread/%.o: ../lib/boost.src/thread/src/pthread/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache g++ -DLINUX -DPARALLEL_DYNAMIC -UNO_THREADS -I../include -I../lib -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


