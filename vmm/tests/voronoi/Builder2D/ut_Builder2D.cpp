// ============================================================================
// File: ut_Builder2D.cpp
// Description: build_mesh_2d(): DEC-011 invariants on the square with a hole
//              and an L interface (E1, E2), anchor A1, scale and order
//              invariance, determinism, cocircular grids and errors.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "mesh_checks.hpp"
#include "test_domains.hpp"
#include <vmm/backend/cgal.hpp>
#include <vmm/core/random.hpp>
#include <vmm/sites/sources.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace {

using vmm::RegionId;

struct Case {
    vmm::Partition2D partition;
    vmm::SiteSet sites;
};

Case square_case(double scale, bool pairs, std::uint64_t seed = 7) {
    const auto backend = vmm::cgal_backend_2d();
    auto p = backend.build_partition(vmm::test::square_with_hole(scale));
    EXPECT_TRUE(p);
    std::vector<vmm::RegionSites> src{vmm::sites_for(RegionId::from_index(0), vmm::UniformRandomSource(0.07 * scale)),
                                      vmm::sites_for(RegionId::from_index(1), vmm::UniformRandomSource(0.07 * scale))};
    vmm::SiteGenerationOptions opt;
    opt.seed = seed;
    if (pairs) opt.interface_pairs = vmm::InterfacePairs(0.07 * scale);
    auto s = vmm::generate_sites(*p, src, opt);
    EXPECT_TRUE(s) << (s ? "" : s.error().message());
    return {*p, s ? *s : vmm::SiteSet{}};
}

vmm::Build2D build(const Case& c, vmm::BuildOptions2D o = {}) {
    auto b = vmm::build_mesh_2d(c.partition, c.sites, vmm::cgal_backend_2d(), o);
    EXPECT_TRUE(b) << (b ? "" : b.error().message());
    return b ? std::move(*b) : vmm::Build2D{};
}

std::vector<std::uint32_t> identity_keys(std::size_t n) {
    std::vector<std::uint32_t> k(n);
    std::iota(k.begin(), k.end(), 0u);
    return k;
}

TEST(Builder2D, SquareWithHoleE1) {
    const auto c = square_case(1, false);
    const auto b = build(c);
    const auto r = vmm::test::expect_invariants(c.partition, b, "E1");
    EXPECT_GT(r.interface_faces, 0u);
    EXPECT_EQ(b.stats.fragmented_cells, 0u);
    EXPECT_GT(b.stats.fast_cells, 0u);
    EXPECT_GT(b.stats.clipped_cells, 0u);
    EXPECT_EQ(b.mesh.cell_count(), c.sites.size());
}

TEST(Builder2D, FastPathDoesNotChangeTheMesh) {
    const auto c = square_case(1, false);
    const auto fast = build(c);
    vmm::BuildOptions2D slow;
    slow.fast_path = false;
    const auto exact = build(c, slow);
    EXPECT_EQ(exact.stats.fast_cells, 0u);
    EXPECT_EQ(fast.mesh.points(), exact.mesh.points());
    EXPECT_EQ(std::vector<vmm::CellId>(fast.mesh.owners().begin(), fast.mesh.owners().end()),
              std::vector<vmm::CellId>(exact.mesh.owners().begin(), exact.mesh.owners().end()));
}

TEST(Builder2D, SquareWithHoleE2MirrorPairs) {
    const auto c = square_case(1, true);
    const auto b = build(c);
    const auto r = vmm::test::expect_invariants(c.partition, b, "E2");
    EXPECT_GT(r.interface_faces, 0u);
}

