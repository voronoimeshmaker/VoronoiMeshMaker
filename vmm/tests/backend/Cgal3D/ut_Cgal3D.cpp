// ============================================================================
// File: ut_Cgal3D.cpp
// Description: cgal_backend_3d(): every callable of Backend3D (partition
//              checks, preparation, Delaunay 3D, exact circumcentres,
//              boundary test, labelled exact clipping) and its errors.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
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

namespace {

using vmm::Box3;
using vmm::ErrorCode;
using vmm::FaceLabel;
using vmm::RegionId;
using vmm::Vec3;

vmm::Declaration3D cube_declaration() {
    vmm::Declaration3D d;
    const auto m = *d.media().add("m");
    EXPECT_TRUE(d.add_region("cube", m, vmm::Cuboid({0, 0, 0}, {1, 1, 1}, {"w", "e", "s", "n", "b", "t"})));
    return d;
}

/// Axis-aligned box as a labelled polyhedron (labels: neighbour ids 0..5).
vmm::LabelledPolyhedron3 box_cell(const Vec3& lo, const Vec3& hi) {
    vmm::LabelledPolyhedron3 c;
    for (std::uint32_t k = 0; k < 8; ++k) {
        c.points.push_back({(k & 1) ? hi[0] : lo[0], (k & 2) ? hi[1] : lo[1], (k & 4) ? hi[2] : lo[2]});
    }
    const std::array<std::array<std::uint32_t, 4>, 6> quads{
        {{0, 4, 6, 2}, {1, 3, 7, 5}, {0, 1, 5, 4}, {2, 6, 7, 3}, {0, 2, 3, 1}, {4, 5, 7, 6}}};
    for (std::uint32_t q = 0; q < 6; ++q) {
        c.faces.push_row(quads[q]);
        c.labels.push_back(q);
    }
    return c;
}

TEST(Cgal3D, FactoryIsComplete) {
    const auto b = vmm::cgal_backend_3d();
    EXPECT_TRUE(b.complete());
    EXPECT_EQ(b.info().name, "cgal");
    EXPECT_FALSE(vmm::Backend3D{}.complete());
    for (int k = 0; k < 7; ++k) {
        auto partial = b;
        if (k == 0) partial.build_partition = nullptr;
        if (k == 1) partial.prepare = nullptr;
        if (k == 2) partial.delaunay_pairs = nullptr;
        if (k == 3) partial.circumcentre = nullptr;
        if (k == 4) partial.touches_boundary = nullptr;
        if (k == 5) partial.clip_cell = nullptr;
        if (k == 6) partial.info = nullptr;
        EXPECT_FALSE(partial.complete()) << k;
    }
}

TEST(Cgal3D, BuildPartition) {
    const auto b = vmm::cgal_backend_3d();
    const auto p = b.build_partition(cube_declaration());
    ASSERT_TRUE(p) << p.error().message();
    EXPECT_EQ(p->region_count(), 1u);
    EXPECT_EQ(p->triangles().size(), 12u);
    EXPECT_EQ(p->patches().size(), 6u);
    EXPECT_DOUBLE_EQ(p->total_volume(), 1.0);
    EXPECT_FALSE(p->triangles().front().outside.valid());
}

TEST(Cgal3D, BuildPartitionErrors) {
    const auto b = vmm::cgal_backend_3d();
    EXPECT_EQ(b.build_partition(vmm::Declaration3D{}).error().code(), ErrorCode::EmptyDeclaration);
    auto two = cube_declaration();
    (void)two.add_region("other", vmm::MediumId::from_index(0), vmm::Cuboid({2, 0, 0}, {3, 1, 1}));
    EXPECT_EQ(b.build_partition(two).error().code(), ErrorCode::InvalidArgument);
    // Two overlapping cubes in one surface: closed and oriented, but self-intersecting.
    auto a = *vmm::Cuboid({0, 0, 0}, {1, 1, 1}).surface({});
    const auto c = *vmm::Cuboid({0.5, 0.5, 0.5}, {1.5, 1.5, 1.5}).surface({});
    auto pts = a.points();
    auto tris = a.triangles();
    for (const auto& t : c.triangles()) tris.push_back({t[0] + 8, t[1] + 8, t[2] + 8});
    pts.insert(pts.end(), c.points().begin(), c.points().end());
    const auto overlap = vmm::TriangleSurface::make(pts, tris, std::vector<std::uint32_t>(24, 0), {"p"});
    ASSERT_TRUE(overlap);
    vmm::Declaration3D d;
    const auto m = *d.media().add("m");
    ASSERT_TRUE(d.add_region_surface("x", m, *overlap));
    EXPECT_EQ(b.build_partition(d).error().code(), ErrorCode::InvalidSurface);
}

TEST(Cgal3D, PrepareAndTouchesBoundary) {
    const auto b = vmm::cgal_backend_3d();
    const auto p = *b.build_partition(cube_declaration());
    EXPECT_EQ(b.prepare(p, RegionId::from_index(1)).error().code(), ErrorCode::InvalidArgument);
    const auto d = b.prepare(p, RegionId::from_index(0));
    ASSERT_TRUE(d) << d.error().message();
    EXPECT_TRUE(b.touches_boundary(*d, Box3({0.9, 0.4, 0.4}, {1.1, 0.6, 0.6})));
    EXPECT_FALSE(b.touches_boundary(*d, Box3({0.4, 0.4, 0.4}, {0.6, 0.6, 0.6})));
    EXPECT_FALSE(b.touches_boundary(*d, Box3{}));
    EXPECT_FALSE(b.touches_boundary(vmm::PreparedDomain3{}, Box3({0, 0, 0}, {1, 1, 1})));
}

TEST(Cgal3D, DelaunayPairsAndCircumcentre) {
    const auto b = vmm::cgal_backend_3d();
    const std::vector<Vec3> s{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const auto pairs = b.delaunay_pairs(s);
    EXPECT_EQ(pairs.size(), 6u);  // a tetrahedron: every pair
    const auto c = b.circumcentre({s[0], s[1], s[2], s[3]});
    ASSERT_TRUE(c);
    EXPECT_EQ(*c, (Vec3{0.5, 0.5, 0.5}));
    EXPECT_FALSE(b.circumcentre({Vec3{0, 0, 0}, Vec3{1, 0, 0}, Vec3{0, 1, 0}, Vec3{1, 1, 0}}));
}

TEST(Cgal3D, ClipCellAgainstTheCube) {
    const auto b = vmm::cgal_backend_3d();
    const auto p = *b.build_partition(cube_declaration());
    const auto d = *b.prepare(p, RegionId::from_index(0));
    // Half of the cell sticks out through x = 1.
    const auto c = b.clip_cell(box_cell({0.5, 0.25, 0.25}, {1.5, 0.75, 0.75}), d);
    ASSERT_TRUE(c.error.empty()) << c.error;
    EXPECT_TRUE(c.local);  // only the cube triangles near the cell were used
    EXPECT_DOUBLE_EQ(c.volume, 0.5 * 0.5 * 0.5);
    EXPECT_EQ(c.components, 1u);
    std::set<FaceLabel> labels(c.cell.labels.begin(), c.cell.labels.end());
    // Five faces of the cell and the cube face x = 1, one piece per domain triangle (two).
    EXPECT_EQ(c.cell.faces.rows(), 7u);
    EXPECT_EQ(std::ranges::count_if(c.cell.labels, vmm::is_domain_face), 2);
    EXPECT_FALSE(labels.contains(1));  // the x+ face of the cell is outside
    EXPECT_TRUE(std::ranges::any_of(labels, vmm::is_domain_face));
    for (const FaceLabel l : labels) EXPECT_FALSE(vmm::is_box_face(l));
    // Loops start at their smallest point (canonical order).
    for (std::size_t f = 0; f < c.cell.faces.rows(); ++f) {
        const auto row = c.cell.faces.row(f);
        for (const auto v : row) EXPECT_LE(c.cell.points[row[0]], c.cell.points[v]);
    }
}

TEST(Cgal3D, CoplanarFacesFallBackToTheWholeRegion) {
    const auto b = vmm::cgal_backend_3d();
    const auto p = *b.build_partition(cube_declaration());
    const auto d = *b.prepare(p, RegionId::from_index(0));
    // The x+ face of the cell lies on the cube face x = 1: the local result is not certain.
    const auto c = b.clip_cell(box_cell({0.5, 0.25, 0.25}, {1.0, 0.75, 0.75}), d);
    ASSERT_TRUE(c.error.empty()) << c.error;
    EXPECT_FALSE(c.local);
    EXPECT_DOUBLE_EQ(c.volume, 0.5 * 0.5 * 0.5);
    EXPECT_EQ(c.components, 1u);
}

TEST(Cgal3D, ClipCellErrors) {
    const auto b = vmm::cgal_backend_3d();
    const auto p = *b.build_partition(cube_declaration());
    const auto d = *b.prepare(p, RegionId::from_index(0));
    EXPECT_FALSE(b.clip_cell(box_cell({0, 0, 0}, {1, 1, 1}), vmm::PreparedDomain3{}).error.empty());
    auto open = box_cell({0.5, 0.5, 0.5}, {1.5, 1.5, 1.5});
    open.faces = {};
    open.labels.clear();
    const std::array<std::uint32_t, 4> one{0, 4, 6, 2};
    open.faces.push_row(one);
    open.labels.push_back(0);
    EXPECT_FALSE(b.clip_cell(open, d).error.empty());
    auto repeated = box_cell({0.5, 0.5, 0.5}, {1.5, 1.5, 1.5});
    repeated.faces.push_row(std::array<std::uint32_t, 4>{0, 4, 6, 2});
    repeated.labels.push_back(7);
    EXPECT_FALSE(b.clip_cell(repeated, d).error.empty());
}

TEST(Cgal3D, LabelHelpers) {
    EXPECT_TRUE(vmm::is_neighbour_face(5));
    EXPECT_TRUE(vmm::is_domain_face(vmm::kDomainFace | 3));
    EXPECT_TRUE(vmm::is_box_face(vmm::kBoxFace | 2));
    EXPECT_FALSE(vmm::is_domain_face(vmm::kBoxFace | vmm::kDomainFace));
    EXPECT_FALSE(vmm::is_domain_face(7));
    EXPECT_FALSE(vmm::is_box_face(vmm::kDomainFace | 1));
    EXPECT_FALSE(vmm::is_neighbour_face(vmm::kDomainFace | 1));
    EXPECT_FALSE(vmm::is_neighbour_face(vmm::kBoxFace));
}

}  // namespace
