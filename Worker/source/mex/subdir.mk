################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/mex/Factor.cpp \
../source/mex/factorgraph.cpp \
../source/mex/grapmodel.cpp 

OBJS += \
./source/mex/Factor.o \
./source/mex/factorgraph.o \
./source/mex/graphmodel.o 

CPP_DEPS += \
./source/mex/Factor.d \
./source/mex/factorgraph.d \
./source/mex/graphmodel.d 


# Each subdirectory must supply rules for building sources it contributes
source/mex/%.o: ../source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache g++ -DLINUX -DNOTHREADS -UPARALLEL_MODE -I../include -I../lib -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


