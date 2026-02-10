#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#if defined(FAST_HEX_AVX2) || defined(FAST_HEX_AVX)
#    if defined(__GNUC__)
#        include <immintrin.h>
#    elif defined(_MSC_VER)
#        include <intrin.h>
#    endif
#endif

#if defined(FAST_HEX_NEON)
#    include <arm_neon.h>
#endif

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

#ifdef FAST_HEX_STATIC_SHARED_LIBRARY
#    define FAST_HEX_FUNCTION_INLINE
#else
#    define FAST_HEX_FUNCTION_INLINE inline
#endif

#if defined(_MSC_VER)
#    define FAST_HEX_BSWAP64(x) _byteswap_uint64(x)
#else
#    define FAST_HEX_BSWAP64(x) __builtin_bswap64(x)
#endif

#if (defined(__SIZEOF_INT128__) || defined(__INT128__))
#    define FAST_HEX_HAS_INT128 1
#endif

// This implementation is by https://github.com/zbjornson/fast-hex
// Only introduced some minor modernisations and style changes to those functions
// decodeHexLUT
// decodeHexLUT4
// decodeHexBMI
// decodeHexVec
// encodeHexImpl
// encodeHexVecImpl

FAST_HEX_NAMESPACE_OPEN

// Length of raw data in bytes (e.g "Test123" is 7 bytes)
enum class RawLength : size_t;

