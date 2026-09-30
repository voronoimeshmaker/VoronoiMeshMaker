// ============================================================================
// File: ut_Metrics.cpp
// Description: Finite-volume metrics against analytic cases (P11 criterion):
//              cartesian mesh (squares) and hexagonal mesh (regular hexagons);
//              quality report.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "simple_meshes.hpp"
#include <vmm/backend/cgal.hpp>
#include <vmm/mesh/metrics.hpp>
#include <vmm/sites/sources.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace {

using vmm::Vec2;

vmm::Build2D lattice_mesh(bool hexagonal, double h) {
    const auto backend = vmm::cgal_backend_2d();
    vmm::Declaration2D d;
    (void)d.add_region("box", *d.media().add("m"), vmm::Rectangle(Vec2{0, 0}, Vec2{2, 2}));
    const auto p = *backend.build_partition(d);
    std::vector<vmm::RegionSites> src;
    if (hexagonal) {
        src.push_back(vmm::sites_for(vmm::RegionId{0}, vmm::HexagonalGridSource(h, Vec2{0.013, 0.017})));
    } else {
        src.push_back(vmm::sites_for(vmm::RegionId{0}, vmm::CartesianGridSource(h, Vec2{0.5 * h, 0.5 * h})));
    }
    return *vmm::build_mesh_2d(p, *vmm::generate_sites(p, src), backend);
}

TEST(Metrics, TwoSquares) {
    const auto m = *vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    const auto x = vmm::compute_metrics(m);
    EXPECT_DOUBLE_EQ(x.cell_measure[0], 1.0);
    EXPECT_EQ(x.cell_centroid[1], (Vec2{1.5, 0.5}));
    EXPECT_DOUBLE_EQ(x.distance[0], 1.0);
    EXPECT_EQ(x.intersection[0], (Vec2{1, 0.5}));
    EXPECT_EQ(x.nonorthogonality[0], 0.0);
    EXPECT_EQ(x.skewness[0], 0.0);
    EXPECT_DOUBLE_EQ(x.distance[1], 0.5);  // site to the bottom wall
    EXPECT_EQ(x.intersection[1], (Vec2{0.5, 0}));
    EXPECT_DOUBLE_EQ(x.cell_aspect_ratio[0], std::sqrt(0.5) / 0.5);
}

TEST(Metrics, CartesianIsAnalytic) {
    const double h = 0.1;
    const auto b = lattice_mesh(false, h);
    const auto x = vmm::compute_metrics(b.mesh);
    for (const vmm::CellId c : b.mesh.cells()) {
        ASSERT_NEAR(x.cell_measure[c.index()], h * h, 1e-15);
        ASSERT_NEAR(vmm::norm(x.cell_centroid[c.index()] - b.mesh.site(c)), 0, 1e-15);
    }
    for (const vmm::FaceId f : b.mesh.internal_faces()) {
        ASSERT_NEAR(x.face_measure[f.index()], h, 1e-15);
        ASSERT_NEAR(x.distance[f.index()], h, 1e-15);
        ASSERT_LT(x.nonorthogonality[f.index()], 1e-12);
        ASSERT_LT(x.skewness[f.index()], 1e-12);
    }
}

TEST(Metrics, HexagonalIsAnalytic) {
    const double a = 0.1;
    const auto b = lattice_mesh(true, a);
    const auto x = vmm::compute_metrics(b.mesh);
    const vmm::CellFaceIndex index(b.mesh);
    std::size_t checked = 0;
    for (const vmm::CellId c : index.internal_cells()) {
        const Vec2 x0 = b.mesh.site(c);
        if (x0[0] < 0.3 || x0[0] > 1.7 || x0[1] < 0.3 || x0[1] > 1.7) continue;  // full ring of neighbours
        // Regular hexagon: area sqrt(3)/2 a^2, six faces of length a / sqrt(3).
        ASSERT_NEAR(x.cell_measure[c.index()], std::sqrt(3.0) / 2 * a * a, 1e-15);
        ASSERT_EQ(index.faces_of(c).size(), 6u);
        for (const vmm::FaceId f : index.faces_of(c)) {
            ASSERT_NEAR(x.face_measure[f.index()], a / std::sqrt(3.0), 1e-15);
            ASSERT_NEAR(x.distance[f.index()], a, 1e-15);
            ASSERT_LT(x.skewness[f.index()], 1e-12);
        }
        ++checked;
    }
    EXPECT_GT(checked, 100u);
}

TEST(Metrics, QualityReport) {
    const auto b = lattice_mesh(false, 0.1);
    const auto x = vmm::compute_metrics(b.mesh);
    const auto q = vmm::quality_report(b.mesh, x);
    ASSERT_EQ(q.regions.size(), 1u);
    EXPECT_TRUE(q.within_limits());
    EXPECT_EQ(q.regions[0].cells, 400u);
    EXPECT_LT(q.regions[0].max_nonortho_internal, 1e-12);
    EXPECT_NEAR(q.regions[0].p99_aspect_ratio, std::sqrt(2.0), 1e-12);
    EXPECT_NEAR(q.regions[0].min_face_fraction, 1.0, 1e-12);
    vmm::QualityLimits strict;
    strict.min_face_fraction = 2;
    strict.max_skewness = -1;
    const auto s = vmm::quality_report(b.mesh, x, strict);
    EXPECT_FALSE(s.within_limits());
    EXPECT_GT(s.short_faces, 0u);
    EXPECT_GT(s.skewness_violations, 0u);
}

TEST(Metrics, WithinLimitsEachCondition) {
    vmm::QualityReport q;
    EXPECT_TRUE(q.within_limits());
    q.nonortho_violations = 1;
    EXPECT_FALSE(q.within_limits());
    q = {};
    q.skewness_violations = 1;
    EXPECT_FALSE(q.within_limits());
    q = {};
    q.short_faces = 1;
    EXPECT_FALSE(q.within_limits());
}

}  // namespace
