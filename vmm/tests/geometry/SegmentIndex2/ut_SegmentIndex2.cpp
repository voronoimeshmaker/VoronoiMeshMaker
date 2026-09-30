// ============================================================================
// File: ut_SegmentIndex2.cpp
// Description: SegmentIndex2: conservative box-segment proximity.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/random.hpp>
#include <vmm/geometry/polygon.hpp>

namespace {

using vmm::Box2;
using vmm::Vec2;

TEST(SegmentIndex2, EmptyIndexTouchesNothing) {
    const vmm::SegmentIndex2 index;
    EXPECT_EQ(index.segment_count(), 0u);
    EXPECT_FALSE(index.may_touch(Box2(Vec2{0, 0}, Vec2{1, 1})));
}

TEST(SegmentIndex2, SquareBoundary) {
    const std::vector<Vec2> a{{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    const std::vector<Vec2> b{{1, 0}, {1, 1}, {0, 1}, {0, 0}};
    const vmm::SegmentIndex2 index(a, b, 4);
    EXPECT_EQ(index.segment_count(), 4u);
    EXPECT_FALSE(index.may_touch(Box2(Vec2{0.4, 0.4}, Vec2{0.6, 0.6})));
    EXPECT_TRUE(index.may_touch(Box2(Vec2{0.9, 0.4}, Vec2{1.1, 0.6})));
    EXPECT_FALSE(index.may_touch(Box2(Vec2{2, 2}, Vec2{3, 3})));
}

TEST(SegmentIndex2, AgreesWithBruteForce) {
    vmm::Random rng(11);
    std::vector<Vec2> a;
    std::vector<Vec2> b;
    for (int k = 0; k < 200; ++k) {
        const Vec2 p{rng.uniform(), rng.uniform()};
        a.push_back(p);
        b.push_back(p + Vec2{0.02 * rng.uniform(), 0.02 * rng.uniform()});
    }
    const vmm::SegmentIndex2 index(a, b);
    for (int q = 0; q < 500; ++q) {
        const Vec2 c{rng.uniform(-0.1, 1.1), rng.uniform(-0.1, 1.1)};
        const Box2 box(c, c + Vec2{0.03, 0.03});
        bool brute = false;
        for (std::size_t k = 0; k < a.size(); ++k) {
            Box2 s;
            s.expand(a[k]);
            s.expand(b[k]);
            brute = brute || s.overlaps(box);
        }
        ASSERT_EQ(index.may_touch(box), brute) << q;
    }
}

TEST(SegmentIndex2, DegenerateExtent) {
    const std::vector<Vec2> a{{0, 0}};
    const std::vector<Vec2> b{{0, 0}};
    const vmm::SegmentIndex2 index(a, b);
    EXPECT_TRUE(index.may_touch(Box2(Vec2{-1, -1}, Vec2{1, 1})));
}

}  // namespace
