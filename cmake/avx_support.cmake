# For cross-compilation scenarios, we check the target architecture (CMAKE_SYSTEM_PROCESSOR)
# For native compilation, we check the host architecture (CMAKE_HOST_SYSTEM_PROCESSOR)
set(target_arch "${CMAKE_SYSTEM_PROCESSOR}")

# If not cross-compiling, use the host architecture instead
if(NOT CMAKE_CROSSCOMPILING)
    set(target_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
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

# Nothing is needed for x86-64: the headers compile in all the AVX/AVX2/AVX-512 code with GCC/Clang,
# each SIMD function carries its own __attribute__((target(...))) and the Auto functions check the CPU
# at runtime. No -mavx* flags either, as they would let the compiler auto-vectorize the scalar code.
function(fast_hex_target_enable_simd target_name)
    # Apply NEON if enabled
    if(fast_hex_ENABLE_NEON)
        message(STATUS "Enabling NEON for: ${target_name}")
        target_compile_definitions(${target_name} PRIVATE FAST_HEX_NEON=1)
    endif()
endfunction()
