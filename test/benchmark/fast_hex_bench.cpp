#include "fast_hex/fast_hex.hpp"
#include "fast_hex/fast_hex_arm_neon.hpp"
#include "fast_hex/fast_hex_x86_64_icelake.hpp"
#include "fast_hex/fast_hex_x86_64_v3.hpp"
#include "fast_hex/fast_hex_x86_64_v4.hpp"

#include <benchmark/benchmark.h>

#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

using namespace heks;

namespace
{
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
    encodeHex(hex.data(), binary.data(), RawLength{binarySize}, lower);
    return hex;
}

// The AVX2 and AVX-512 code is compiled in even where the CPU can't run it, so those benchmarks are skipped there
enum class Isa
{
    None,
    Avx2,
    Avx512,
    Avx512Vbmi
};

bool skipUnsupported([[maybe_unused]] Isa isa, [[maybe_unused]] benchmark::State & state)
{
#if defined(FAST_HEX_X86_64)
    if ((isa == Isa::Avx2 && !cpuHasAvx2()) || (isa == Isa::Avx512 && !cpuHasAvx512()) || (isa == Isa::Avx512Vbmi && !cpuHasAvx512Vbmi()))
    {
        state.SkipWithMessage("CPU does not support the instruction set");
        return true;
    }
#endif
    return false;
}

template <auto Func, bool Encode>
void run(uint8_t * dst, const uint8_t * src, size_t size)
{
    if constexpr (Encode)
        Func(dst, src, RawLength{size}, lower);
    else
        Func(dst, src, RawLength{size});
}

template <auto Func, bool Encode, Isa I>
void BM_hex_Runtime(benchmark::State & state)
{
    if (skipUnsupported(I, state))
        return;
    const auto size = static_cast<size_t>(state.range(0));
    const auto offset = static_cast<size_t>(state.range(1));
    auto data = Encode ? createBinaryData(size) : createHexData(size);
    std::vector<uint8_t> input(data.size() + 128);
    std::vector<uint8_t> output((Encode ? size * 2 : size) + 128);
    auto * src = reinterpret_cast<uint8_t *>((reinterpret_cast<uintptr_t>(input.data()) + 63) & ~uintptr_t{63}) + offset;
    auto * dst = reinterpret_cast<uint8_t *>((reinterpret_cast<uintptr_t>(output.data()) + 63) & ~uintptr_t{63}) + offset;
    std::memcpy(src, data.data(), data.size());
    for (auto _ : state)
    {
        run<Func, Encode>(dst, src, size);
        benchmark::DoNotOptimize(dst);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(size));
}

// Output right after input in one buffer (neighbors)
template <auto Func, bool Encode, Isa I>
void BM_hex_Adjacent(benchmark::State & state)
{
    if (skipUnsupported(I, state))
        return;
    const auto size = static_cast<size_t>(state.range(0));
    auto data = Encode ? createBinaryData(size) : createHexData(size);
    std::vector<uint8_t> buffer(data.size() + (Encode ? size * 2 : size));
    auto * src = buffer.data();
    auto * dst = src + data.size();
    std::memcpy(src, data.data(), data.size());
    for (auto _ : state)
    {
        run<Func, Encode>(dst, src, size);
        benchmark::DoNotOptimize(dst);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(size));
}

// A 256-bit integer the way ClickHouse's UInt256 stores it
struct UInt256
{
    uint64_t limbs[4];
};

template <typename T>
std::vector<T> createInputData(size_t count)
{
    std::mt19937_64 rng(42);
    std::vector<T> data(count);
    for (auto & value : data)
    {
        uint64_t limbs[(sizeof(T) + 7) / 8];
        for (auto & limb : limbs)
        {
            limb = rng();
        }
        std::memcpy(&value, limbs, sizeof(T));
    }
    return data;
}

