################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../lib/boost.src/thread/src/tss_null.cpp 

OBJS += \
./lib/boost.src/thread/src/tss_null.o 

CPP_DEPS += \
./lib/boost.src/thread/src/tss_null.d 


# Each subdirectory must supply rules for building sources it contributes
lib/boost.src/thread/src/%.o: ../lib/boost.src/thread/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache g++ -DDEBUG -DLINUX -I../include -I../lib -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


