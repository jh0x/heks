#ifdef FAST_HEX_STATIC_SHARED_LIBRARY
#    include <fast_hex/fast_hex.hpp>
#else
#    include "fast_hex/fast_hex_inline.hpp"
#endif
#include "../cpu_features.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include <doctest/doctest.h>

#if defined(__unix__) || defined(__APPLE__)
#    include <unistd.h>
#    include <sys/mman.h>
#endif

#if FAST_HEX_USE_NAMESPACE
using namespace heks;
#endif

namespace
{
constexpr size_t MaxLength = 300;
constexpr size_t MaxOffset = 64; // Covers every misalignment of the widest (AVX-512) vector
constexpr uint8_t Canary = 0xA5;

[[maybe_unused]] bool allCanaries(const uint8_t * first, const uint8_t * last)
{
    return std::all_of(first, last, [](uint8_t x) { return x == Canary; });
}

struct TestData
{
    std::array<uint8_t, MaxLength> raw;
    std::array<uint8_t, MaxLength * 2> lower;
    std::array<uint8_t, MaxLength * 2> upper;
    std::array<uint8_t, MaxLength * 2> mixed;

    TestData()
    {
        constexpr uint8_t Lower[] = "0123456789abcdef";
        constexpr uint8_t Upper[] = "0123456789ABCDEF";
        // cppcheck takes the hex arrays to be MaxLength long, not MaxLength * 2
        // cppcheck-suppress-begin containerOutOfBounds
        for (size_t i = 0; i < raw.size(); ++i)
        {
            raw[i] = static_cast<uint8_t>(i * 131 + 7);
            lower[i * 2] = Lower[raw[i] >> 4];
            lower[i * 2 + 1] = Lower[raw[i] & 15];
            upper[i * 2] = Upper[raw[i] >> 4];
            upper[i * 2 + 1] = Upper[raw[i] & 15];
            mixed[i * 2] = Lower[raw[i] >> 4];
            mixed[i * 2 + 1] = Upper[raw[i] & 15];
        }
        // cppcheck-suppress-end containerOutOfBounds
    }
};

template <auto EncodeLower, auto EncodeUpper, auto Decode>
void testUnalignedBuffers()
{
    const TestData data;
    constexpr size_t Lengths[] = {0, 1, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 257, 300};
    for (size_t len : Lengths)
    {
        CAPTURE(len);
        for (size_t offset = 0; offset < MaxOffset; ++offset)
        {
            CAPTURE(offset);
            // Room for the offset in front and for canaries behind the data
            std::vector<uint8_t> input(len * 2 + MaxOffset * 2, Canary);
            std::vector<uint8_t> output(len * 2 + MaxOffset * 2, Canary);
            auto * src = input.data() + offset;
            // 7 is coprime with MaxOffset, so dest_offset also takes every value in [0, MaxOffset)
            const size_t dest_offset = (offset * 7) % MaxOffset;
            auto * dest = output.data() + dest_offset;
            auto check = [&](const uint8_t * expected, size_t output_length)
            {
                REQUIRE(std::equal(expected, expected + output_length, dest));
                REQUIRE(allCanaries(output.data(), dest));
                REQUIRE(allCanaries(dest + output_length, output.data() + output.size()));
                std::fill(std::begin(output), std::end(output), Canary);
            };
            std::memcpy(src, data.raw.data(), len);
            EncodeLower(dest, src, RawLength{len});
            check(data.lower.data(), len * 2);
            EncodeUpper(dest, src, RawLength{len});
            check(data.upper.data(), len * 2);
            std::memcpy(src, data.mixed.data(), len * 2);
            Decode(dest, src, RawLength{len});
            check(data.raw.data(), len);
        }
    }
}

#if defined(__unix__) || defined(__APPLE__)
// One read/write page between two PROT_NONE guard pages
class GuardedPage
{
public:
    GuardedPage()
    {
        const long page_size = ::sysconf(_SC_PAGESIZE);
        REQUIRE(page_size >= static_cast<long>(MaxLength * 2));
        m_size = static_cast<size_t>(page_size);
        m_mapping = ::mmap(nullptr, m_size * 3, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        REQUIRE(m_mapping != MAP_FAILED);
        m_data = static_cast<uint8_t *>(m_mapping) + m_size;
        const int result = ::mprotect(m_data, m_size, PROT_READ | PROT_WRITE);
        if (result != 0)
            ::munmap(m_mapping, m_size * 3);
        REQUIRE(result == 0);
    }
    ~GuardedPage() { ::munmap(m_mapping, m_size * 3); }
    GuardedPage(const GuardedPage &) = delete;
    GuardedPage & operator=(const GuardedPage &) = delete;

    uint8_t * buffer(size_t length, bool at_end) { return at_end ? m_data + m_size - length : m_data; }
    void reset() { std::memset(m_data, Canary, m_size); }
    bool canariesIntact(const uint8_t * dest, size_t length) const
    {
        return allCanaries(m_data, dest) && allCanaries(dest + length, m_data + m_size);
    }

private:
    void * m_mapping{nullptr};
    uint8_t * m_data{nullptr};
    size_t m_size{};
};

template <auto EncodeLower, auto EncodeUpper, auto Decode>
void testGuardedBuffers()
{
    const TestData data;
    GuardedPage input;
    GuardedPage output;
    for (size_t len = 0; len <= MaxLength; ++len)
    {
        CAPTURE(len);
        for (bool src_at_end : {false, true})
        {
            for (bool dest_at_end : {false, true})
            {
                CAPTURE(src_at_end);
                CAPTURE(dest_at_end);
                auto * src = input.buffer(len, src_at_end);
                auto * dest = output.buffer(len * 2, dest_at_end);
                if (len != 0)
                    std::memcpy(src, data.raw.data(), len);
                output.reset();
                EncodeLower(dest, src, RawLength{len});
                REQUIRE(std::equal(data.lower.begin(), data.lower.begin() + len * 2, dest));
                REQUIRE(output.canariesIntact(dest, len * 2));
                output.reset();
                EncodeUpper(dest, src, RawLength{len});
                REQUIRE(std::equal(data.upper.begin(), data.upper.begin() + len * 2, dest));
                REQUIRE(output.canariesIntact(dest, len * 2));

                src = input.buffer(len * 2, src_at_end);
                dest = output.buffer(len, dest_at_end);
                if (len != 0)
                    std::memcpy(src, data.mixed.data(), len * 2);
                output.reset();
                Decode(dest, src, RawLength{len});
                REQUIRE(std::equal(data.raw.begin(), data.raw.begin() + len, dest));
                REQUIRE(output.canariesIntact(dest, len));
            }
        }
    }
}
#endif
} // namespace

#if defined(FAST_HEX_AVX2)
TEST_CASE("hexVec_unaligned_buffers" * doctest::skip(!cpuHasAvx2()))
{
    testUnalignedBuffers<encodeHexLowerVec, encodeHexUpperVec, decodeHexVec>();
}
#    if defined(__unix__) || defined(__APPLE__)
TEST_CASE("hexVec_guarded_buffers" * doctest::skip(!cpuHasAvx2()))
{
    testGuardedBuffers<encodeHexLowerVec, encodeHexUpperVec, decodeHexVec>();
}
#    endif
#endif

#if defined(FAST_HEX_AVX512)
TEST_CASE("hexVec512_unaligned_buffers" * doctest::skip(!cpuHasAvx512()))
{
    testUnalignedBuffers<encodeHexLowerVec512, encodeHexUpperVec512, decodeHexVec512>();
}
#    if defined(__unix__) || defined(__APPLE__)
TEST_CASE("hexVec512_guarded_buffers" * doctest::skip(!cpuHasAvx512()))
{
    testGuardedBuffers<encodeHexLowerVec512, encodeHexUpperVec512, decodeHexVec512>();
}
#    endif
#endif

// Whichever path the CPU gets, it has to stay inside the buffers as well
TEST_CASE("hexAuto_unaligned_buffers")
{
    testUnalignedBuffers<encodeHexLowerAuto, encodeHexUpperAuto, decodeHexAuto>();
}
#if defined(__unix__) || defined(__APPLE__)
TEST_CASE("hexAuto_guarded_buffers")
{
    testGuardedBuffers<encodeHexLowerAuto, encodeHexUpperAuto, decodeHexAuto>();
}
#endif
