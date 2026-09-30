// ============================================================================
// File: ut_AdaptiveOctreeSource3D.cpp
// Description: AdaptiveOctreeSource3D: denser where the spacing is smaller,
//              uniform spacing gives the expected count, determinism, errors.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/random.hpp>
#include <vmm/domain/shapes3d.hpp>
#include <vmm/sites/sources3d.hpp>

namespace {

using vmm::AdaptiveOctreeSource3D;
using vmm::Vec3;

vmm::TriangleSurface cube() { return *vmm::Cuboid({0, 0, 0}, {1, 1, 1}).surface({}); }

TEST(AdaptiveOctreeSource3D, UniformSpacing) {
    vmm::Random rng(1);
    const auto pts = AdaptiveOctreeSource3D([](const Vec3&) { return 0.25; }, 0.01, 0.0, 0.1).generate(cube(), rng);
    ASSERT_TRUE(pts) << pts.error().message();
    EXPECT_EQ(pts->size(), 64u);  // 4 x 4 x 4 leaves, centres at 0.125 + k/4
}

TEST(AdaptiveOctreeSource3D, DenserWhereTheSpacingIsSmaller) {
    vmm::Random a(3);
    vmm::Random b(3);
    const auto field = [](const Vec3& x) { return x[2] < 0.5 ? 0.06 : 0.25; };
    const auto pts = AdaptiveOctreeSource3D(field, 0.01).generate(cube(), a);
    const auto again = AdaptiveOctreeSource3D(field, 0.01).generate(cube(), b);
    ASSERT_TRUE(pts);
    ASSERT_TRUE(again);
    EXPECT_EQ(*pts, *again);
    std::size_t low = 0;
    for (const Vec3& p : *pts) {
        low += p[2] < 0.5;
        EXPECT_TRUE(cube().contains(p));
    }
    EXPECT_GT(low, 10 * (pts->size() - low));
}

TEST(AdaptiveOctreeSource3D, Errors) {
    vmm::Random rng(1);
    EXPECT_EQ(AdaptiveOctreeSource3D([](const Vec3&) { return 0.1; }, 0).generate(cube(), rng).error().code(),
              vmm::ErrorCode::InvalidSpacing);
    EXPECT_EQ(AdaptiveOctreeSource3D({}, 0.1).generate(cube(), rng).error().code(), vmm::ErrorCode::InvalidSpacing);
    EXPECT_EQ(AdaptiveOctreeSource3D([](const Vec3&) { return -1.0; }, 0.1).generate(cube(), rng).error().code(),
              vmm::ErrorCode::InvalidSpacing);
}

}  // namespace
