cmake_minimum_required(VERSION 3.14)

project(fast_hexTests LANGUAGES CXX)

include(../../cmake/project-is-top-level.cmake)
include(../../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
    find_package(fast_hex REQUIRED)
    enable_testing()
endif()
# ---- Tests ----

add_executable(fast_hex_fuzz fuzz_roundtrip.cpp)
target_link_libraries(fast_hex_fuzz PRIVATE fast_hex::fast_hex)
target_compile_features(fast_hex_fuzz PRIVATE cxx_std_17)
target_compile_options(
    fast_hex_fuzz
    PRIVATE -fsanitize=fuzzer -fsanitize=address -fno-omit-frame-pointer
)

target_link_options(fast_hex_fuzz PRIVATE -fsanitize=fuzzer -fsanitize=address)

set(FUZZ_RUNTIME
    10
    CACHE STRING
    "Number of seconds to run fuzz tests during ctest run"
)

# ---- AVX Support ----
include(../../cmake/avx_support.cmake)
fast_hex_target_enable_avx(fast_hex_fuzz)

# ---- End-of-file commands ----

add_folders(Fuzz)
