#include "fast_hex/fast_hex.hpp"

#include <string_view>

#if defined(__AVX2__)
#    if defined(__GNUC__)
#        include <immintrin.h>
#    elif defined(_MSC_VER)
#        include <intrin.h>
#    endif
#endif

FAST_HEX_NAMESPACE_OPEN

// This implementation is by https://github.com/zbjornson/fast-hex
// Only introduced some minor modernisations and style changes to those functions
// decodeHexLUT
// decodeHexLUT4
// decodeHexVec
// encodeHexImpl
// encodeHexVecImpl


using namespace std::literals::string_view_literals;

enum class HexCase
{
    Lower,
    Upper
};

// clang-format off
// ASCII -> hex value as a string_view
static constexpr std::string_view unhex_table_sv =
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
static constexpr std::string_view unhex_table4_sv =
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
// clang-format on

// Static assert to ensure the tables have the right size
static_assert(unhex_table_sv.size() == 256, "unhex_table_sv must have 256 elements");
static_assert(unhex_table4_sv.size() == 256, "unhex_table4_sv must have 256 elements");

// Looks up the value for the lower nibble.
static constexpr uint8_t unhexB(uint8_t x)
{
    return static_cast<uint8_t>(unhex_table_sv[x]);
}

// Looks up the value for the upper nibble. Equivalent to `unhexB(x) << 4`.
static constexpr uint8_t unhexA(uint8_t x)
{
    return static_cast<uint8_t>(unhex_table4_sv[x]);
}

#if defined(__AVX2__)
static constexpr int8_t unhexBitManip(uint8_t x)
{
    return 9 * (x >> 6) + (x & 0xf);
}

static const __m256i _9 = _mm256_set1_epi16(9);
static const __m256i _15 = _mm256_set1_epi16(0xf);

