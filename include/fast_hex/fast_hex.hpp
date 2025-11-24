#pragma once

#include <cstddef>
#include <cstdint>

#include "fast_hex/fast_hex_export.hpp"

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

FAST_HEX_NAMESPACE_OPEN

// Decoders
// Decode src hex string (e.g "54657374313233" into dest bytes (e.g. "Test123")

// Length of raw data in bytes (e.g "Test123" is 7 bytes)
enum class RawLength : size_t;

// Scalar look-up table version (two chars at a time). Uses a single lookup table, but requires a shift.
// [0xAB, 0xCD] -> [a = 0xA, b = 0xB] -> dest = (a << 4) | b = 0xAB
FAST_HEX_EXPORT void decodeHexLUT(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

// Scalar look-up table version (two chars at a time). Uses two lookup tables to avoid the shift.
// [0xAB, 0xCD] -> [a = 0xA, b = 0xB] -> dest = a | b = 0xAB
FAST_HEX_EXPORT void decodeHexLUT4(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

#if defined(__AVX2__)
// Optimal AVX2 vectorized version. len is number of dest bytes (1/2 the size of src).
FAST_HEX_EXPORT void decodeHexVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
#endif // defined(__AVX2__)

// Encoders
// Encode src bytes (e.g. "Test123") into dest hex string (e.g. "54657374313233")

// Scalar version. len is number of src bytes. dest must be twice the size of src.
// [0xAB, 0xCD] -> [hi = 0xA, lo = 0xB] -> [dest[0] = 'A', dest[1] = 'B']
//                 [hi = 0xC, lo = 0xD] -> [dest[2] = 'C', dest[3] = 'D']
FAST_HEX_EXPORT void encodeHexLower(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
FAST_HEX_EXPORT void encodeHexUpper(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);


#if defined(__AVX__)
// Fast specialized paths for fixed-size encoding
// Encode exactly 8 bytes (source) into 16 hex characters (dest)
// [0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, 0x9A]
//     -> hi = [0xA, 0xC, 0xE, 0x1, 0x3, 0x5, 0x7, 0x9]
//     -> lo = [0xB, 0xD, 0xF, 0x2, 0x4, 0x6, 0x8, 0xA]
//         -> nibs = [hi[0], lo[0], hi[1], lo[1], ...]
//                 = [0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9]
//             -> LUT lookup
//                 -> result = [A, B, C, D, E, F, 1, 2, 3, 4, 5, 6, 7, 8, 9]
FAST_HEX_EXPORT void encodeHex8LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
FAST_HEX_EXPORT void encodeHex8UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
#endif

#if defined(__AVX2__)
// AVX2 vectorized version. len is number of src bytes. dest must be twice the size of src.
FAST_HEX_EXPORT void encodeHexLowerVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
FAST_HEX_EXPORT void encodeHexUpperVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

// Encode exactly 16 bytes (source) into 32 hex characters (dest)
FAST_HEX_EXPORT void encodeHex16LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
FAST_HEX_EXPORT void encodeHex16UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
#endif // defined(__AVX2__)

FAST_HEX_NAMESPACE_CLOSE
