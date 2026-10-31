#pragma once

// AVX-512 functions for CPUs with AVX-512 F, BW, VL and VBMI: Ice Lake, Zen 4 and later. Skylake-X and
// Cascade Lake don't have VBMI. Empty unless FAST_HEX_X86_64 is defined. See fast_hex.hpp for the contract.
//
//   encodeHexAvx512Vbmi, decodeHexAvx512Vbmi
//   cpuHasAvx512Vbmi
//
// VBMI is not part of x86-64-v4, so a -march=x86-64-v4 build is not enough for these. fast_hex_x86_64_v4.hpp
// has versions without VBMI.

#include "fast_hex_x86_64_v3.hpp" // Inputs under 32 bytes use the AVX2 code

#if defined(FAST_HEX_X86_64)

#    define FAST_HEX_TARGET_AVX512_VBMI __attribute__((target("avx512f,avx512bw,avx512vl,avx512vbmi")))

namespace heks
{
namespace heks_detail
{

// 32 bytes (low half of in) -> 64 hex chars
template <HexCase H>
FAST_HEX_TARGET_AVX512_VBMI inline __m512i encodeHex32VecVbmi(__m512i in)
{
    // clang-format off
    // Output byte j <- input byte j / 2
    const __m512i DUP = _mm512_set_epi8(
        31, 31, 30, 30, 29, 29, 28, 28, 27, 27, 26, 26, 25, 25, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17, 16, 16,
        15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0);
    const __m512i HEX_LUT_LOWER = _mm512_broadcast_i32x4(_mm_setr_epi8('0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'));
    const __m512i HEX_LUT_UPPER = _mm512_broadcast_i32x4(_mm_setr_epi8('0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'));
    // clang-format on
    // Bit offsets for each byte p of a qword: even p takes bits from 8p + 4 (high nibble),
    // odd p takes bits from 8p (low nibble). Bits above the nibble are ignored by the lookup.
    const __m512i NIBBLE_SHIFTS = _mm512_set1_epi64(0x38'34'28'24'18'14'08'04);

    __m512i dup = _mm512_permutexvar_epi8(DUP, in);
    __m512i nibs = _mm512_multishift_epi64_epi8(NIBBLE_SHIFTS, dup);
    // vpermb uses the low 6 bits of each index; the 16-entry table is repeated 4 times
    return _mm512_permutexvar_epi8(nibs, H == HexCase::Lower ? HEX_LUT_LOWER : HEX_LUT_UPPER);
}

// len is number of src bytes, >= 32
template <HexCase H>
FAST_HEX_TARGET_AVX512_VBMI inline void
encodeHexVecVbmiLarge(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    assert(len >= 32);
    auto raw_length = len;
    while (raw_length >= 64)
    {
        __m512i a = _mm512_castsi256_si512(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
        __m512i b = _mm512_castsi256_si512(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 32)));
        _mm512_storeu_si512(dest, encodeHex32VecVbmi<H>(a));
        _mm512_storeu_si512(dest + 64, encodeHex32VecVbmi<H>(b));
        src += 64;
        dest += 128;
        raw_length -= 64;
    }
    if (raw_length >= 32)
    {
        __m512i a = _mm512_castsi256_si512(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
        _mm512_storeu_si512(dest, encodeHex32VecVbmi<H>(a));
        src += 32;
        dest += 64;
        raw_length -= 32;
    }
    if (raw_length > 0)
    {
        // Encode the last 32 bytes again with a plain load/store. This overlaps output that is already
        // written, but stays inside the buffers.
        __m512i a = _mm512_castsi256_si512(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + raw_length - 32)));
        _mm512_storeu_si512(dest + raw_length * 2 - 64, encodeHex32VecVbmi<H>(a));
    }
}

template <HexCase H>
FAST_HEX_TARGET_AVX512_VBMI inline void
encodeHexVecVbmiImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    if (len < 32)
        encodeHexSmallVec<H>(dest, src, len);
    else
        encodeHexVecVbmiLarge<H>(dest, src, len);
}

