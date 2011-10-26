################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../lib/gzstream.cpp 

C_SRCS += \
../lib/adler32.c \
../lib/compress.c \
../lib/crc32.c \
../lib/deflate.c \
../lib/gzio.c \
../lib/infback.c \
../lib/inffast.c \
../lib/inflate.c \
../lib/inftrees.c \
../lib/trees.c \
../lib/uncompr.c \
../lib/zutil.c 

OBJS += \
./lib/adler32.o \
./lib/compress.o \
./lib/crc32.o \
./lib/deflate.o \
./lib/gzio.o \
./lib/gzstream.o \
./lib/infback.o \
./lib/inffast.o \
./lib/inflate.o \
./lib/inftrees.o \
./lib/trees.o \
./lib/uncompr.o \
./lib/zutil.o 

C_DEPS += \
./lib/adler32.d \
./lib/compress.d \
./lib/crc32.d \
./lib/deflate.d \
./lib/gzio.d \
./lib/infback.d \
./lib/inffast.d \
./lib/inflate.d \
./lib/inftrees.d \
./lib/trees.d \
./lib/uncompr.d \
./lib/zutil.d 

CPP_DEPS += \
./lib/gzstream.d 


# Each subdirectory must supply rules for building sources it contributes
lib/%.o: ../lib/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	ccache i586-mingw32msvc-gcc -DWINDOWS -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

lib/%.o: ../lib/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache i586-mingw32msvc-g++ -DWINDOWS -DNOTHREADS -I../include -I../lib -I/home/lars/workspace/boost_1_39_0 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