void decodeHexLUT(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void decodeHexLUT4(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void decodeHexBMI(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

void encodeHexLower(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void encodeHexUpper(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

#if defined(FAST_HEX_AVX)
void encodeHex8LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
void encodeHex8UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
#endif // defined(FAST_HEX_AVX)

#if defined(FAST_HEX_AVX2)
void decodeHexVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

void encodeHexLowerVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void encodeHexUpperVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void encodeHex16LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
void encodeHex16UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
#endif // defined(FAST_HEX_AVX2)

#if defined(FAST_HEX_NEON)
void encodeHexNeonLower(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void encodeHexNeonUpper(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
#endif // FAST_HEX_NEON

/////////////////////////////////////////////////////////////////////////////

namespace heks_detail
{
using namespace std::literals::string_view_literals;

enum class HexCase
{
    Lower,
    Upper
};

enum class Reverse
{
    No,
    Yes64,
    Yes128,
};


// clang-format off
// ASCII -> hex value as a string_view
constexpr inline std::string_view unhex_table_sv =
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x00-0x07
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x08-0x0F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x10-0x17
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x18-0x1F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x20-0x27
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x28-0x2F
    "\x00\x01\x02\x03\x04\x05\x06\x07" // 0x30-0x37
    "\x08\x09\xFF\xFF\xFF\xFF\xFF\xFF" // 0x38-0x3F
    "\xFF\x0A\x0B\x0C\x0D\x0E\x0F\xFF" // 0x40-0x47
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x48-0x4F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x50-0x57
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x58-0x5F
    "\xFF\x0A\x0B\x0C\x0D\x0E\x0F\xFF" // 0x60-0x67
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x68-0x6F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x70-0x77
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x78-0x7F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x80-0x87
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x88-0x8F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x90-0x97
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x98-0x9F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xA0-0xA7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xA8-0xAF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xB0-0xB7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xB8-0xBF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xC0-0xC7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xC8-0xCF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xD0-0xD7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xD8-0xDF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xE0-0xE7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xE8-0xEF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xF0-0xF8
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"sv // 0xF8-0xFF
    ;

// ASCII -> hex value << 4 (upper nibble) as a string_view
constexpr inline std::string_view unhex_table4_sv =
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x00-0x07
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x08-0x0F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x10-0x17
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x18-0x1F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x20-0x27
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x28-0x2F
    "\x00\x10\x20\x30\x40\x50\x60\x70" // 0x30-0x37
    "\x80\x90\xFF\xFF\xFF\xFF\xFF\xFF" // 0x38-0x3F
    "\xFF\xA0\xB0\xC0\xD0\xE0\xF0\xFF" // 0x40-0x47
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x48-0x4F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x50-0x57
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x58-0x5F
    "\xFF\xA0\xB0\xC0\xD0\xE0\xF0\xFF" // 0x60-0x67
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x68-0x6F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x70-0x77
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x78-0x7F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x80-0x87
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x88-0x8F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x90-0x97
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0x98-0x9F
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xA0-0xA7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xA8-0xAF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xB0-0xB7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xB8-0xBF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xC0-0xC7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xC8-0xCF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xD0-0xD7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xD8-0xDF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xE0-0xE7
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xE8-0xEF
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" // 0xF0-0xF8
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"sv // 0xF8-0xFF
    ;

constexpr inline std::string_view hex_to_char_upper_sv =
    "000102030405060708090A0B0C0D0E0F"
    "101112131415161718191A1B1C1D1E1F"
    "202122232425262728292A2B2C2D2E2F"
    "303132333435363738393A3B3C3D3E3F"
    "404142434445464748494A4B4C4D4E4F"
    "505152535455565758595A5B5C5D5E5F"
    "606162636465666768696A6B6C6D6E6F"
    "707172737475767778797A7B7C7D7E7F"
    "808182838485868788898A8B8C8D8E8F"
    "909192939495969798999A9B9C9D9E9F"
    "A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
    "B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
    "C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
    "D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
    "E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
    "F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF"sv;

constexpr inline std::string_view hex_to_char_lower_sv =
    "000102030405060708090a0b0c0d0e0f"
    "101112131415161718191a1b1c1d1e1f"
    "202122232425262728292a2b2c2d2e2f"
    "303132333435363738393a3b3c3d3e3f"
    "404142434445464748494a4b4c4d4e4f"
    "505152535455565758595a5b5c5d5e5f"
    "606162636465666768696a6b6c6d6e6f"
    "707172737475767778797a7b7c7d7e7f"
    "808182838485868788898a8b8c8d8e8f"
    "909192939495969798999a9b9c9d9e9f"
    "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
    "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
    "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
    "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
    "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
    "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

// clang-format on

static_assert(unhex_table_sv.size() == 256, "unhex_table_sv must have 256 elements");
static_assert(unhex_table4_sv.size() == 256, "unhex_table4_sv must have 256 elements");

// Looks up the value for the lower nibble.
constexpr uint8_t unhexB(uint8_t x)
{
    return static_cast<uint8_t>(unhex_table_sv[x]);
}

// Looks up the value for the upper nibble. Equivalent to `unhexB(x) << 4`.
constexpr uint8_t unhexA(uint8_t x)
{
    return static_cast<uint8_t>(unhex_table4_sv[x]);
}

constexpr int8_t unhexBitManip(uint8_t x)
{
    return 9 * (x >> 6) + (x & 0xf);
}


#if defined(FAST_HEX_AVX2)
__attribute__((target("avx2"))) inline __m256i unhexBitManip(const __m256i value)
{
    const __m256i _9 = _mm256_set1_epi16(9);
    const __m256i _15 = _mm256_set1_epi16(0xf);

    __m256i and15 = _mm256_and_si256(value, _15);

#    ifndef NO_MADDUBS
    __m256i sr6 = _mm256_srai_epi16(value, 6);
    __m256i mul = _mm256_maddubs_epi16(sr6, _9); // this has a latency of 5
#    else
    // ... while this I think has a latency of 4, but worse throughput(?).
    // (x >> 6) * 9 is x * 8 + x:
    // ((x >> 6) << 3) + (x >> 6)
    // We need & 0b11 to emulate 8-bit operations (narrowest shift is 16b) -- or a left shift
    // (((x >> 6) & 0b11) << 3) + ((x >> 6) & 0b11)
    // or
    // tmp = (x >> 6) & 0b11
    // tmp << 3 + tmp
    // there's no carry due to the mask+shift combo, so + is |
    // tmp << 3 | tmp
    __m256i sr6_lo2 = _mm256_and_si256(_mm256_srli_epi16(value, 6), _mm256_set1_epi16(0b11));
    __m256i sr6_lo2_sl3 = _mm256_slli_epi16(sr6_lo2, 3);
    __m256i mul = _mm256_or_si256(sr6_lo2_sl3, sr6_lo2);
#    endif

    __m256i add = _mm256_add_epi16(mul, and15);
    return add;
}

template <HexCase H>
__attribute__((target("avx2"))) inline __m256i hex(__m256i value)
{
    if constexpr (H == HexCase::Lower)
    {
        // clang-format off
        const __m256i HEX_LUTR_LOWER = _mm256_setr_epi8('0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f');
        // clang-format on
        return _mm256_shuffle_epi8(HEX_LUTR_LOWER, value);
    }
    else if constexpr (H == HexCase::Upper)
    {
        // clang-format off
        const __m256i HEX_LUTR_UPPER = _mm256_setr_epi8('0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F');
        // clang-format on
        return _mm256_shuffle_epi8(HEX_LUTR_UPPER, value);
    }
    else
        []() { static_assert(H != H, "Unsupported HexCase"); }();
}

// (a << 4) | b;
// a and b must be 16-bit elements. Output is packed 8-bit elements.
__attribute__((target("avx2"))) inline __m256i nib2byte(__m256i a1, __m256i b1, __m256i a2, __m256i b2)
{
    __m256i a4_1 = _mm256_slli_epi16(a1, 4);
    __m256i a4_2 = _mm256_slli_epi16(a2, 4);
    __m256i a4orb_1 = _mm256_or_si256(a4_1, b1);
    __m256i a4orb_2 = _mm256_or_si256(a4_2, b2);
    __m256i pck1 = _mm256_packus_epi16(a4orb_1, a4orb_2); // lo1 lo2 hi1 hi2
    const int _0213 = 0b11'01'10'00;
    __m256i pck64 = _mm256_permute4x64_epi64(pck1, _0213);
    return pck64;
}

// a -> [a >> 4, a & 0b1111]
__attribute__((target("avx2"))) inline __m256i byte2nib(__m128i val)
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

#endif // defined(FAST_HEX_AVX2)

// clang-format off
// Hex character lookup as a string_view
constexpr inline std::string_view hex_table_lower_sv = "0123456789abcdef"sv;
constexpr inline std::string_view hex_table_upper_sv = "0123456789ABCDEF"sv;
// clang-format on

static_assert(hex_table_lower_sv.size() == 16, "hex_table_lower_sv must have 16 elements");
static_assert(hex_table_upper_sv.size() == 16, "hex_table_upper_sv must have 16 elements");

template <HexCase H>
constexpr char hex(uint8_t value)
{
    if constexpr (H == HexCase::Lower)
        return hex_table_lower_sv[value & 0xF];
    else if constexpr (H == HexCase::Upper)
        return hex_table_upper_sv[value & 0xF];
    else
        []() { static_assert(H != H, "Unsupported HexCase"); }();
}

// len is number of src bytes
template <HexCase H>
inline void encodeHexImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    using namespace heks_detail;
    const auto & hex_table = (H == HexCase::Lower) ? hex_to_char_lower_sv : hex_to_char_upper_sv;

    const auto raw_length = static_cast<size_t>(len);
    for (size_t i = 0; i < raw_length; i++)
    {
        std::memcpy(dest + i * 2, &hex_table[static_cast<size_t>(src[i]) * 2], 2);
    }
}

#if defined(FAST_HEX_AVX)

template <HexCase H>
__attribute__((target("avx"))) inline void encodeHex8Fast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    const __m128i HEX_LUT_LOWER = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f');
    const __m128i HEX_LUT_UPPER = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F');

    __m128i v8 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src));

    __m128i hi = _mm_and_si128(_mm_srli_epi16(v8, 4), _mm_set1_epi8(0x0F));
    __m128i lo = _mm_and_si128(v8, _mm_set1_epi8(0x0F));

    // Interleave: hi[0], lo[0], hi[1], lo[1], ...
    __m128i nibs = _mm_unpacklo_epi8(hi, lo);

    __m128i result = _mm_shuffle_epi8(H == HexCase::Lower ? HEX_LUT_LOWER : HEX_LUT_UPPER, nibs);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), result);
}
#endif

#if defined(FAST_HEX_AVX2)
// len is number of src bytes
template <HexCase H>
__attribute__((target("avx2"))) inline void
encodeHexVecImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    const auto raw_length = static_cast<size_t>(len);
    const __m128i * input128 = reinterpret_cast<const __m128i *>(src);
    __m256i * output256 = reinterpret_cast<__m256i *>(dest);

    size_t tailLen = raw_length % 16;
    size_t vectLen = (raw_length - tailLen) >> 4;
    for (size_t i = 0; i < vectLen; i++)
    {
        __m128i av = _mm_lddqu_si128(&input128[i]);
        __m256i nibs = byte2nib(av);
        __m256i hexed = hex<H>(nibs);
        _mm256_storeu_si256(&output256[i], hexed);
    }

    encodeHexImpl<H>(dest + (vectLen << 5), src + (vectLen << 4), RawLength{tailLen});
}

template <HexCase H, Reverse R = Reverse::No>
__attribute__((target("avx2"))) inline void encodeHex16Fast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    __m128i v16 = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(src));
    if constexpr (R == Reverse::Yes128)
    {
        const __m128i REV_FULL = _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        v16 = _mm_shuffle_epi8(v16, REV_FULL);
    }
    else if constexpr (R == Reverse::Yes64)
    {
        const __m128i REV_HALVES = _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
        v16 = _mm_shuffle_epi8(v16, REV_HALVES);
    }
    __m256i nibs = byte2nib(v16);
    __m256i hexed = hex<H>(nibs);
    // Store all 32 bytes (16 input → 32 hex chars)
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), hexed);
}
#endif // defined(FAST_HEX_AVX2)

