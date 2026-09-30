// ============================================================================
// File: ut_Rectangle.cpp
// Description: Rectangle outline and side tags.
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

TEST(Rectangle, OutlineAndTags) {
    const auto o = vmm::Rectangle(vmm::Vec2{0, 0}, vmm::Vec2{2, 1}, {"b", "r", "t", "l"}).outline({});
    ASSERT_TRUE(o);
    EXPECT_DOUBLE_EQ(o->area(), 2.0);
    EXPECT_EQ(o->outer()[0], (vmm::Vec2{0, 0}));
    EXPECT_EQ(o->outer_tags()[0], "b");
    EXPECT_EQ(o->outer_tags()[3], "l");
}

TEST(Rectangle, DegenerateIsRejected) {
    EXPECT_EQ(vmm::Rectangle(vmm::Vec2{0, 0}, vmm::Vec2{0, 1}).outline({}).error().code(), vmm::ErrorCode::DegenerateShape);
}

static_assert(vmm::Shape2D<vmm::Rectangle>);

}  // namespace
