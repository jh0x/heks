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

// Scalar look-up table version. len is number of dest bytes (1/2 the size of src).
FAST_HEX_EXPORT void decodeHexLUT(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

// Optimized scalar look-up table version (avoids a shift). len is number of dest bytes (1/2 the size of src).
FAST_HEX_EXPORT void decodeHexLUT4(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);

#if defined(__AVX2__)
// Optimal AVX2 vectorized version. len is number of dest bytes (1/2 the size of src).
FAST_HEX_EXPORT void decodeHexVec(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
#endif // defined(__AVX2__)

// Encoders
// Encode src bytes (e.g. "Test123") into dest hex string (e.g. "54657374313233")

// Scalar version. len is number of src bytes. dest must be twice the size of src.
FAST_HEX_EXPORT void encodeHexLower(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);
FAST_HEX_EXPORT void encodeHexUpper(uint8_t * FAST_HEX_RESTRICT dest, const uint8_t * FAST_HEX_RESTRICT src, RawLength len);


#if defined(__AVX__)
// Fast specialized paths for fixed-size encoding
// Encode exactly 8 bytes (source) into 16 hex characters (dest)
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
