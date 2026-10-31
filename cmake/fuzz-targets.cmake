# CMake function to define a fuzz target with common settings
function(add_fuzz_target TARGET_NAME FUZZ_SOURCE_FILE)
    add_executable(${TARGET_NAME} ${FUZZ_SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE fast_hex::headers)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    # -fno-sanitize-recover makes a UBSan finding abort, so libFuzzer records it
    # as a crash. -U_FORTIFY_SOURCE because the checked libc calls bypass ASan
    target_compile_options(
        ${TARGET_NAME}
        PRIVATE
            -fsanitize=fuzzer,address,undefined
            -fno-sanitize-recover=all
            -fno-omit-frame-pointer
            -U_FORTIFY_SOURCE
    )
    target_link_options(
        ${TARGET_NAME}
        PRIVATE -fsanitize=fuzzer,address,undefined
    )

    add_test(
        NAME ${TARGET_NAME}
        COMMAND ${TARGET_NAME} -max_total_time=${FUZZ_RUNTIME}
    )
endfunction()
