#include <cstdint>
#ifdef FAST_HEX_STATIC_SHARED_LIBRARY
#    include <fast_hex/fast_hex.hpp>
#else
#    include "fast_hex/fast_hex_inline.hpp"
#endif

#include <benchmark/benchmark.h>

#include <cstring>
#include <random>
#include <vector>

#ifdef FAST_HEX_USE_NAMESPACE
using namespace heks;
#endif


std::vector<uint8_t> createBinaryData(size_t size)
{
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i)
    {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    return data;
}

std::vector<uint8_t> createHexData(size_t binarySize)
{
    auto binary = createBinaryData(binarySize);
    std::vector<uint8_t> hex(binarySize * 2);
    encodeHexLower(hex.data(), binary.data(), RawLength{binarySize});
    return hex;
}

template <typename T>
std::vector<T> createInputData(size_t count, T)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
    std::vector<T> data(count);
    for (size_t i = 0; i < count; ++i)
    {
        if constexpr (sizeof(T) <= 8)
        {
            data[i] = static_cast<T>(dist(rng));
        }
        else
        {
            data[i] = (static_cast<T>(dist(rng)) << 64) | dist(rng);
        }
    }
    return data;
}

#define DEFINE_ENCODE_BENCHMARK(func_name, size_val, size_name) \
    static void BM_##func_name##_##size_name(benchmark::State & state) \
    { \
        auto data = createBinaryData(size_val); \
        std::vector<uint8_t> hex(size_val * 2); \
\
        for (auto _ : state) \
        { \
            func_name(hex.data(), data.data(), RawLength{size_val}); \
            benchmark::DoNotOptimize(hex); \
        } \
    } \
    BENCHMARK(BM_##func_name##_##size_name);

#define DEFINE_ENCODE_BENCHMARK_FAST(func_name, size_val, size_name) \
    static void BM_##func_name##_##size_name(benchmark::State & state) \
    { \
        auto data = createBinaryData(size_val); \
        std::vector<uint8_t> hex(size_val * 2); \
\
        for (auto _ : state) \
        { \
            func_name(hex.data(), data.data()); \
            benchmark::DoNotOptimize(hex); \
        } \
    } \
    BENCHMARK(BM_##func_name##_##size_name);

#define DEFINE_DECODE_BENCHMARK(func_name, size_val, size_name) \
    static void BM_##func_name##_##size_name(benchmark::State & state) \
    { \
        auto hex = createHexData(size_val); \
        std::vector<uint8_t> binary(size_val); \
\
        for (auto _ : state) \
        { \
            func_name(binary.data(), hex.data(), RawLength{size_val}); \
            benchmark::DoNotOptimize(binary); \
        } \
    } \
    BENCHMARK(BM_##func_name##_##size_name);

#define DEFINE_ENCODE_INTEGRAL_BENCHMARK(func_name, Type, size_val, size_name) \
    static void BM_##func_name##_##size_name(benchmark::State & state) \
    { \
        auto data = createInputData(size_val, Type{}); \
        std::vector<uint8_t> hex(size_val * 2 * sizeof(Type)); \
        for (auto _ : state) \
        { \
            for (size_t i = 0; i < data.size(); ++i) \
            { \
                func_name(hex.data() + i * sizeof(Type) * 2, data[i], lower); \
            } \
            benchmark::DoNotOptimize(hex); \
        } \
    } \
    BENCHMARK(BM_##func_name##_##size_name);

#define DEFINE_ENCODE_INTEGRAL_BENCHMARK2x8(func_name, size_val, size_name) \
    static void BM_##func_name##_##size_name(benchmark::State & state) \
    { \
        auto data = createInputData(size_val * 2, uint64_t{}); \
        std::vector<uint8_t> hex(size_val * 2 * sizeof(uint64_t) * 2); \
        for (auto _ : state) \
        { \
            for (size_t i = 0; i < data.size(); i += 2) \
            { \
                func_name(hex.data() + i * sizeof(uint64_t) * 2, &data[i], lower); \
            } \
            benchmark::DoNotOptimize(hex); \
        } \
    } \
    BENCHMARK(BM_##func_name##_##size_name);

#define DEFINE_DECODE_INTEGRAL_BENCHMARK(func_name, Type, size_val, size_name) \
    static void BM_##func_name##_##size_name(benchmark::State & state) \
    { \
        auto data = createInputData(size_val, Type{}); \
        std::vector<uint8_t> hex(size_val * 2 * sizeof(Type)); \
        for (size_t i = 0; i < data.size(); ++i) \
        { \
            encode_integral_naive<Type>(hex.data() + i * sizeof(Type) * 2, data[i], lower); \
        } \
        std::vector<Type> output(size_val); \
        for (auto _ : state) \
        { \
            for (size_t i = 0; i < data.size(); ++i) \
            { \
                output[i] = func_name<Type>(hex.data() + i * sizeof(Type) * 2); \
            } \
            benchmark::DoNotOptimize(output); \
        } \
    } \
    BENCHMARK(BM_##func_name##_##size_name);

// clang-format off

// ---- Encoding Benchmarks ----

DEFINE_ENCODE_BENCHMARK(encodeHexLower, 8, 8B)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 16, 16B)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 32, 32B)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 64, 64B)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 1024, 1KB)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 1024 * 1024, 1MB)

#if defined(FAST_HEX_AVX)
DEFINE_ENCODE_BENCHMARK_FAST(encodeHex8LowerFast, 8, 8B)
#endif

#if defined(FAST_HEX_AVX2)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 8, 8B)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 16, 16B)
DEFINE_ENCODE_BENCHMARK_FAST(encodeHex16LowerFast, 16, 16B)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 32, 32B)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 64, 64B)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 1024, 1KB)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 1024 * 1024, 1MB)
#endif // defined(FAST_HEX_AVX2)

