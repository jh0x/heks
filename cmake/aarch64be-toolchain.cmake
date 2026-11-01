# Cross-compile for big-endian AArch64 (aarch64_be) using a Bootlin
# toolchain, and run test executables under QEMU user-mode emulation.
#
# Download the "aarch64be--glibc--stable" toolchain from
# https://toolchains.bootlin.com/downloads/releases/toolchains/aarch64be/tarballs/
# and set BOOTLIN_TC_ROOT (environment variable) to the extracted top-level
# directory. An environment variable is used because this file is read before
# -D command-line cache variables are processed.

if(NOT "$ENV{BOOTLIN_TC_ROOT}" STREQUAL "")
    set(BOOTLIN_TC_ROOT "$ENV{BOOTLIN_TC_ROOT}")
else()
    set(BOOTLIN_TC_ROOT /opt/aarch64be)
endif()

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64_be)

set(CROSS aarch64_be-buildroot-linux-gnu-)

set(CMAKE_C_COMPILER ${BOOTLIN_TC_ROOT}/bin/${CROSS}gcc)
set(CMAKE_CXX_COMPILER ${BOOTLIN_TC_ROOT}/bin/${CROSS}g++)
set(CMAKE_AR ${BOOTLIN_TC_ROOT}/bin/${CROSS}ar)
set(CMAKE_RANLIB ${BOOTLIN_TC_ROOT}/bin/${CROSS}ranlib)
set(CMAKE_STRIP ${BOOTLIN_TC_ROOT}/bin/${CROSS}strip)

set(CMAKE_SYSROOT ${BOOTLIN_TC_ROOT}/aarch64_be-buildroot-linux-gnu/sysroot)
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# CTest runs every test executable through this automatically.
# -cpu max,sve=on makes the SVE code paths reachable under emulation.
set(CMAKE_CROSSCOMPILING_EMULATOR
    "qemu-aarch64_be-static;-cpu;max,sve=on;-L;${CMAKE_SYSROOT}"
)
