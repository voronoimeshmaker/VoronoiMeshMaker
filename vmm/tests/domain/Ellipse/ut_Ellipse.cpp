// ============================================================================
// File: ut_Ellipse.cpp
// Description: Ellipse polygonization.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <numbers>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes.hpp>

namespace {

TEST(Ellipse, AreaConvergesToPiAB) {
    const auto o = vmm::Ellipse(vmm::Vec2{1, 2}, 3, 1, 0.3, "e").outline({1024});
    ASSERT_TRUE(o);
    EXPECT_EQ(o->outer().size(), 1024u);
    EXPECT_NEAR(o->area(), std::numbers::pi * 3, 1e-3);
    EXPECT_EQ(o->outer_tags()[5], "e");
}

TEST(Ellipse, MinimumSegmentsAndInvalidAxes) {
    const auto o = vmm::Ellipse(vmm::Vec2{0, 0}, 1, 1).outline({2});
    ASSERT_TRUE(o);
    EXPECT_EQ(o->outer().size(), 8u);
    EXPECT_FALSE(vmm::Ellipse(vmm::Vec2{0, 0}, 0, 1).outline({}));
    EXPECT_FALSE(vmm::Ellipse(vmm::Vec2{0, 0}, 1, -1).outline({}));
}

}  // namespace
