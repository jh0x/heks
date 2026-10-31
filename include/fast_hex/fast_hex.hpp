#pragma once

// Hex encoding and decoding. Everything is in namespace heks. This header has the functions for any CPU:
//
//   encodeHex, decodeHex                         buffers
//   decodeHexLUT, decodeHexLUT4                  buffers, faster for short inputs
//   encodeHexInt, decodeHexInt                   integers of any size
//   encodeHexInt2x8                              two 64-bit integers
//
// Each of these headers adds the functions for one instruction set, and is empty where the compiler can't target it:
//
//   fast_hex_x86_64_v3.hpp       AVX2                            (FAST_HEX_X86_64)
//       encodeHexAvx2, decodeHexAvx2, encodeHexIntAvx2, decodeHexIntAvx2, encodeHexInt2x8Avx2, cpuHasAvx2
//   fast_hex_x86_64_icelake.hpp  AVX-512 F, BW, VL and VBMI      (FAST_HEX_X86_64)
//       encodeHexAvx512Vbmi, decodeHexAvx512Vbmi, cpuHasAvx512Vbmi
//   fast_hex_arm_neon.hpp        NEON                            (FAST_HEX_NEON)
//       encodeHexNeon, encodeHexIntNeon, encodeHexInt2x8Neon
//
// Nothing here checks the CPU: the caller picks the function. The AVX2 and AVX-512 functions carry their
// own target attributes, so they are compiled whatever the build flags. Calling one is only valid where the
// CPU has the instruction set: in a build that already requires it, or behind a check such as cpuHasAvx2().
//
// RawLength is the length of the raw (binary) data: encoding reads len bytes and writes len * 2 hex chars,
// decoding reads len * 2 hex chars and writes len bytes. Encoding takes the case as heks::lower or heks::upper.
// Decoding accepts either case. Input that is not hex decodes to unspecified bytes, nothing worse.

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

#if defined(_MSC_VER)
#    define FAST_HEX_RESTRICT __restrict // The C99 keyword, available as a C++ extension
#else
#    define FAST_HEX_RESTRICT __restrict__
#endif

// MSVC has no target attributes, so it only gets the scalar code on x86-64
#if defined(__GNUC__) && defined(__x86_64__)
#    define FAST_HEX_X86_64 1
#endif

#if defined(__ARM_NEON)
#    define FAST_HEX_NEON 1
#endif

// This implementation is by https://github.com/zbjornson/fast-hex
// Only introduced some minor modernisations and style changes to those functions
// decodeHex (decodeHexBMI upstream)
// decodeHexLUT, decodeHexLUT4
// encodeHexImpl

