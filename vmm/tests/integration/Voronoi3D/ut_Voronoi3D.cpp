// ============================================================================
// File: ut_Voronoi3D.cpp
// Description: 3D construction end to end (P16): the P15a cases through the
//              library (convex, curved, non-convex, sharp edges, complex
//              boundary cells, near-degenerate sites, input order, scale),
//              each checked with the DEC-011 invariants; the facade, the
//              native round trip and the VTU writer.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <span>
#include <sstream>
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
#include <vmm/backend/cgal.hpp>
#include <vmm/io/native.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/vmm.hpp>

namespace {

vmm::SiteGenerationOptions3D seeded(std::uint64_t seed) {
    vmm::SiteGenerationOptions3D o;
    o.seed = seed;
    return o;
}

using vmm::Real;
using vmm::RegionId;
using vmm::Vec2;
using vmm::Vec3;

struct Outcome {
    vmm::Build3D build;
    vmm::InvariantReport report;
    vmm::InvariantReference reference;
};

vmm::Declaration3D one_region(const vmm::TriangleSurface& s) {
    vmm::Declaration3D d;
    const auto m = *d.media().add("rock");
    EXPECT_TRUE(d.add_region_surface("domain", m, s));
    return d;
}

template <vmm::Shape3D S>
vmm::TriangleSurface surface_of(const S& shape, vmm::PolygonizeOptions3 o = {}) {
    auto s = shape.surface(o);
    EXPECT_TRUE(s) << s.error().message();
    return *s;
}

/// Builds and checks; `sites` empty means a uniform random source with ~n sites.
Outcome run(const vmm::TriangleSurface& surface, std::vector<Vec3> sites, std::size_t n = 0, std::uint64_t seed = 1) {
    const auto backend = vmm::cgal_backend_3d();
    const auto partition = backend.build_partition(one_region(surface));
    EXPECT_TRUE(partition) << partition.error().message();
    vmm::SiteSet3D set;
    if (sites.empty()) {
        const Real h = std::cbrt(surface.volume() / static_cast<Real>(n));
        const std::vector<vmm::RegionSites3D> src{vmm::sites_for_3d(RegionId::from_index(0), vmm::UniformRandomSource3D(h))};
        auto s = vmm::generate_sites_3d(*partition, src, seeded(seed));
        EXPECT_TRUE(s) << s.error().message();
        set = std::move(*s);
    } else {
        set.append(sites, RegionId::from_index(0));
    }
    auto build = vmm::build_mesh_3d(*partition, set, backend);
    EXPECT_TRUE(build) << build.error().message();
    Outcome o{std::move(*build), {}, vmm::invariant_reference(*partition)};
    o.reference.cell_measure = o.build.cell_volume;
    o.report = vmm::check_invariants(o.build.mesh, o.reference);
    EXPECT_TRUE(o.report.passed(o.reference)) << o.report.first_problem << " | volume " << o.report.total_relative_error
                                              << " closure " << o.report.max_closure;
    EXPECT_EQ(o.build.stats.cells, o.build.mesh.cell_count());
    return o;
}

vmm::TriangleSurface extrusion(std::vector<Vec2> ring, Real z0, Real z1) {
    auto outline = vmm::ShapeOutline::make(std::move(ring), {});
    EXPECT_TRUE(outline) << outline.error().message();
    return surface_of(vmm::Extrusion(*outline, z0, z1));
}

std::vector<Vec2> l_shape() { return {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}}; }

std::vector<Vec2> star(int spikes, Real r_out, Real r_in) {
    std::vector<Vec2> p;
    for (int k = 0; k < 2 * spikes; ++k) {
        const Real a = std::numbers::pi * k / spikes;
        const Real r = k % 2 == 0 ? r_out : r_in;
        p.push_back({r * std::cos(a), r * std::sin(a)});
    }
    return p;
}

TEST(Voronoi3D, CubeRandomSites) {
    const auto o = run(surface_of(vmm::Cuboid({0, 0, 0}, {1, 1, 1})), {}, 600);
    EXPECT_GT(o.build.stats.fast_cells, 0u);
    EXPECT_GT(o.build.stats.clipped_cells, 0u);
    EXPECT_EQ(o.build.mesh.patches().size(), 1u);  // untagged: "boundary"
}

TEST(Voronoi3D, SphereCurvedBoundary) {
    const auto o = run(surface_of(vmm::Sphere({0, 0, 0}, 1), {64, 2}), {}, 500, 2);
    EXPECT_EQ(o.build.stats.fragmented_cells, 0u);
}

TEST(Voronoi3D, NonConvexLShape) { run(extrusion(l_shape(), 0, 1), {}, 600, 3); }

TEST(Voronoi3D, SharpThreeDegreeWedge) {
    const Real a = 3 * std::numbers::pi / 180;
    run(extrusion({{0, 0}, {2, 0}, {2 * std::cos(a), 2 * std::sin(a)}}, 0, 1), {}, 300, 4);
}

TEST(Voronoi3D, CellsSpanningSeveralSpikesStayWhole) {
    const auto o = run(extrusion(star(12, 1, 0.35), 0, 0.3), {}, 150, 6);
    EXPECT_GT(o.build.stats.fragmented_cells, 0u);  // DEC-035
}

TEST(Voronoi3D, CartesianGridCosphericalSites) {
    std::vector<Vec3> grid;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            for (int k = 0; k < 8; ++k) grid.push_back({(i + 0.5) / 8, (j + 0.5) / 8, (k + 0.5) / 8});
        }
    }
    const auto o = run(surface_of(vmm::Cuboid({0, 0, 0}, {1, 1, 1})), grid);
    EXPECT_EQ(o.build.mesh.cell_count(), 512u);
    EXPECT_GT(o.build.stats.merged_vertices, 0u);
    EXPECT_LT(o.report.max_nonortho_internal, 1e-12);
}

