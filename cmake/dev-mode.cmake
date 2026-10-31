include(cmake/folders.cmake)

# Must come before the test directory is added, so the flags reach its targets
option(
    fast_hex_ENABLE_SANITIZERS
    "Build tests with AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)
if(fast_hex_ENABLE_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "fast_hex_ENABLE_SANITIZERS needs GCC or Clang")
    endif()
    # The CI presets define _FORTIFY_SOURCE=3, whose checked libc calls bypass
    # ASan's instrumentation. These options come after CMAKE_CXX_FLAGS_<CONFIG>
    # on the command line, so the -U wins
    add_compile_options(
        -fsanitize=address,undefined
        -fno-sanitize-recover=all
        -fno-omit-frame-pointer
        -g
        -U_FORTIFY_SOURCE
    )
    add_link_options(-fsanitize=address,undefined)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

#option(BUILD_MCSS_DOCS "Build documentation using Doxygen and m.css" OFF)
#if(BUILD_MCSS_DOCS)
#    include(cmake/docs.cmake)
#endif()

option(ENABLE_COVERAGE "Enable coverage support separate from CTest's" OFF)
if(ENABLE_COVERAGE)
    include(cmake/coverage.cmake)
endif()

option(fast_hex_ENABLE_FUZZ "Enable fuzz support" OFF)

option(fast_hex_ENABLE_BENCHMARK "Enable benchmark support" OFF)
if(fast_hex_ENABLE_BENCHMARK)
    include(ExternalProject)

    ExternalProject_Add(
        google_benchmark
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/contrib/benchmark
        BINARY_DIR ${CMAKE_BINARY_DIR}/benchmark-build
        INSTALL_DIR ${CMAKE_BINARY_DIR}/benchmark-install
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DBENCHMARK_ENABLE_TESTING=OFF -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
            -DBENCHMARK_ENABLE_EXAMPLES=OFF -DBENCHMARK_INSTALL_DOCS=OFF
        BUILD_ALWAYS OFF
        INSTALL_COMMAND ""
        # Ninja must know the libraries come from this step to order the build
        BUILD_BYPRODUCTS
            <BINARY_DIR>/src/libbenchmark.a
            <BINARY_DIR>/src/libbenchmark_main.a
    )

    add_library(benchmark::benchmark STATIC IMPORTED GLOBAL) # Add GLOBAL here
    add_library(benchmark::benchmark_main STATIC IMPORTED GLOBAL) # Add GLOBAL here

    add_dependencies(benchmark::benchmark google_benchmark)
    add_dependencies(benchmark::benchmark_main google_benchmark)

    # Set the imported locations
    set_target_properties(
        benchmark::benchmark
        PROPERTIES
            IMPORTED_LOCATION
                ${CMAKE_BINARY_DIR}/benchmark-build/src/libbenchmark.a
            INTERFACE_INCLUDE_DIRECTORIES
                ${CMAKE_SOURCE_DIR}/contrib/benchmark/include
    )

    set_target_properties(
        benchmark::benchmark_main
        PROPERTIES
            IMPORTED_LOCATION
                ${CMAKE_BINARY_DIR}/benchmark-build/src/libbenchmark_main.a
            INTERFACE_INCLUDE_DIRECTORIES
                ${CMAKE_SOURCE_DIR}/contrib/benchmark/include
            INTERFACE_LINK_LIBRARIES benchmark::benchmark
    )
endif()

include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)

add_folders(Project)