TEST(Builder2D, ScaleAndOrderInvariance) {
    const auto base_case = square_case(1, false);
    const auto base = build(base_case);
    const auto keys = identity_keys(base_case.sites.size());
    const auto sig = vmm::test::signature(base.mesh, keys);
    for (const double scale : {1e-3, 1e6}) {
        auto scaled = square_case(scale, false);
        // Same unit sites, scaled (the generator scales with the spacing, but use the base ones to be exact).
        vmm::SiteSet s;
        for (std::size_t i = 0; i < base_case.sites.size(); ++i) {
            s.add(scale * base_case.sites.positions()[i], base_case.sites.regions()[i]);
        }
        scaled.sites = s;
        const auto b = build(scaled);
        vmm::test::expect_invariants(scaled.partition, b, "scaled");
        EXPECT_EQ(vmm::test::signature(b.mesh, keys), sig) << scale;
    }
    vmm::Random rng(99);
    for (int order = 0; order < 5; ++order) {
        std::vector<std::uint32_t> perm = identity_keys(base_case.sites.size());
        if (order == 1) std::ranges::reverse(perm);
        if (order > 1) rng.shuffle(std::span<std::uint32_t>(perm));
        Case c{base_case.partition, {}};
        std::vector<std::uint32_t> key(perm.size());
        for (std::size_t k = 0; k < perm.size(); ++k) {
            c.sites.add(base_case.sites.positions()[perm[k]], base_case.sites.regions()[perm[k]]);
            key[k] = perm[k];
        }
        const auto b = build(c);
        EXPECT_EQ(vmm::test::signature(b.mesh, key), sig) << order;
        // Canonical order makes the mesh identical bit for bit (R18).
        EXPECT_EQ(b.mesh.points(), base.mesh.points()) << order;
    }
}

TEST(Builder2D, AnchorA1) {
    const auto backend = vmm::cgal_backend_2d();
    auto p = backend.build_partition(vmm::test::anchor_a1());
    ASSERT_TRUE(p);
    const vmm::SpacingField h = [](const vmm::Vec2& x) { return std::min(5.0, 0.5 + 0.15 * std::abs(x[1] + 5)); };
    std::vector<vmm::RegionSites> src;
    for (std::size_t r = 0; r < 3; ++r) {
        src.push_back(vmm::sites_for(RegionId::from_index(r), vmm::AdaptiveQuadtreeSource(h, 0.5)));
    }
    auto s = vmm::generate_sites(*p, src, {});
    ASSERT_TRUE(s) << s.error().message();
    const auto b = build({*p, *s});
    const auto r = vmm::test::expect_invariants(*p, b, "A1");
    EXPECT_GT(r.interface_faces, 0u);
    EXPECT_GT(b.mesh.cell_count(), 1000u);
}

TEST(Builder2D, CocircularGridCollapsesDegenerateFaces) {
    const auto backend = vmm::cgal_backend_2d();
    vmm::Declaration2D d;
    const auto m = *d.media().add("m");
    (void)d.add_region("box", m, vmm::Rectangle(vmm::Vec2{0, 0}, vmm::Vec2{4, 3}));
    auto p = backend.build_partition(d);
    ASSERT_TRUE(p);
    std::vector<vmm::RegionSites> src{vmm::sites_for(RegionId::from_index(0), vmm::CartesianGridSource(0.1, vmm::Vec2{0.05, 0.05}))};
    auto s = vmm::generate_sites(*p, src);
    ASSERT_TRUE(s);
    EXPECT_EQ(s->size(), 1200u);
    const auto b = build({*p, *s});
    vmm::test::expect_invariants(*p, b, "grid");
    // Every cell is a 0.1 x 0.1 square: 4 faces, area 0.01.
    const vmm::CellFaceIndex index(b.mesh);
    for (const vmm::CellId c : b.mesh.cells()) ASSERT_EQ(index.faces_of(c).size(), 4u) << c.value;
}

TEST(Builder2D, Errors) {
    const auto c = square_case(1, false);
    EXPECT_EQ(vmm::build_mesh_2d(c.partition, c.sites, vmm::Backend2D{}).error().code(), vmm::ErrorCode::InvalidArgument);
    vmm::SiteSet bad = c.sites;
    bad.add(vmm::Vec2{0.5, 0.5}, RegionId::from_index(1));  // inside the hole
    EXPECT_EQ(vmm::build_mesh_2d(c.partition, bad, vmm::cgal_backend_2d()).error().code(), vmm::ErrorCode::SiteOutsideRegion);
    EXPECT_EQ(vmm::build_mesh_2d(vmm::Partition2D{}, c.sites, vmm::cgal_backend_2d()).error().code(),
              vmm::ErrorCode::InvalidLengthScale);
}

}  // namespace

