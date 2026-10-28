// Inline header so the code under test is compiled here with the sanitizer and coverage instrumentation
#include "fast_hex/fast_hex_inline.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#if FAST_HEX_USE_NAMESPACE
using namespace heks;
#endif


extern "C" int LLVMFuzzerTestOneInput(const uint8_t * Data, size_t Size)
{
    const std::size_t encoded_size = 2 * Size;
    const std::size_t decoded_size = Size;

    // Independent of the library, so encoders are checked byte for byte, including the case
    auto reference_encode = [&](const char * digits)
    {
        std::vector<uint8_t> encoded(encoded_size);
        for (std::size_t i = 0; i < Size; ++i)
        {
            encoded[2 * i] = static_cast<uint8_t>(digits[Data[i] >> 4]);
            encoded[2 * i + 1] = static_cast<uint8_t>(digits[Data[i] & 0x0F]);
        }
        return encoded;
    };
    const std::vector<uint8_t> expected_lower = reference_encode("0123456789abcdef");
    const std::vector<uint8_t> expected_upper = reference_encode("0123456789ABCDEF");

    auto decode = [&](const auto & source, auto decode_func)
    {
        std::vector<uint8_t> decoded(decoded_size);
        decode_func(decoded.data(), source.data(), RawLength{decoded_size});
        if (std::memcmp(Data, decoded.data(), decoded_size) != 0)
        {
            std::abort();
        }
    };

    auto test_encode_scalar = [&](auto encode_func, const std::vector<uint8_t> & expected)
    {
        std::vector<uint8_t> encoded(encoded_size);
        encode_func(encoded.data(), Data, RawLength{Size});
        if (encoded != expected)
        {
            std::abort();
        }

        decode(encoded, decodeHexLUT);
        decode(encoded, decodeHexLUT4);
        decode(encoded, decodeHexBMI);
#if defined(FAST_HEX_AVX2)
        decode(encoded, decodeHexVec);
#endif
#if defined(FAST_HEX_AVX512)
        decode(encoded, decodeHexVec512);
#endif
    };

#if defined(FAST_HEX_AVX2) || defined(FAST_HEX_AVX512) || defined(FAST_HEX_NEON)
    auto test_encode_vec = [&](auto encode_func, const std::vector<uint8_t> & expected)
    {
        std::vector<uint8_t> encoded(encoded_size);
        encode_func(encoded.data(), Data, RawLength{Size});
        if (encoded != expected)
        {
            std::abort();
        }

        decode(encoded, decodeHexLUT);
        decode(encoded, decodeHexLUT4);
#    if defined(FAST_HEX_AVX2)
        decode(encoded, decodeHexVec);
#    endif
#    if defined(FAST_HEX_AVX512)
        decode(encoded, decodeHexVec512);
#    endif
    };
#endif

    test_encode_scalar(encodeHexLower, expected_lower);
    test_encode_scalar(encodeHexUpper, expected_upper);
#if defined(FAST_HEX_AVX2)
    test_encode_vec(encodeHexLowerVec, expected_lower);
    test_encode_vec(encodeHexUpperVec, expected_upper);
#endif
#if defined(FAST_HEX_AVX512)
    test_encode_vec(encodeHexLowerVec512, expected_lower);
    test_encode_vec(encodeHexUpperVec512, expected_upper);
#endif
#if defined(FAST_HEX_NEON)
    test_encode_vec(encodeHexNeonLower, expected_lower);
    test_encode_vec(encodeHexNeonUpper, expected_upper);
#endif

    return 0;
}
