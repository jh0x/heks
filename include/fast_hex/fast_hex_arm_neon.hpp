#pragma once

// NEON functions, for AArch64 and ARMv7 with NEON. Empty unless FAST_HEX_NEON is defined. See fast_hex.hpp for
// the contract.
//
//   encodeHexNeon
//   encodeHexIntNeon, encodeHexInt2x8Neon
//
// There is no NEON decoder: decodeHex is vectorized on ARM too.

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

template <HexCase H>
inline uint8x16_t neonHexLut()
{
    // clang-format off
    alignas(16) static constexpr uint8_t HEX_LUT_LOWER[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    alignas(16) static constexpr uint8_t HEX_LUT_UPPER[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    // clang-format on
    return vld1q_u8(H == HexCase::Lower ? HEX_LUT_LOWER : HEX_LUT_UPPER);
}

template <HexCase H>
void encodeHexNeonImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    const uint8x16_t lut = neonHexLut<H>();

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

    encodeHexImpl<H>(dest + (i * 2), src + i, len);
}

// 8 bytes -> 16 hex chars
template <HexCase H>
void encodeHex8Neon(uint8_t * dest, uint8x8_t in)
{
    const uint8x16_t lut = neonHexLut<H>();

    uint8x8_t hi = vshr_n_u8(in, 4);
    uint8x8_t lo = vand_u8(in, vdup_n_u8(0x0F));

    uint8x8_t hi_chars = neon_tbl(lut, hi);
    uint8x8_t lo_chars = neon_tbl(lut, lo);

    uint8x8x2_t zipped = vzip_u8(hi_chars, lo_chars);
    vst1_u8(dest, zipped.val[0]);
    vst1_u8(dest + 8, zipped.val[1]);
}

// 16 bytes -> 32 hex chars
template <HexCase H>
void encodeHex16Neon(uint8_t * dest, uint8x16_t in)
{
    const uint8x16_t lut = neonHexLut<H>();

    uint8x16_t hi_nibbles = vshrq_n_u8(in, 4);
    uint8x16_t lo_nibbles = vandq_u8(in, vdupq_n_u8(0x0F));

    uint8x16_t hi_chars = neon_tbl_q(lut, hi_nibbles);
    uint8x16_t lo_chars = neon_tbl_q(lut, lo_nibbles);

    uint8x16x2_t interleaved = vzipq_u8(hi_chars, lo_chars);
    vst1q_u8(dest, interleaved.val[0]);
    vst1q_u8(dest + 16, interleaved.val[1]);
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
void encodeHexNeon(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len, Case)
{
    heks_detail::encodeHexNeonImpl<Case::value>(dest, src, static_cast<size_t>(len));
}

// encodeHexInt for 8, 16 and 32-byte types
template <class T, class Case>
void encodeHexIntNeon(uint8_t * dest, T value, Case)
{
    heks_detail::checkSimdIntType<T>();
    constexpr auto H = Case::value;
    constexpr bool little = std::endian::native == std::endian::little;
    const auto * bytes = reinterpret_cast<const uint8_t *>(&value);
    using heks_detail::reversed16;
    if constexpr (sizeof(T) == 8)
    {
        uint8x8_t in = vld1_u8(bytes);
        heks_detail::encodeHex8Neon<H>(dest, little ? vrev64_u8(in) : in);
    }
    else if constexpr (sizeof(T) == 16)
    {
        uint8x16_t in = vld1q_u8(bytes);
        heks_detail::encodeHex16Neon<H>(dest, little ? reversed16(in) : in);
    }
    else
    {
        // The more significant half first
        uint8x16_t hi = vld1q_u8(little ? bytes + 16 : bytes);
        uint8x16_t lo = vld1q_u8(little ? bytes : bytes + 16);
        heks_detail::encodeHex16Neon<H>(dest, little ? reversed16(hi) : hi);
        heks_detail::encodeHex16Neon<H>(dest + 32, little ? reversed16(lo) : lo);
    }
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
    heks_detail::encodeHex16Neon<Case::value>(dest, little ? vrev64q_u8(in) : in);
}

} // namespace heks

#endif // defined(FAST_HEX_NEON)
