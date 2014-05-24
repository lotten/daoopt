################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../lib/cvo/cvo_samples.cpp 

OBJS += \
./lib/cvo/cvo_samples.o 

CPP_DEPS += \
./lib/cvo/cvo_samples.d 


# Each subdirectory must supply rules for building sources it contributes
lib/cvo/%.o: ../lib/cvo/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache g++ -DDEBUG -DLINUX -I../include -I../lib -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


