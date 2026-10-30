cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    fast_hex
    VERSION 0.1.0
    DESCRIPTION "Hex encode/decode"
    HOMEPAGE_URL "https://github.com/jh0x/fast_hex"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)
include(cmake/avx_support.cmake)

# Apply -march=native globally if enabled
if(fast_hex_ENABLE_MARCH_NATIVE)
    message(STATUS "Enabling -march=native for all targets")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

# ---- Declare library ----

add_library(fast_hex_headers INTERFACE)
add_library(fast_hex::headers ALIAS fast_hex_headers)
set_property(TARGET fast_hex_headers PROPERTY EXPORT_NAME headers)

# The install include directory is set by install(TARGETS ... INCLUDES) in install-rules.cmake
target_include_directories(
    fast_hex_headers
    ${warning_guard}
    INTERFACE "\$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_compile_features(fast_hex_headers INTERFACE cxx_std_20)

if(fast_hex_USE_NAMESPACE)
    target_compile_definitions(
        fast_hex_headers
        INTERFACE FAST_HEX_USE_NAMESPACE=1
    )
else()
    target_compile_definitions(
        fast_hex_headers
        INTERFACE FAST_HEX_USE_NAMESPACE=0
    )
endif()

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT fast_hex_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(
        AUTHOR_WARNING
        "Developer mode is intended for developers of fast_hex"
    )
endif()

include(cmake/dev-mode.cmake)
