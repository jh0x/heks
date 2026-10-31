#pragma once

// NEON functions, for AArch64 and ARMv7 with NEON. Empty unless FAST_HEX_NEON is defined. See fast_hex.hpp for
// the contract.
//
//   encodeHexNeon, decodeHexNeon
//   encodeHexIntNeon, decodeHexIntNeon, encodeHexInt2x8Neon
//
// Inputs that are not a multiple of the vector width finish with a vector block that overlaps the previous one
// rather than a scalar loop. The overlapped bytes are written twice with the same value, which is only valid
// because source and destination never overlap.

#include "fast_hex.hpp"

#if defined(FAST_HEX_NEON)

#    include <arm_neon.h>

namespace heks
{
namespace heks_detail
{

// On AArch64, vqtbl1q_u8/vqtbl1_u8 perform full-width table lookups in a single instruction.
// On ARMv7, fall back to vtbl2_u8 with split/combine.
#    if defined(__aarch64__)
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

// The 16 hex digits, as the tables of the NEON and SVE lookups
// clang-format off
alignas(16) inline constexpr uint8_t hex_digits_lower[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
alignas(16) inline constexpr uint8_t hex_digits_upper[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
// clang-format on

template <HexCase H>
constexpr const uint8_t * hexDigits()
{
    return H == HexCase::Lower ? hex_digits_lower : hex_digits_upper;
}

template <HexCase H>
inline uint8x16_t neonHexLut()
{
    return vld1q_u8(hexDigits<H>());
}

// 16 bytes -> 32 hex chars
inline void encodeHex16Neon(uint8_t * dest, uint8x16_t in, uint8x16_t lut)
{
    uint8x16_t hi_chars = neon_tbl_q(lut, vshrq_n_u8(in, 4));
    uint8x16_t lo_chars = neon_tbl_q(lut, vandq_u8(in, vdupq_n_u8(0x0F)));
    uint8x16x2_t interleaved = vzipq_u8(hi_chars, lo_chars);
#    if defined(__GNUC__)
    // Clang fuses a zip and its store into ST2, which is slower on Neoverse cores: about 10% on long inputs on
    // Neoverse-N1. The empty asm hides that the stored values are zips
    __asm__("" : "+w"(interleaved.val[0]), "+w"(interleaved.val[1]));
#    endif
    vst1q_u8(dest, interleaved.val[0]);
    vst1q_u8(dest + 16, interleaved.val[1]);
}

// 8 bytes -> 16 hex chars, as two halves of 8 chars
inline uint8x8x2_t encodeHex8Neon(uint8x8_t in, uint8x16_t lut)
{
    uint8x8_t hi_chars = neon_tbl(lut, vshr_n_u8(in, 4));
    uint8x8_t lo_chars = neon_tbl(lut, vand_u8(in, vdup_n_u8(0x0F)));
    return vzip_u8(hi_chars, lo_chars);
}

inline void encodeHex8Neon(uint8_t * dest, uint8x8_t in, uint8x16_t lut)
{
    uint8x8x2_t zipped = encodeHex8Neon(in, lut);
    vst1_u8(dest, zipped.val[0]);
    vst1_u8(dest + 8, zipped.val[1]);
}

// len < 16
template <HexCase H>
inline void encodeHexNeonShort(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len, uint8x16_t lut)
{
    if (len >= 8)
    {
        // The first and last 8 bytes, which overlap below 16
        uint8x8_t a = vld1_u8(src);
        uint8x8_t b = vld1_u8(src + len - 8);
        encodeHex8Neon(dest, a, lut);
        encodeHex8Neon(dest + (len - 8) * 2, b, lut);
    }
    else if (len >= 4)
    {
        // The first and last 4 bytes in one vector, 8 hex chars each
        uint8_t buf[8];
        std::memcpy(buf, src, 4);
        std::memcpy(buf + 4, src + len - 4, 4);
        uint8x8x2_t zipped = encodeHex8Neon(vld1_u8(buf), lut);
        vst1_u8(dest, zipped.val[0]);
        vst1_u8(dest + (len - 4) * 2, zipped.val[1]);
    }
    else
    {
        encodeHexImpl<H>(dest, src, len);
    }
}

// len > 32. Kept out of line, so the code for short inputs is small enough to inline into callers. The table
// comes in a register from the inlined caller, which can keep it out of its own loop, rather than being loaded
// at the start of every call. The last len % 16 bytes go through encodeHexNeonShort rather than a 16-byte block
// overlapping the loop's last stores, which is slower on Neoverse-N1 than even the scalar code for one byte
template <HexCase H>
__attribute__((noinline)) void
encodeHexNeonLarge(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len, uint8x16_t lut)
{
    size_t i = 0;
    for (; i + 16 <= len; i += 16)
        encodeHex16Neon(dest + i * 2, vld1q_u8(src + i), lut);
    encodeHexNeonShort<H>(dest + i * 2, src + i, len - i, lut);
}

// Up to 32 bytes without a loop, always inlined so that short inputs cost no call
template <HexCase H>
__attribute__((always_inline)) inline void
encodeHexNeonImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    const uint8x16_t lut = neonHexLut<H>();
    if (len > 32)
        return encodeHexNeonLarge<H>(dest, src, len, lut);
    if (len >= 16)
    {
        // The first and last 16 bytes, which overlap below 32. Exactly 16 skips the second: storing the same
        // 32 chars twice costs more than the branch
        uint8x16_t a = vld1q_u8(src);
        uint8x16_t b = vld1q_u8(src + len - 16);
        encodeHex16Neon(dest, a, lut);
        if (len > 16)
            encodeHex16Neon(dest + (len - 16) * 2, b, lut);
    }
    else
    {
        encodeHexNeonShort<H>(dest, src, len, lut);
    }
}

// Hex chars -> nibbles, as unhexBitManip: '0'-'9' -> 0-9, 'A'-'F' and 'a'-'f' -> 10-15
inline uint8x16_t unhexNibblesNeon(uint8x16_t c)
{
    return vmlaq_u8(vandq_u8(c, vdupq_n_u8(0x0F)), vshrq_n_u8(c, 6), vdupq_n_u8(9));
}
inline uint8x8_t unhexNibblesNeon(uint8x8_t c)
{
    return vmla_u8(vand_u8(c, vdup_n_u8(0x0F)), vshr_n_u8(c, 6), vdup_n_u8(9));
}

// 32 hex chars -> 16 bytes. The shift and insert keeps the low 4 bits of the low nibble, so input that is not
// hex can decode differently than decodeHex
inline uint8x16_t decodeHex16Neon(const uint8_t * src)
{
    uint8x16x2_t chars = vld2q_u8(src); // Even chars (high nibbles) in val[0], odd in val[1]
    return vsliq_n_u8(unhexNibblesNeon(chars.val[1]), unhexNibblesNeon(chars.val[0]), 4);
}

// Even chars of a and b in val[0], odd in val[1] -> 8 bytes
inline uint8x8_t decodeHex8Neon(uint8x8x2_t chars)
{
    return vsli_n_u8(unhexNibblesNeon(chars.val[1]), unhexNibblesNeon(chars.val[0]), 4);
}

// len < 16
inline void decodeHexNeonShort(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    if (len >= 8)
    {
        // The first and last 8 bytes, which overlap below 16
        vst1_u8(dest, decodeHex8Neon(vld2_u8(src)));
        if (len > 8)
            vst1_u8(dest + len - 8, decodeHex8Neon(vld2_u8(src + (len - 8) * 2)));
    }
    else if (len >= 4)
    {
        // The first and last 8 chars: 4 bytes each
        uint8x8_t first = vld1_u8(src);
        uint8x8_t last = vld1_u8(src + (len - 4) * 2);
        // Each half as a 32-bit lane: through a buffer, Clang moves the bytes one at a time
        const uint32x2_t out = vreinterpret_u32_u8(decodeHex8Neon(vuzp_u8(first, last)));
        const uint32_t lo = vget_lane_u32(out, 0);
        const uint32_t hi = vget_lane_u32(out, 1);
        std::memcpy(dest, &lo, 4);
        std::memcpy(dest + len - 4, &hi, 4);
    }
    else if (len > 0)
    {
        // Unrolled by hand: as a loop, Clang vectorizes it, which costs more than these few bytes. The low nibble
        // keeps only its 4 bits, as in the vector code's sli, so input that is not hex decodes the same at any length
        auto byte
            = [src](size_t i) { return static_cast<uint8_t>((unhexBitManip(src[i * 2]) << 4) | (unhexBitManip(src[i * 2 + 1]) & 0x0F)); };
        dest[0] = byte(0);
        if (len > 1)
            dest[1] = byte(1);
        if (len > 2)
            dest[2] = byte(2);
    }
}

// len > 32. Out of line, with the same tail as encodeHexNeonLarge
__attribute__((noinline)) inline void
decodeHexNeonLarge(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    size_t i = 0;
    for (; i + 16 <= len; i += 16)
        vst1q_u8(dest + i, decodeHex16Neon(src + i * 2));
    decodeHexNeonShort(dest + i, src + i * 2, len - i);
}

// Up to 32 bytes without a loop, always inlined so that short inputs cost no call
__attribute__((always_inline)) inline void
decodeHexNeonImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    if (len > 32)
        return decodeHexNeonLarge(dest, src, len);
    if (len >= 16)
    {
        // The first and last 16 bytes, which overlap below 32
        uint8x16_t a = decodeHex16Neon(src);
        vst1q_u8(dest, a);
        if (len > 16)
            vst1q_u8(dest + len - 16, decodeHex16Neon(src + (len - 16) * 2));
    }
    else
    {
        decodeHexNeonShort(dest, src, len);
    }
}

// The 16 bytes of a little-endian 128-bit value, most significant first
inline uint8x16_t reversed16(uint8x16_t in)
{
    // Reverse within 64-bit halves, then swap halves
    in = vrev64q_u8(in);
    return vextq_u8(in, in, 8);
}

} // namespace heks_detail

template <class Case>
__attribute__((always_inline)) inline void
encodeHexNeon(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len, Case)
{
    heks_detail::encodeHexNeonImpl<Case::value>(dest, src, static_cast<size_t>(len));
}

__attribute__((always_inline)) inline void
decodeHexNeon(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    heks_detail::decodeHexNeonImpl(dest, src, static_cast<size_t>(len));
}

// encodeHexInt for 8, 16 and 32-byte types
template <class T, class Case>
void encodeHexIntNeon(uint8_t * dest, T value, Case)
{
    heks_detail::checkSimdIntType<T>();
    const uint8x16_t lut = heks_detail::neonHexLut<Case::value>();
    constexpr bool little = std::endian::native == std::endian::little;
    const auto * bytes = reinterpret_cast<const uint8_t *>(&value);
    using heks_detail::reversed16;
    if constexpr (sizeof(T) == 8)
    {
        uint8x8_t in = vld1_u8(bytes);
        heks_detail::encodeHex8Neon(dest, little ? vrev64_u8(in) : in, lut);
    }
    else if constexpr (sizeof(T) == 16)
    {
        uint8x16_t in = vld1q_u8(bytes);
        heks_detail::encodeHex16Neon(dest, little ? reversed16(in) : in, lut);
    }
    else
    {
        // The more significant half first
        uint8x16_t hi = vld1q_u8(little ? bytes + 16 : bytes);
        uint8x16_t lo = vld1q_u8(little ? bytes : bytes + 16);
        heks_detail::encodeHex16Neon(dest, little ? reversed16(hi) : hi, lut);
        heks_detail::encodeHex16Neon(dest + 32, little ? reversed16(lo) : lo, lut);
    }
}

// decodeHexInt for 8, 16 and 32-byte types
template <class T>
T decodeHexIntNeon(const uint8_t * src)
{
    heks_detail::checkSimdIntType<T>();
    constexpr bool little = std::endian::native == std::endian::little;
    using heks_detail::reversed16;
    alignas(16) std::array<uint8_t, sizeof(T)> buf;
    uint8_t * bytes = buf.data();
    if constexpr (sizeof(T) == 8)
    {
        uint8x8_t v = heks_detail::decodeHex8Neon(vld2_u8(src));
        vst1_u8(bytes, little ? vrev64_u8(v) : v);
    }
    else if constexpr (sizeof(T) == 16)
    {
        uint8x16_t v = heks_detail::decodeHex16Neon(src);
        vst1q_u8(bytes, little ? reversed16(v) : v);
    }
    else
    {
        // The first 32 chars are the more significant half
        uint8x16_t hi = heks_detail::decodeHex16Neon(src);
        uint8x16_t lo = heks_detail::decodeHex16Neon(src + 32);
        vst1q_u8(little ? bytes + 16 : bytes, little ? reversed16(hi) : hi);
        vst1q_u8(little ? bytes : bytes + 16, little ? reversed16(lo) : lo);
    }
    return std::bit_cast<T>(buf);
}

// encodeHexInt2x8 for 8-byte types
template <class T, class Case>
void encodeHexInt2x8Neon(uint8_t * dest, const T * first, Case)
{
    heks_detail::checkIntType<T>();
    static_assert(sizeof(T) == 8, "T must be 8 bytes");
    constexpr bool little = std::endian::native == std::endian::little;
    uint8x16_t in = vld1q_u8(reinterpret_cast<const uint8_t *>(first));
    // Each value's bytes most significant first, in place
    heks_detail::encodeHex16Neon(dest, little ? vrev64q_u8(in) : in, heks_detail::neonHexLut<Case::value>());
}

} // namespace heks

#endif // defined(FAST_HEX_NEON)
