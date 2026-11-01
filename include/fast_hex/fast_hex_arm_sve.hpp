#pragma once

// SVE functions, for AArch64 CPUs with SVE (Graviton 3, Neoverse V1). Empty unless FAST_HEX_ARM_SVE is defined.
// See fast_hex.hpp for the contract.
//
//   encodeHexSve, decodeHexSve
//   cpuHasSve
//
// Like the x86-64 functions, they carry their own target attribute, target("+sve") as in ClickHouse's ARM_SVE code,
// so they need no build flags, but calling one is only valid where cpuHasSve() is true.
//
// Up to 32 bytes they run the NEON code, which is faster there. Above that, an SVE loop over full vectors, then
// one predicated vector for the rest, with no scalar code. Where SVE vectors are 128 bits (Graviton 4, Neoverse V2
// and N2), no wider than NEON, encodeHexSve runs the NEON loop instead, as its SVE loop is 30% slower there, and
// decodeHexSve keeps its SVE loop, still 3-10% faster than NEON. Both measured on Neoverse-V1 with the vector
// length cut to 128 bits, not on those CPUs. There are no integer functions: the NEON ones already take an 8 to
// 32-byte value in one or two vectors.

#include "fast_hex_arm_neon.hpp"

#if defined(FAST_HEX_ARM_SVE)

#    include <arm_sve.h>

#    if defined(__linux__) || defined(__FreeBSD__)
#        include <sys/auxv.h>
#    endif

#    define FAST_HEX_TARGET_SVE __attribute__((target("+sve")))

