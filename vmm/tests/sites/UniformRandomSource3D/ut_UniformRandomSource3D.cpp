// ============================================================================
// File: ut_UniformRandomSource3D.cpp
// Description: UniformRandomSource3D: spacing, margin, determinism, invalid parameters.
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

vmm::TriangleSurface cube() { return *vmm::Cuboid({0, 0, 0}, {1, 1, 1}).surface({}); }

TEST(UniformRandomSource3D, SpacingAndMargin) {
    vmm::Random rng(3);
    const auto s = cube();
    const auto pts = vmm::UniformRandomSource3D(0.2).min_distance_fraction(0.7).boundary_margin_fraction(0.5).failure_limit(2000).generate(s, rng);
    ASSERT_TRUE(pts) << pts.error().message();
    EXPECT_GT(pts->size(), 30u);
    for (std::size_t i = 0; i < pts->size(); ++i) {
        EXPECT_TRUE(s.contains((*pts)[i]));
        EXPECT_GE(s.distance((*pts)[i]), 0.1);
        for (std::size_t j = 0; j < i; ++j) EXPECT_GE(vmm::norm((*pts)[i] - (*pts)[j]), 0.14);
    }
}

TEST(UniformRandomSource3D, InvalidParameters) {
    vmm::Random rng(1);
    EXPECT_EQ(vmm::UniformRandomSource3D(0).generate(cube(), rng).error().code(), vmm::ErrorCode::InvalidSpacing);
    EXPECT_EQ(vmm::UniformRandomSource3D(0.1).min_distance_fraction(0).generate(cube(), rng).error().code(),
              vmm::ErrorCode::InvalidSpacing);
}

}  // namespace
