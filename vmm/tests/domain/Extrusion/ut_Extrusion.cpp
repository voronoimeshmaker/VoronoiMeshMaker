// ============================================================================
// File: ut_Extrusion.cpp
// Description: Extrusion: prism of a 2D outline, side tags from the 2D edges, caps, invalid input.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes.hpp>
#include <vmm/domain/shapes3d.hpp>

namespace {

using vmm::Extrusion;
using vmm::ShapeOutline;
using vmm::Vec2;

TEST(Extrusion, LShapedPrism) {
    const auto outline = ShapeOutline::make({{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}}, {"a", "b", "c", "d", "e", "f"});
    ASSERT_TRUE(outline);
    const auto s = Extrusion(*outline, -1, 2, "floor", "roof").surface({});
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_DOUBLE_EQ(s->volume(), 3.0 * 3.0);
    EXPECT_EQ(s->patches(), (std::vector<std::string>{"floor", "roof", "a", "b", "c", "d", "e", "f"}));
    EXPECT_TRUE(s->contains(vmm::Vec3{0.5, 1.5, 0}));
    EXPECT_FALSE(s->contains(vmm::Vec3{1.5, 1.5, 0}));
}

TEST(Extrusion, CollinearVerticesAreKept) {
    const auto outline = ShapeOutline::make({{0, 0}, {1, 0}, {2, 0}, {2, 1}, {0, 1}}, {});
    ASSERT_TRUE(outline);
    const auto s = Extrusion(*outline, 0, 1).surface({});
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_DOUBLE_EQ(s->volume(), 2.0);
}

TEST(Extrusion, RejectsHolesAndBadHeights) {
    const auto holed = ShapeOutline::make({{0, 0}, {4, 0}, {4, 4}, {0, 4}}, {}, {{{1, 1}, {2, 1}, {2, 2}, {1, 2}}});
    ASSERT_TRUE(holed);
    EXPECT_EQ(Extrusion(*holed, 0, 1).surface({}).error().code(), vmm::ErrorCode::InvalidShapeParameter);
    const auto square = ShapeOutline::make({{0, 0}, {1, 0}, {1, 1}, {0, 1}}, {});
    EXPECT_EQ(Extrusion(*square, 1, 1).surface({}).error().code(), vmm::ErrorCode::DegenerateShape);
    EXPECT_EQ(Extrusion(ShapeOutline{}, 0, 1).surface({}).error().code(), vmm::ErrorCode::InvalidPolygon);
}

}  // namespace
