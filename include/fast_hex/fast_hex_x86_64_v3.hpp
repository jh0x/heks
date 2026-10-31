#pragma once

// AVX2 functions, for x86-64-v3 CPUs (Haswell, Zen and later). Empty unless FAST_HEX_X86_64 is defined.
// See fast_hex.hpp for the contract.
//
//   encodeHexAvx2, decodeHexAvx2
//   encodeHexIntAvx2, decodeHexIntAvx2, encodeHexInt2x8Avx2
//   cpuHasAvx2

#include "fast_hex.hpp"

#if defined(FAST_HEX_X86_64)

#    include <cassert>
#    include <immintrin.h>

#    define FAST_HEX_TARGET_AVX2 __attribute__((target("avx2")))

// The 32 byte loops of decodeHexVecLarge and encodeHexVecLarge are by https://github.com/zbjornson/fast-hex,
// the decoder since reworked to decode nibbles via a pshufb delta table. The overlapping final blocks and the
// code for inputs under 32 bytes are not from upstream

namespace heks
{
namespace heks_detail
{

// 32 hex chars -> 16 decoded bytes, each in the low byte of a 16-bit lane.
// The nibble is c + delta[c >> 4], where delta is -'0' for '0'-'9' (0x3_), -'A' + 10 for 'A'-'F' (0x4_)
// and -'a' + 10 for 'a'-'f' (0x6_), so no masking is needed afterwards.
// maddubs then combines each pair of nibbles as hi * 16 + lo.
FAST_HEX_TARGET_AVX2 inline __m256i unhexPairs(const __m256i value)
{
    // clang-format off
    const __m256i DELTA = _mm256_setr_epi8(
        0, 0, 0, -0x30, -0x37, 0, -0x57, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -0x30, -0x37, 0, -0x57, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    // clang-format on

    __m256i hi = _mm256_and_si256(_mm256_srli_epi16(value, 4), _mm256_set1_epi8(0x0F));
    __m256i nibs = _mm256_add_epi8(value, _mm256_shuffle_epi8(DELTA, hi));
    return _mm256_maddubs_epi16(nibs, _mm256_set1_epi16(0x0110));
}

// 64 hex chars -> 32 bytes
FAST_HEX_TARGET_AVX2 inline __m256i decodeHex32(const uint8_t * src)
{
    __m256i a = unhexPairs(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
    __m256i b = unhexPairs(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 32)));
    __m256i packed = _mm256_packus_epi16(a, b); // a.lo b.lo a.hi b.hi
    return _mm256_permute4x64_epi64(packed, 0xD8);
}

FAST_HEX_TARGET_AVX2 inline void decodeHex32Vec(uint8_t * dest, const uint8_t * src)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), decodeHex32(src));
}

// 32 hex chars -> 16 bytes
FAST_HEX_TARGET_AVX2 inline __m128i decodeHex16(const uint8_t * src)
{
    __m256i pairs = unhexPairs(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
    return _mm_packus_epi16(_mm256_castsi256_si128(pairs), _mm256_extracti128_si256(pairs, 1));
}

FAST_HEX_TARGET_AVX2 inline void decodeHex16Vec(uint8_t * dest, const uint8_t * src)
{
    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), decodeHex16(src));
}

