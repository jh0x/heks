function(fast_hex_target_enable_avx target_name)
    include(CheckCXXCompilerFlag)

    # For cross-compilation scenarios, we check the target architecture (CMAKE_SYSTEM_PROCESSOR)
    # For native compilation, we check the host architecture (CMAKE_HOST_SYSTEM_PROCESSOR)
    set(target_arch "${CMAKE_SYSTEM_PROCESSOR}")

    # If not cross-compiling, use the host architecture instead
    if(NOT CMAKE_CROSSCOMPILING)
        set(target_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()

    # Only x86/x86_64 architectures support AVX
    if(NOT target_arch MATCHES "^(x86_64|AMD64|x86|i[3-6]86)$")
        message(
            STATUS
            "Target architecture does not support AVX: ${target_arch}"
        )
        return()
    endif()

    check_cxx_compiler_flag("-mavx2" CXX_SUPPORTS_MAVX2)
    if(CXX_SUPPORTS_MAVX2)
        message(STATUS "Enabling AVX2 for: ${target_name}")
        target_compile_options(${target_name} PRIVATE -mavx2)
        target_compile_definitions(${target_name} PRIVATE FAST_HEX_AVX2_ENABLED)
    endif()
endfunction()
