#include "fast_hex/fast_hex.hpp"

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

    auto decode = [&](const auto & source, auto decode_func)
    {
        std::vector<uint8_t> decoded(decoded_size);
        decode_func(decoded.data(), source.data(), RawLength{decoded_size});
        if (std::memcmp(Data, decoded.data(), decoded_size) != 0)
        {
            std::abort();
        }
    };

    auto test_encode_scalar = [&](auto encode_func)
    {
        std::vector<uint8_t> encoded(encoded_size);
        encode_func(encoded.data(), Data, RawLength{Size});

        decode(encoded, decodeHexLUT);
        decode(encoded, decodeHexLUT4);
        decode(encoded, decodeHexBMI);
#if defined(FAST_HEX_AVX2)
        decode(encoded, decodeHexVec);
#endif
    };

#if defined(FAST_HEX_AVX2) || defined(FAST_HEX_NEON)
    auto test_encode_vec = [&](auto encode_func)
    {
        std::vector<uint8_t> encoded(encoded_size);

        encode_func(encoded.data(), Data, RawLength{Size});
        decode(encoded, decodeHexLUT);
        decode(encoded, decodeHexLUT4);
#    if defined(FAST_HEX_AVX2)
        decode(encoded, decodeHexVec);
#    endif
    };
#endif

    test_encode_scalar(encodeHexLower);
    test_encode_scalar(encodeHexUpper);
#if defined(FAST_HEX_AVX2)
    test_encode_vec(encodeHexLowerVec);
    test_encode_vec(encodeHexUpperVec);
#endif
#if defined(FAST_HEX_NEON)
    test_encode_vec(encodeHexNeonLower);
    test_encode_vec(encodeHexNeonUpper);
#endif

    return 0;
}
