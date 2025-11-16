cmake_minimum_required(VERSION 3.14)

project(fast-hexTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
    find_package(fast-hex REQUIRED)
    enable_testing()
endif()

add_library(doctest INTERFACE)
target_include_directories(
    doctest
    SYSTEM
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/doctest
)
# ---- Tests ----

add_executable(
    fast-hex_test
    source/main.cpp
    source/test_valid_inputs.cpp
    source/test_invalid_inputs.cpp
)
target_link_libraries(fast-hex_test PRIVATE fast-hex::fast-hex doctest)
target_compile_features(fast-hex_test PRIVATE cxx_std_17)

add_test(NAME fast-hex_test COMMAND fast-hex_test)

# ---- End-of-file commands ----

add_folders(Test)