#if defined(FAST_HEX_NEON)
DEFINE_ENCODE_BENCHMARK(encodeHexNeonLower, 8, 8B)
DEFINE_ENCODE_BENCHMARK_FAST(encodeHex8LowerNeon, 8, 8B)
DEFINE_ENCODE_BENCHMARK(encodeHexNeonLower, 16, 16B)
DEFINE_ENCODE_BENCHMARK(encodeHexNeonLower, 32, 32B)
DEFINE_ENCODE_BENCHMARK(encodeHexNeonLower, 64, 64B)
DEFINE_ENCODE_BENCHMARK(encodeHexNeonLower, 1024, 1KB)
DEFINE_ENCODE_BENCHMARK(encodeHexNeonLower, 1024 * 1024, 1MB)
#endif // FAST_HEX_NEON

// ---- Decoding Benchmarks ----

DEFINE_DECODE_BENCHMARK(decodeHexLUT, 8, 8B)
DEFINE_DECODE_BENCHMARK(decodeHexLUT, 16, 16B)
DEFINE_DECODE_BENCHMARK(decodeHexLUT, 32, 32B)
DEFINE_DECODE_BENCHMARK(decodeHexLUT, 64, 64B)
DEFINE_DECODE_BENCHMARK(decodeHexLUT, 1024, 1KB)
DEFINE_DECODE_BENCHMARK(decodeHexLUT, 1024 * 1024, 1MB)

DEFINE_DECODE_BENCHMARK(decodeHexLUT4, 8, 8B)
DEFINE_DECODE_BENCHMARK(decodeHexLUT4, 16, 16B)
DEFINE_DECODE_BENCHMARK(decodeHexLUT4, 32, 32B)
DEFINE_DECODE_BENCHMARK(decodeHexLUT4, 64, 64B)
DEFINE_DECODE_BENCHMARK(decodeHexLUT4, 1024, 1KB)
DEFINE_DECODE_BENCHMARK(decodeHexLUT4, 1024 * 1024, 1MB)

DEFINE_DECODE_BENCHMARK(decodeHexBMI, 8, 8B)
DEFINE_DECODE_BENCHMARK(decodeHexBMI, 16, 16B)
DEFINE_DECODE_BENCHMARK(decodeHexBMI, 32, 32B)
DEFINE_DECODE_BENCHMARK(decodeHexBMI, 64, 64B)
DEFINE_DECODE_BENCHMARK(decodeHexBMI, 1024, 1KB)
DEFINE_DECODE_BENCHMARK(decodeHexBMI, 1024 * 1024, 1MB)

