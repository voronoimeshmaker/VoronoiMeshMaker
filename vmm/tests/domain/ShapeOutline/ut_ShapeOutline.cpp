// ============================================================================
// File: ut_ShapeOutline.cpp
// Description: ShapeOutline validation and orientation with tags.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <limits>
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
#include <vmm/geometry/polygon.hpp>

namespace {

using vmm::ErrorCode;
using vmm::ShapeOutline;
using vmm::Vec2;

TEST(ShapeOutline, ClockwiseRingIsReversedWithItsTags) {
    // Clockwise square: edges (0,0)->(0,1) "w", (0,1)->(1,1) "n", (1,1)->(1,0) "e", (1,0)->(0,0) "s".
    const auto s = ShapeOutline::make({{0, 0}, {0, 1}, {1, 1}, {1, 0}}, {"w", "n", "e", "s"});
    ASSERT_TRUE(s);
    EXPECT_GT(vmm::signed_area(s->outer()), 0.0);
    const auto& p = s->outer();
    const auto& t = s->outer_tags();
    for (std::size_t k = 0; k < p.size(); ++k) {
        const Vec2 a = p[k];
        const Vec2 b = p[(k + 1) % p.size()];
        const std::string expected = a[0] == 0 && b[0] == 0 ? "w" : a[1] == 1 && b[1] == 1 ? "n" : a[0] == 1 && b[0] == 1 ? "e" : "s";
        EXPECT_EQ(t[k], expected) << k;
    }
    EXPECT_DOUBLE_EQ(s->area(), 1.0);
}

TEST(ShapeOutline, HolesAreClockwiseAndReduceArea) {
    const auto s = ShapeOutline::make({{0, 0}, {4, 0}, {4, 4}, {0, 4}}, {}, {{{1, 1}, {2, 1}, {2, 2}, {1, 2}}});
    ASSERT_TRUE(s);
    EXPECT_LT(vmm::signed_area(s->holes()[0]), 0.0);
    EXPECT_EQ(s->hole_tags()[0].size(), 4u);
    EXPECT_DOUBLE_EQ(s->area(), 15.0);
}

TEST(ShapeOutline, RejectsInvalidInput) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(ShapeOutline::make({{0, 0}, {1, 0}}, {}).error().code(), ErrorCode::InvalidPolygon);
    EXPECT_EQ(ShapeOutline::make({{0, 0}, {1, 0}, {nan, 1}}, {}).error().code(), ErrorCode::InvalidPolygon);
    EXPECT_EQ(ShapeOutline::make({{0, 0}, {1, 0}, {2, 0}}, {}).error().code(), ErrorCode::DegenerateShape);
    EXPECT_EQ(ShapeOutline::make({{0, 0}, {1, 0}, {0, 1}}, {"a"}).error().code(), ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(ShapeOutline::make({{0, 0}, {3, 0}, {0, 3}}, {}, {{{0, 0}, {1, 0}}}).error().code(), ErrorCode::InvalidPolygon);
    EXPECT_EQ(ShapeOutline::make({{0, 0}, {3, 0}, {0, 3}}, {}, {{{1, 1}, {1, 1}, {1, 1}}}).error().code(), ErrorCode::DegenerateShape);
    EXPECT_EQ(ShapeOutline::make({{0, 0}, {3, 0}, {0, 3}}, {}, {{{1, 1}, {2, 1}, {1, 2}}}, {{"a"}, {"b"}}).error().code(),
              ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(ShapeOutline::make({{0, 0}, {3, 0}, {0, 3}}, {}, {{{1, 1}, {1.5, 1}, {1, 1.5}}}, {{"a"}}).error().code(),
              ErrorCode::InvalidShapeParameter);
}

}  // namespace
