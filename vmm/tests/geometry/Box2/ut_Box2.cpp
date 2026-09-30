// ============================================================================
// File: ut_Box2.cpp
// Description: Box2.
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
#include <vmm/geometry/polygon.hpp>

namespace {

using vmm::Box2;
using vmm::Vec2;

TEST(Box2, DefaultIsEmpty) {
    const Box2 b;
    EXPECT_TRUE(b.empty());
    EXPECT_EQ(b.diagonal(), 0.0);
    EXPECT_FALSE(b.contains(Vec2{0, 0}));
    EXPECT_FALSE(b.overlaps(Box2(Vec2{0, 0}, Vec2{1, 1})));
    EXPECT_TRUE(b.inflated(1.0).empty());
}

TEST(Box2, OfPointsAndExpand) {
    const std::vector<Vec2> pts{{1, 2}, {-1, 5}, {3, 0}};
    Box2 b = Box2::of(pts);
    EXPECT_EQ(b.lo(), (Vec2{-1, 0}));
    EXPECT_EQ(b.hi(), (Vec2{3, 5}));
    EXPECT_DOUBLE_EQ(b.diagonal(), 5.0 * 1.2806248474865698);
    b.expand(Box2(Vec2{10, 10}, Vec2{11, 11}));
    EXPECT_EQ(b.hi(), (Vec2{11, 11}));
    b.expand(Box2{});
    EXPECT_EQ(b.hi(), (Vec2{11, 11}));
}

TEST(Box2, ContainsOverlapsInflated) {
    const Box2 b(Vec2{0, 0}, Vec2{1, 1});
    EXPECT_TRUE(b.contains(Vec2{0.5, 1.0}));
    EXPECT_FALSE(b.contains(Vec2{1.5, 0.5}));
    EXPECT_TRUE(b.overlaps(Box2(Vec2{1, 1}, Vec2{2, 2})));
    EXPECT_FALSE(b.overlaps(Box2(Vec2{1.1, 0}, Vec2{2, 2})));
    const Box2 g = b.inflated(0.5);
    EXPECT_EQ(g.lo(), (Vec2{-0.5, -0.5}));
    EXPECT_EQ(g.hi(), (Vec2{1.5, 1.5}));
}

}  // namespace