template <typename T, auto Encode, Isa I>
void BM_encodeInt(benchmark::State & state)
{
    if (skipUnsupported(I, state))
        return;
    const auto data = createInputData<T>(static_cast<size_t>(state.range(0)));
    std::vector<uint8_t> hex(data.size() * sizeof(T) * 2);
    for (auto _ : state)
    {
        for (size_t i = 0; i < data.size(); ++i)
        {
            Encode(hex.data() + i * sizeof(T) * 2, data[i], lower);
        }
        benchmark::DoNotOptimize(hex.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

template <typename T, auto Decode, Isa I>
void BM_decodeInt(benchmark::State & state)
{
    if (skipUnsupported(I, state))
        return;
    const auto data = createInputData<T>(static_cast<size_t>(state.range(0)));
    std::vector<uint8_t> hex(data.size() * sizeof(T) * 2);
    for (size_t i = 0; i < data.size(); ++i)
    {
        encodeHexInt(hex.data() + i * sizeof(T) * 2, data[i], lower);
    }
    std::vector<T> decoded(data.size());
    for (auto _ : state)
    {
        for (size_t i = 0; i < decoded.size(); ++i)
        {
            decoded[i] = Decode(hex.data() + i * sizeof(T) * 2);
        }
        benchmark::DoNotOptimize(decoded.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
} // namespace

#define HEX_BENCHMARK(func, encode, isa) \
    BENCHMARK_TEMPLATE(BM_hex_Runtime, func, encode, isa) \
        ->ArgsProduct({{8, 15, 16, 31, 32, 33, 63, 64, 65, 1024, 1024 * 1024}, {0, 1}}) \
        ->ArgNames({"bytes", "offset"}); \
    BENCHMARK_TEMPLATE(BM_hex_Adjacent, func, encode, isa)->ArgsProduct({{1, 3, 7, 8, 15, 16, 31, 32}})->ArgNames({"bytes"});

#define INT_BENCHMARK(T, encode, isa) BENCHMARK_TEMPLATE(BM_encodeInt, T, encode<T, lower_t>, isa)->Arg(1)->Arg(1024)->ArgName("count");
#define INT_DECODE_BENCHMARK(T, decode, isa) BENCHMARK_TEMPLATE(BM_decodeInt, T, decode<T>, isa)->Arg(1)->Arg(1024)->ArgName("count");

HEX_BENCHMARK(encodeHex<lower_t>, true, Isa::None)
HEX_BENCHMARK(decodeHex, false, Isa::None)
HEX_BENCHMARK(decodeHexLUT, false, Isa::None)
HEX_BENCHMARK(decodeHexLUT4, false, Isa::None)
INT_BENCHMARK(uint64_t, encodeHexInt, Isa::None)
#if defined(__SIZEOF_INT128__)
INT_BENCHMARK(__uint128_t, encodeHexInt, Isa::None)
#endif
INT_BENCHMARK(UInt256, encodeHexInt, Isa::None)
INT_DECODE_BENCHMARK(uint64_t, decodeHexInt, Isa::None)
#if defined(__SIZEOF_INT128__)
INT_DECODE_BENCHMARK(__uint128_t, decodeHexInt, Isa::None)
#endif
INT_DECODE_BENCHMARK(UInt256, decodeHexInt, Isa::None)

#if defined(FAST_HEX_X86_64)
HEX_BENCHMARK(encodeHexAvx2<lower_t>, true, Isa::Avx2)
HEX_BENCHMARK(decodeHexAvx2, false, Isa::Avx2)
INT_BENCHMARK(uint64_t, encodeHexIntAvx2, Isa::Avx2)
INT_BENCHMARK(__uint128_t, encodeHexIntAvx2, Isa::Avx2)
INT_BENCHMARK(UInt256, encodeHexIntAvx2, Isa::Avx2)
INT_DECODE_BENCHMARK(uint64_t, decodeHexIntAvx2, Isa::Avx2)
INT_DECODE_BENCHMARK(__uint128_t, decodeHexIntAvx2, Isa::Avx2)
INT_DECODE_BENCHMARK(UInt256, decodeHexIntAvx2, Isa::Avx2)

HEX_BENCHMARK(encodeHexAvx512<lower_t>, true, Isa::Avx512)
HEX_BENCHMARK(decodeHexAvx512, false, Isa::Avx512)

HEX_BENCHMARK(encodeHexAvx512Vbmi<lower_t>, true, Isa::Avx512Vbmi)
HEX_BENCHMARK(decodeHexAvx512Vbmi, false, Isa::Avx512Vbmi)
#endif

#if defined(FAST_HEX_NEON)
HEX_BENCHMARK(encodeHexNeon<lower_t>, true, Isa::None)
INT_BENCHMARK(uint64_t, encodeHexIntNeon, Isa::None)
#    if defined(__SIZEOF_INT128__)
INT_BENCHMARK(__uint128_t, encodeHexIntNeon, Isa::None)
#    endif
INT_BENCHMARK(UInt256, encodeHexIntNeon, Isa::None)
#endif

BENCHMARK_MAIN();
