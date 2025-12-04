include(cmake/folders.cmake)

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
            -DBENCHMARK_ENABLE_TESTING=OFF -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
            -DBENCHMARK_ENABLE_EXAMPLES=OFF -DBENCHMARK_INSTALL_DOCS=OFF
        BUILD_ALWAYS OFF
        INSTALL_COMMAND ""
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