#if defined(FAST_HEX_NEON)
template <HexCase H>
void encodeHexNeon_impl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength raw_len)
{
    auto len = static_cast<size_t>(raw_len);
    // clang-format off
    alignas(8) constexpr uint8_t HEX_LUT_LOWER[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    alignas(8) constexpr uint8_t HEX_LUT_UPPER[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    // clang-format on

    // Pick and load the right LUTs based on HexCase
    const uint8_t * lut1 = (H == HexCase::Upper) ? HEX_LUT_UPPER : HEX_LUT_LOWER;
    const uint8_t * lut2 = (H == HexCase::Upper) ? (HEX_LUT_UPPER + 8) : (HEX_LUT_LOWER + 8);
    uint8x8_t lut_vec1 = vld1_u8(lut1);
    uint8x8_t lut_vec2 = vld1_u8(lut2);

    size_t i = 0;

    while (len >= 16)
    {
        // Load 16 bytes from the source
        uint8x16_t invec = vld1q_u8(src + i);

        // Extract high and low nibbles
        uint8x16_t hi_nibbles = vshrq_n_u8(invec, 4); // Shift right by 4 bits (high nibble)
        uint8x16_t lo_nibbles = vandq_u8(invec, vdupq_n_u8(0x0F)); // Mask lower 4 bits (low nibble)

        // Process the low 8 bytes
        uint8x8_t hi_lo = vget_low_u8(hi_nibbles);
        uint8x8_t lo_lo = vget_low_u8(lo_nibbles);

        // Lookup the hex characters
        uint8x8_t hi_chars_lo = vtbl2_u8({lut_vec1, lut_vec2}, hi_lo);
        uint8x8_t lo_chars_lo = vtbl2_u8({lut_vec1, lut_vec2}, lo_lo);

        // Process the high 8 bytes
        uint8x8_t hi_hi = vget_high_u8(hi_nibbles);
        uint8x8_t lo_hi = vget_high_u8(lo_nibbles);

        // Lookup the hex characters
        uint8x8_t hi_chars_hi = vtbl2_u8({lut_vec1, lut_vec2}, hi_hi);
        uint8x8_t lo_chars_hi = vtbl2_u8({lut_vec1, lut_vec2}, lo_hi);

        // Combine the low and high parts of the results
        uint8x16_t hi_chars = vcombine_u8(hi_chars_lo, hi_chars_hi);
        uint8x16_t lo_chars = vcombine_u8(lo_chars_lo, lo_chars_hi);

        // Zip high and low
        uint8x16x2_t interleaved = vzipq_u8(hi_chars, lo_chars);

        // Store the zipped result
        vst1q_u8(dest + (i * 2), interleaved.val[0]);
        vst1q_u8(dest + (i * 2) + 16, interleaved.val[1]);

        i += 16;
        len -= 16;
    }

    encodeHexImpl<H>(dest + (i * 2), src + i, RawLength{len});
}

template <HexCase H, Reverse R = Reverse::No>
void encodeHexNeon8_impl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    static_assert(R != Reverse::Yes128, "For 8-byte input, only Reverse::No and Reverse::Yes64 is supported.");
    static const uint8_t HEX_LUT_LOWER[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    static const uint8_t HEX_LUT_UPPER[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    const uint8_t * lut1 = (H == HexCase::Upper) ? HEX_LUT_UPPER : HEX_LUT_LOWER;
    const uint8_t * lut2 = (H == HexCase::Upper) ? HEX_LUT_UPPER + 8 : HEX_LUT_LOWER + 8;

    uint8x8_t lut_vec1 = vld1_u8(lut1);
    uint8x8_t lut_vec2 = vld1_u8(lut2);

    uint8x8x2_t tbl;
    tbl.val[0] = lut_vec1;
    tbl.val[1] = lut_vec2;

    uint8x8_t in = vld1_u8(src);
    if constexpr (R == Reverse::Yes64)
    {
        in = vrev64_u8(in);
    }

    uint8x8_t hi = vshr_n_u8(in, 4);
    uint8x8_t lo = vand_u8(in, vdup_n_u8(0x0F));

    uint8x8_t hi_chars = vtbl2_u8(tbl, hi);
    uint8x8_t lo_chars = vtbl2_u8(tbl, lo);

    uint8x8x2_t zipped = vzip_u8(hi_chars, lo_chars);
    uint8x8_t out0 = zipped.val[0];
    uint8x8_t out1 = zipped.val[1];

    vst1_u8(dest, out0);
    vst1_u8(dest + 8, out1);
}

#endif // FAST_HEX_NEON

} // namespace heks_detail


// len is number of dest bytes
FAST_HEX_FUNCTION_INLINE void decodeHexLUT(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    using namespace heks_detail;
    const auto raw_length = static_cast<size_t>(len);
    for (size_t i = 0; i < raw_length; i++)
    {
        uint8_t a = *src++;
        uint8_t b = *src++;
        a = unhexB(a);
        b = unhexB(b);
        dest[i] = static_cast<uint8_t>((a << 4) | b);
    }
}

// len is number of dest bytes
FAST_HEX_FUNCTION_INLINE void decodeHexLUT4(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    using namespace heks_detail;
    const auto raw_length = static_cast<size_t>(len);
    for (size_t i = 0; i < raw_length; i++)
    {
        uint8_t a = *src++;
        uint8_t b = *src++;
        a = unhexA(a);
        b = unhexB(b);
        dest[i] = a | b;
    }
}


// len is number or dest bytes (i.e. half of src length)
FAST_HEX_FUNCTION_INLINE void decodeHexBMI(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    using namespace heks_detail;
    const auto raw_length = static_cast<size_t>(len);
    for (size_t i = 0; i < raw_length; i++)
    {
        uint8_t a = *src++;
        uint8_t b = *src++;
        a = static_cast<uint8_t>(unhexBitManip(a));
        b = static_cast<uint8_t>(unhexBitManip(b));
        dest[i] = static_cast<uint8_t>((a << 4) | b);
    }
}


FAST_HEX_FUNCTION_INLINE void encodeHexLower(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::encodeHexImpl<heks_detail::HexCase::Lower>(dest, src, len);
}
FAST_HEX_FUNCTION_INLINE void encodeHexUpper(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::encodeHexImpl<heks_detail::HexCase::Upper>(dest, src, len);
}


#if defined(FAST_HEX_AVX)
__attribute__((target("avx"))) FAST_HEX_FUNCTION_INLINE void
encodeHex8LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    heks_detail::encodeHex8Fast<heks_detail::HexCase::Lower>(dest, src);
}

__attribute__((target("avx"))) FAST_HEX_FUNCTION_INLINE void
encodeHex8UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    heks_detail::encodeHex8Fast<heks_detail::HexCase::Upper>(dest, src);
}
#endif

