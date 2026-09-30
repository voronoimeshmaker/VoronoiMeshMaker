// ============================================================================
// File: ut_Facade.cpp
// Description: generate_mesh_2d(): anchor A1 end to end; validation errors
//              stop the pipeline; warnings are returned.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <utility>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "test_domains.hpp"
#include <vmm/vmm.hpp>

namespace {

using vmm::RegionId;
using vmm::Vec2;

TEST(Facade, AnchorA1EndToEnd) {
    vmm::MeshRequest2D req{vmm::test::anchor_a1(), {}, {}, {}, {}};
    const vmm::SpacingField h = [](const Vec2& x) { return std::min(5.0, 0.5 + 0.15 * std::abs(x[1] + 5)); };
    for (std::size_t r = 0; r < 3; ++r) req.sources.push_back(vmm::sites_for(RegionId::from_index(r), vmm::AdaptiveQuadtreeSource(h, 0.5)));
    const auto res = vmm::generate_mesh_2d(req);
    ASSERT_TRUE(res) << res.error().message();
    EXPECT_GT(res->mesh.cell_count(), 1000u);
    EXPECT_EQ(res->partition.region_count(), 3u);
    EXPECT_TRUE(res->validation.ok());
    EXPECT_GT(res->invariants.interface_faces, 0u);
    EXPECT_EQ(res->stats.cells, res->mesh.cell_count());
}

TEST(Facade, VoidStopsThePipeline) {
    vmm::Declaration2D d;
    const auto m = *d.media().add("m");
    (void)d.add_region("s", m, vmm::Rectangle(Vec2{0, 0}, Vec2{3, 1}));
    (void)d.add_region("w", m, vmm::Rectangle(Vec2{0, 0}, Vec2{1, 3}));
    (void)d.add_region("e", m, vmm::Rectangle(Vec2{2, 0}, Vec2{3, 3}));
    (void)d.add_region("n", m, vmm::Rectangle(Vec2{0, 2}, Vec2{3, 3}));
    vmm::MeshRequest2D req{std::move(d), {}, {}, {}, {}};
    const auto res = vmm::generate_mesh_2d(req);
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), vmm::ErrorCode::DomainVoid);
}

TEST(Facade, ErrorsFromEachStage) {
    vmm::MeshRequest2D empty{vmm::Declaration2D{}, {}, {}, {}, {}};
    EXPECT_EQ(vmm::generate_mesh_2d(empty).error().code(), vmm::ErrorCode::EmptyDeclaration);
    vmm::MeshRequest2D no_sites{vmm::test::square_with_hole(), {}, {}, {}, {}};
    EXPECT_EQ(vmm::generate_mesh_2d(no_sites).error().code(), vmm::ErrorCode::RegionWithoutSites);
    vmm::MeshRequest2D fragmented{vmm::Declaration2D{}, {}, {}, {}, {}};
    const auto m = *fragmented.declaration.media().add("m");
    (void)fragmented.declaration.add_region("plain", m, vmm::Rectangle(Vec2{0, 0}, Vec2{2, 1}));
    (void)fragmented.declaration.add_region("river", m, vmm::Rectangle(Vec2{0.9, 0}, Vec2{1.1, 1}));
    fragmented.sources = {vmm::sites_for(RegionId{0}, vmm::UniformRandomSource(0.05)),
                          vmm::sites_for(RegionId{1}, vmm::UniformRandomSource(0.02))};
    const auto res = vmm::generate_mesh_2d(fragmented);
    ASSERT_TRUE(res) << res.error().message();
    EXPECT_FALSE(res->validation.warnings().empty());  // plain is split in two
}

TEST(Facade, SiteAndValidationErrors) {
    vmm::MeshRequest2D bad_spacing{vmm::test::square_with_hole(), {}, {}, {}, {}};
    bad_spacing.sources = {vmm::sites_for(RegionId{0}, vmm::UniformRandomSource(-1)),
                           vmm::sites_for(RegionId{1}, vmm::UniformRandomSource(0.05))};
    EXPECT_EQ(vmm::generate_mesh_2d(bad_spacing).error().code(), vmm::ErrorCode::InvalidSpacing);

    vmm::Declaration2D d;
    const auto m = *d.media().add("m");
    (void)d.add_region("under", m, vmm::Rectangle(Vec2{0, 0}, Vec2{1, 1}));
    (void)d.add_region("over", m, vmm::Rectangle(Vec2{0, 0}, Vec2{1, 1 - 1e-7}));
    vmm::MeshRequest2D sliver{std::move(d), {}, {}, {}, {}};
    sliver.validation.slivers_are_errors = true;
    EXPECT_EQ(vmm::generate_mesh_2d(sliver).error().code(), vmm::ErrorCode::Sliver);
}

}  // namespace
