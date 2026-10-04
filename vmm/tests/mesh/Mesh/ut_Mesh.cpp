// ============================================================================
// File: ut_Mesh.cpp
// Description: Mesh::from_data checks, accessors, face geometry, adjacency.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <span>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "simple_meshes.hpp"
#include <vmm/mesh/mesh.hpp>

namespace {

using vmm::CellId;
using vmm::FaceId;

TEST(Mesh, AccessorsOfTwoSquares) {
    const auto m = vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    ASSERT_TRUE(m);
    EXPECT_EQ(m->cell_count(), 2u);
    EXPECT_EQ(m->face_count(), 7u);
    EXPECT_EQ(m->internal_face_count(), 1u);
    EXPECT_EQ(m->point_count(), 6u);
    EXPECT_EQ(m->internal_faces().size(), 1u);
    EXPECT_EQ(m->boundary_faces().size(), 6u);
    EXPECT_TRUE(m->is_internal(FaceId{0}));
    EXPECT_FALSE(m->is_internal(FaceId{3}));
    EXPECT_EQ(m->neighbour(FaceId{0}), CellId{1});
    EXPECT_FALSE(m->neighbour(FaceId{2}).valid());
    EXPECT_EQ(m->patch(FaceId{2}).value, 0u);
    EXPECT_FALSE(m->patch(FaceId{0}).valid());
    EXPECT_EQ(m->patch_faces(vmm::PatchId{0}).size(), 6u);
    EXPECT_EQ(m->owner(FaceId{5}), CellId{1});
    EXPECT_EQ(m->face_vertices(FaceId{0}).size(), 2u);
    EXPECT_EQ(m->point(vmm::VertexId{4}), (vmm::Vec2{1, 1}));
    EXPECT_EQ(m->site(CellId{1}), (vmm::Vec2{1.5, 0.5}));
    EXPECT_EQ(m->region(CellId{0}).value, 0u);
    EXPECT_EQ(m->cell_input_sites()[0].value, 1u);
    EXPECT_EQ(m->regions()[0].name, "r");
    EXPECT_EQ(m->media()[0], "m");
    EXPECT_TRUE(m->site_weights().empty());
    EXPECT_TRUE(m->face_periodic_offsets().empty());
    EXPECT_EQ(m->owners().size(), 7u);
    EXPECT_EQ(m->neighbours().size(), 1u);
    EXPECT_EQ(m->patches().size(), 1u);
    EXPECT_EQ(m->cell_regions().size(), 2u);
    EXPECT_EQ(m->sites().size(), 2u);
    EXPECT_EQ(m->face_vertex_csr().rows(), 7u);
    EXPECT_EQ(m->data().points.size(), 6u);
    EXPECT_EQ(m->points().size(), 6u);
    EXPECT_EQ(m->cells().size(), 2u);
    EXPECT_EQ(m->faces().size(), 7u);
}

TEST(Mesh, FaceGeometry) {
    const auto m = *vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    const auto g = vmm::face_geometry(m, FaceId{0});
    EXPECT_EQ(g.area_vector, (vmm::Vec2{1, 0}));
    EXPECT_EQ(g.centroid, (vmm::Vec2{1, 0.5}));
    const std::vector<vmm::Vec3> tri{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    const auto t = vmm::face_geometry(std::span<const vmm::Vec3>(tri));
    EXPECT_EQ(t.area_vector, (vmm::Vec3{0, 0, 0.5}));
    EXPECT_DOUBLE_EQ(t.centroid[0], 1.0 / 3.0);
    const std::vector<vmm::Vec3> flat{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};
    EXPECT_EQ(vmm::face_geometry(std::span<const vmm::Vec3>(flat)).centroid, (vmm::Vec3{0, 0, 0}));
}

TEST(Mesh, RejectsInconsistentData) {
    auto check = [](auto mutate) {
        auto d = vmm::test::two_squares_data();
        mutate(d);
        const auto m = vmm::Mesh2D::from_data(d);
        ASSERT_FALSE(m);
        EXPECT_EQ(m.error().code(), vmm::ErrorCode::InvariantViolated);
    };
    check([](auto& d) { d.cell_region.pop_back(); });
    check([](auto& d) { d.owner.pop_back(); });
    check([](auto& d) { d.neighbour.assign(8, CellId{1}); });
    check([](auto& d) { d.site_weight = {1.0}; });
    check([](auto& d) { d.face_periodic_offset = {vmm::Vec2{}}; });
    check([](auto& d) { d.face_vertices.values[0] = vmm::VertexId{99}; });
    check([](auto& d) { d.owner[0] = CellId{9}; });
    check([](auto& d) { d.neighbour[0] = CellId{0}; });
    check([](auto& d) { d.patches[0].start = 2; });
    check([](auto& d) { d.patches[0].count = 5; });
    check([](auto& d) { d.cell_region[0] = vmm::RegionId{3}; });
    check([](auto& d) { d.regions[0].medium = vmm::MediumId{4}; });
    check([](auto& d) {
        vmm::Csr<vmm::VertexId> one_vertex;
        for (std::size_t f = 0; f < 7; ++f) {
            const vmm::VertexId v[1] = {vmm::VertexId{0}};
            one_vertex.push_row(v);
        }
        d.face_vertices = one_vertex;
    });
}

TEST(Mesh, InternalFacesMustBeUpperTriangular) {
    auto d = vmm::test::two_squares_data();
    d.sites.push_back({2.5, 0.5});
    d.cell_region.push_back(vmm::RegionId{0});
    d.cell_input_site.push_back(vmm::SiteId{2});
    d.neighbour = {CellId{2}, CellId{1}};  // (0,2) before (0,1)
    d.owner[1] = CellId{0};
    d.patches[0] = {"wall", 2, 5};
    const auto m = vmm::Mesh2D::from_data(d);
    ASSERT_FALSE(m);
    EXPECT_EQ(m.error().code(), vmm::ErrorCode::InvariantViolated);
}

TEST(Mesh, CellAdjacency) {
    const auto m = *vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    const auto adj = vmm::cell_adjacency(m);
    EXPECT_EQ(adj.row(0).size(), 1u);
    EXPECT_EQ(adj.row(1)[0], CellId{0});
}

}  // namespace

TEST(Mesh, FaceCentroidSmallTranslatedTriangle) {
    const std::vector<vmm::Vec3> points{{100,100,100},{100.001,100,100},{100.001,100.001,100}};
    const auto g=vmm::face_geometry(std::span<const vmm::Vec3>(points));
    EXPECT_DOUBLE_EQ(g.centroid[0],100.+2.*(100.001-100.)/3.);
    EXPECT_DOUBLE_EQ(g.centroid[1],100.+(100.001-100.)/3.);
    EXPECT_DOUBLE_EQ(g.centroid[2],100.);
}