#if defined(FAST_HEX_AVX2)

// len is number or dest bytes (i.e. half of src length)
__attribute__((target("avx2"))) FAST_HEX_FUNCTION_INLINE void
decodeHexVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    using namespace heks_detail;
    auto raw_length = static_cast<size_t>(len);
    const __m256i A_MASK = _mm256_setr_epi8(
        0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14, -1, 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14, -1);
    const __m256i B_MASK = _mm256_setr_epi8(
        1, -1, 3, -1, 5, -1, 7, -1, 9, -1, 11, -1, 13, -1, 15, -1, 1, -1, 3, -1, 5, -1, 7, -1, 9, -1, 11, -1, 13, -1, 15, -1);

    const __m256i * val3 = reinterpret_cast<const __m256i *>(src);
    __m256i * dec256 = reinterpret_cast<__m256i *>(dest);

    while (raw_length >= 32)
    {
        __m256i av1 = _mm256_loadu_si256(val3++);
        __m256i av2 = _mm256_loadu_si256(val3++);
        __m256i a1 = _mm256_shuffle_epi8(av1, A_MASK);
        __m256i b1 = _mm256_shuffle_epi8(av1, B_MASK);
        __m256i a2 = _mm256_shuffle_epi8(av2, A_MASK);
        __m256i b2 = _mm256_shuffle_epi8(av2, B_MASK);

        a1 = unhexBitManip(a1);
        a2 = unhexBitManip(a2);
        b1 = unhexBitManip(b1);
        b2 = unhexBitManip(b2);

        __m256i bytes = nib2byte(a1, b1, a2, b2);
        _mm256_storeu_si256(dec256++, bytes);
        raw_length -= 32;
    }

    src = reinterpret_cast<const uint8_t *>(val3);
    dest = reinterpret_cast<uint8_t *>(dec256);
    decodeHexBMI(dest, src, RawLength{raw_length});
}

