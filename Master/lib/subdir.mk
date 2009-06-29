################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_UPPER_SRCS += \
../lib/gzstream.C 

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

C_UPPER_DEPS += \
./lib/gzstream.d 


# Each subdirectory must supply rules for building sources it contributes
lib/%.o: ../lib/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	ccache gcc -DLINUX -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

lib/%.o: ../lib/%.C
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	ccache g++ -DLINUX -DUSE_THREADS -I../include -I../lib -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


