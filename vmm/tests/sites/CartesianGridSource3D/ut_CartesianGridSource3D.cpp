// ============================================================================
// File: ut_CartesianGridSource3D.cpp
// Description: CartesianGridSource3D: lattice points inside with the margin; invalid parameters.
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

TEST(CartesianGridSource3D, LatticeInsideTheCube) {
    vmm::Random rng(0);
    const auto pts = vmm::CartesianGridSource3D(0.25).generate(cube(), rng);
    ASSERT_TRUE(pts);
    EXPECT_EQ(pts->size(), 27u);  // 0.25, 0.5, 0.75 in each direction
    const auto shifted = vmm::CartesianGridSource3D(0.25, {0.125, 0.125, 0.125}, 0.25).generate(cube(), rng);
    ASSERT_TRUE(shifted);
    EXPECT_EQ(shifted->size(), 64u);
}

TEST(CartesianGridSource3D, InvalidParameters) {
    vmm::Random rng(0);
    EXPECT_EQ(vmm::CartesianGridSource3D(0).generate(cube(), rng).error().code(), vmm::ErrorCode::InvalidSpacing);
    EXPECT_EQ(vmm::CartesianGridSource3D(0.1, {}, -1).generate(cube(), rng).error().code(), vmm::ErrorCode::InvalidSpacing);
}

}  // namespace
