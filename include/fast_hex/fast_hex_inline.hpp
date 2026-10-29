#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

// With GCC/Clang on x86-64 all the SIMD code is compiled in: each function has its own target
// attribute and the Auto functions check the CPU at runtime. MSVC has no target attributes.
// Define FAST_HEX_NO_AVX, FAST_HEX_NO_AVX2 or FAST_HEX_NO_AVX512 to leave that code out.
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


#if defined(FAST_HEX_AVX2) || defined(FAST_HEX_AVX512)
#    define FAST_HEX_AVX2_HELPERS
#endif

#if defined(FAST_HEX_AVX512) || defined(FAST_HEX_AVX2) || defined(FAST_HEX_AVX)
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

#define FAST_HEX_AVX512_TARGET __attribute__((target("avx512f,avx512bw,avx512vl,avx512vbmi")))

#if (defined(__SIZEOF_INT128__) || defined(__INT128__))
#    define FAST_HEX_HAS_INT128 1
#endif

// This implementation is by https://github.com/zbjornson/fast-hex
// Only introduced some minor modernisations and style changes to those functions
// decodeHexLUT
// decodeHexLUT4
// decodeHexBMI
// decodeHexVec (since reworked to decode nibbles via a pshufb delta table)
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

void decodeHexAuto(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void encodeHexLowerAuto(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void encodeHexUpperAuto(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

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

#if defined(FAST_HEX_AVX512)
void decodeHexVec512(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

void encodeHexLowerVec512(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void encodeHexUpperVec512(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
#endif // defined(FAST_HEX_AVX512)

#if defined(FAST_HEX_NEON)
void encodeHexNeonLower(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void encodeHexNeonUpper(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
void encodeHex16LowerNeon(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
void encodeHex16UpperNeon(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
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


#if defined(FAST_HEX_AVX2_HELPERS)
// 32 hex chars -> 16 decoded bytes, each in the low byte of a 16-bit lane.
// The nibble is c + delta[c >> 4], where delta is -'0' for '0'-'9' (0x3_), -'A' + 10 for 'A'-'F' (0x4_)
// and -'a' + 10 for 'a'-'f' (0x6_), so no masking is needed afterwards.
// maddubs then combines each pair of nibbles as hi * 16 + lo.
__attribute__((target("avx2"))) inline __m256i unhexPairs(const __m256i value)
{
    // clang-format off
    const __m256i DELTA = _mm256_setr_epi8(
        0, 0, 0, -0x30, -0x37, 0, -0x57, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -0x30, -0x37, 0, -0x57, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    // clang-format on

    __m256i hi = _mm256_and_si256(_mm256_srli_epi16(value, 4), _mm256_set1_epi8(0x0F));
    __m256i nibs = _mm256_add_epi8(value, _mm256_shuffle_epi8(DELTA, hi));
    return _mm256_maddubs_epi16(nibs, _mm256_set1_epi16(0x0110));
}

__attribute__((target("avx2"))) inline void decodeHex32Vec(uint8_t * dest, const uint8_t * src)
{
    __m256i a = unhexPairs(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
    __m256i b = unhexPairs(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 32)));
    __m256i packed = _mm256_packus_epi16(a, b); // a.lo b.lo a.hi b.hi
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), _mm256_permute4x64_epi64(packed, 0xD8));
}

__attribute__((target("avx2"))) inline void decodeHex16Vec(uint8_t * dest, const uint8_t * src)
{
    __m256i pairs = unhexPairs(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
    __m128i packed = _mm_packus_epi16(_mm256_castsi256_si128(pairs), _mm256_extracti128_si256(pairs, 1));
    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), packed);
}

// 2 <= len < 16: decodes the first and last W hex chars in one pass, W={16,8,4} with
// W <= len * 2 < W * 2, so the two always overlap (they are the same block when len * 2 == W)
__attribute__((target("avx2"))) inline void decodeHexTinyVec(uint8_t * dest, const uint8_t * src, size_t len)
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
__attribute__((target("avx2"))) inline void decodeHexSmallVec(uint8_t * dest, const uint8_t * src, size_t len)
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
__attribute__((target("avx2"))) inline void decodeHexVecLarge(uint8_t * dest, const uint8_t * src, size_t len)
{
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

#endif // defined(FAST_HEX_AVX2_HELPERS)

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

#if defined(FAST_HEX_AVX) || defined(FAST_HEX_AVX2_HELPERS)

// Low 8 bytes of v8 -> 16 hex chars
template <HexCase H>
__attribute__((target("avx"))) inline __m128i encodeHex8(__m128i v8)
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
__attribute__((target("avx"))) inline void encodeHex8Fast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    __m128i v8 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src));
    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), encodeHex8<H>(v8));
}
#endif

#if defined(FAST_HEX_AVX2_HELPERS)
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

template <HexCase H>
__attribute__((target("avx2"))) inline void encodeHex32Vec(uint8_t * dest, const uint8_t * src)
{
    // Reorder qwords to [q0 q2 | q1 q3] before the unpacks
    __m256i input = _mm256_permute4x64_epi64(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)), 0xD8);
    const __m256i mask = _mm256_set1_epi8(0x0F);
    __m256i hi = hex<H>(_mm256_and_si256(_mm256_srli_epi16(input, 4), mask));
    __m256i lo = hex<H>(_mm256_and_si256(input, mask));
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), _mm256_unpacklo_epi8(hi, lo));
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest + 32), _mm256_unpackhi_epi8(hi, lo));
}

