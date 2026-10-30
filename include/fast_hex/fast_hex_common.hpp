#pragma once

// Configuration shared by fast_hex_inline.hpp and the tests

#if defined(_MSC_VER)
#    define FAST_HEX_RESTRICT __restrict // The C99 keyword, available as a C++ extension
#else
#    define FAST_HEX_RESTRICT __restrict__
#endif

#if FAST_HEX_USE_NAMESPACE
#    define FAST_HEX_NAMESPACE_OPEN \
        namespace heks \
        {
#    define FAST_HEX_NAMESPACE_CLOSE }
#else
#    define FAST_HEX_NAMESPACE_OPEN
#    define FAST_HEX_NAMESPACE_CLOSE
#endif

// With GCC/Clang on x86-64 all the SIMD code is compiled in: each function has its own target
// attribute and the Auto functions check the CPU at runtime. MSVC has no target attributes.
// Define FAST_HEX_NO_AVX, FAST_HEX_NO_AVX2 or FAST_HEX_NO_AVX512 to leave out that path's functions.
#if defined(__GNUC__) && defined(__x86_64__)
#    if !defined(FAST_HEX_AVX) && !defined(FAST_HEX_NO_AVX)
#        define FAST_HEX_AVX 1
#    endif
#    if !defined(FAST_HEX_AVX2) && !defined(FAST_HEX_NO_AVX2)
#        define FAST_HEX_AVX2 1
#    endif
#    if !defined(FAST_HEX_AVX512) && !defined(FAST_HEX_NO_AVX512)
#        define FAST_HEX_AVX512 1
#    endif
#endif

// The AVX-512 code handles inputs under 32 bytes with the AVX2 (and AVX) helpers, so those stay in with
// FAST_HEX_NO_AVX2 as long as AVX-512 is enabled. Every AVX-512 CPU has AVX2.
#if defined(FAST_HEX_AVX2) || defined(FAST_HEX_AVX512)
#    define FAST_HEX_AVX2_HELPERS
#endif

FAST_HEX_NAMESPACE_OPEN

namespace heks_detail
{
// Whether the CPU running the code can execute the AVX2 / AVX-512 functions that are compiled in.
// Checked on every call, which is a single load: the compiler runtime fills the feature bits in from
// an early constructor. Before that they read as 0, so a call from a very early constructor gets the
// scalar code. A function-local static would add a guard check to every call.
// No shortcut for __AVX2__ etc.: these are inline, so their definition must not depend on per-file
// flags (the linker keeps one copy for files built with and without them).
#if defined(FAST_HEX_AVX2_HELPERS)
inline bool cpuHasAvx2()
{
    return __builtin_cpu_supports("avx2") != 0;
}
#endif // defined(FAST_HEX_AVX2_HELPERS)

#if defined(FAST_HEX_AVX512)
// GCC and Clang fold the four checks into one load and one mask test
inline bool cpuHasAvx512()
{
    return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") && __builtin_cpu_supports("avx512vl")
        && __builtin_cpu_supports("avx512vbmi");
}
#endif // defined(FAST_HEX_AVX512)
} // namespace heks_detail

FAST_HEX_NAMESPACE_CLOSE