inline static __m256i unhexBitManip(const __m256i value)
{
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

// clang-format off
static const __m256i HEX_LUTR_LOWER = _mm256_setr_epi8('0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f');
static const __m256i HEX_LUTR_UPPER = _mm256_setr_epi8('0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F');
// clang-format on
template <HexCase H>
inline static __m256i hex(__m256i value)
{
    if constexpr (H == HexCase::Lower)
        return _mm256_shuffle_epi8(HEX_LUTR_LOWER, value);
    else if constexpr (H == HexCase::Upper)
        return _mm256_shuffle_epi8(HEX_LUTR_UPPER, value);
    else
        []() { static_assert(H != H, "Unsupported HexCase"); }();
}

// (a << 4) | b;
// a and b must be 16-bit elements. Output is packed 8-bit elements.
inline static __m256i nib2byte(__m256i a1, __m256i b1, __m256i a2, __m256i b2)
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

static const __m256i ROT2 = _mm256_setr_epi8(
    -1, 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14, -1, 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14);
// a -> [a >> 4, a & 0b1111]
inline static __m256i byte2nib(__m128i val)
{
    __m256i doubled = _mm256_cvtepu8_epi16(val);
    __m256i hi = _mm256_srli_epi16(doubled, 4);
    __m256i lo = _mm256_shuffle_epi8(doubled, ROT2);
    __m256i bytes = _mm256_or_si256(hi, lo);
    bytes = _mm256_and_si256(bytes, _mm256_set1_epi8(0b1111));
    return bytes;
}

// len is number or dest bytes (i.e. half of src length)
void decodeHexBMI(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
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

// len is number or dest bytes (i.e. half of src length)
void decodeHexVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    auto raw_length = static_cast<size_t>(len);
    const __m256i A_MASK = _mm256_setr_epi8(
        0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14, -1, 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14, -1);
    const __m256i B_MASK = _mm256_setr_epi8(
        1, -1, 3, -1, 5, -1, 7, -1, 9, -1, 11, -1, 13, -1, 15, -1, 1, -1, 3, -1, 5, -1, 7, -1, 9, -1, 11, -1, 13, -1, 15, -1);

    const __m256i * val3 = reinterpret_cast<const __m256i *>(src);
    __m256i * dec256 = reinterpret_cast<__m256i *>(dest);

    while (raw_length >= 128)
    {
        // Load 4 pairs of 32-byte chunks
        __m256i av1 = _mm256_loadu_si256(val3++);
        __m256i av2 = _mm256_loadu_si256(val3++);
        __m256i av3 = _mm256_loadu_si256(val3++);
        __m256i av4 = _mm256_loadu_si256(val3++);
        __m256i av5 = _mm256_loadu_si256(val3++);
        __m256i av6 = _mm256_loadu_si256(val3++);
        __m256i av7 = _mm256_loadu_si256(val3++);
        __m256i av8 = _mm256_loadu_si256(val3++);

        // Separate high and low nibbles for all chunks
        __m256i a1 = _mm256_shuffle_epi8(av1, A_MASK);
        __m256i b1 = _mm256_shuffle_epi8(av1, B_MASK);
        __m256i a2 = _mm256_shuffle_epi8(av2, A_MASK);
        __m256i b2 = _mm256_shuffle_epi8(av2, B_MASK);
        __m256i a3 = _mm256_shuffle_epi8(av3, A_MASK);
        __m256i b3 = _mm256_shuffle_epi8(av3, B_MASK);
        __m256i a4 = _mm256_shuffle_epi8(av4, A_MASK);
        __m256i b4 = _mm256_shuffle_epi8(av4, B_MASK);
        __m256i a5 = _mm256_shuffle_epi8(av5, A_MASK);
        __m256i b5 = _mm256_shuffle_epi8(av5, B_MASK);
        __m256i a6 = _mm256_shuffle_epi8(av6, A_MASK);
        __m256i b6 = _mm256_shuffle_epi8(av6, B_MASK);
        __m256i a7 = _mm256_shuffle_epi8(av7, A_MASK);
        __m256i b7 = _mm256_shuffle_epi8(av7, B_MASK);
        __m256i a8 = _mm256_shuffle_epi8(av8, A_MASK);
        __m256i b8 = _mm256_shuffle_epi8(av8, B_MASK);

        // Convert ASCII values to nibbles
        a1 = unhexBitManip(a1);
        a3 = unhexBitManip(a3);
        a5 = unhexBitManip(a5);
        a7 = unhexBitManip(a7);

        a2 = unhexBitManip(a2);
        a4 = unhexBitManip(a4);
        a6 = unhexBitManip(a6);
        a8 = unhexBitManip(a8);

        b1 = unhexBitManip(b1);
        b3 = unhexBitManip(b3);
        b5 = unhexBitManip(b5);
        b7 = unhexBitManip(b7);

        b2 = unhexBitManip(b2);
        b4 = unhexBitManip(b4);
        b6 = unhexBitManip(b6);
        b8 = unhexBitManip(b8);

        // Nibbles to bytes and store
        __m256i bytes1 = nib2byte(a1, b1, a2, b2);
        __m256i bytes2 = nib2byte(a3, b3, a4, b4);
        __m256i bytes3 = nib2byte(a5, b5, a6, b6);
        __m256i bytes4 = nib2byte(a7, b7, a8, b8);

        _mm256_storeu_si256(dec256++, bytes1);
        _mm256_storeu_si256(dec256++, bytes2);
        _mm256_storeu_si256(dec256++, bytes3);
        _mm256_storeu_si256(dec256++, bytes4);
        raw_length -= 128;
    }

    // Handle remaining 32-byte chunks with original approach
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
#endif // defined(__AVX2__)

// len is number of dest bytes
void decodeHexLUT(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
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
void decodeHexLUT4(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
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

// clang-format off
// Hex character lookup as a string_view
static constexpr std::string_view hex_table_lower_sv = "0123456789abcdef"sv;
static constexpr std::string_view hex_table_upper_sv = "0123456789ABCDEF"sv;
// clang-format on

static_assert(hex_table_lower_sv.size() == 16, "hex_table_lower_sv must have 16 elements");
static_assert(hex_table_upper_sv.size() == 16, "hex_table_upper_sv must have 16 elements");

template <HexCase H>
inline static constexpr char hex(uint8_t value)
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
void encodeHexImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    const auto raw_length = static_cast<size_t>(len);
    for (size_t i = 0; i < raw_length; i++)
    {
        uint8_t a = src[i];
        uint8_t lo = a & 0b1111;
        uint8_t hi = a >> 4;
        *dest++ = static_cast<uint8_t>(hex<H>(hi));
        *dest++ = static_cast<uint8_t>(hex<H>(lo));
    }
}

void encodeHexLower(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    encodeHexImpl<HexCase::Lower>(dest, src, len);
}
void encodeHexUpper(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    encodeHexImpl<HexCase::Upper>(dest, src, len);
}

#if defined(__AVX__)
static const __m128i HEX_LUT_LOWER = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f');
static const __m128i HEX_LUT_UPPER = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F');

template <HexCase H>
void encodeHex8Fast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    __m128i v8 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src));

    __m128i hi = _mm_and_si128(_mm_srli_epi16(v8, 4), _mm_set1_epi8(0x0F));
    __m128i lo = _mm_and_si128(v8, _mm_set1_epi8(0x0F));

    // Interleave: hi[0], lo[0], hi[1], lo[1], ...
    __m128i nibs = _mm_unpacklo_epi8(hi, lo);

    __m128i result = _mm_shuffle_epi8(H == HexCase::Lower ? HEX_LUT_LOWER : HEX_LUT_UPPER, nibs);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(dest), result);
}
#endif

#if defined(__AVX2__)
// len is number of src bytes
template <HexCase H>
void encodeHexVecImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
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

void encodeHexLowerVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    encodeHexVecImpl<HexCase::Lower>(dest, src, len);
}
void encodeHexUpperVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    encodeHexVecImpl<HexCase::Upper>(dest, src, len);
}

template <HexCase H>
inline void encodeHex16Fast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    __m128i v16 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
    __m256i nibs = byte2nib(v16);
    __m256i hexed = hex<H>(nibs);
    // Store all 32 bytes (16 input → 32 hex chars)
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest), hexed);
}

void encodeHex8LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    encodeHex8Fast<HexCase::Lower>(dest, src);
}

void encodeHex8UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    encodeHex8Fast<HexCase::Upper>(dest, src);
}

void encodeHex16LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    encodeHex16Fast<HexCase::Lower>(dest, src);
}

void encodeHex16UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src)
{
    encodeHex16Fast<HexCase::Upper>(dest, src);
}

#endif // defined(__AVX2__)

FAST_HEX_NAMESPACE_CLOSE
