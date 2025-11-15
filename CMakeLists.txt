cmake_minimum_required(VERSION 3.14)

project(fast-hex
    VERSION 1.0.0
    DESCRIPTION "Hardware-accelerated hex encoding/decoding library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static lib...
add_library(fast-hex STATIC
    src/hex.cc
)
add_library(fast-hex::fast-hex ALIAS fast-hex)

# The header
target_sources(fast-hex PRIVATE
    src/hex.h
)

target_include_directories(fast-hex
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(fast-hex PRIVATE -march=native)

    target_compile_options(fast-hex PRIVATE -Wall -Wextra -Werror -Wpedantic)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(fast-hex PRIVATE -O3)
    endif()
elseif(MSVC)
    target_compile_options(fast-hex PRIVATE /arch:AVX2)

    target_compile_options(fast-hex PRIVATE /W4)
    target_compile_definitions(fast-hex PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Set CMAKE_PREFIX_PATH and installation
include(GNUInstallDirs)
install(TARGETS fast-hex
    EXPORT fast-hex-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES src/hex.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT fast-hex-config
    FILE fast-hex-config.cmake
    NAMESPACE fast-hex::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fast-hex
)