namespace heks
{
namespace heks_detail
{

template <HexCase H>
FAST_HEX_TARGET_SVE inline svuint8_t sveHexLut()
{
    // The 16 digits in every 128-bit segment, so tbl finds them at indices 0-15 whatever the vector length
    return svld1rq_u8(svptrue_b8(), hexDigits<H>());
}

// The bytes in pg -> their hex chars, the first half of the chars in the first returned vector
FAST_HEX_TARGET_SVE inline svuint8x2_t encodeHexSveVec(svbool_t pg, svuint8_t in, svuint8_t lut)
{
    const svuint8_t hi = svtbl_u8(lut, svlsr_n_u8_x(pg, in, 4));
    const svuint8_t lo = svtbl_u8(lut, svand_n_u8_x(pg, in, 0x0F));
    // Two zips and plain stores: an interleaving st2b is 3-7% slower on long inputs on Neoverse-V1
    return svcreate2_u8(svzip1_u8(hi, lo), svzip2_u8(hi, lo));
}

// len > 32. Out of line, like encodeHexNeonLarge. With 128-bit vectors, the NEON loop: a second call, which
// costs 0.05 ns there, while checking the length in encodeHexSve instead cost 0.2 ns at 63 bytes with 256-bit vectors
template <HexCase H>
FAST_HEX_TARGET_SVE __attribute__((noinline)) void
encodeHexSveLarge(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    const size_t vl = svcntb();
    if (vl < 32)
        return encodeHexNeonLarge<H>(dest, src, len, neonHexLut<H>());

    const svuint8_t lut = sveHexLut<H>();
    const svbool_t all = svptrue_b8();
    size_t i = 0;
    for (; i + vl <= len; i += vl)
    {
        const svuint8x2_t chars = encodeHexSveVec(all, svld1_u8(all, src + i), lut);
        svst1_u8(all, dest + i * 2, svget2_u8(chars, 0));
        svst1_u8(all, dest + i * 2 + vl, svget2_u8(chars, 1));
    }
    if (i < len)
    {
        // The rest, under 1 vector of bytes: the stores take the lanes below the end of the 2 * len chars
        const size_t chars_len = len * 2;
        const svbool_t pg = svwhilelt_b8_u64(i, len);
        const svuint8x2_t chars = encodeHexSveVec(pg, svld1_u8(pg, src + i), lut);
        svst1_u8(svwhilelt_b8_u64(i * 2, chars_len), dest + i * 2, svget2_u8(chars, 0));
        // One vector on from the first store's address, so no pointer past the end of dest is formed when all the
        // lanes are off, as at 33 bytes with 256-bit vectors
        svst1_vnum_u8(svwhilelt_b8_u64(i * 2 + vl, chars_len), dest + i * 2, 1, svget2_u8(chars, 1));
    }
}

// 2 * vl hex chars -> vl bytes, for the lanes in pg: hi << 4 | (lo & 0xF) of the unhexBitManip nibbles, as
// decodeHexNeon, so the two decode input that is not hex the same. Neither nibble needs the & 0xF of
// unhexBitManip first: the high one's upper bits leave when it is multiplied by 16, and the low one is masked after
FAST_HEX_TARGET_SVE inline svuint8_t decodeHexSveVec(svbool_t pg, const uint8_t * src)
{
    const svuint8x2_t chars = svld2_u8(pg, src); // Even chars (high nibbles) in the first vector, odd in the second
    const svuint8_t ch = svget2_u8(chars, 0);
    const svuint8_t cl = svget2_u8(chars, 1);
    const svuint8_t hi = svmla_n_u8_x(pg, ch, svlsr_n_u8_x(pg, ch, 6), 9);
    const svuint8_t lo = svand_n_u8_x(pg, svmla_n_u8_x(pg, cl, svlsr_n_u8_x(pg, cl, 6), 9), 0x0F);
    return svmla_n_u8_x(pg, lo, hi, 16);
}

// len > 32. Out of line, like decodeHexNeonLarge. Any vector length
FAST_HEX_TARGET_SVE __attribute__((noinline)) inline void
decodeHexSveLarge(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    const size_t vl = svcntb();
    const svbool_t all = svptrue_b8();
    size_t i = 0;
    for (; i + vl * 2 <= len; i += vl * 2)
    {
        const svuint8_t a = decodeHexSveVec(all, src + i * 2);
        const svuint8_t b = decodeHexSveVec(all, src + i * 2 + vl * 2);
        svst1_u8(all, dest + i, a);
        svst1_u8(all, dest + i + vl, b);
    }
    if (i + vl <= len)
    {
        svst1_u8(all, dest + i, decodeHexSveVec(all, src + i * 2));
        i += vl;
    }
    if (i < len)
    {
        const svbool_t pg = svwhilelt_b8_u64(i, len);
        svst1_u8(pg, dest + i, decodeHexSveVec(pg, src + i * 2));
    }
}

} // namespace heks_detail

// Whether the CPU can run the SVE functions: false where the OS can't tell (only Linux and FreeBSD can here).
// Cheap, but not free: hoist it out of hot loops.
inline bool cpuHasSve()
{
    constexpr unsigned long HWCAP_SVE_BIT = 1UL << 22; // HWCAP_SVE, the same bit on Linux and FreeBSD
#    if defined(__linux__)
    return (getauxval(AT_HWCAP) & HWCAP_SVE_BIT) != 0;
#    elif defined(__FreeBSD__)
    unsigned long hwcap = 0;
    return elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap)) == 0 && (hwcap & HWCAP_SVE_BIT) != 0;
#    else
    return false;
#    endif
}

template <class Case>
FAST_HEX_TARGET_SVE void encodeHexSve(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len, Case)
{
    const auto n = static_cast<size_t>(len);
    if (n > 32)
        return heks_detail::encodeHexSveLarge<Case::value>(dest, src, n);
    // The NEON code for up to 32 bytes, inlined: its own branch to the NEON loop is never taken
    heks_detail::encodeHexNeonImpl<Case::value>(dest, src, n);
}

FAST_HEX_TARGET_SVE inline void decodeHexSve(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    const auto n = static_cast<size_t>(len);
    if (n > 32)
        return heks_detail::decodeHexSveLarge(dest, src, n);
    heks_detail::decodeHexNeonImpl(dest, src, n);
}

} // namespace heks

#    undef FAST_HEX_TARGET_SVE

#endif // defined(FAST_HEX_ARM_SVE)
