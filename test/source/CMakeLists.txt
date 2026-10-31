cmake_minimum_required(VERSION 3.14)

project(fast_hexTests LANGUAGES CXX)

include(../../cmake/project-is-top-level.cmake)
include(../../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
    find_package(fast_hex REQUIRED)
    enable_testing()
endif()

add_library(doctest INTERFACE)
target_include_directories(
    doctest
    SYSTEM
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../../contrib/doctest
)
# ---- Tests ----

add_executable(
    fast_hex_test
    main.cpp
    test_encode_integral.cpp
    test_valid_inputs.cpp
    test_invalid_inputs.cpp
    test_simd_bounds.cpp
)
target_link_libraries(fast_hex_test PRIVATE fast_hex::headers doctest)
target_compile_features(fast_hex_test PRIVATE cxx_std_20)

add_test(NAME fast_hex_test COMMAND fast_hex_test)

# ---- End-of-file commands ----

add_folders(Test)