__attribute__((target("avx2"))) FAST_HEX_FUNCTION_INLINE void
encodeHexLowerVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::encodeHexVecImpl<heks_detail::HexCase::Lower>(dest, src, len);
}
__attribute__((target("avx2"))) FAST_HEX_FUNCTION_INLINE void
encodeHexUpperVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::encodeHexVecImpl<heks_detail::HexCase::Upper>(dest, src, len);
}

__attribute__((target("avx2"))) FAST_HEX_FUNCTION_INLINE void
encodeHex16LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    heks_detail::encodeHex16Fast<heks_detail::HexCase::Lower>(dest, src);
}

__attribute__((target("avx2"))) FAST_HEX_FUNCTION_INLINE void
encodeHex16UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    heks_detail::encodeHex16Fast<heks_detail::HexCase::Upper>(dest, src);
}
#endif // defined(FAST_HEX_AVX2)

#if defined(FAST_HEX_NEON)

FAST_HEX_FUNCTION_INLINE void encodeHexNeonLower(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::encodeHexNeon_impl<heks_detail::HexCase::Lower>(dest, src, len);
}

FAST_HEX_FUNCTION_INLINE void encodeHexNeonUpper(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::encodeHexNeon_impl<heks_detail::HexCase::Upper>(dest, src, len);
}

