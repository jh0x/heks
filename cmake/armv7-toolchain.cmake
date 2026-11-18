# Cross-compile for 32-bit ARMv7 with NEON (armhf) using the distribution's
# cross compiler, and run test executables under QEMU user-mode emulation.
#
# On Debian and Ubuntu: apt-get install g++-arm-linux-gnueabihf qemu-user

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

set(CROSS arm-linux-gnueabihf)

# The armhf default is VFPv3-D16 without NEON: the NEON functions need it on.
# Given as compiler arguments rather than in CMAKE_CXX_FLAGS_INIT, which the
# presets override by setting CMAKE_CXX_FLAGS. A compiler with arguments needs
# CMake 3.19, which the project's minimum of 3.20 covers
set(ARMV7_NEON_FLAGS -march=armv7-a -mfpu=neon -mfloat-abi=hard)
set(CMAKE_C_COMPILER ${CROSS}-gcc ${ARMV7_NEON_FLAGS})
set(CMAKE_CXX_COMPILER ${CROSS}-g++ ${ARMV7_NEON_FLAGS})

set(CMAKE_FIND_ROOT_PATH /usr/${CROSS})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# CTest runs every test executable through this automatically
set(CMAKE_CROSSCOMPILING_EMULATOR "qemu-arm;-L;/usr/${CROSS}")
