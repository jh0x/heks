#include <fast_hex/fast_hex.hpp>

#include <benchmark/benchmark.h>

#include <cstring>
#include <vector>

#if FAST_HEX_USE_NAMESPACE
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

// ---- Encoding Benchmarks ----

DEFINE_ENCODE_BENCHMARK(encodeHexLower, 8, 8B)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 16, 16B)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 32, 32B)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 64, 64B)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 1024, 1KB)
DEFINE_ENCODE_BENCHMARK(encodeHexLower, 1024 * 1024, 1MB)

#if defined(__AVX__)
DEFINE_ENCODE_BENCHMARK_FAST(encodeHex8LowerFast, 8, 8B)
#endif

#if defined(__AVX2__)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 8, 8B)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 16, 16B)
DEFINE_ENCODE_BENCHMARK_FAST(encodeHex16LowerFast, 16, 16B)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 32, 32B)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 64, 64B)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 1024, 1KB)
DEFINE_ENCODE_BENCHMARK(encodeHexLowerVec, 1024 * 1024, 1MB)
#endif // defined(__AVX2__)

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

#if defined(__AVX2__)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 8, 8B)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 16, 16B)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 32, 32B)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 64, 64B)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 1024, 1KB)
DEFINE_DECODE_BENCHMARK(decodeHexVec, 1024 * 1024, 1MB)
#endif // defined(__AVX2__)

BENCHMARK_MAIN();
