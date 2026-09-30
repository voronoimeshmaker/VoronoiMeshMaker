// ============================================================================
// File: ut_CellFaceIndex.cpp
// Description: Faces of each cell and internal/boundary cell lists, on a
//              3 x 3 cartesian mesh (one internal cell).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "simple_meshes.hpp"
#include <vmm/backend/cgal.hpp>
#include <vmm/sites/sources.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace {

TEST(CellFaceIndex, TwoSquares) {
    const auto m = *vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    const vmm::CellFaceIndex index(m);
    EXPECT_EQ(index.faces_of(vmm::CellId{0}).size(), 4u);
    EXPECT_EQ(index.faces_of(vmm::CellId{1})[0], vmm::FaceId{0});
    EXPECT_TRUE(index.internal_cells().empty());
    EXPECT_EQ(index.boundary_cells().size(), 2u);
    EXPECT_EQ(index.csr().values.size(), 2 * m.internal_face_count() + m.boundary_faces().size());
}

TEST(CellFaceIndex, ThreeByThreeHasOneInternalCell) {
    const auto backend = vmm::cgal_backend_2d();
    vmm::Declaration2D d;
    (void)d.add_region("box", *d.media().add("m"), vmm::Rectangle(vmm::Vec2{0, 0}, vmm::Vec2{3, 3}));
    const auto p = *backend.build_partition(d);
    std::vector<vmm::RegionSites> src{vmm::sites_for(vmm::RegionId{0}, vmm::CartesianGridSource(1, vmm::Vec2{0.5, 0.5}))};
    const auto s = *vmm::generate_sites(p, src);
    const auto b = *vmm::build_mesh_2d(p, s, backend);
    const vmm::CellFaceIndex index(b.mesh);
    ASSERT_EQ(index.internal_cells().size(), 1u);
    EXPECT_EQ(b.mesh.site(index.internal_cells()[0]), (vmm::Vec2{1.5, 1.5}));
    EXPECT_EQ(index.boundary_cells().size(), 8u);
    for (const vmm::FaceId f : index.faces_of(index.internal_cells()[0])) EXPECT_TRUE(b.mesh.is_internal(f));
}

}  // namespace
