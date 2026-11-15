#pragma once

// Runs a test at each SVE vector length the CPU supports: see sve_set_vector_length.hpp for when it can set them.

#include "sve_set_vector_length.hpp"

#include <cstddef>

namespace heks_test
{

// What the forEachSveVectorLength calls did, for FAST_HEX_TEST_REQUIRE. All zero where there is no SVE
struct SveVectorLengthRuns
{
    std::size_t tests = 0; // Calls that ran their test to the end
    std::size_t fewest_lengths = 0; // The fewest vector lengths that one of them set
    bool length_not_set = false; // One of them couldn't set a length from sveVectorLengths
};

inline SveVectorLengthRuns & sveVectorLengthRuns()
{
    static SveVectorLengthRuns runs;
    return runs;
}

} // namespace heks_test

#if defined(FAST_HEX_ARM_SVE)

#    include <algorithm>

#    include <doctest/doctest.h>

namespace heks_test
{

// atEachSveVectorLength for a test: names the length in a failure, and records the call
template <class F>
void forEachSveVectorLength(F test)
{
    const std::size_t lengths = sveVectorLengthsOnce().size();
    const std::size_t lengths_set = atEachSveVectorLength(
        [&](int vector_bytes)
        {
            CAPTURE(vector_bytes); // 0 for the current one
            test();
        });
#    if defined(FAST_HEX_TEST_SVE_SET_VL) // Otherwise the tests don't set it, by design
    if (lengths_set == 0)
        MESSAGE("Couldn't set the SVE vector length: tested at the current one only");
    else if (lengths_set != lengths)
        MESSAGE("Couldn't set " << lengths - lengths_set << " of the SVE vector lengths any more");
#    endif
    // After the tests, so that it only counts lengths whose tests ran to the end
    SveVectorLengthRuns & runs = sveVectorLengthRuns();
    runs.fewest_lengths = runs.tests == 0 ? lengths_set : std::min(runs.fewest_lengths, lengths_set);
    runs.length_not_set = runs.length_not_set || lengths_set != lengths;
    ++runs.tests;
}

} // namespace heks_test

#endif // defined(FAST_HEX_ARM_SVE)
