// ============================================================================
// File: ut_Circle.cpp
// Description: Circle polygonization.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes.hpp>
#include <vmm/geometry/polygon.hpp>

namespace {

TEST(Circle, VerticesOnTheCircle) {
    const auto o = vmm::Circle(vmm::Vec2{1, 1}, 2, "c").outline({64});
    ASSERT_TRUE(o);
    for (const auto& p : o->outer()) EXPECT_NEAR(vmm::norm(p - vmm::Vec2{1, 1}), 2.0, 1e-14);
    EXPECT_FALSE(vmm::Circle(vmm::Vec2{0, 0}, 0).outline({}));
}

}  // namespace
