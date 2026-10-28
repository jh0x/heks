include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)

# For cross-compilation scenarios, we check the target architecture (CMAKE_SYSTEM_PROCESSOR)
# For native compilation, we check the host architecture (CMAKE_HOST_SYSTEM_PROCESSOR)
set(target_arch "${CMAKE_SYSTEM_PROCESSOR}")

# If not cross-compiling, use the host architecture instead
if(NOT CMAKE_CROSSCOMPILING)
    set(target_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

# Check compiler support for AVX and AVX2, and adjust flags if not supported
if(target_arch MATCHES "^(x86_64|AMD64|x86|i[3-6]86)$")
    check_cxx_compiler_flag("-mavx" CXX_SUPPORTS_MAVX)
    if(NOT CXX_SUPPORTS_MAVX AND fast_hex_ENABLE_AVX)
        message(
            WARNING
            "Disabling fast_hex_ENABLE_AVX because compiler doesn't support -mavx flag"
        )
        set(fast_hex_ENABLE_AVX OFF)
    endif()

    check_cxx_compiler_flag("-mavx2" CXX_SUPPORTS_MAVX2)
    if(NOT CXX_SUPPORTS_MAVX2 AND fast_hex_ENABLE_AVX2)
        message(
            WARNING
            "Disabling fast_hex_ENABLE_AVX2 because compiler doesn't support -mavx2 flag"
        )
        set(fast_hex_ENABLE_AVX2 OFF)
    endif()

    if(fast_hex_ENABLE_AVX512)
        check_cxx_compiler_flag("-mavx512vbmi" CXX_SUPPORTS_MAVX512VBMI)
        if(NOT CXX_SUPPORTS_MAVX512VBMI)
            message(
                WARNING
                "Disabling fast_hex_ENABLE_AVX512 because compiler doesn't support -mavx512vbmi flag"
            )
            set(fast_hex_ENABLE_AVX512 OFF)
        elseif(CMAKE_CROSSCOMPILING)
            message(
                STATUS
                "Disabling fast_hex_ENABLE_AVX512 because the target CPU cannot be checked when cross-compiling"
            )
            set(fast_hex_ENABLE_AVX512 OFF)
        else()
            # Unlike AVX2, AVX-512 is far from universal on x86-64, so only enable it if the host has it
            check_cxx_source_runs(
                "int main() { __builtin_cpu_init(); return (__builtin_cpu_supports(\"avx512bw\") && __builtin_cpu_supports(\"avx512vl\") && __builtin_cpu_supports(\"avx512vbmi\")) ? 0 : 1; }"
                HOST_SUPPORTS_AVX512VBMI
            )
            if(NOT HOST_SUPPORTS_AVX512VBMI)
                message(
                    STATUS
                    "Disabling fast_hex_ENABLE_AVX512 because host CPU doesn't support AVX-512 BW/VL/VBMI"
                )
                set(fast_hex_ENABLE_AVX512 OFF)
            endif()
        endif()
    endif()
else()
    # Architecture doesn't support AVX/AVX2, disable the flags
    if(fast_hex_ENABLE_AVX)
        message(
            STATUS
            "Target architecture does not support AVX: ${target_arch}, disabling fast_hex_ENABLE_AVX"
        )
        set(fast_hex_ENABLE_AVX OFF)
    endif()
    if(fast_hex_ENABLE_AVX2)
        message(
            STATUS
            "Target architecture does not support AVX2: ${target_arch}, disabling fast_hex_ENABLE_AVX2"
        )
        set(fast_hex_ENABLE_AVX2 OFF)
    endif()
    if(fast_hex_ENABLE_AVX512)
        message(
            STATUS
            "Target architecture does not support AVX-512: ${target_arch}, disabling fast_hex_ENABLE_AVX512"
        )
        set(fast_hex_ENABLE_AVX512 OFF)
    endif()
endif()

# For NEON, check architecture compatibility
if(
    NOT target_arch MATCHES "^(aarch64|armv7|arm|ARM64|arm64)$"
    AND fast_hex_ENABLE_NEON
)
    message(
        STATUS
        "Target architecture does not support NEON: ${target_arch}, disabling fast_hex_ENABLE_NEON"
    )
    set(fast_hex_ENABLE_NEON OFF)
endif()

function(fast_hex_target_enable_simd target_name)
    # Apply AVX if enabled
    if(fast_hex_ENABLE_AVX)
        message(STATUS "Enabling AVX for: ${target_name}")
        target_compile_options(${target_name} PRIVATE -mavx)
        target_compile_definitions(${target_name} PRIVATE FAST_HEX_AVX=1)
    endif()

    # Apply AVX2 if enabled
    if(fast_hex_ENABLE_AVX2)
        message(STATUS "Enabling AVX2 for: ${target_name}")
        target_compile_options(${target_name} PRIVATE -mavx2)
        target_compile_definitions(${target_name} PRIVATE FAST_HEX_AVX2=1)
    endif()

    # Apply AVX-512 if enabled
    if(fast_hex_ENABLE_AVX512)
        message(STATUS "Enabling AVX-512 for: ${target_name}")
        target_compile_options(
            ${target_name}
            PRIVATE -mavx512f -mavx512bw -mavx512vl -mavx512vbmi
        )
        target_compile_definitions(${target_name} PRIVATE FAST_HEX_AVX512=1)
    endif()

    # Apply NEON if enabled
    if(fast_hex_ENABLE_NEON)
        message(STATUS "Enabling NEON for: ${target_name}")
        target_compile_definitions(${target_name} PRIVATE FAST_HEX_NEON=1)
    endif()
endfunction()
