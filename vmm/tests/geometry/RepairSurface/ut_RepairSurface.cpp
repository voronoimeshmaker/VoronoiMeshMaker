// ============================================================================
// File: ut_RepairSurface.cpp
// Description: repair_surface: welding, collapsed and duplicated triangles,
//              orientation per component, holes, non-manifold edges and the
//              input checks.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes3d.hpp>
#include <vmm/geometry/repair.hpp>

namespace {

using vmm::ErrorCode;
using vmm::SurfaceRepairReport;
using vmm::TriangleSoup;
using vmm::Vec3;

/// The unit cube as an STL-like soup: three separate points per triangle.
TriangleSoup cube_soup() {
    const auto s = *vmm::Cuboid({0, 0, 0}, {1, 1, 1}).surface({});
    TriangleSoup soup;
    soup.patches = {"stl"};
    for (const auto& t : s.triangles()) {
        const auto base = static_cast<std::uint32_t>(soup.points.size());
        for (const auto v : t) soup.points.push_back(s.points()[v]);
        soup.triangles.push_back({base, base + 1, base + 2});
        soup.triangle_patch.push_back(0);
    }
    return soup;
}

TEST(RepairSurface, WeldsAnStlSoup) {
    SurfaceRepairReport rep;
    const auto s = vmm::repair_surface(cube_soup(), {}, &rep);
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_EQ(s->points().size(), 8u);
    EXPECT_EQ(rep.welded_points, 36u - 8u);
    EXPECT_EQ(rep.components, 1u);
    EXPECT_EQ(rep.flipped_triangles, 0u);
    EXPECT_DOUBLE_EQ(s->volume(), 1.0);
}

TEST(RepairSurface, WeldsWithinTheTolerance) {
    auto soup = cube_soup();
    for (std::size_t k = 0; k < soup.points.size(); ++k) {
        soup.points[k] = soup.points[k] + Vec3{1e-12 * static_cast<double>(k % 7), 0, 0};  // noise per point
    }
    const auto s = vmm::repair_surface(soup);
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_EQ(s->points().size(), 8u);
    EXPECT_FALSE(vmm::repair_surface(soup, {0}).has_value());  // no welding: every edge is open
}

TEST(RepairSurface, FlipsMinorityTrianglesAndTurnsOutward) {
    auto soup = cube_soup();
    std::swap(soup.triangles[3][1], soup.triangles[3][2]);
    SurfaceRepairReport rep;
    auto s = vmm::repair_surface(soup, {}, &rep);
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_EQ(rep.flipped_triangles, 1u);
    EXPECT_DOUBLE_EQ(s->volume(), 1.0);
    for (auto& t : soup.triangles) std::swap(t[1], t[2]);  // everything inward
    s = vmm::repair_surface(soup, {}, &rep);
    ASSERT_TRUE(s);
    EXPECT_DOUBLE_EQ(s->volume(), 1.0);
}

TEST(RepairSurface, RemovesCollapsedAndDuplicatedTriangles) {
    auto soup = cube_soup();
    const auto n = static_cast<std::uint32_t>(soup.points.size());
    soup.points.push_back({0, 0, 0});
    soup.points.push_back({0, 0, 0});
    soup.points.push_back({1, 0, 0});
    soup.triangles.push_back({n, n + 1, n + 2});  // collapses after welding
    soup.triangle_patch.push_back(0);
    soup.triangles.push_back(soup.triangles[0]);  // same orientation: one copy kept
    soup.triangle_patch.push_back(0);
    SurfaceRepairReport rep;
    const auto s = vmm::repair_surface(soup, {}, &rep);
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_EQ(rep.collapsed_triangles, 1u);
    EXPECT_EQ(rep.duplicate_triangles, 1u);
    EXPECT_EQ(s->triangle_count(), 12u);
    // A pair of opposite copies is an internal wall: both go.
    auto wall = cube_soup();
    const auto& t0 = wall.triangles[0];
    wall.triangles.push_back({t0[0], t0[2], t0[1]});
    wall.triangle_patch.push_back(0);
    wall.triangles.push_back(t0);
    wall.triangle_patch.push_back(0);
    EXPECT_TRUE(vmm::repair_surface(wall));
}

TEST(RepairSurface, Failures) {
    auto open = cube_soup();
    open.triangles.pop_back();
    open.triangle_patch.pop_back();
    EXPECT_EQ(vmm::repair_surface(open).error().code(), ErrorCode::InvalidSurface);
    auto fin = cube_soup();
    fin.points.push_back({0.5, 0.5, 2});  // a fin on an edge of triangle 0 (three triangles on one edge)
    fin.triangles.push_back({fin.triangles[0][0], fin.triangles[0][1], static_cast<std::uint32_t>(fin.points.size() - 1)});
    fin.triangle_patch.push_back(0);
    EXPECT_EQ(vmm::repair_surface(fin).error().code(), ErrorCode::InvalidSurface);
    EXPECT_EQ(vmm::repair_surface(TriangleSoup{}).error().code(), ErrorCode::InvalidSurface);
    auto bad = cube_soup();
    bad.triangle_patch.pop_back();
    EXPECT_EQ(vmm::repair_surface(bad).error().code(), ErrorCode::InvalidArgument);
    bad = cube_soup();
    bad.triangles[0][0] = 999;
    EXPECT_EQ(vmm::repair_surface(bad).error().code(), ErrorCode::InvalidArgument);
    bad = cube_soup();
    bad.triangle_patch[0] = 4;
    EXPECT_EQ(vmm::repair_surface(bad).error().code(), ErrorCode::InvalidArgument);
    bad = cube_soup();
    bad.points[0][1] = std::numeric_limits<double>::infinity();
    EXPECT_EQ(vmm::repair_surface(bad).error().code(), ErrorCode::InvalidSurface);
    EXPECT_EQ(vmm::repair_surface(cube_soup(), {-1}).error().code(), ErrorCode::InvalidSurface);
    TriangleSoup twice;  // a triangle and its opposite: nothing left
    twice.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    twice.triangles = {{0, 1, 2}, {0, 2, 1}};
    twice.triangle_patch = {0, 0};
    twice.patches = {"p"};
    EXPECT_EQ(vmm::repair_surface(twice).error().code(), ErrorCode::InvalidSurface);
}

}  // namespace
