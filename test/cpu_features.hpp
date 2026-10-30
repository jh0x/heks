#pragma once

#include "fast_hex/fast_hex_common.hpp"

// The build compiles in AVX, AVX2 and AVX-512 code without requiring the build machine to support it,
// so anything calling those functions directly has to check the CPU first. AVX2 and AVX-512 use the
// library's own checks. doctest evaluates skip() from static constructors, which can run before the
// compiler runtime has filled in the CPU feature bits, hence the __builtin_cpu_init() calls.
inline bool cpuHasAvx()
{
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx") != 0;
#else
    return false;
#endif
}

inline bool cpuHasAvx2()
{
#if defined(FAST_HEX_AVX2_HELPERS)
#    if FAST_HEX_USE_NAMESPACE
    using namespace heks;
#    endif
    __builtin_cpu_init();
    return heks_detail::cpuHasAvx2();
#else
    return false;
#endif
}

inline bool cpuHasAvx512()
{
#if defined(FAST_HEX_AVX512)
#    if FAST_HEX_USE_NAMESPACE
    using namespace heks;
#    endif
    __builtin_cpu_init();
    return heks_detail::cpuHasAvx512();
#else
    return false;
#endif
}
