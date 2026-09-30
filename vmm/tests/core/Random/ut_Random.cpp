// ============================================================================
// File: ut_Random.cpp
// Description: Portable Random: reference values, ranges, shuffle.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstdint>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/random.hpp>

namespace {

TEST(Random, FirstValueMatchesTheStandardReference) {
    // The standard fixes the 10000th output of a default-seeded mt19937_64.
    vmm::Random r(5489u);
    std::uint64_t x = 0;
    for (int k = 0; k < 10000; ++k) x = r.next();
    EXPECT_EQ(x, 9981545732273789042ull);
}

TEST(Random, UniformIsInHalfOpenUnitInterval) {
    vmm::Random r(1);
    for (int k = 0; k < 10000; ++k) {
        const double u = r.uniform();
        ASSERT_GE(u, 0.0);
        ASSERT_LT(u, 1.0);
    }
    const double v = r.uniform(-2.0, 3.0);
    EXPECT_GE(v, -2.0);
    EXPECT_LT(v, 3.0);
}

TEST(Random, SameSeedSameSequence) {
    vmm::Random a(42);
    vmm::Random b(42);
    for (int k = 0; k < 100; ++k) EXPECT_EQ(a.uniform(), b.uniform());
}

TEST(Random, BelowIsInRangeAndCoversIt) {
    vmm::Random r(7);
    std::vector<int> hits(5, 0);
    for (int k = 0; k < 5000; ++k) {
        const auto x = r.below(5);
        ASSERT_LT(x, 5u);
        ++hits[x];
    }
    for (int h : hits) EXPECT_GT(h, 800);
    EXPECT_EQ(r.below(1), 0u);
    EXPECT_LT(r.below(~std::uint64_t{0}), ~std::uint64_t{0});
}

TEST(Random, ShuffleIsAPermutationAndDeterministic) {
    std::vector<int> a{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> b = a;
    vmm::Random ra(3);
    vmm::Random rb(3);
    ra.shuffle(std::span<int>(a));
    rb.shuffle(std::span<int>(b));
    EXPECT_EQ(a, b);
    std::vector<int> sorted = a;
    std::ranges::sort(sorted);
    EXPECT_EQ(sorted, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

}  // namespace
