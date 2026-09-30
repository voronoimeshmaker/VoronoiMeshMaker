// ============================================================================
// File: ut_Cuboid.cpp
// Description: Cuboid: closed surface, volume, area, tags as patches, invalid boxes.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <cmath>
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

using vmm::Cuboid;
using vmm::Vec3;

TEST(Cuboid, SurfaceMeasuresAndPatches) {
    const auto s = Cuboid({0, 0, 0}, {2, 3, 4}, {"w", "e", "s", "n", "b", "t"}).surface({});
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_DOUBLE_EQ(s->volume(), 24.0);
    EXPECT_DOUBLE_EQ(s->area(), 2 * (6.0 + 8.0 + 12.0));
    EXPECT_EQ(s->triangle_count(), 12u);
    EXPECT_EQ(s->patches(), (std::vector<std::string>{"w", "e", "s", "n", "b", "t"}));
    EXPECT_TRUE(s->contains(Vec3{1, 1, 1}));
}

TEST(Cuboid, UntaggedFacesShareOnePatch) {
    const auto s = Cuboid({0, 0, 0}, {1, 1, 1}).surface({});
    ASSERT_TRUE(s);
    EXPECT_EQ(s->patches(), (std::vector<std::string>{"boundary"}));
}

TEST(Cuboid, RejectsDegenerateBoxes) {
    EXPECT_EQ(Cuboid({0, 0, 0}, {1, 0, 1}).surface({}).error().code(), vmm::ErrorCode::DegenerateShape);
    EXPECT_EQ(Cuboid({0, 0, 0}, {1, 1, std::nan("")}).surface({}).error().code(), vmm::ErrorCode::DegenerateShape);
}

}  // namespace
