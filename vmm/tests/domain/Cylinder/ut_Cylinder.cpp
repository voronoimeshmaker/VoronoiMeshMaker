// ============================================================================
// File: ut_Cylinder.cpp
// Description: Cylinder: volume of the polygonized cylinder, tags, invalid parameters.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes3d.hpp>

namespace {

using vmm::Cylinder;

TEST(Cylinder, VolumeAndPatches) {
    const int n = 128;
    const auto s = Cylinder({1, 1, -1}, 2, 3, {"wall", "floor", "lid"}).surface({n, 3});
    ASSERT_TRUE(s) << s.error().message();
    const double polygon_area = 0.5 * n * 4 * std::sin(2 * std::numbers::pi / n);
    EXPECT_NEAR(s->volume(), polygon_area * 3, 1e-12);
    EXPECT_EQ(s->patches(), (std::vector<std::string>{"floor", "lid", "wall"}));
    EXPECT_NEAR(s->bounding_box().lo()[2], -1.0, 1e-15);
    EXPECT_NEAR(s->bounding_box().hi()[2], 2.0, 1e-15);
}

TEST(Cylinder, FewSegmentsAreRaisedToEight) {
    const auto s = Cylinder({0, 0, 0}, 1, 1).surface({3, 3});
    ASSERT_TRUE(s);
    EXPECT_EQ(s->points().size(), 16u);
}

TEST(Cylinder, RejectsInvalidParameters) {
    EXPECT_EQ(Cylinder({0, 0, 0}, -1, 1).surface({}).error().code(), vmm::ErrorCode::DegenerateShape);
    EXPECT_EQ(Cylinder({0, 0, 0}, 1, 0).surface({}).error().code(), vmm::ErrorCode::DegenerateShape);
}

}  // namespace
