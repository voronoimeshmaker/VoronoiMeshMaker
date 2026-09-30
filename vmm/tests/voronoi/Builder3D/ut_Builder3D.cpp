// ============================================================================
// File: ut_Builder3D.cpp
// Description: build_mesh_3d: statistics, the fast path switch, the input map,
//              invariant_reference of a 3D partition and the errors.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/cgal.hpp>
#include <vmm/domain/shapes3d.hpp>
#include <vmm/voronoi/builder3d.hpp>

namespace {

using vmm::ErrorCode;
using vmm::RegionId;
using vmm::SiteSet3D;
using vmm::Vec3;

vmm::Partition3D cube() {
    vmm::Declaration3D d;
    const auto m = *d.media().add("m");
    EXPECT_TRUE(d.add_region("cube", m, vmm::Cuboid({0, 0, 0}, {1, 1, 1})));
    return *vmm::cgal_backend_3d().build_partition(d);
}

SiteSet3D lattice(int n) {
    SiteSet3D s;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                // Slightly perturbed lattice: no cospherical sites.
                const double e = 0.01 * ((i * 7 + j * 3 + k * 5) % 11) / 11.0;
                s.add(Vec3{(i + 0.5 + e) / n, (j + 0.5 - e) / n, (k + 0.5 + 0.5 * e) / n}, RegionId::from_index(0));
            }
        }
    }
    return s;
}

TEST(Builder3D, StatisticsAndInputMap) {
    const auto p = cube();
    const auto sites = lattice(5);
    const auto b = vmm::build_mesh_3d(p, sites, vmm::cgal_backend_3d());
    ASSERT_TRUE(b) << b.error().message();
    EXPECT_EQ(b->stats.cells, 125u);
    EXPECT_EQ(b->stats.fast_cells + b->stats.clipped_cells, 125u);
    EXPECT_GT(b->stats.fast_cells, 0u);
    EXPECT_EQ(b->stats.fragmented_cells, 0u);
    EXPECT_GE(b->stats.seconds_delaunay, 0.0);
    EXPECT_EQ(b->cell_volume.size(), 125u);
    for (const auto c : b->mesh.cells()) {
        EXPECT_EQ(b->mesh.site(c), sites.positions()[b->mesh.cell_input_sites()[c.index()].index()]);
    }
    auto ref = vmm::invariant_reference(p);
    ref.cell_measure = b->cell_volume;
    EXPECT_TRUE(vmm::check_invariants(b->mesh, ref).passed(ref));
}

TEST(Builder3D, WithoutTheFastPathTheMeshIsTheSame) {
    const auto p = cube();
    const auto sites = lattice(3);
    const auto fast = vmm::build_mesh_3d(p, sites, vmm::cgal_backend_3d());
    const auto slow = vmm::build_mesh_3d(p, sites, vmm::cgal_backend_3d(), {1e-12, false});
    ASSERT_TRUE(fast);
    ASSERT_TRUE(slow);
    EXPECT_EQ(slow->stats.clipped_cells, 27u);
    EXPECT_EQ(fast->mesh.owners().size(), slow->mesh.owners().size());
    EXPECT_TRUE(std::ranges::equal(fast->mesh.neighbours(), slow->mesh.neighbours()));
}

TEST(Builder3D, InvariantReference) {
    const auto ref = vmm::invariant_reference(cube());
    EXPECT_DOUBLE_EQ(ref.total_measure, 1.0);
    EXPECT_DOUBLE_EQ(ref.boundary_measure, 6.0);
    ASSERT_EQ(ref.region_measure.size(), 1u);
    EXPECT_TRUE(ref.interface_measure.empty());
}

TEST(Builder3D, Errors) {
    const auto p = cube();
    const auto backend = vmm::cgal_backend_3d();
    EXPECT_EQ(vmm::build_mesh_3d(p, lattice(2), vmm::Backend3D{}).error().code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(vmm::build_mesh_3d(p, SiteSet3D{}, backend).error().code(), ErrorCode::RegionWithoutSites);
    EXPECT_EQ(vmm::build_mesh_3d(vmm::Partition3D{}, lattice(2), backend).error().code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(vmm::build_mesh_3d(p, lattice(2), backend, {-1, true}).error().code(), ErrorCode::InvalidLengthScale);
    SiteSet3D region;
    region.add(Vec3{0.5, 0.5, 0.5}, RegionId::from_index(2));
    EXPECT_EQ(vmm::build_mesh_3d(p, region, backend).error().code(), ErrorCode::SiteOutsideRegion);
    SiteSet3D twice = lattice(2);
    twice.add(twice.positions()[0], RegionId::from_index(0));
    EXPECT_EQ(vmm::build_mesh_3d(p, twice, backend).error().code(), ErrorCode::DuplicateSite);
    auto broken = backend;
    broken.prepare = [](const vmm::Partition3D&, RegionId) -> vmm::Result<vmm::PreparedDomain3> {
        return vmm::fail(ErrorCode::BackendFailure, "test");
    };
    EXPECT_EQ(vmm::build_mesh_3d(p, lattice(2), broken).error().code(), ErrorCode::BackendFailure);
    auto failing_clip = backend;
    failing_clip.clip_cell = [](const vmm::LabelledPolyhedron3&, const vmm::PreparedDomain3&) {
        vmm::CellClip3 c;
        c.error = "test";
        return c;
    };
    EXPECT_EQ(vmm::build_mesh_3d(p, lattice(2), failing_clip).error().code(), ErrorCode::BackendFailure);
}

}  // namespace
