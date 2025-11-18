# CMake function to define a fuzz target with common settings
function(add_fuzz_target TARGET_NAME FUZZ_SOURCE_FILE)
    add_executable(${TARGET_NAME} ${FUZZ_SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE fast_hex::fast_hex)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
    target_compile_options(
        ${TARGET_NAME}
        PRIVATE -fsanitize=fuzzer -fsanitize=address -fno-omit-frame-pointer
    )
    target_link_options(
        ${TARGET_NAME}
        PRIVATE -fsanitize=fuzzer -fsanitize=address
    )

    # Apply AVX support if available
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/avx_support.cmake)
    fast_hex_target_enable_avx(${TARGET_NAME})
endfunction()
