# CMake function to define a fuzz target with common settings
function(add_fuzz_target TARGET_NAME FUZZ_SOURCE_FILE)
    add_executable(${TARGET_NAME} ${FUZZ_SOURCE_FILE})
    # Fuzz sources use fast_hex_inline.hpp, so only the headers are needed. Linking the library
    # as well would give a second, uninstrumented definition of every function under test
    target_link_libraries(${TARGET_NAME} PRIVATE fast_hex::headers)
    # Fuzz with the same namespace setting the library is built with
    if(fast_hex_USE_NAMESPACE)
        target_compile_definitions(
            ${TARGET_NAME}
            PRIVATE FAST_HEX_USE_NAMESPACE=1
        )
    else()
        target_compile_definitions(
            ${TARGET_NAME}
            PRIVATE FAST_HEX_USE_NAMESPACE=0
        )
    endif()
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    target_compile_options(
        ${TARGET_NAME}
        PRIVATE -fsanitize=fuzzer -fsanitize=address -fno-omit-frame-pointer
    )
    target_link_options(
        ${TARGET_NAME}
        PRIVATE -fsanitize=fuzzer -fsanitize=address
    )

    # Apply SIMD support if available
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/avx_support.cmake)
    fast_hex_target_enable_simd(${TARGET_NAME})

    add_test(
        NAME ${TARGET_NAME}
        COMMAND ${TARGET_NAME} -max_total_time=${FUZZ_RUNTIME}
    )
endfunction()
