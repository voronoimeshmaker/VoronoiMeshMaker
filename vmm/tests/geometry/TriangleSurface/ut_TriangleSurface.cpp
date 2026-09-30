// ============================================================================
// File: ut_TriangleSurface.cpp
// Description: TriangleSurface: checks of make (closed, consistently oriented,
//              non-degenerate), re-orientation, measures, components, point
//              containment and distance; distance_to_triangle.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/geometry/surface.hpp>

namespace {

using vmm::ErrorCode;
using vmm::Triangle;
using vmm::TriangleSurface;
using vmm::Vec3;

/// Unit tetrahedron (0,0,0), (1,0,0), (0,1,0), (0,0,1), outward, offset by `o`.
std::vector<Vec3> tet_points(Vec3 o = {}) { return {o, o + Vec3{1, 0, 0}, o + Vec3{0, 1, 0}, o + Vec3{0, 0, 1}}; }
std::vector<Triangle> tet_faces(std::uint32_t k = 0) {
    return {{k, k + 2, k + 1}, {k, k + 1, k + 3}, {k, k + 3, k + 2}, {k + 1, k + 2, k + 3}};
}

vmm::Result<TriangleSurface> tet() { return TriangleSurface::make(tet_points(), tet_faces(), {0, 0, 0, 1}, {"base", "top"}); }

TEST(TriangleSurface, TetrahedronMeasures) {
    const auto s = tet();
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_NEAR(s->volume(), 1.0 / 6, 1e-15);
    EXPECT_NEAR(s->area(), 1.5 + std::sqrt(3.0) / 2, 1e-15);
    EXPECT_EQ(s->triangle_count(), 4u);
    EXPECT_EQ(s->patches().size(), 2u);
    EXPECT_EQ(s->triangle_patch()[3], 1u);
    EXPECT_EQ(s->component_count(), 1u);
    EXPECT_EQ(s->bounding_box().hi(), (Vec3{1, 1, 1}));
}

TEST(TriangleSurface, InwardSurfaceIsReoriented) {
    auto faces = tet_faces();
    for (auto& f : faces) std::swap(f[1], f[2]);
    const auto s = TriangleSurface::make(tet_points(), faces, {0, 0, 0, 0}, {"p"});
    ASSERT_TRUE(s);
    EXPECT_GT(s->volume(), 0.0);
}

TEST(TriangleSurface, ContainsAndDistance) {
    const auto s = *tet();
    EXPECT_TRUE(s.contains(Vec3{0.1, 0.1, 0.1}));
    EXPECT_FALSE(s.contains(Vec3{1, 1, 1}));
    EXPECT_FALSE(s.contains(Vec3{0, 0, 0}));  // on a vertex
    EXPECT_NEAR(s.distance(Vec3{0.1, 0.2, 0.3}), 0.1, 1e-15);
    EXPECT_NEAR(s.distance(Vec3{-1, 0, 0}), 1.0, 1e-15);
}

TEST(TriangleSurface, TwoComponents) {
    auto pts = tet_points();
    const auto more = tet_points(Vec3{5, 0, 0});
    pts.insert(pts.end(), more.begin(), more.end());
    auto faces = tet_faces();
    const auto f2 = tet_faces(4);
    faces.insert(faces.end(), f2.begin(), f2.end());
    const auto s = TriangleSurface::make(pts, faces, std::vector<std::uint32_t>(8, 0), {"p"});
    ASSERT_TRUE(s);
    EXPECT_EQ(s->component_count(), 2u);
    EXPECT_NEAR(s->volume(), 1.0 / 3, 1e-15);
    EXPECT_TRUE(s->contains(Vec3{5.1, 0.1, 0.1}));
}

TEST(TriangleSurface, RejectsInvalidInput) {
    // Too few triangles, one patch per triangle, indices.
    EXPECT_EQ(TriangleSurface::make(tet_points(), {{0, 1, 2}}, {0}, {"p"}).error().code(), ErrorCode::InvalidSurface);
    EXPECT_EQ(TriangleSurface::make(tet_points(), tet_faces(), {0}, {"p"}).error().code(), ErrorCode::InvalidArgument);
    auto bad = tet_faces();
    bad[0][0] = 9;
    EXPECT_EQ(TriangleSurface::make(tet_points(), bad, {0, 0, 0, 0}, {"p"}).error().code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(TriangleSurface::make(tet_points(), tet_faces(), {0, 0, 0, 3}, {"p"}).error().code(), ErrorCode::InvalidArgument);
    // Non-finite point.
    auto pts = tet_points();
    pts[1][0] = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(TriangleSurface::make(pts, tet_faces(), {0, 0, 0, 0}, {"p"}).error().code(), ErrorCode::InvalidSurface);
    // Degenerate triangle (collinear points).
    auto flat = tet_points();
    flat[3] = Vec3{2, 0, 0};
    EXPECT_EQ(TriangleSurface::make(flat, tet_faces(), {0, 0, 0, 0}, {"p"}).error().code(), ErrorCode::InvalidSurface);
    // Open surface (a face missing twice) and an edge used twice in one direction.
    auto open = tet_faces();
    open.pop_back();
    open.push_back({0, 2, 1});
    EXPECT_EQ(TriangleSurface::make(tet_points(), open, {0, 0, 0, 0}, {"p"}).error().code(), ErrorCode::InvalidSurface);
    auto flipped = tet_faces();
    std::swap(flipped[3][1], flipped[3][2]);
    EXPECT_EQ(TriangleSurface::make(tet_points(), flipped, {0, 0, 0, 0}, {"p"}).error().code(), ErrorCode::InvalidSurface);
}

TEST(TriangleSurface, ZeroVolumeIsRejected) {
    // A closed, consistently oriented but flat double-sided square (different diagonals).
    const std::vector<Vec3> p{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    const std::vector<Triangle> f{{0, 1, 2}, {0, 2, 3}, {1, 0, 3}, {1, 3, 2}};
    EXPECT_EQ(TriangleSurface::make(p, f, {0, 0, 0, 0}, {"p"}).error().code(), ErrorCode::InvalidSurface);
}

TEST(TriangleSurface, DistanceToTriangleRegions) {
    const Vec3 a{0, 0, 0};
    const Vec3 b{1, 0, 0};
    const Vec3 c{0, 1, 0};
    EXPECT_DOUBLE_EQ(vmm::distance_to_triangle(Vec3{-1, -1, 0}, a, b, c), std::sqrt(2.0));  // vertex a
    EXPECT_DOUBLE_EQ(vmm::distance_to_triangle(Vec3{2, 0, 0}, a, b, c), 1.0);               // vertex b
    EXPECT_DOUBLE_EQ(vmm::distance_to_triangle(Vec3{0, 2, 0}, a, b, c), 1.0);               // vertex c
    EXPECT_DOUBLE_EQ(vmm::distance_to_triangle(Vec3{0.5, -1, 0}, a, b, c), 1.0);            // edge ab
    EXPECT_DOUBLE_EQ(vmm::distance_to_triangle(Vec3{-1, 0.5, 0}, a, b, c), 1.0);            // edge ac
    EXPECT_NEAR(vmm::distance_to_triangle(Vec3{1, 1, 0}, a, b, c), std::sqrt(0.5), 1e-15);  // edge bc
    EXPECT_DOUBLE_EQ(vmm::distance_to_triangle(Vec3{0.2, 0.2, 3}, a, b, c), 3.0);           // face
}

}  // namespace