template <HexCase H>
__attribute__((target("avx2"))) inline void
encodeHexSmallVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
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
        encodeHexImpl<H>(dest, src, RawLength{len});
}

// len is number of src bytes >= 32
template <HexCase H>
__attribute__((target("avx2"))) inline void
encodeHexVecLarge(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    while (len >= 32)
    {
        encodeHex32Vec<H>(dest, src);
        src += 32;
        dest += 64;
        len -= 32;
    }

    if (len > 0)
    {
        // Re-encode the final full block, overalpping output already written by the loop
        // Everything inside original buffers
        encodeHex32Vec<H>(dest + len * 2 - 64, src + len - 32);
    }
}

template <HexCase H>
__attribute__((target("avx2"))) inline void
encodeHexVecImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    const auto raw_length = static_cast<size_t>(len);
    if (raw_length < 32)
        encodeHexSmallVec<H>(dest, src, raw_length);
    else
        encodeHexVecLarge<H>(dest, src, raw_length);
}

#endif // defined(FAST_HEX_AVX2_HELPERS)

#if defined(FAST_HEX_AVX512)
// 32 bytes (low half of in) -> 64 hex chars
template <HexCase H>
FAST_HEX_AVX512_TARGET inline __m512i encodeHex32Vec512(__m512i in)
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
FAST_HEX_AVX512_TARGET inline void encodeHexVec512Large(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    auto raw_length = len;
    while (raw_length >= 64)
    {
        __m512i a = _mm512_castsi256_si512(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
        __m512i b = _mm512_castsi256_si512(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 32)));
        _mm512_storeu_si512(dest, encodeHex32Vec512<H>(a));
        _mm512_storeu_si512(dest + 64, encodeHex32Vec512<H>(b));
        src += 64;
        dest += 128;
        raw_length -= 64;
    }
    if (raw_length >= 32)
    {
        __m512i a = _mm512_castsi256_si512(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
        _mm512_storeu_si512(dest, encodeHex32Vec512<H>(a));
        src += 32;
        dest += 64;
        raw_length -= 32;
    }
    if (raw_length > 0)
    {
        // Encode the last 32 bytes again with a plain load/store. This overlaps output that is already
        // written, but stays inside the buffers.
        __m512i a = _mm512_castsi256_si512(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + raw_length - 32)));
        _mm512_storeu_si512(dest + raw_length * 2 - 64, encodeHex32Vec512<H>(a));
    }
}

template <HexCase H>
FAST_HEX_AVX512_TARGET inline void
encodeHexVec512Impl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    const auto raw_length = static_cast<size_t>(len);
    if (raw_length < 32)
        encodeHexSmallVec<H>(dest, src, raw_length);
    else
        encodeHexVec512Large<H>(dest, src, raw_length);
}

