CC=/storage/Tablet/cm/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin/arm-linux-androideabi-gcc 
LD=/storage/Tablet/cm/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin/arm-linux-androideabi-ld
# LIBS=-L/storage/Tablet/cm/prebuilts/ndk/android-ndk-r7/platforms/android-14/arch-arm/usr/lib 
LIBS=-L/storage/Tablet/cm/out/target/product/generic/system/lib
INC=-I/storage/Tablet/cm/system/core/include -I/storage/Tablet/cm/prebuilts/ndk/android-ndk-r7/platforms/android-14/arch-arm/usr/include -I/storage/Tablet/cm/bionic/libc/include -I/storage/Tablet/cm/development/ndk/platforms/android-3/arch-arm/include -I/storage/Tablet/cm/prebuilts/ndk/android-ndk-r4/platforms/android-3/arch-arm/usr/include
# SROOT=--sysroot=/storage/Tablet/cm/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6
SROOT=--sysroot=/storage/Tablet/cm/prebuilts/ndk/android-ndk-r7/platforms/android-14/arch-arm
FLAGS=-static
FLAGS=
LFLAGS=-Wl,-static,$(SROOT)
LFLAGS=-Wl,-Bdynamic,$(SROOT)


all: 12d1_1003 12d1_1404 em820w_tool

12d1_1003: 12d1_1003.c
	$(CC) $(SROOT) $(FLAGS) $(LFLAGS) $(INC) $(LIBS) -lc 12d1_1003.c -o 12d1_1003

12d1_1404: 12d1_1404.c
	$(CC) $(SROOT) $(FLAGS) $(LFLAGS) $(INC) $(LIBS) -lc 12d1_1404.c -o 12d1_1404

em820w_tool: em820w_tool.c
	$(CC) $(SROOT) $(FLAGS) $(LFLAGS) $(INC) $(LIBS) -lcutils -lc em820w_tool.c -o em820w_tool
	strip em820w_tool
