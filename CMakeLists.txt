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

target_include_directories(
    fast_hex_headers
    ${warning_guard}
    INTERFACE
        "\$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "\$<INSTALL_INTERFACE:include>"
)

if(fast_hex_BUILD_LIBS)
    add_library(fast_hex_fast_hex source/fast_hex.cpp)
    add_library(fast_hex::fast_hex ALIAS fast_hex_fast_hex)

    include(GenerateExportHeader)
    generate_export_header(
        fast_hex_fast_hex
        BASE_NAME fast_hex
        EXPORT_FILE_NAME export/fast_hex/fast_hex_export.hpp
        CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
    )

    if(NOT fast_hex_BUILD_SHARED_LIBS)
        target_compile_definitions(
            fast_hex_fast_hex
            PUBLIC FAST_HEX_STATIC_DEFINE
        )
    endif()

    set_target_properties(
        fast_hex_fast_hex
        PROPERTIES
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN YES
            VERSION "${PROJECT_VERSION}"
            SOVERSION "${PROJECT_VERSION_MAJOR}"
            EXPORT_NAME fast_hex
            OUTPUT_NAME fast_hex
    )

    # Set compile definitions based on options
    if(fast_hex_USE_NAMESPACE)
        target_compile_definitions(
            fast_hex_fast_hex
            PUBLIC FAST_HEX_USE_NAMESPACE=1
        )
    else()
        target_compile_definitions(
            fast_hex_fast_hex
            PUBLIC FAST_HEX_USE_NAMESPACE=0
        )
    endif()

    target_include_directories(
        fast_hex_fast_hex
        ${warning_guard}
        PUBLIC "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    )

    target_include_directories(
        fast_hex_fast_hex
        SYSTEM
        PUBLIC "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
    )

    target_compile_features(fast_hex_fast_hex PUBLIC cxx_std_20)

    fast_hex_target_enable_avx(fast_hex_fast_hex)
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
