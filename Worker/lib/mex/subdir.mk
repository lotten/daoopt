################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../lib/mex/Factor.cpp \
../lib/mex/factorgraph.cpp \
../lib/mex/graphmodel.cpp 

OBJS += \
./lib/mex/Factor.o \
./lib/mex/factorgraph.o \
./lib/mex/graphmodel.o 

CPP_DEPS += \
./lib/mex/Factor.d \
./lib/mex/factorgraph.d \
./lib/mex/graphmodel.d 


# Each subdirectory must supply rules for building sources it contributes
lib/mex/%.o: ../lib/mex/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache g++ -DLINUX -DNOTHREADS -I../include -I../lib -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


