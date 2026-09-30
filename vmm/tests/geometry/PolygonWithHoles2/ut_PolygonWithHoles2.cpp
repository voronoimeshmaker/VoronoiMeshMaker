// ============================================================================
// File: ut_PolygonWithHoles2.cpp
// Description: PolygonWithHoles2.
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

using vmm::PolygonWithHoles2;
using vmm::Vec2;

PolygonWithHoles2 square_with_hole() {
    // Outer given clockwise and hole counter-clockwise: the constructor fixes both.
    return PolygonWithHoles2({{0, 0}, {0, 4}, {4, 4}, {4, 0}}, {{{1, 1}, {3, 1}, {3, 3}, {1, 3}}});
}

TEST(PolygonWithHoles2, OrientationIsNormalised) {
    const auto p = square_with_hole();
    EXPECT_GT(vmm::signed_area(p.outer()), 0.0);
    EXPECT_LT(vmm::signed_area(p.holes()[0]), 0.0);
}

TEST(PolygonWithHoles2, AreaPerimeterBoxCount) {
    const auto p = square_with_hole();
    EXPECT_DOUBLE_EQ(p.area(), 12.0);
    EXPECT_DOUBLE_EQ(p.perimeter(), 24.0);
    EXPECT_EQ(p.bounding_box().hi(), (Vec2{4, 4}));
    EXPECT_EQ(p.vertex_count(), 8u);
}

TEST(PolygonWithHoles2, ContainsAndDistance) {
    const auto p = square_with_hole();
    EXPECT_TRUE(p.contains(Vec2{0.5, 0.5}));
    EXPECT_FALSE(p.contains(Vec2{2, 2}));
    EXPECT_FALSE(p.contains(Vec2{5, 2}));
    EXPECT_DOUBLE_EQ(p.distance_to_boundary(Vec2{0.5, 2}), 0.5);
    EXPECT_DOUBLE_EQ(p.distance_to_boundary(Vec2{2, 2}), 1.0);
}

TEST(PolygonWithHoles2, DefaultIsEmpty) {
    const PolygonWithHoles2 p;
    EXPECT_EQ(p.area(), 0.0);
    EXPECT_EQ(p.vertex_count(), 0u);
    EXPECT_FALSE(p.contains(Vec2{0, 0}));
}

}  // namespace
