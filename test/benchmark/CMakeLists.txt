cmake_minimum_required(VERSION 3.14)

project(fast_hexTests LANGUAGES CXX)

include(../../cmake/project-is-top-level.cmake)
include(../../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
    find_package(fast_hex REQUIRED)
    enable_testing()
endif()

# ---- Benchmark ----

add_executable(fast_hex_bench fast_hex_bench.cpp)

target_link_libraries(
    fast_hex_bench
    PRIVATE fast_hex::fast_hex benchmark::benchmark benchmark::benchmark_main
)
target_compile_features(fast_hex_bench PRIVATE cxx_std_17)

# ---- AVX Support ----
include(../../cmake/avx_support.cmake)
fast_hex_target_enable_avx(fast_hex_bench)

if(fast_hex_ENABLE_MARCH_NATIVE)
    message(STATUS "Enabling -march=native for fast_hex_bench")
    target_compile_options(fast_hex_bench PRIVATE -march=native)
endif()

add_folders(Bench)