FAST_HEX_FUNCTION_INLINE void encodeHex8LowerNeon(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    heks_detail::encodeHexNeon8_impl<heks_detail::HexCase::Lower>(dest, src);
}
FAST_HEX_FUNCTION_INLINE void encodeHex8UpperNeon(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    heks_detail::encodeHexNeon8_impl<heks_detail::HexCase::Upper>(dest, src);
}

#endif

struct upper_t
{
    static constexpr heks_detail::HexCase value = heks_detail::HexCase::Upper;
};
struct lower_t
{
    static constexpr heks_detail::HexCase value = heks_detail::HexCase::Lower;
};
inline constexpr upper_t upper{};
inline constexpr lower_t lower{};

template <class Case>
inline void encode_auto(uint8_t * FAST_HEX_RESTRICT d, const uint8_t * FAST_HEX_RESTRICT s, RawLength n, Case)
{
    constexpr auto case_type = Case::value;
#if defined(__x86_64__) || defined(_M_X64)
#    if defined(FAST_HEX_AVX2)
    heks_detail::encodeHexVecImpl<case_type>(d, s, n);
#    else
    heks_detail::encodeHexImpl<case_type>(d, s, n);
#    endif
#elif defined(FAST_HEX_NEON)
    heks_detail::encodeHexNeon_impl<case_type>(d, s, n);
#else
    heks_detail::encodeHexImpl<case_type>(d, s, n);
#endif
}

