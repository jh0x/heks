#pragma once

// The build compiles in AVX, AVX2 and AVX-512 code without requiring the build machine to support it,
// so anything calling those functions directly has to check the CPU first
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
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2") != 0;
#else
    return false;
#endif
}

inline bool cpuHasAvx512()
{
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") && __builtin_cpu_supports("avx512vl")
        && __builtin_cpu_supports("avx512vbmi");
#else
    return false;
#endif
}
