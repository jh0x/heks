#pragma once

// Runs code at each SVE vector length the CPU supports, so the 128-bit code is tested on CPUs with longer vectors.
// Linux only, and not when the code is built for SVE: code inlined here could keep values sized by the old length.
// Otherwise the code runs once, at the current length.
//
// Needs no doctest, so the fuzzers use it too. The tests use forEachSveVectorLength of sve_vector_lengths.hpp.

#include "fast_hex/fast_hex_arm_sve.hpp"

#if defined(FAST_HEX_ARM_SVE)

#    include <algorithm>
#    include <cstddef>
#    include <vector>

#    if defined(__linux__)
#        include <sys/prctl.h>
#    endif

#    if defined(__linux__) && defined(PR_SVE_GET_VL) && !defined(__ARM_FEATURE_SVE)
#        define FAST_HEX_TEST_SVE_SET_VL 1
#    endif

namespace heks_test
{

#    if defined(FAST_HEX_TEST_SVE_SET_VL)

// Sets the vector length to the longest one the CPU supports up to bytes, and returns it: 0 if it can't be set
inline int setSveVectorLength(int bytes)
{
    const int config = prctl(PR_SVE_SET_VL, static_cast<unsigned long>(bytes));
    return config >= 0 ? config & PR_SVE_VL_LEN_MASK : 0;
}

// Puts back the vector length it was created under, however the scope ends: doctest's REQUIRE throws
struct SveVectorLengthRestore
{
    int config = prctl(PR_SVE_GET_VL);
    SveVectorLengthRestore() = default;
    SveVectorLengthRestore(const SveVectorLengthRestore &) = delete;
    SveVectorLengthRestore & operator=(const SveVectorLengthRestore &) = delete;
    ~SveVectorLengthRestore()
    {
        if (config >= 0)
            static_cast<void>(prctl(PR_SVE_SET_VL, static_cast<unsigned long>(config)));
    }
};

#    endif

// The vector lengths in bytes that the tests can set, shortest first. Empty if they can set none: not Linux, no
// SVE, the tests are built for SVE, or a seccomp filter say. Leaves the current length as it was
inline std::vector<int> sveVectorLengths()
{
    std::vector<int> lengths;
#    if defined(FAST_HEX_TEST_SVE_SET_VL)
    const SveVectorLengthRestore restore;
    for (int bytes = 16; bytes <= 256; bytes += 16)
    {
        const int vector_bytes = setSveVectorLength(bytes);
        if (vector_bytes != 0 && std::find(lengths.begin(), lengths.end(), vector_bytes) == lengths.end())
            lengths.push_back(vector_bytes);
    }
#    endif
    return lengths;
}

// sveVectorLengths, probed once: the lengths the CPU supports don't change while the process runs
inline const std::vector<int> & sveVectorLengthsOnce()
{
    static const std::vector<int> lengths = sveVectorLengths();
    return lengths;
}

// Calls run(vector_bytes) at each of the lengths from sveVectorLengths, then puts the length back. A length that
// can't be set any more is left out. Returns how many it set: if none, it calls run(0) once at the current length
template <class F>
std::size_t atEachSveVectorLength(F run)
{
    std::size_t lengths_set = 0;
#    if defined(FAST_HEX_TEST_SVE_SET_VL)
    if (!sveVectorLengthsOnce().empty())
    {
        const SveVectorLengthRestore restore;
        for (const int vector_bytes : sveVectorLengthsOnce())
        {
            if (setSveVectorLength(vector_bytes) != vector_bytes)
                continue;
            run(vector_bytes);
            ++lengths_set;
        }
    }
#    endif
    if (lengths_set == 0)
        run(0);
    return lengths_set;
}

} // namespace heks_test

#endif // defined(FAST_HEX_ARM_SVE)
