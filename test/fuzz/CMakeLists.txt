cmake_minimum_required(VERSION 3.14)

project(fast_hexTests LANGUAGES CXX)

include(../../cmake/project-is-top-level.cmake)
include(../../cmake/folders.cmake)

# Include the fuzz target function
include(../../cmake/fuzz-targets.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
    find_package(fast_hex REQUIRED)
    enable_testing()
endif()

# ---- Tests ----

# Fast hex roundtrip fuzz target
add_fuzz_target(fast_hex_fuzz_roundtrip fuzz_roundtrip.cpp)

# Fast hex decode fuzz target
add_fuzz_target(fast_hex_fuzz_decode fuzz_decode.cpp)

set(FUZZ_RUNTIME
    10
    CACHE STRING
    "Number of seconds to run fuzz tests during ctest run"
)

# ---- End-of-file commands ----

add_folders(Fuzz)
