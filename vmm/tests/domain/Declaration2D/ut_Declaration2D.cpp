// ============================================================================
// File: ut_Declaration2D.cpp
// Description: Declaration by precedence: layers, regions, background, errors.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/declaration.hpp>

namespace {

using vmm::ErrorCode;
using vmm::Rectangle;
using vmm::Vec2;

TEST(Declaration2D, LayersKeepOrderAndRegions) {
    vmm::Declaration2D d({32});
    EXPECT_EQ(d.polygonize_options().segments_per_curve, 32);
    const auto m = *d.media().add("solid");
    const auto a = d.add_region("a", m, Rectangle(Vec2{0, 0}, Vec2{1, 1}));
    ASSERT_TRUE(a);
    ASSERT_TRUE(d.add_hole(Rectangle(Vec2{0.2, 0.2}, Vec2{0.4, 0.4})));
    const auto b = d.add_region("b", m, vmm::Circle(Vec2{0.5, 0.5}, 0.1));
    ASSERT_TRUE(b);
    ASSERT_EQ(d.layers().size(), 3u);
    EXPECT_EQ(d.layers()[0].region, *a);
    EXPECT_FALSE(d.layers()[1].region.valid());
    EXPECT_EQ(d.regions()[b->index()].name, "b");
    EXPECT_FALSE(d.background().valid());
    const auto bg = d.set_background("bg", m);
    ASSERT_TRUE(bg);
    EXPECT_EQ(d.background(), *bg);
    EXPECT_EQ(d.set_background("bg2", m).error().code(), ErrorCode::DuplicateName);
    EXPECT_EQ(d.layers().size(), 3u);
}

TEST(Declaration2D, Errors) {
    vmm::Declaration2D d;
    const auto m = *d.media().add("solid");
    EXPECT_EQ(d.add_region("", m, Rectangle(Vec2{0, 0}, Vec2{1, 1})).error().code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(d.add_region("x", vmm::MediumId::from_index(7), Rectangle(Vec2{0, 0}, Vec2{1, 1})).error().code(),
              ErrorCode::UnknownMedium);
    EXPECT_EQ(d.add_region("x", m, Rectangle(Vec2{0, 0}, Vec2{0, 1})).error().code(), ErrorCode::DegenerateShape);
    EXPECT_TRUE(d.add_region("x", m, Rectangle(Vec2{0, 0}, Vec2{1, 1})));
    EXPECT_EQ(d.add_region("x", m, Rectangle(Vec2{0, 0}, Vec2{1, 1})).error().code(), ErrorCode::DuplicateName);
    EXPECT_EQ(d.add_hole(Rectangle(Vec2{0, 0}, Vec2{0, 1})).error().code(), ErrorCode::DegenerateShape);
}

}  // namespace
