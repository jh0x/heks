include(cmake/folders.cmake)

option(
    fast_hex_ENABLE_SANITIZERS
    "Build tests with AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)

    # The fuzz targets set their own. The CI presets define _FORTIFY_SOURCE=3,
    # whose checked libc calls bypass ASan's instrumentation. Target options come
    # after CMAKE_CXX_FLAGS_<CONFIG> on the command line, so the -U wins
    if(fast_hex_ENABLE_SANITIZERS)
        target_compile_options(
            fast_hex_test
            PRIVATE
                -fsanitize=address,undefined
                -fno-sanitize-recover=all
                -fno-omit-frame-pointer
                -g
                -U_FORTIFY_SOURCE
        )
        target_link_options(fast_hex_test PRIVATE -fsanitize=address,undefined)
    endif()
endif()

option(fast_hex_ENABLE_FUZZ "Enable fuzz support" OFF)

option(fast_hex_ENABLE_BENCHMARK "Enable benchmark support" OFF)
if(fast_hex_ENABLE_BENCHMARK)
    include(ExternalProject)

    # A toolchain file may give the compiler arguments that CMAKE_CXX_COMPILER
    # alone would lose, as armv7-toolchain.cmake does, so it is passed on too.
    # One that names no compiler leaves the choice to CMAKE_CXX_COMPILER.
    # project() has already made a relative one absolute in the cache
    set(benchmark_compiler_args "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
    if(CMAKE_TOOLCHAIN_FILE)
        list(
            APPEND
            benchmark_compiler_args
            "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        )
    endif()

    ExternalProject_Add(
        google_benchmark
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/contrib/benchmark
        BINARY_DIR ${CMAKE_BINARY_DIR}/benchmark-build
        INSTALL_DIR ${CMAKE_BINARY_DIR}/benchmark-install
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=Release
            ${benchmark_compiler_args} -DBENCHMARK_ENABLE_TESTING=OFF
            -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DBENCHMARK_ENABLE_EXAMPLES=OFF
            -DBENCHMARK_INSTALL_DOCS=OFF
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
