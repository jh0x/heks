#pragma once

#include <cstddef>
#include <cstdint>

#include "fast_hex/fast_hex_export.hpp"

#if defined(_MSC_VER)
#    define FAST_HEX_RESTRICT __restrict // The C99 keyword, available as a C++ extension
#else
#    define FAST_HEX_RESTRICT __restrict__
#endif

// Decoders
// Decode src hex string into dest bytes

// Scalar look-up table version. len is number of dest bytes (1/2 the size of src).
FAST_HEX_EXPORT void decodeHexLUT(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len);

// Optimized scalar look-up table version (avoids a shift). len is number of dest bytes (1/2 the size of src).
FAST_HEX_EXPORT void decodeHexLUT4(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len);

#if defined(__AVX2__)
// Optimal AVX2 vectorized version. len is number of dest bytes (1/2 the size of src).
FAST_HEX_EXPORT void decodeHexVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len);
#endif // defined(__AVX2__)

// Encoders
// Encode src bytes into dest hex string

// Scalar version. len is number of src bytes. dest must be twice the size of src.
FAST_HEX_EXPORT void encodeHexLower(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len);
FAST_HEX_EXPORT void encodeHexUpper(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len);

#if defined(__AVX2__)
// AVX2 vectorized version. len is number of src bytes. dest must be twice the size of src.
FAST_HEX_EXPORT void encodeHexLowerVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len);
FAST_HEX_EXPORT void encodeHexUpperVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, size_t len);

// Fast specialized paths for fixed-size encoding
// Encode exactly 8 bytes (source) into 16 hex characters (dest)
FAST_HEX_EXPORT void encodeHex8LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
FAST_HEX_EXPORT void encodeHex8UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);

// Encode exactly 16 bytes (source) into 32 hex characters (dest)
FAST_HEX_EXPORT void encodeHex16LowerFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
FAST_HEX_EXPORT void encodeHex16UpperFast(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src);
#endif // defined(__AVX2__)