TEST(Builder2D, BoundaryCoincidentWithBisectorKeepsFiniteVertices) {
    for (double thickness : {0.05, 0.1, 0.5, 1.0})
    for (double offset : {0.0, 1e-9, 0.01}) {
        SCOPED_TRACE(::testing::Message() << "thickness=" << thickness << " offset=" << offset);
        vmm::Declaration2D d;
        const auto medium = *d.media().add("water");
        const auto region = d.add_region("block", medium,
            vmm::Rectangle({0, 0}, {4, 2.1}));
        ASSERT_TRUE(region);
        ASSERT_TRUE(d.add_hole(vmm::Rectangle({0.6, 1.0}, {3.4, 1.0 + thickness})));
        const auto backend = vmm::cgal_backend_2d();
        const auto partition = backend.build_partition(d);
        ASSERT_TRUE(partition);
        const std::vector<vmm::RegionSites> sources{vmm::sites_for(*region,
            vmm::CartesianGridSource(0.5, {0.25 + offset, 0.25 + offset}))};
        const auto sites = vmm::generate_sites(*partition, sources);
        ASSERT_TRUE(sites);
        const auto result = vmm::build_mesh_2d(*partition, *sites, backend);
        ASSERT_TRUE(result) << result.error().message();
        EXPECT_EQ(result->stats.unmatched_bisector_pieces, 0u);
        vmm::test::expect_invariants(*partition, *result, "coincident boundary");
    }
}

TEST(Builder2D, RotatedThinObstaclesWithGridSources) {
    const auto backend = vmm::cgal_backend_2d();
    for (bool hexagonal : {false, true})
    for (double thickness : {0.05, 0.1, 1.0})
    for (double angle : {0.0, 1.5707963267948966, 0.7853981633974483})
    for (vmm::Vec2 origin : {vmm::Vec2{0, 0}, vmm::Vec2{13, -7}}) {
        SCOPED_TRACE(::testing::Message() << "hex=" << hexagonal << " t=" << thickness << " angle=" << angle);
        const auto transform = [&](vmm::Vec2 p) {
            return origin + vmm::Vec2{std::cos(angle)*p[0]-std::sin(angle)*p[1],
                                     std::sin(angle)*p[0]+std::cos(angle)*p[1]};
        };
        vmm::Declaration2D initial;
        const auto medium = *initial.media().add("water");
        const auto region = *initial.add_region("block", medium, vmm::Rectangle({0,0},{4,2.1}));
        ASSERT_TRUE(initial.add_hole(vmm::Rectangle({0.6,1},{3.4,1+thickness})));
        const auto domain = backend.build_partition(initial);
        ASSERT_TRUE(domain);
        std::vector<vmm::RegionSites> sources;
        if (hexagonal) sources.push_back(vmm::sites_for(region,vmm::HexagonalGridSource(0.5,{0.25,0.25})));
        else sources.push_back(vmm::sites_for(region,vmm::CartesianGridSource(0.5,{0.25,0.25})));
        const auto sites = vmm::generate_sites(*domain,sources);
        ASSERT_TRUE(sites);
        vmm::Declaration2D rotated;
        const auto rm = *rotated.media().add("water");
        ASSERT_TRUE(rotated.add_region("block",rm,vmm::PolygonShape({transform({0,0}),transform({4,0}),transform({4,2.1}),transform({0,2.1})})));
        ASSERT_TRUE(rotated.add_hole(vmm::PolygonShape({transform({0.6,1}),transform({3.4,1}),transform({3.4,1+thickness}),transform({0.6,1+thickness})})));
        const auto partition = backend.build_partition(rotated);
        ASSERT_TRUE(partition);
        vmm::SiteSet moved;
        for (const auto point : sites->positions()) moved.add(transform(point),region);
        const auto result = vmm::build_mesh_2d(*partition,moved,backend);
        ASSERT_TRUE(result) << result.error().message();
        vmm::test::expect_invariants(*partition,*result,"rotated thin obstacle");
    }
}