// 64 hex chars -> 32 decoded bytes, each in the low byte of a 16-bit lane
FAST_HEX_TARGET_AVX512_VBMI inline __m512i unhexPairsVbmi(__m512i value)
{
    // clang-format off
    // Indexed by the low 6 bits of the char: '0'-'9' -> 0x30-0x39, 'A'-'F' -> 0x01-0x06, 'a'-'f' -> 0x21-0x26
    const __m512i UNHEX_LUT = _mm512_set_epi8(
        0, 0, 0, 0, 0, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,        // 0x3F-0x30
        0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 14, 13, 12, 11, 10, 0,  // 0x2F-0x20
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,        // 0x1F-0x10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 14, 13, 12, 11, 10, 0); // 0x0F-0x00
    // clang-format on
    __m512i nibs = _mm512_permutexvar_epi8(value, UNHEX_LUT);
    return _mm512_maddubs_epi16(nibs, _mm512_set1_epi16(0x0110));
}

// len >= 32
FAST_HEX_TARGET_AVX512_VBMI inline void
decodeHexVecVbmiLarge(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    assert(len >= 32);
    auto raw_length = len;
    // clang-format off
    // Low byte of every 16 bit lane of a, then b
    const __m512i EVEN_BYTES = _mm512_set_epi8(
            126, 124, 122, 120, 118, 116, 114, 112, 110, 108, 106, 104, 102, 100, 98, 96, 94, 92, 90, 88, 86, 84, 82, 80, 78, 76, 74, 72, 70, 68, 66, 64,
            62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0);
    // clang-format on
    while (raw_length >= 64)
    {
        __m512i a = unhexPairsVbmi(_mm512_loadu_si512(src));
        __m512i b = unhexPairsVbmi(_mm512_loadu_si512(src + 64));
        _mm512_storeu_si512(dest, _mm512_permutex2var_epi8(a, EVEN_BYTES, b));
        src += 128;
        dest += 64;
        raw_length -= 64;
    }
    if (raw_length >= 32)
    {
        __m512i a = unhexPairsVbmi(_mm512_loadu_si512(src));
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), _mm512_cvtepi16_epi8(a));
        src += 64;
        dest += 32;
        raw_length -= 32;
    }
    if (raw_length > 0)
    {
        __m512i a = unhexPairsVbmi(_mm512_loadu_si512(src + raw_length * 2 - 64));
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest + raw_length - 32), _mm512_cvtepi16_epi8(a));
    }
}

// len is number of dest bytes (i.e. half of src length). Always inlined into decodeHexAvx512Vbmi, as in the v4 header
FAST_HEX_TARGET_AVX512_VBMI __attribute__((always_inline)) inline void
decodeHexVecVbmiImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    if (len > 16 && len < 32)
    {
        __m256i first = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
        __m256i last = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + len * 2 - 32));
        __m256i a = _mm512_cvtepi16_epi8(unhexPairsVbmi(_mm512_inserti64x4(_mm512_castsi256_si512(first), last, 1)));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), _mm256_castsi256_si128(a));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + len - 16), _mm256_extracti128_si256(a, 1));
    }
    else if (len < 32)
        decodeHexSmallVec(dest, src, len);
    else
        decodeHexVecVbmiLarge(dest, src, len);
}

} // namespace heks_detail

// Whether the CPU can run the AVX-512 VBMI functions. Works from static constructors too. Cheap, but not free:
// hoist it out of hot loops.
inline bool cpuHasAvx512Vbmi()
{
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") && __builtin_cpu_supports("avx512vl")
        && __builtin_cpu_supports("avx512vbmi");
}

template <class Case>
FAST_HEX_TARGET_AVX512_VBMI void
encodeHexAvx512Vbmi(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len, Case)
{
    heks_detail::encodeHexVecVbmiImpl<Case::value>(dest, src, static_cast<size_t>(len));
}

FAST_HEX_TARGET_AVX512_VBMI inline void
decodeHexAvx512Vbmi(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::decodeHexVecVbmiImpl(dest, src, static_cast<size_t>(len));
}

} // namespace heks

#    undef FAST_HEX_TARGET_AVX512_VBMI

#endif // defined(FAST_HEX_X86_64)
