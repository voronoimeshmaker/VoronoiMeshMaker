// ============================================================================
// File: ut_PolygonShape.cpp
// Description: PolygonShape outline.
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

namespace {

TEST(PolygonShape, TrapezoidWithTagsAndHole) {
    const auto o = vmm::PolygonShape({{-20, 0}, {-10, -5}, {10, -5}, {20, 0}}, {"", "", "", "top"}).outline({});
    ASSERT_TRUE(o);
    EXPECT_DOUBLE_EQ(o->area(), 150.0);
    const auto h = vmm::PolygonShape({{0, 0}, {3, 0}, {3, 3}, {0, 3}}, {}, {{{1, 1}, {2, 1}, {2, 2}, {1, 2}}}).outline({});
    ASSERT_TRUE(h);
    EXPECT_DOUBLE_EQ(h->area(), 8.0);
}

TEST(PolygonShape, InvalidPolygon) {
    EXPECT_FALSE(vmm::PolygonShape({{0, 0}, {1, 1}}).outline({}));
}

}  // namespace