namespace heks
{

// Length of raw data in bytes (e.g "Test123" is 7 bytes)
enum class RawLength : size_t;

namespace heks_detail
{
enum class HexCase
{
    Lower,
    Upper
};
} // namespace heks_detail

// Case tags for the encoding functions
struct lower_t
{
    static constexpr heks_detail::HexCase value = heks_detail::HexCase::Lower;
};
struct upper_t
{
    static constexpr heks_detail::HexCase value = heks_detail::HexCase::Upper;
};
inline constexpr lower_t lower{};
inline constexpr upper_t upper{};

namespace heks_detail
{
using namespace std::literals::string_view_literals;

// clang-format off
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
    "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"sv;
// clang-format on

template <HexCase H>
constexpr const std::string_view & hexPairs()
{
    return H == HexCase::Lower ? hex_to_char_lower_sv : hex_to_char_upper_sv;
}

// ASCII -> hex value << Shift, 0xFF for chars that are not hex
template <unsigned Shift>
constexpr std::array<uint8_t, 256> makeUnhexTable()
{
    std::array<uint8_t, 256> table{};
    for (unsigned c = 0; c < 256; ++c)
    {
        unsigned value = 0xFF;
        if (c >= '0' && c <= '9')
            value = (c - '0') << Shift;
        else if (c >= 'A' && c <= 'F')
            value = (c - 'A' + 10) << Shift;
        else if (c >= 'a' && c <= 'f')
            value = (c - 'a' + 10) << Shift;
        table[c] = static_cast<uint8_t>(value);
    }
    return table;
}
inline constexpr std::array<uint8_t, 256> unhex_table = makeUnhexTable<0>();
inline constexpr std::array<uint8_t, 256> unhex_table4 = makeUnhexTable<4>(); // The high nibble, already shifted

// '0'-'9' -> 0-9, 'A'-'F' and 'a'-'f' -> 10-15
constexpr uint8_t unhexBitManip(uint8_t x)
{
    return static_cast<uint8_t>(9 * (x >> 6) + (x & 0xf));
}

// len is number of src bytes
template <HexCase H>
inline void encodeHexImpl(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len)
{
    const auto & hex_table = hexPairs<H>();
    for (size_t i = 0; i < len; i++)
    {
        std::memcpy(dest + i * 2, &hex_table[static_cast<size_t>(src[i]) * 2], 2);
    }
}

template <class T>
constexpr void checkIntType()
{
    static_assert(std::is_trivially_copyable_v<T>, "T must be an integer type, stored in native byte order");
}

template <class T>
constexpr void checkSimdIntType()
{
    checkIntType<T>();
    static_assert(sizeof(T) == 8 || sizeof(T) == 16 || sizeof(T) == 32, "T must be 8, 16 or 32 bytes");
}

} // namespace heks_detail

template <class Case>
void encodeHex(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len, Case)
{
    heks_detail::encodeHexImpl<Case::value>(dest, src, static_cast<size_t>(len));
}

inline void decodeHex(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    // Looping until dest reaches its end is what Clang (-O2 and up) and GCC (-O3 only) vectorize. GCC does
    // not vectorize an i < length count when the length is only known at runtime, and Clang loads src a
    // byte at a time when the loop runs until src reaches its end
    uint8_t * const dest_end = dest + static_cast<size_t>(len);
    while (dest != dest_end)
    {
        const uint8_t a = heks_detail::unhexBitManip(*src++);
        const uint8_t b = heks_detail::unhexBitManip(*src++);
        *dest++ = static_cast<uint8_t>((a << 4) | b);
    }
}

// Table lookups, which no compiler vectorizes: faster than decodeHex below about 8 bytes, and level with it up to
// about 16 when the length is not a multiple of 8, as decodeHex then decodes the last bytes one at a time
inline void decodeHexLUT(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    const auto raw_length = static_cast<size_t>(len);
    for (size_t i = 0; i < raw_length; i++)
    {
        const uint8_t a = heks_detail::unhex_table[*src++];
        const uint8_t b = heks_detail::unhex_table[*src++];
        dest[i] = static_cast<uint8_t>((a << 4) | b);
    }
}

// decodeHexLUT with a second table for the high nibble, so there is no shift
inline void decodeHexLUT4(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len)
{
    const auto raw_length = static_cast<size_t>(len);
    for (size_t i = 0; i < raw_length; i++)
    {
        const uint8_t a = heks_detail::unhex_table4[*src++];
        const uint8_t b = heks_detail::unhex_table[*src++];
        dest[i] = a | b;
    }
}

// Writes value as sizeof(T) * 2 hex chars, most significant first. T is any integer type stored in native
// byte order, including 128 and 256-bit types
template <class T, class Case>
void encodeHexInt(uint8_t * dest, T value, Case)
{
    heks_detail::checkIntType<T>();
    const auto & hex_table = heks_detail::hexPairs<Case::value>();
    const auto * bytes = reinterpret_cast<const uint8_t *>(&value);
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        const size_t byte = std::endian::native == std::endian::little ? bytes[sizeof(T) - 1 - i] : bytes[i];
        std::memcpy(dest + i * 2, &hex_table[byte * 2], 2);
    }
}

// The inverse of encodeHexInt: reads sizeof(T) * 2 hex chars, most significant first, in either case
template <class T>
T decodeHexInt(const uint8_t * src)
{
    heks_detail::checkIntType<T>();
    std::array<uint8_t, sizeof(T)> bytes;
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        const uint8_t a = heks_detail::unhexBitManip(src[i * 2]);
        const uint8_t b = heks_detail::unhexBitManip(src[i * 2 + 1]);
        bytes[std::endian::native == std::endian::little ? sizeof(T) - 1 - i : i] = static_cast<uint8_t>((a << 4) | b);
    }
    return std::bit_cast<T>(bytes);
}

// Writes first[0] then first[1], each as 16 hex chars, most significant first
template <class T, class Case>
void encodeHexInt2x8(uint8_t * dest, const T * first, Case)
{
    heks_detail::checkIntType<T>();
    static_assert(sizeof(T) == 8, "T must be 8 bytes");
    encodeHexInt(dest, first[0], Case{});
    encodeHexInt(dest + 16, first[1], Case{});
}

} // namespace heks
