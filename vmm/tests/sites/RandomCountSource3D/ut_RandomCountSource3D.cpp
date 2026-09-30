// ============================================================================
// File: ut_RandomCountSource3D.cpp
// Description: RandomCountSource3D: exact count inside with margin; impossible margin; invalid margin.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
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

vmm::TriangleSurface cube() { return *vmm::Cuboid({0, 0, 0}, {1, 1, 1}).surface({}); }

TEST(RandomCountSource3D, ExactCount) {
    vmm::Random rng(9);
    const auto s = cube();
    const auto pts = vmm::RandomCountSource3D(200, 0.05).generate(s, rng);
    ASSERT_TRUE(pts);
    EXPECT_EQ(pts->size(), 200u);
    for (const auto& p : *pts) EXPECT_GE(s.distance(p), 0.05);
}

TEST(RandomCountSource3D, Failures) {
    vmm::Random rng(9);
    EXPECT_EQ(vmm::RandomCountSource3D(3, 0.6).generate(cube(), rng).error().code(), vmm::ErrorCode::SiteGenerationFailed);
    EXPECT_EQ(vmm::RandomCountSource3D(3, -1).generate(cube(), rng).error().code(), vmm::ErrorCode::InvalidSpacing);
}

}  // namespace