// 2 <= len < 16: decodes the first and last W hex chars in one pass, W={16,8,4} with
// W <= len * 2 < W * 2, so the two always overlap (they are the same block when len * 2 == W)
FAST_HEX_TARGET_AVX2 inline void decodeHexTinyVec(uint8_t * dest, const uint8_t * src, size_t len)
{
    const uint8_t * src_end = src + len * 2;
    __m128i first;
    __m128i last;
    if (len >= 8)
    {
        first = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
        last = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src_end - 16));
    }
    else if (len >= 4)
    {
        first = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src));
        last = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src_end - 8));
    }
    else
    {
        uint32_t a = 0;
        uint32_t b = 0;
        std::memcpy(&a, src, 4);
        std::memcpy(&b, src_end - 4, 4);
        first = _mm_cvtsi32_si128(static_cast<int>(a));
        last = _mm_cvtsi32_si128(static_cast<int>(b));
    }
    __m256i pairs = unhexPairs(_mm256_inserti128_si256(_mm256_castsi128_si256(first), last, 1));
    __m128i decoded = _mm_packus_epi16(_mm256_castsi256_si128(pairs), _mm256_extracti128_si256(pairs, 1));
    if (len >= 8)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dest), decoded);
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dest + len - 8), _mm_unpackhi_epi64(decoded, decoded));
    }
    else if (len >= 4)
    {
        const auto a = static_cast<uint32_t>(_mm_cvtsi128_si32(decoded));
        const auto b = static_cast<uint32_t>(_mm_extract_epi32(decoded, 2));
        std::memcpy(dest, &a, 4);
        std::memcpy(dest + len - 4, &b, 4);
    }
    else
    {
        const auto a = static_cast<uint16_t>(_mm_extract_epi16(decoded, 0));
        const auto b = static_cast<uint16_t>(_mm_extract_epi16(decoded, 4));
        std::memcpy(dest, &a, 2);
        std::memcpy(dest + len - 2, &b, 2);
    }
}

// len < 32, decoded in overlapping blocks with plain loads/stores
FAST_HEX_TARGET_AVX2 inline void decodeHexSmallVec(uint8_t * dest, const uint8_t * src, size_t len)
{
    if (len >= 16)
    {
        decodeHex16Vec(dest, src);
        if (len > 16)
            decodeHex16Vec(dest + len - 16, src + len * 2 - 32);
    }
    else if (len >= 2)
        decodeHexTinyVec(dest, src, len);
    else if (len == 1)
        dest[0] = static_cast<uint8_t>((unhexBitManip(src[0]) << 4) | unhexBitManip(src[1]));
}

// len >= 32
FAST_HEX_TARGET_AVX2 inline void decodeHexVecLarge(uint8_t * dest, const uint8_t * src, size_t len)
{
    assert(len >= 32);
    while (len >= 32)
    {
        decodeHex32Vec(dest, src);
        src += 64;
        dest += 32;
        len -= 32;
    }
    if (len > 0)
    {
        // Decode the final full block again
        decodeHex32Vec(dest + len - 32, src + len * 2 - 64);
    }
}

// len is number of dest bytes (i.e. half of src length). Always inlined into decodeHexAvx2, as in the v4 header
FAST_HEX_TARGET_AVX2 __attribute__((always_inline)) inline void
decodeHexVecImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    if (len < 32)
        decodeHexSmallVec(dest, src, len);
    else
        decodeHexVecLarge(dest, src, len);
}

// Nibbles (0-15) -> hex chars
template <HexCase H>
FAST_HEX_TARGET_AVX2 inline __m256i hex(__m256i value)
{
    // clang-format off
    if constexpr (H == HexCase::Lower)
        return _mm256_shuffle_epi8(_mm256_setr_epi8('0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'), value);
    else
        return _mm256_shuffle_epi8(_mm256_setr_epi8('0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'), value);
    // clang-format on
}

// a -> [a >> 4, a & 0b1111]
FAST_HEX_TARGET_AVX2 inline __m256i byte2nib(__m128i val)
{
    const __m256i ROT2 = _mm256_setr_epi8(
        -1, 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14, -1, 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14);
    __m256i doubled = _mm256_cvtepu8_epi16(val);
    __m256i hi = _mm256_srli_epi16(doubled, 4);
    __m256i lo = _mm256_shuffle_epi8(doubled, ROT2);
    __m256i bytes = _mm256_or_si256(hi, lo);
    bytes = _mm256_and_si256(bytes, _mm256_set1_epi8(0b1111));
    return bytes;
}