inline void decode_auto(uint8_t * FAST_HEX_RESTRICT d, const uint8_t * FAST_HEX_RESTRICT s, RawLength n)
{
#if defined(__x86_64__) || defined(_M_X64)
#    if defined(FAST_HEX_AVX2)
    decodeHexVec(d, s, n);
#    elif defined(__BMI__)
    decodeHexBMI(d, s, n);
#    else
    decodeHexLUT4(d, s, n);
#    endif
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#    if defined(FAST_HEX_ARM)
    decodeHexBMI(d, s, n);
#    else
    decodeHexLUT(d, s, n);
#    endif
#else
    decodeHexLUT4(d, s, n);
#endif
}

template <typename T>
T decode_integral_naive(const uint8_t * src)
{
    using namespace heks_detail;

    if constexpr (sizeof(T) == 1)
    {
        return unhexA(src[0]) | unhexB(src[1]);
    }
    else if constexpr (sizeof(T) == 2)
    {
        uint8_t byte0 = unhexA(src[2 * 0]) | unhexB(src[2 * 0 + 1]);
        uint8_t byte1 = unhexA(src[2 * 1]) | unhexB(src[2 * 1 + 1]);
        return static_cast<T>((static_cast<uint16_t>(byte0) << 8) | byte1);
    }
    else if constexpr (sizeof(T) == 4)
    {
        uint8_t byte0 = unhexA(src[2 * 0]) | unhexB(src[2 * 0 + 1]);
        uint8_t byte1 = unhexA(src[2 * 1]) | unhexB(src[2 * 1 + 1]);
        uint8_t byte2 = unhexA(src[2 * 2]) | unhexB(src[2 * 2 + 1]);
        uint8_t byte3 = unhexA(src[2 * 3]) | unhexB(src[2 * 3 + 1]);
        return static_cast<T>(
            (static_cast<uint32_t>(byte0) << 24) | (static_cast<uint32_t>(byte1) << 16) | (static_cast<uint32_t>(byte2) << 8) | byte3);
    }
    else if constexpr (sizeof(T) == 8)
    {
        uint8_t byte0 = unhexA(src[2 * 0]) | unhexB(src[2 * 0 + 1]);
        uint8_t byte1 = unhexA(src[2 * 1]) | unhexB(src[2 * 1 + 1]);
        uint8_t byte2 = unhexA(src[2 * 2]) | unhexB(src[2 * 2 + 1]);
        uint8_t byte3 = unhexA(src[2 * 3]) | unhexB(src[2 * 3 + 1]);
        uint8_t byte4 = unhexA(src[2 * 4]) | unhexB(src[2 * 4 + 1]);
        uint8_t byte5 = unhexA(src[2 * 5]) | unhexB(src[2 * 5 + 1]);
        uint8_t byte6 = unhexA(src[2 * 6]) | unhexB(src[2 * 6 + 1]);
        uint8_t byte7 = unhexA(src[2 * 7]) | unhexB(src[2 * 7 + 1]);
        return static_cast<T>(
            (static_cast<uint64_t>(byte0) << 56) | (static_cast<uint64_t>(byte1) << 48) | (static_cast<uint64_t>(byte2) << 40)
            | (static_cast<uint64_t>(byte3) << 32) | (static_cast<uint64_t>(byte4) << 24) | (static_cast<uint64_t>(byte5) << 16)
            | (static_cast<uint64_t>(byte6) << 8) | static_cast<uint64_t>(byte7));
    }
    else if constexpr (sizeof(T) == 16)
    {
        uint64_t high_part = decode_integral_naive<uint64_t>(src);
        uint64_t low_part = decode_integral_naive<uint64_t>(src + 16);
        return static_cast<T>((static_cast<T>(high_part) << 64) | low_part);
    }
}

