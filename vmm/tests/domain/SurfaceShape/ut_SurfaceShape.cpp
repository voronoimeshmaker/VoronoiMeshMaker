// ============================================================================
// File: ut_SurfaceShape.cpp
// Description: SurfaceShape: a user surface is returned unchanged.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes3d.hpp>

namespace {

TEST(SurfaceShape, ReturnsTheSurface) {
    const auto box = vmm::Cuboid({0, 0, 0}, {1, 2, 3}).surface({});
    ASSERT_TRUE(box);
    const vmm::SurfaceShape shape(*box);
    static_assert(vmm::Shape3D<vmm::SurfaceShape>);
    const auto s = shape.surface({});
    ASSERT_TRUE(s);
    EXPECT_EQ(s->points(), box->points());
    EXPECT_EQ(s->triangles(), box->triangles());
    EXPECT_DOUBLE_EQ(s->volume(), 6.0);
}

}  // namespace