// Low 8 bytes of v8 -> 16 hex chars
template <HexCase H>
FAST_HEX_TARGET_AVX2 inline __m128i encodeHex8(__m128i v8)
{
    const __m128i HEX_LUT_LOWER = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f');
    const __m128i HEX_LUT_UPPER = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F');

    __m128i hi = _mm_and_si128(_mm_srli_epi16(v8, 4), _mm_set1_epi8(0x0F));
    __m128i lo = _mm_and_si128(v8, _mm_set1_epi8(0x0F));

    // Interleave: hi[0], lo[0], hi[1], lo[1], ...
    __m128i nibs = _mm_unpacklo_epi8(hi, lo);

    return _mm_shuffle_epi8(H == HexCase::Lower ? HEX_LUT_LOWER : HEX_LUT_UPPER, nibs);
}

template <HexCase H>
FAST_HEX_TARGET_AVX2 inline void encodeHex8Fast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    __m128i v8 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src));
    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), encodeHex8<H>(v8));
}

// 16 bytes -> 32 hex chars
template <HexCase H>
FAST_HEX_TARGET_AVX2 inline void encodeHex16(uint8_t * dest, __m128i v16)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), hex<H>(byte2nib(v16)));
}

template <HexCase H>
FAST_HEX_TARGET_AVX2 inline void encodeHex16Fast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    encodeHex16<H>(dest, _mm_lddqu_si128(reinterpret_cast<const __m128i *>(src)));
}

// x with its bytes in reverse order, for the _mm_set* intrinsics
inline int64_t reversed(uint64_t x)
{
    return static_cast<int64_t>(__builtin_bswap64(x));
}

template <HexCase H>
FAST_HEX_TARGET_AVX2 inline void encodeHex32Vec(uint8_t * dest, const uint8_t * src)
{
    // Reorder qwords to [q0 q2 | q1 q3] before the unpacks
    __m256i input = _mm256_permute4x64_epi64(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)), 0xD8);
    const __m256i mask = _mm256_set1_epi8(0x0F);
    __m256i hi = hex<H>(_mm256_and_si256(_mm256_srli_epi16(input, 4), mask));
    __m256i lo = hex<H>(_mm256_and_si256(input, mask));
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), _mm256_unpacklo_epi8(hi, lo));
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest + 32), _mm256_unpackhi_epi8(hi, lo));
}

// len < 32, encoded in overlapping blocks with plain loads/stores
template <HexCase H>
FAST_HEX_TARGET_AVX2 inline void encodeHexSmallVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    if (len >= 16)
    {
        encodeHex16Fast<H>(dest, src);
        if (len > 16)
            encodeHex16Fast<H>(dest + len * 2 - 32, src + len - 16);
    }
    else if (len >= 8)
    {
        encodeHex8Fast<H>(dest, src);
        if (len > 8)
            encodeHex8Fast<H>(dest + len * 2 - 16, src + len - 8);
    }
    else if (len >= 4)
    {
        uint32_t lo = 0;
        uint32_t hi = 0;
        std::memcpy(&lo, src, 4);
        std::memcpy(&hi, src + len - 4, 4);
        __m128i chars = encodeHex8<H>(_mm_cvtsi64_si128(static_cast<int64_t>(uint64_t(hi) << 32 | lo)));
        auto first = static_cast<uint64_t>(_mm_cvtsi128_si64(chars));
        auto last = static_cast<uint64_t>(_mm_extract_epi64(chars, 1));
        std::memcpy(dest, &first, 8);
        std::memcpy(dest + len * 2 - 8, &last, 8);
    }
    else
        encodeHexImpl<H>(dest, src, len);
}

// len is number of src bytes >= 32
template <HexCase H>
FAST_HEX_TARGET_AVX2 inline void encodeHexVecLarge(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    assert(len >= 32);
    while (len >= 32)
    {
        encodeHex32Vec<H>(dest, src);
        src += 32;
        dest += 64;
        len -= 32;
    }

    if (len > 0)
    {
        // Re-encode the final full block, overlapping output already written by the loop
        // Everything inside original buffers
        encodeHex32Vec<H>(dest + len * 2 - 64, src + len - 32);
    }
}

template <HexCase H>
FAST_HEX_TARGET_AVX2 inline void encodeHexVecImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    if (len < 32)
        encodeHexSmallVec<H>(dest, src, len);
    else
        encodeHexVecLarge<H>(dest, src, len);
}

} // namespace heks_detail