#if defined(FAST_HEX_AVX2)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 8, 8B)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 16, 16B)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 32, 32B)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 64, 64B)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 1024, 1KB)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 1024 * 1024, 1MB)
#endif // defined(FAST_HEX_AVX2)

#ifndef FAST_HEX_STATIC_SHARED_LIBRARY
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral_naive, uint64_t, 1, 1_uint64)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral_naive, uint64_t, 8, 8_uint64)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral_naive, uint64_t, 1024, 1024_uint64)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral_naive, uint64_t, 1024 * 1024, 1048576_uint64)

#if defined(FAST_HEX_HAS_INT128)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral_naive, __uint128_t, 1, 1_uint128)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral_naive, __uint128_t, 8, 8_uint128)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral_naive, __uint128_t, 1024, 1024_uint128)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral_naive, __uint128_t, 1024 * 1024, 1048576_uint128)
#endif

#if defined(FAST_HEX_AVX) || defined(FAST_HEX_NEON)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral8, uint64_t, 1, 1_uint64)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral8, uint64_t, 8, 8_uint64)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral8, uint64_t, 1024, 1024_uint64)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral8, uint64_t, 1024 * 1024, 1048576_uint64)
#endif // defined(FAST_HEX_AVX)

#if defined(FAST_HEX_AVX2)
DEFINE_ENCODE_INTEGRAL_BENCHMARK2x8(encode_integral2x8, 1, 1_uint64x2)
DEFINE_ENCODE_INTEGRAL_BENCHMARK2x8(encode_integral2x8, 8, 8_uint64x2)
DEFINE_ENCODE_INTEGRAL_BENCHMARK2x8(encode_integral2x8, 1024, 1024_uint64x2)
DEFINE_ENCODE_INTEGRAL_BENCHMARK2x8(encode_integral2x8, 1024 * 1024, 1048576_uint64x2)

DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral16, __uint128_t, 1, 1_uint128)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral16, __uint128_t, 8, 8_uint128)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral16, __uint128_t, 1024, 1024_uint128)
DEFINE_ENCODE_INTEGRAL_BENCHMARK(encode_integral16, __uint128_t, 1024 * 1024, 1048576_uint128)
#endif // defined(FAST_HEX_AVX2)

#if defined(FAST_HEX_HAS_INT128)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, __uint128_t, 1, 1_uint128)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, __uint128_t, 8, 8_uint128)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, __uint128_t, 64, 64_uint128)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, __uint128_t, 1024, 1024_uint128)
#endif


DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint64_t, 1, 1_uint64)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint64_t, 8, 8_uint64)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint64_t, 64, 64_uint64)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint64_t, 1024, 1024_uint64)

DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint32_t, 1, 1_uint32)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint32_t, 8, 8_uint32)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint32_t, 64, 64_uint32)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint32_t, 1024, 1024_uint32)

DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint16_t, 1, 1_uint16)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint16_t, 8, 8_uint16)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint16_t, 64, 64_uint16)
DEFINE_DECODE_INTEGRAL_BENCHMARK(decode_integral_naive, uint16_t, 1024, 1024_uint16)

#if defined(FAST_HEX_AVX)
template<typename T>
inline auto __attribute__((always_inline)) _decode_integral8(const uint8_t* src)
{
    return decode_integral8(src);
}
DEFINE_DECODE_INTEGRAL_BENCHMARK(_decode_integral8, uint64_t, 1, 1_uint64)
DEFINE_DECODE_INTEGRAL_BENCHMARK(_decode_integral8, uint64_t, 8, 8_uint64)
DEFINE_DECODE_INTEGRAL_BENCHMARK(_decode_integral8, uint64_t, 64, 64_uint64)
DEFINE_DECODE_INTEGRAL_BENCHMARK(_decode_integral8, uint64_t, 1024, 1024_uint64)
#endif // defined(FAST_HEX_AVX)


#endif // FAST_HEX_STATIC_SHARED_LIBRARY

BENCHMARK_MAIN();

// clang-format on