// 64 hex chars -> 32 decoded bytes, each in the low byte of a 16-bit lane
FAST_HEX_AVX512_TARGET inline __m512i unhexPairs512(__m512i value)
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
FAST_HEX_AVX512_TARGET inline void decodeHexVec512Large(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    auto raw_length = len;
    // clang-format off
    // Low byte of every 16 bit lane of a, than b
    const __m512i EVEN_BYTES = _mm512_set_epi8(
            126, 124, 122, 120, 118, 116, 114, 112, 110, 108, 106, 104, 102, 100, 98, 96, 94, 92, 90, 88, 86, 84, 82, 80, 78, 76, 74, 72, 70, 68, 66, 64,
            62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0);
    // clang-format on
    while (raw_length >= 64)
    {
        __m512i a = unhexPairs512(_mm512_loadu_si512(src));
        __m512i b = unhexPairs512(_mm512_loadu_si512(src + 64));
        _mm512_storeu_si512(dest, _mm512_permutex2var_epi8(a, EVEN_BYTES, b));
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
#endif // defined(FAST_HEX_AVX512)

#if defined(FAST_HEX_NEON)

// On AArch64, vqtbl1q_u8/vqtbl1_u8 perform full-width table lookups in a single instruction.
// On ARMv7, fall back to vtbl2_u8 with split/combine.
#    if defined(__aarch64__) || defined(_M_ARM64)
inline uint8x16_t neon_tbl_q(uint8x16_t lut, uint8x16_t idx)
{
    return vqtbl1q_u8(lut, idx);
}
inline uint8x8_t neon_tbl(uint8x16_t lut, uint8x8_t idx)
{
    return vqtbl1_u8(lut, idx);
}
#    else
inline uint8x16_t neon_tbl_q(uint8x16_t lut, uint8x16_t idx)
{
    uint8x8x2_t tbl = {{vget_low_u8(lut), vget_high_u8(lut)}};
    return vcombine_u8(vtbl2_u8(tbl, vget_low_u8(idx)), vtbl2_u8(tbl, vget_high_u8(idx)));
}
inline uint8x8_t neon_tbl(uint8x16_t lut, uint8x8_t idx)
{
    uint8x8x2_t tbl = {{vget_low_u8(lut), vget_high_u8(lut)}};
    return vtbl2_u8(tbl, idx);
}
#    endif

template <HexCase H>
void encodeHexNeon_impl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength raw_len)
{
    auto len = static_cast<size_t>(raw_len);
    // clang-format off
    alignas(16) constexpr uint8_t HEX_LUT_LOWER[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    alignas(16) constexpr uint8_t HEX_LUT_UPPER[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    // clang-format on

    uint8x16_t lut = vld1q_u8((H == HexCase::Upper) ? HEX_LUT_UPPER : HEX_LUT_LOWER);

    size_t i = 0;

    while (len >= 16)
    {
        uint8x16_t invec = vld1q_u8(src + i);

        uint8x16_t hi_nibbles = vshrq_n_u8(invec, 4);
        uint8x16_t lo_nibbles = vandq_u8(invec, vdupq_n_u8(0x0F));

        uint8x16_t hi_chars = neon_tbl_q(lut, hi_nibbles);
        uint8x16_t lo_chars = neon_tbl_q(lut, lo_nibbles);

        uint8x16x2_t interleaved = vzipq_u8(hi_chars, lo_chars);

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
    // clang-format off
    alignas(16) static const uint8_t HEX_LUT_LOWER[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    alignas(16) static const uint8_t HEX_LUT_UPPER[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    // clang-format on

    uint8x16_t lut = vld1q_u8((H == HexCase::Upper) ? HEX_LUT_UPPER : HEX_LUT_LOWER);

    uint8x8_t in = vld1_u8(src);
    if constexpr (R == Reverse::Yes64)
    {
        in = vrev64_u8(in);
    }

    uint8x8_t hi = vshr_n_u8(in, 4);
    uint8x8_t lo = vand_u8(in, vdup_n_u8(0x0F));

    uint8x8_t hi_chars = neon_tbl(lut, hi);
    uint8x8_t lo_chars = neon_tbl(lut, lo);

    uint8x8x2_t zipped = vzip_u8(hi_chars, lo_chars);
    vst1_u8(dest, zipped.val[0]);
    vst1_u8(dest + 8, zipped.val[1]);
}

template <HexCase H, Reverse R = Reverse::No>
void encodeHexNeon16_impl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    // clang-format off
    alignas(16) static const uint8_t HEX_LUT_LOWER[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    alignas(16) static const uint8_t HEX_LUT_UPPER[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    // clang-format on

    uint8x16_t lut = vld1q_u8((H == HexCase::Upper) ? HEX_LUT_UPPER : HEX_LUT_LOWER);

    uint8x16_t in = vld1q_u8(src);
    if constexpr (R == Reverse::Yes128)
    {
        // Full 16-byte reverse: reverse within 64-bit halves, then swap halves
        in = vrev64q_u8(in);
        in = vextq_u8(in, in, 8);
    }
    else if constexpr (R == Reverse::Yes64)
    {
        in = vrev64q_u8(in);
    }

    uint8x16_t hi_nibbles = vshrq_n_u8(in, 4);
    uint8x16_t lo_nibbles = vandq_u8(in, vdupq_n_u8(0x0F));

    uint8x16_t hi_chars = neon_tbl_q(lut, hi_nibbles);
    uint8x16_t lo_chars = neon_tbl_q(lut, lo_nibbles);

    uint8x16x2_t interleaved = vzipq_u8(hi_chars, lo_chars);
    vst1q_u8(dest, interleaved.val[0]);
    vst1q_u8(dest + 16, interleaved.val[1]);
}

#endif // FAST_HEX_NEON

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
inline bool cpuHasAvx512()
{
    return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") && __builtin_cpu_supports("avx512vl")
        && __builtin_cpu_supports("avx512vbmi");
}
#endif // defined(FAST_HEX_AVX512)

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

    if (raw_length < 32)
        decodeHexSmallVec(dest, src, raw_length);
    else
        decodeHexVecLarge(dest, src, raw_length);
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

#if defined(FAST_HEX_AVX512)

// len is number or dest bytes (i.e. half of src length)
FAST_HEX_AVX512_TARGET FAST_HEX_FUNCTION_INLINE void
decodeHexVec512(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    using namespace heks_detail;
    const auto raw_length = static_cast<size_t>(len);
    if (raw_length > 16 && raw_length < 32)
    {
        __m256i first = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
        __m256i last = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + raw_length * 2 - 32));
        __m256i a = _mm512_cvtepi16_epi8(unhexPairs512(_mm512_inserti64x4(_mm512_castsi256_si512(first), last, 1)));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), _mm256_castsi256_si128(a));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + raw_length - 16), _mm256_extracti128_si256(a, 1));
    }
    else if (raw_length < 32)
        decodeHexSmallVec(dest, src, raw_length);
    else
        decodeHexVec512Large(dest, src, raw_length);
}

FAST_HEX_AVX512_TARGET FAST_HEX_FUNCTION_INLINE void
encodeHexLowerVec512(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::encodeHexVec512Impl<heks_detail::HexCase::Lower>(dest, src, len);
}
FAST_HEX_AVX512_TARGET FAST_HEX_FUNCTION_INLINE void
encodeHexUpperVec512(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::encodeHexVec512Impl<heks_detail::HexCase::Upper>(dest, src, len);
}
#endif // defined(FAST_HEX_AVX512)

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

FAST_HEX_FUNCTION_INLINE void encodeHex16LowerNeon(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    heks_detail::encodeHexNeon16_impl<heks_detail::HexCase::Lower>(dest, src);
}
FAST_HEX_FUNCTION_INLINE void encodeHex16UpperNeon(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    heks_detail::encodeHexNeon16_impl<heks_detail::HexCase::Upper>(dest, src);
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

// The x86-64 paths are picked at runtime from those compiled in, so the CPU only needs what it runs
template <class Case>
inline void encode_auto(uint8_t * FAST_HEX_RESTRICT d, const uint8_t * FAST_HEX_RESTRICT s, RawLength n, Case)
{
    constexpr auto case_type = Case::value;
#if defined(__x86_64__) || defined(_M_X64)
    const auto len = static_cast<size_t>(n);
#    if defined(FAST_HEX_AVX2_HELPERS)
    if (len < 32 && heks_detail::cpuHasAvx2())
    {
        heks_detail::encodeHexSmallVec<case_type>(d, s, len);
        return;
    }
#    endif
#    if defined(FAST_HEX_AVX512)
    if (len >= 32 && heks_detail::cpuHasAvx512())
    {
        heks_detail::encodeHexVec512Large<case_type>(d, s, len);
        return;
    }
#    endif
#    if defined(FAST_HEX_AVX2)
    if (len >= 32 && heks_detail::cpuHasAvx2())
    {
        heks_detail::encodeHexVecLarge<case_type>(d, s, len);
        return;
    }
#    endif
    heks_detail::encodeHexImpl<case_type>(d, s, n);
#elif defined(FAST_HEX_NEON)
    heks_detail::encodeHexNeon_impl<case_type>(d, s, n);
#else
    heks_detail::encodeHexImpl<case_type>(d, s, n);
#endif
}

inline void decode_auto(uint8_t * FAST_HEX_RESTRICT d, const uint8_t * FAST_HEX_RESTRICT s, RawLength n)
{
#if defined(__x86_64__) || defined(_M_X64)
    const auto len = static_cast<size_t>(n);
#    if defined(FAST_HEX_AVX2_HELPERS)
    if (len < 32 && heks_detail::cpuHasAvx2())
    {
        heks_detail::decodeHexSmallVec(d, s, len);
        return;
    }
#    endif
#    if defined(FAST_HEX_AVX512)
    if (len >= 32 && heks_detail::cpuHasAvx512())
    {
        heks_detail::decodeHexVec512Large(d, s, len);
        return;
    }
#    endif
#    if defined(FAST_HEX_AVX2)
    if (len >= 32 && heks_detail::cpuHasAvx2())
    {
        heks_detail::decodeHexVecLarge(d, s, len);
        return;
    }
#    endif
    decodeHexBMI(d, s, n);
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

// Non-template entry points for encode_auto / decode_auto, so the library can export them
FAST_HEX_FUNCTION_INLINE void decodeHexAuto(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    decode_auto(dest, src, len);
}
FAST_HEX_FUNCTION_INLINE void encodeHexLowerAuto(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    encode_auto(dest, src, len, lower);
}
FAST_HEX_FUNCTION_INLINE void encodeHexUpperAuto(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    encode_auto(dest, src, len, upper);
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
__attribute__((target("avx2"))) void encode_integral2x8(uint8_t * FAST_HEX_RESTRICT output, const T * FAST_HEX_RESTRICT first, Case)
{
    using namespace heks_detail;
    static_assert(sizeof(T) == 8, "T must be 8 bytes (64 bits) in size");
    constexpr auto case_type = Case::value;
    const auto * input = reinterpret_cast<const uint8_t *>(first);
    encodeHex16Fast<case_type, Reverse::Yes64>(output, input);
}
#elif defined(FAST_HEX_NEON)
template <typename T, typename Case>
void encode_integral16(uint8_t * output, T number, Case)
{
    using namespace heks_detail;
    static_assert(sizeof(T) == 16, "T must be 16 bytes (128 bits) in size");
    constexpr auto case_type = Case::value;
    const auto * input = reinterpret_cast<const uint8_t *>(&number);
    if constexpr (std::endian::native == std::endian::little)
        encodeHexNeon16_impl<case_type, Reverse::Yes128>(output, input);
    else
        encodeHexNeon16_impl<case_type, Reverse::No>(output, input);
}
template <typename T, typename Case>
void encode_integral2x8(uint8_t * FAST_HEX_RESTRICT output, const T * FAST_HEX_RESTRICT first, Case)
{
    using namespace heks_detail;
    static_assert(sizeof(T) == 8, "T must be 8 bytes (64 bits) in size");
    constexpr auto case_type = Case::value;
    const auto * input = reinterpret_cast<const uint8_t *>(first);
    if constexpr (std::endian::native == std::endian::little)
        encodeHexNeon16_impl<case_type, Reverse::Yes64>(output, input);
    else
        encodeHexNeon16_impl<case_type, Reverse::No>(output, input);
}
#endif

FAST_HEX_NAMESPACE_CLOSE