TEST(Voronoi3D, SitesVeryCloseToTheBoundary) {
    std::vector<Vec3> sites;
    for (int k = 0; k < 20; ++k) {
        const Real t = (k + 0.5) / 20;
        sites.push_back({1e-9, t, 0.5});
        sites.push_back({t, 1 - 1e-9, 0.37});
        sites.push_back({0.5, t, 0.5 + 0.2 * std::sin(7 * t)});
    }
    run(surface_of(vmm::Cuboid({0, 0, 0}, {1, 1, 1})), sites);
}

TEST(Voronoi3D, InputOrderGivesTheSameMeshBitForBit) {
    const auto surface = extrusion(l_shape(), 0, 1);
    const auto base = run(surface, {}, 400, 10);
    std::vector<Vec3> sites(base.build.mesh.sites().begin(), base.build.mesh.sites().end());
    vmm::Random rng(7);
    rng.shuffle(std::span<Vec3>(sites));
    const auto other = run(surface, sites);
    const auto& a = base.build.mesh.data();
    const auto& b = other.build.mesh.data();
    EXPECT_EQ(a.points, b.points);
    EXPECT_EQ(a.face_vertices.values, b.face_vertices.values);
    EXPECT_EQ(a.face_vertices.offsets, b.face_vertices.offsets);
    EXPECT_EQ(a.owner, b.owner);
    EXPECT_EQ(a.neighbour, b.neighbour);
    for (const auto c : other.build.mesh.cells()) {
        EXPECT_EQ(other.build.mesh.site(c), sites[other.build.mesh.cell_input_sites()[c.index()].index()]);
    }
}

TEST(Voronoi3D, ScaleDoesNotChangeTheTopology) {
    std::vector<Vec3> sites;
    // Named: the range-for lifetime extension of temporaries (P2718) is GCC 15+.
    const auto seed_run = run(extrusion(l_shape(), 0, 1), {}, 300, 11);
    for (const auto& p : seed_run.build.mesh.sites()) sites.push_back(p);
    const auto topology = [](const vmm::Mesh3D& m) {
        return std::pair(std::vector(m.owners().begin(), m.owners().end()),
                         std::vector(m.neighbours().begin(), m.neighbours().end()));
    };
    const auto base = run(extrusion(l_shape(), 0, 1), sites);
    for (const Real s : {1e-3, 1e6}) {
        std::vector<Vec2> ring;
        for (const Vec2& p : l_shape()) ring.push_back(s * p);
        std::vector<Vec3> scaled;
        for (const Vec3& p : sites) scaled.push_back(s * p);
        const auto o = run(extrusion(ring, 0, s), scaled);
        EXPECT_EQ(topology(o.build.mesh), topology(base.build.mesh)) << s;
    }
}

TEST(Voronoi3D, FacadeNativeRoundTripAndVtu) {
    vmm::MeshRequest3D req;
    const auto m = *req.declaration.media().add("water");
    const auto r = req.declaration.add_region("tank", m, vmm::Cylinder({0, 0, 0}, 1, 2, {"wall", "floor", "lid"}));
    ASSERT_TRUE(r) << r.error().message();
    req.sources = {vmm::sites_for_3d(*r, vmm::UniformRandomSource3D(0.35))};
    const auto res = vmm::generate_mesh_3d(req);
    ASSERT_TRUE(res) << res.error().message();
    std::vector<std::string> names;
    for (const auto& patch : res->mesh.patches()) names.push_back(patch.name);
    std::ranges::sort(names);
    EXPECT_EQ(names, (std::vector<std::string>{"floor", "lid", "wall"}));
    std::stringstream native;
    ASSERT_TRUE(vmm::write_native(res->mesh, native));
    const auto back = vmm::read_native<3>(native);
    ASSERT_TRUE(back) << back.error().message();
    EXPECT_EQ(back->data().points, res->mesh.data().points);
    std::stringstream vtu;
    ASSERT_TRUE(vmm::write_vtu(res->mesh, vtu));
    EXPECT_NE(vtu.str().find("Name=\"faceoffsets\""), std::string::npos);
    EXPECT_NE(vtu.str().find("          42\n"), std::string::npos);
}

TEST(Voronoi3D, FacadeErrors) {
    vmm::MeshRequest3D empty;
    EXPECT_EQ(vmm::generate_mesh_3d(empty).error().code(), vmm::ErrorCode::EmptyDeclaration);
    vmm::MeshRequest3D no_sites;
    const auto m = *no_sites.declaration.media().add("m");
    (void)no_sites.declaration.add_region("a", m, vmm::Cuboid({0, 0, 0}, {1, 1, 1}));
    EXPECT_EQ(vmm::generate_mesh_3d(no_sites).error().code(), vmm::ErrorCode::RegionWithoutSites);
    vmm::MeshRequest3D covered;
    const auto mc = *covered.declaration.media().add("m");
    (void)covered.declaration.add_region("a", mc, vmm::Cuboid({0, 0, 0}, {1, 1, 1}));
    (void)covered.declaration.add_region("b", mc, vmm::Cuboid({-1, -1, -1}, {2, 2, 2}));
    EXPECT_EQ(vmm::generate_mesh_3d(covered).error().code(), vmm::ErrorCode::RegionEmptied);
}

}  // namespace
