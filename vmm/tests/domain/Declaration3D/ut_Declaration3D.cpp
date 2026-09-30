// ============================================================================
// File: ut_Declaration3D.cpp
// Description: Declaration3D: regions from shapes and surfaces, media, errors.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <string>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/declaration3d.hpp>

namespace {

using vmm::Cuboid;
using vmm::Declaration3D;

TEST(Declaration3D, RegionsFromShapes) {
    Declaration3D d({32, 2});
    EXPECT_EQ(d.polygonize_options().segments_per_circle, 32);
    const auto rock = d.media().add("rock");
    ASSERT_TRUE(rock);
    const auto a = d.add_region("block", *rock, Cuboid({0, 0, 0}, {1, 1, 1}));
    ASSERT_TRUE(a) << a.error().message();
    const auto b = d.add_region("ball", *rock, vmm::Sphere({3, 0, 0}, 1));
    ASSERT_TRUE(b);
    EXPECT_EQ(d.layers().size(), 2u);
    EXPECT_EQ(d.regions()[1].name, "ball");
    EXPECT_EQ(d.layers()[1].surface.triangle_count(), 320u);
    const Declaration3D& c = d;
    EXPECT_EQ(c.media().size(), 1u);
}

TEST(Declaration3D, Errors) {
    Declaration3D d;
    const auto m = *d.media().add("m");
    EXPECT_EQ(d.add_region("", m, Cuboid({0, 0, 0}, {1, 1, 1})).error().code(), vmm::ErrorCode::InvalidArgument);
    EXPECT_EQ(d.add_region("a", vmm::MediumId::from_index(4), Cuboid({0, 0, 0}, {1, 1, 1})).error().code(),
              vmm::ErrorCode::UnknownMedium);
    EXPECT_EQ(d.add_region("a", m, Cuboid({0, 0, 0}, {0, 1, 1})).error().code(), vmm::ErrorCode::DegenerateShape);
    ASSERT_TRUE(d.add_region("a", m, Cuboid({0, 0, 0}, {1, 1, 1})));
    EXPECT_EQ(d.add_region("a", m, Cuboid({0, 0, 0}, {1, 1, 1})).error().code(), vmm::ErrorCode::DuplicateName);
    EXPECT_EQ(d.add_region_surface("b", m, vmm::TriangleSurface{}).error().code(), vmm::ErrorCode::InvalidSurface);
}

}  // namespace
