#include "fast_hex/fast_hex.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace heks;


extern "C" int LLVMFuzzerTestOneInput(const uint8_t * Data, size_t Size)
{
    const std::size_t encoded_size = 2 * Size;
    const std::size_t decoded_size = Size;

    auto decode = [&](const auto & source, auto decode_func)
    {
        std::vector<uint8_t> decoded(decoded_size);
        decode_func(decoded.data(), source.data(), decoded_size);
        assert(std::memcmp(Data, decoded.data(), decoded_size) == 0);
    };

    auto test_encode_scalar = [&](auto encode_func)
    {
        std::vector<uint8_t> encoded(encoded_size);
        encode_func(encoded.data(), Data, Size);

        decode(encoded, decodeHexLUT);
        decode(encoded, decodeHexLUT4);
#if defined(__AVX2__)
        decode(encoded, decodeHexVec);
#endif
    };

#if defined(__AVX2__)
    auto test_encode_vec = [&](auto encode_func)
    {
        std::vector<uint8_t> encoded(encoded_size);

        encode_func(encoded.data(), Data, Size);
        decode(encoded, decodeHexLUT);
        decode(encoded, decodeHexLUT4);
        decode(encoded, decodeHexVec);
    };
#endif

    test_encode_scalar(encodeHexLower);
    test_encode_scalar(encodeHexUpper);
#if defined(__AVX2__)
    test_encode_vec(encodeHexLowerVec);
    test_encode_vec(encodeHexUpperVec);
#endif

    return 0;
}
