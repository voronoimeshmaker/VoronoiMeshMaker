// ============================================================================
// File: ut_Sphere.cpp
// Description: Sphere: subdivided icosahedron, volume converging to 4/3 pi r^3, invalid parameters.
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
#include <vmm/domain/shapes3d.hpp>

namespace {

using vmm::Sphere;

TEST(Sphere, VolumeConvergesWithSubdivisions) {
    const double exact = 4.0 / 3.0 * std::numbers::pi * 8;
    const auto coarse = Sphere({1, 2, 3}, 2, "shell").surface({64, 1});
    const auto fine = Sphere({1, 2, 3}, 2, "shell").surface({64, 4});
    ASSERT_TRUE(coarse);
    ASSERT_TRUE(fine);
    EXPECT_EQ(coarse->triangle_count(), 80u);
    EXPECT_EQ(fine->triangle_count(), 5120u);
    EXPECT_LT(std::abs(fine->volume() - exact), std::abs(coarse->volume() - exact));
    EXPECT_NEAR(fine->volume(), exact, 0.01 * exact);
    EXPECT_EQ(fine->patches().front(), "shell");
    for (const auto& p : fine->points()) EXPECT_NEAR(vmm::norm(p - vmm::Vec3{1, 2, 3}), 2.0, 1e-14);
}

TEST(Sphere, SubdivisionsAreClamped) {
    const auto s = Sphere({0, 0, 0}, 1).surface({64, -3});
    ASSERT_TRUE(s);
    EXPECT_EQ(s->triangle_count(), 20u);
}

TEST(Sphere, RejectsInvalidParameters) {
    EXPECT_EQ(Sphere({0, 0, 0}, 0).surface({}).error().code(), vmm::ErrorCode::DegenerateShape);
    EXPECT_EQ(Sphere({0, 0, std::nan("")}, 1).surface({}).error().code(), vmm::ErrorCode::DegenerateShape);
}

}  // namespace
