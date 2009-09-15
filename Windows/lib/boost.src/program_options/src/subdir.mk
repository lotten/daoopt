################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../lib/boost.src/program_options/src/cmdline.cpp \
../lib/boost.src/program_options/src/config_file.cpp \
../lib/boost.src/program_options/src/convert.cpp \
../lib/boost.src/program_options/src/options_description.cpp \
../lib/boost.src/program_options/src/parsers.cpp \
../lib/boost.src/program_options/src/positional_options.cpp \
../lib/boost.src/program_options/src/utf8_codecvt_facet.cpp \
../lib/boost.src/program_options/src/value_semantic.cpp \
../lib/boost.src/program_options/src/variables_map.cpp \
../lib/boost.src/program_options/src/winmain.cpp 

OBJS += \
./lib/boost.src/program_options/src/cmdline.o \
./lib/boost.src/program_options/src/config_file.o \
./lib/boost.src/program_options/src/convert.o \
./lib/boost.src/program_options/src/options_description.o \
./lib/boost.src/program_options/src/parsers.o \
./lib/boost.src/program_options/src/positional_options.o \
./lib/boost.src/program_options/src/utf8_codecvt_facet.o \
./lib/boost.src/program_options/src/value_semantic.o \
./lib/boost.src/program_options/src/variables_map.o \
./lib/boost.src/program_options/src/winmain.o 

CPP_DEPS += \
./lib/boost.src/program_options/src/cmdline.d \
./lib/boost.src/program_options/src/config_file.d \
./lib/boost.src/program_options/src/convert.d \
./lib/boost.src/program_options/src/options_description.d \
./lib/boost.src/program_options/src/parsers.d \
./lib/boost.src/program_options/src/positional_options.d \
./lib/boost.src/program_options/src/utf8_codecvt_facet.d \
./lib/boost.src/program_options/src/value_semantic.d \
./lib/boost.src/program_options/src/variables_map.d \
./lib/boost.src/program_options/src/winmain.d 


# Each subdirectory must supply rules for building sources it contributes
lib/boost.src/program_options/src/%.o: ../lib/boost.src/program_options/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache i586-mingw32msvc-g++ -DWINDOWS -DNOTHREADS -I../include -I../lib -I/home/lars/workspace/boost_1_39_0 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


