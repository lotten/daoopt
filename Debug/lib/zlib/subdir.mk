################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lib/zlib/adler32.c \
../lib/zlib/compress.c \
../lib/zlib/crc32.c \
../lib/zlib/deflate.c \
../lib/zlib/gzclose.c \
../lib/zlib/gzlib.c \
../lib/zlib/gzread.c \
../lib/zlib/gzwrite.c \
../lib/zlib/infback.c \
../lib/zlib/inffast.c \
../lib/zlib/inflate.c \
../lib/zlib/inftrees.c \
../lib/zlib/trees.c \
../lib/zlib/uncompr.c \
../lib/zlib/zutil.c 

OBJS += \
./lib/zlib/adler32.o \
./lib/zlib/compress.o \
./lib/zlib/crc32.o \
./lib/zlib/deflate.o \
./lib/zlib/gzclose.o \
./lib/zlib/gzlib.o \
./lib/zlib/gzread.o \
./lib/zlib/gzwrite.o \
./lib/zlib/infback.o \
./lib/zlib/inffast.o \
./lib/zlib/inflate.o \
./lib/zlib/inftrees.o \
./lib/zlib/trees.o \
./lib/zlib/uncompr.o \
./lib/zlib/zutil.o 

C_DEPS += \
./lib/zlib/adler32.d \
./lib/zlib/compress.d \
./lib/zlib/crc32.d \
./lib/zlib/deflate.d \
./lib/zlib/gzclose.d \
./lib/zlib/gzlib.d \
./lib/zlib/gzread.d \
./lib/zlib/gzwrite.d \
./lib/zlib/infback.d \
./lib/zlib/inffast.d \
./lib/zlib/inflate.d \
./lib/zlib/inftrees.d \
./lib/zlib/trees.d \
./lib/zlib/uncompr.d \
./lib/zlib/zutil.d 


# Each subdirectory must supply rules for building sources it contributes
lib/zlib/%.o: ../lib/zlib/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	ccache gcc -DDEBUG -DLINUX -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


