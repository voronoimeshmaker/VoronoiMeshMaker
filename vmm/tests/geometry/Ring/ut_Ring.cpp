// ============================================================================
// File: ut_Ring.cpp
// Description: Ring functions: signed_area, ring_centroid, perimeter,
//              ring_contains, distance_to_segment.
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

using vmm::Vec2;

const std::vector<Vec2> kSquare{{0, 0}, {2, 0}, {2, 2}, {0, 2}};

TEST(Ring, SignedAreaAndOrientation) {
    EXPECT_DOUBLE_EQ(vmm::signed_area(kSquare), 4.0);
    const std::vector<Vec2> cw(kSquare.rbegin(), kSquare.rend());
    EXPECT_DOUBLE_EQ(vmm::signed_area(cw), -4.0);
    EXPECT_EQ(vmm::signed_area(std::vector<Vec2>{{0, 0}, {1, 1}}), 0.0);
}

TEST(Ring, Centroid) {
    EXPECT_EQ(vmm::ring_centroid(kSquare), (Vec2{1, 1}));
    const std::vector<Vec2> flat{{0, 0}, {2, 0}, {4, 0}};
    EXPECT_EQ(vmm::ring_centroid(flat), (Vec2{2, 0}));
    EXPECT_EQ(vmm::ring_centroid(std::vector<Vec2>{}), (Vec2{0, 0}));
    const std::vector<Vec2> tri{{0, 0}, {3, 0}, {0, 3}};
    EXPECT_DOUBLE_EQ(vmm::ring_centroid(tri)[0], 1.0);
    EXPECT_DOUBLE_EQ(vmm::ring_centroid(tri)[1], 1.0);
}

TEST(Ring, PerimeterAndContains) {
    EXPECT_DOUBLE_EQ(vmm::perimeter(kSquare), 8.0);
    EXPECT_TRUE(vmm::ring_contains(kSquare, Vec2{1, 1}));
    EXPECT_FALSE(vmm::ring_contains(kSquare, Vec2{3, 1}));
    EXPECT_FALSE(vmm::ring_contains(kSquare, Vec2{1, -1}));
}

TEST(Ring, DistanceToSegment) {
    EXPECT_DOUBLE_EQ(vmm::distance_to_segment(Vec2{1, 1}, Vec2{0, 0}, Vec2{2, 0}), 1.0);
    EXPECT_DOUBLE_EQ(vmm::distance_to_segment(Vec2{3, 0}, Vec2{0, 0}, Vec2{2, 0}), 1.0);
    EXPECT_DOUBLE_EQ(vmm::distance_to_segment(Vec2{0, 3}, Vec2{0, 0}, Vec2{0, 0}), 3.0);
}

}  // namespace
