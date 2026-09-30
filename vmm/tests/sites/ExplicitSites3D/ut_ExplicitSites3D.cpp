// ============================================================================
// File: ut_ExplicitSites3D.cpp
// Description: ExplicitSites3D: keeps only the sites inside the region.
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

TEST(ExplicitSites3D, KeepsTheSitesInside) {
    vmm::Random rng(0);
    const auto pts = vmm::ExplicitSites3D({{0.5, 0.5, 0.5}, {2, 0, 0}, {0.1, 0.9, 0.2}}).generate(cube(), rng);
    ASSERT_TRUE(pts);
    EXPECT_EQ(*pts, (std::vector<vmm::Vec3>{{0.5, 0.5, 0.5}, {0.1, 0.9, 0.2}}));
}

}  // namespace
