// ============================================================================
// File: ut_Box3.cpp
// Description: Box3: empty box, expansion, inflation, containment, overlap.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/geometry/surface.hpp>

namespace {

using vmm::Box3;
using vmm::Vec3;

TEST(Box3, DefaultIsEmpty) {
    const Box3 b;
    EXPECT_TRUE(b.empty());
    EXPECT_EQ(b.diagonal(), 0.0);
    EXPECT_FALSE(b.contains(Vec3{0, 0, 0}));
    EXPECT_FALSE(b.overlaps(Box3(Vec3{0, 0, 0}, Vec3{1, 1, 1})));
    EXPECT_FALSE(Box3(Vec3{0, 0, 0}, Vec3{1, 1, 1}).overlaps(b));
    EXPECT_TRUE(b.inflated(1.0).empty());
}

TEST(Box3, OfPointsAndExpand) {
    const std::vector<Vec3> pts{{1, 2, 3}, {-1, 5, 0}, {3, 0, 1}};
    Box3 b = Box3::of(pts);
    EXPECT_EQ(b.lo(), (Vec3{-1, 0, 0}));
    EXPECT_EQ(b.hi(), (Vec3{3, 5, 3}));
    EXPECT_DOUBLE_EQ(b.diagonal(), std::sqrt(16.0 + 25.0 + 9.0));
    b.expand(Box3(Vec3{-2, -2, -2}, Vec3{0, 0, 0}));
    EXPECT_EQ(b.lo(), (Vec3{-2, -2, -2}));
    b.expand(Box3{});  // empty: no change
    EXPECT_EQ(b.lo(), (Vec3{-2, -2, -2}));
}

TEST(Box3, InflatedContainsOverlaps) {
    const Box3 b(Vec3{0, 0, 0}, Vec3{1, 1, 1});
    EXPECT_TRUE(b.contains(Vec3{1, 0.5, 0}));
    EXPECT_FALSE(b.contains(Vec3{1.5, 0.5, 0.5}));
    EXPECT_FALSE(b.contains(Vec3{0.5, 0.5, -0.1}));
    const Box3 g = b.inflated(0.5);
    EXPECT_TRUE(g.contains(Vec3{1.5, 0.5, 0.5}));
    EXPECT_TRUE(b.inflated(-0.6).empty());
    EXPECT_TRUE(Box3(Vec3{0, 1, 0}, Vec3{1, 0, 1}).empty());
    EXPECT_TRUE(Box3(Vec3{0, 0, 1}, Vec3{1, 1, 0}).empty());
    EXPECT_TRUE(b.overlaps(Box3(Vec3{1, 1, 1}, Vec3{2, 2, 2})));
    EXPECT_FALSE(b.overlaps(Box3(Vec3{0, 0, 1.1}, Vec3{2, 2, 2})));
    EXPECT_FALSE(b.overlaps(Box3(Vec3{-2, 0, 0}, Vec3{-1, 1, 1})));
}

}  // namespace