#if defined(FAST_HEX_AVX)
// Based on https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/blob/master/2023/07/27/src/base16.c
__attribute__((target("avx"))) inline uint64_t decode_integral8(const uint8_t * src)
{
    // Rebase constants for hex digits
    // clang-format off
    const __m128i delta_rebase = _mm_setr_epi8(
        0, 0, -47, -47, -54, 0, -86, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    );
    // clang-format on

    // Load 16 hex characters
    __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
    // Convert ASCII to 0-15 values
    __m128i vm1 = _mm_add_epi8(v, _mm_set1_epi8(-1));
    __m128i hash_key = _mm_and_si128(_mm_srli_epi32(vm1, 4), _mm_set1_epi8(0x0F));
    v = _mm_add_epi8(vm1, _mm_shuffle_epi8(delta_rebase, hash_key));

    // v now has 16 bytes, each with hex digit (0-15) in lower nibble
    const __m128i t3 = _mm_maddubs_epi16(v, _mm_set1_epi16(0x0110));
    __m128i packed = _mm_packus_epi16(t3, t3);

    const __m128i reverse_mask = _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
    packed = _mm_shuffle_epi8(packed, reverse_mask);
    uint64_t result;
    _mm_storel_epi64(reinterpret_cast<__m128i *>(&result), packed);
    return result;
}
#endif

template <typename T, typename Case>
void encode_integral_naive(uint8_t * output, T number, Case)
{
    using namespace heks_detail;
    constexpr auto case_type = Case::value;

    const auto & hex_table = (case_type == HexCase::Lower) ? hex_to_char_lower_sv : hex_to_char_upper_sv;
    union
    {
        T value;
        uint8_t uint8[sizeof(T)];
    };
    value = number;

    for (size_t i = 0; i < sizeof(T); ++i)
    {
        if constexpr (std::endian::native == std::endian::little)
            std::memcpy(output + i * 2, &hex_table[static_cast<size_t>(uint8[sizeof(T) - 1 - i]) * 2], 2);
        else
            std::memcpy(output + i * 2, &hex_table[static_cast<size_t>(uint8[i]) * 2], 2);
    }
}

#if defined(FAST_HEX_AVX)
template <typename T, typename Case>
__attribute__((target("avx"))) void encode_integral8(uint8_t * dest, T number, Case)
{
    using namespace heks_detail;
    static_assert(sizeof(T) == 8, "T must be 8 bytes (64 bits) in size");
    constexpr auto case_type = Case::value;
    number = FAST_HEX_BSWAP64(number);
    const auto * input = reinterpret_cast<const uint8_t *>(&number);
    encodeHex8Fast<case_type>(dest, input);
}
#elif defined(FAST_HEX_NEON)
template <typename T, typename Case>
void encode_integral8(uint8_t * output, T number, Case)
{
    using namespace heks_detail;
    static_assert(sizeof(T) == 8, "T must be 8 bytes (64 bits) in size");
    constexpr auto case_type = Case::value;
    const auto * input = reinterpret_cast<const uint8_t *>(&number);
    if constexpr (std::endian::native == std::endian::little)
        encodeHexNeon8_impl<case_type, Reverse::Yes64>(output, input);
    else
        encodeHexNeon8_impl<case_type, Reverse::No>(output, input);
}
#endif

#if defined(FAST_HEX_AVX2)
template <typename T, typename Case>
__attribute__((target("avx2"))) void encode_integral16(uint8_t * output, T number, Case)
{
    using namespace heks_detail;
    static_assert(sizeof(T) == 16, "T must be 16 bytes (128 bits) in size");
    constexpr auto case_type = Case::value;
    const auto * input = reinterpret_cast<const uint8_t *>(&number);
    encodeHex16Fast<case_type, Reverse::Yes128>(output, input);
}
template <typename T, typename Case>
void encode_integral2x8(uint8_t * FAST_HEX_RESTRICT output, const T * FAST_HEX_RESTRICT first, Case)
{
    using namespace heks_detail;
    static_assert(sizeof(T) == 8, "T must be 8 bytes (64 bits) in size");
    constexpr auto case_type = Case::value;
    const auto * input = reinterpret_cast<const uint8_t *>(first);
    encodeHex16Fast<case_type, Reverse::Yes64>(output, input);
}
#endif

FAST_HEX_NAMESPACE_CLOSE