// Whether the CPU can run the AVX2 functions. Works from static constructors too. Cheap, but not free: hoist it
// out of hot loops.
inline bool cpuHasAvx2()
{
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2");
}

template <class Case>
FAST_HEX_TARGET_AVX2 void encodeHexAvx2(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len, Case)
{
    heks_detail::encodeHexVecImpl<Case::value>(dest, src, static_cast<size_t>(len));
}

FAST_HEX_TARGET_AVX2 inline void decodeHexAvx2(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::decodeHexVecImpl(dest, src, static_cast<size_t>(len));
}

// encodeHexInt for 8, 16 and 32-byte types
template <class T, class Case>
FAST_HEX_TARGET_AVX2 void encodeHexIntAvx2(uint8_t * dest, T value, Case)
{
    heks_detail::checkSimdIntType<T>();
    constexpr auto H = Case::value;
    // Through 64-bit general purpose registers: a 16-byte T is passed in two of them, and storing those to
    // memory to load them as one vector stalls (store forwarding fails)
    uint64_t words[sizeof(T) / 8];
    std::memcpy(words, &value, sizeof(T));
    using heks_detail::reversed;
    if constexpr (sizeof(T) == 8)
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), heks_detail::encodeHex8<H>(_mm_cvtsi64_si128(reversed(words[0]))));
    else if constexpr (sizeof(T) == 16)
        heks_detail::encodeHex16<H>(dest, _mm_set_epi64x(reversed(words[0]), reversed(words[1])));
    else
    {
        heks_detail::encodeHex16<H>(dest, _mm_set_epi64x(reversed(words[2]), reversed(words[3])));
        heks_detail::encodeHex16<H>(dest + 32, _mm_set_epi64x(reversed(words[0]), reversed(words[1])));
    }
}

// decodeHexInt for 8, 16 and 32-byte types
template <class T>
FAST_HEX_TARGET_AVX2 T decodeHexIntAvx2(const uint8_t * src)
{
    heks_detail::checkSimdIntType<T>();
    if constexpr (sizeof(T) == 8)
    {
        // The decoded bytes are in the even bytes, most significant first: take them last to first
        const __m128i REVERSE_EVEN = _mm_setr_epi8(14, 12, 10, 8, 6, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1, -1);
        __m128i chars = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
        __m128i pairs = _mm256_castsi256_si128(heks_detail::unhexPairs(_mm256_castsi128_si256(chars)));
        return std::bit_cast<T>(_mm_cvtsi128_si64(_mm_shuffle_epi8(pairs, REVERSE_EVEN)));
    }
    else if constexpr (sizeof(T) == 16)
    {
        const __m128i REVERSE = _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        return std::bit_cast<T>(_mm_shuffle_epi8(heks_detail::decodeHex16(src), REVERSE));
    }
    else
    {
        // clang-format off
        const __m256i REVERSE_QWORDS = _mm256_setr_epi8(
            7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
        // clang-format on
        __m256i a = heks_detail::unhexPairs(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
        __m256i b = heks_detail::unhexPairs(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 32)));
        // Qwords of bytes 0-7, 16-23, 8-15 and 24-31, most significant first. Reverse each, then take them as
        // 24-31, 16-23, 8-15, 0-7
        __m256i packed = _mm256_shuffle_epi8(_mm256_packus_epi16(a, b), REVERSE_QWORDS);
        return std::bit_cast<T>(_mm256_permute4x64_epi64(packed, 0x27));
    }
}

// encodeHexInt2x8 for 8-byte types
template <class T, class Case>
FAST_HEX_TARGET_AVX2 void encodeHexInt2x8Avx2(uint8_t * dest, const T * first, Case)
{
    heks_detail::checkIntType<T>();
    static_assert(sizeof(T) == 8, "T must be 8 bytes");
    const __m128i REVERSE_QWORDS = _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
    __m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i *>(first));
    heks_detail::encodeHex16<Case::value>(dest, _mm_shuffle_epi8(in, REVERSE_QWORDS));
}

} // namespace heks

#    undef FAST_HEX_TARGET_AVX2

#endif // defined(FAST_HEX_X86_64)
