#pragma once

// AVX-512 functions for x86-64-v4 CPUs: AVX-512 F, BW, CD, DQ and VL, as on Skylake-X, Cascade Lake, Ice Lake
// and Zen 4. Empty unless FAST_HEX_X86_64 is defined. See fast_hex.hpp for the contract.
//
//   encodeHexAvx512, decodeHexAvx512
//   cpuHasAvx512
//
// fast_hex_x86_64_icelake.hpp has faster versions for the CPUs that also have VBMI.

#include "fast_hex_x86_64_v3.hpp" // Inputs under 32 bytes use the AVX2 code

#if defined(FAST_HEX_X86_64)

#    define FAST_HEX_TARGET_AVX512 __attribute__((target("avx512f,avx512bw,avx512cd,avx512dq,avx512vl")))

namespace heks
{
namespace heks_detail
{

// 32 bytes -> 64 hex chars
template <HexCase H>
FAST_HEX_TARGET_AVX512 inline __m512i encodeHex32Vec512(__m256i in)
{
    // clang-format off
    const __m512i HEX_LUT_LOWER = _mm512_broadcast_i32x4(_mm_setr_epi8('0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'));
    const __m512i HEX_LUT_UPPER = _mm512_broadcast_i32x4(_mm_setr_epi8('0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'));
    // clang-format on

    // Each byte b in a 16-bit lane, then [b >> 4, b & 0xF] as the low and high byte of the lane
    __m512i wide = _mm512_cvtepu8_epi16(in);
    __m512i nibs = _mm512_or_si512(_mm512_srli_epi16(wide, 4), _mm512_slli_epi16(wide, 8));
    nibs = _mm512_and_si512(nibs, _mm512_set1_epi8(0x0F));
    return _mm512_shuffle_epi8(H == HexCase::Lower ? HEX_LUT_LOWER : HEX_LUT_UPPER, nibs);
}

// len is number of src bytes, >= 32
template <HexCase H>
FAST_HEX_TARGET_AVX512 inline void encodeHexVec512Large(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    assert(len >= 32);
    auto raw_length = len;
    while (raw_length >= 64)
    {
        __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
        __m256i b = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 32));
        _mm512_storeu_si512(dest, encodeHex32Vec512<H>(a));
        _mm512_storeu_si512(dest + 64, encodeHex32Vec512<H>(b));
        src += 64;
        dest += 128;
        raw_length -= 64;
    }
    if (raw_length >= 32)
    {
        _mm512_storeu_si512(dest, encodeHex32Vec512<H>(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src))));
        src += 32;
        dest += 64;
        raw_length -= 32;
    }
    if (raw_length > 0)
    {
        // Encode the last 32 bytes again with a plain load/store. This overlaps output that is already
        // written, but stays inside the buffers.
        __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + raw_length - 32));
        _mm512_storeu_si512(dest + raw_length * 2 - 64, encodeHex32Vec512<H>(a));
    }
}

template <HexCase H>
FAST_HEX_TARGET_AVX512 inline void encodeHexVec512Impl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    if (len < 32)
        encodeHexSmallVec<H>(dest, src, len);
    else
        encodeHexVec512Large<H>(dest, src, len);
}

// 64 hex chars -> 32 decoded bytes, each in the low byte of a 16-bit lane. unhexPairs on 512 bits
FAST_HEX_TARGET_AVX512 inline __m512i unhexPairs512(__m512i value)
{
    const __m512i DELTA = _mm512_broadcast_i32x4(_mm_setr_epi8(0, 0, 0, -0x30, -0x37, 0, -0x57, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    __m512i hi = _mm512_and_si512(_mm512_srli_epi16(value, 4), _mm512_set1_epi8(0x0F));
    __m512i nibs = _mm512_add_epi8(value, _mm512_shuffle_epi8(DELTA, hi));
    return _mm512_maddubs_epi16(nibs, _mm512_set1_epi16(0x0110));
}

// len >= 32
FAST_HEX_TARGET_AVX512 inline void decodeHexVec512Large(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    assert(len >= 32);
    auto raw_length = len;
    // packus gives the qwords of a and b alternately, per 128-bit lane: put a's first
    const __m512i QWORD_ORDER = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);
    while (raw_length >= 64)
    {
        __m512i a = unhexPairs512(_mm512_loadu_si512(src));
        __m512i b = unhexPairs512(_mm512_loadu_si512(src + 64));
        _mm512_storeu_si512(dest, _mm512_permutexvar_epi64(QWORD_ORDER, _mm512_packus_epi16(a, b)));
        src += 128;
        dest += 64;
        raw_length -= 64;
    }
    if (raw_length >= 32)
    {
        __m512i a = unhexPairs512(_mm512_loadu_si512(src));
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), _mm512_cvtepi16_epi8(a));
        src += 64;
        dest += 32;
        raw_length -= 32;
    }
    if (raw_length > 0)
    {
        __m512i a = unhexPairs512(_mm512_loadu_si512(src + raw_length * 2 - 64));
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest + raw_length - 32), _mm512_cvtepi16_epi8(a));
    }
}

// len is number of dest bytes (i.e. half of src length)
FAST_HEX_TARGET_AVX512 inline void decodeHexVec512Impl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    if (len > 16 && len < 32)
    {
        __m256i first = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
        __m256i last = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + len * 2 - 32));
        __m256i a = _mm512_cvtepi16_epi8(unhexPairs512(_mm512_inserti64x4(_mm512_castsi256_si512(first), last, 1)));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), _mm256_castsi256_si128(a));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + len - 16), _mm256_extracti128_si256(a, 1));
    }
    else if (len < 32)
        decodeHexSmallVec(dest, src, len);
    else
        decodeHexVec512Large(dest, src, len);
}

} // namespace heks_detail

// Whether the CPU can run the x86-64-v4 AVX-512 functions. Works from static constructors too. Cheap, but not
// free: hoist it out of hot loops.
inline bool cpuHasAvx512()
{
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") && __builtin_cpu_supports("avx512cd")
        && __builtin_cpu_supports("avx512dq") && __builtin_cpu_supports("avx512vl");
}

template <class Case>
FAST_HEX_TARGET_AVX512 void encodeHexAvx512(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len, Case)
{
    heks_detail::encodeHexVec512Impl<Case::value>(dest, src, static_cast<size_t>(len));
}

FAST_HEX_TARGET_AVX512 inline void decodeHexAvx512(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::decodeHexVec512Impl(dest, src, static_cast<size_t>(len));
}

} // namespace heks

#    undef FAST_HEX_TARGET_AVX512

#endif // defined(FAST_HEX_X86_64)
