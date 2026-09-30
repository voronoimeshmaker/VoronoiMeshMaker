// ============================================================================
// File: ut_MultiRegion3D.cpp
// Description: P18 end to end: 3D regions and holes by precedence, conforming
//              interfaces (common refinement), checked with the DEC-011
//              invariants including the area of every interface.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <span>
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
#include <vmm/vmm.hpp>

namespace {

using vmm::Cuboid;
using vmm::RegionId;
using vmm::UniformRandomSource3D;
using vmm::Vec3;

vmm::MeshResult3D generate(vmm::MeshRequest3D& req, const std::vector<double>& spacing) {
    req.sources.clear();
    for (std::size_t r = 0; r < spacing.size(); ++r) {
        req.sources.push_back(vmm::sites_for_3d(RegionId::from_index(r), UniformRandomSource3D(spacing[r])));
    }
    auto res = vmm::generate_mesh_3d(req);
    if (!res) {
        ADD_FAILURE() << res.error().message();
        return {};
    }
    return std::move(*res);
}

TEST(MultiRegion3D, CubeWithACore) {
    vmm::MeshRequest3D req;
    const auto m = *req.declaration.media().add("rock");
    ASSERT_TRUE(req.declaration.add_region("shell", m, Cuboid({0, 0, 0}, {1, 1, 1})));
    ASSERT_TRUE(req.declaration.add_region("core", m, Cuboid({0.3, 0.3, 0.3}, {0.7, 0.7, 0.7})));
    const auto res = generate(req, {0.12, 0.08});
    EXPECT_GT(res.invariants.interface_faces, 0u);
    EXPECT_EQ(res.stats.interface_faces, res.invariants.interface_faces);
    EXPECT_LT(res.invariants.max_interface_relative_error, 1e-12);
    std::size_t core = 0;
    for (const auto c : res.mesh.cells()) core += res.mesh.region(c) == RegionId::from_index(1);
    EXPECT_GT(core, 0u);
    EXPECT_LT(core, res.mesh.cell_count());
}

TEST(MultiRegion3D, SphereInACubeAndAHole) {
    vmm::MeshRequest3D req{vmm::Declaration3D({48, 2}), {}, {}, {}};
    const auto m = *req.declaration.media().add("m");
    ASSERT_TRUE(req.declaration.add_region("cube", m, Cuboid({0, 0, 0}, {1, 1, 1})));
    ASSERT_TRUE(req.declaration.add_region("ball", m, vmm::Sphere({0.5, 0.5, 0.5}, 0.3)));
    ASSERT_TRUE(req.declaration.add_hole(Cuboid({0.8, 0.8, 0}, {1, 1, 1})));
    const auto res = generate(req, {0.1, 0.07});
    EXPECT_NEAR(res.partition.total_volume(), 1 - 0.04, 1e-12);
    EXPECT_GT(res.invariants.interface_faces, 0u);
}

TEST(MultiRegion3D, CoplanarHalvesAndThreeLayers) {
    vmm::MeshRequest3D req;
    const auto m = *req.declaration.media().add("m");
    ASSERT_TRUE(req.declaration.add_region("low", m, Cuboid({0, 0, 0}, {1, 1, 1})));
    ASSERT_TRUE(req.declaration.add_region("mid", m, Cuboid({0, 0, 0.35}, {1, 1, 1})));
    ASSERT_TRUE(req.declaration.add_region("top", m, Cuboid({0, 0, 0.7}, {1, 1, 1}, {"", "", "", "", "", "sky"})));
    const auto res = generate(req, {0.1, 0.09, 0.11});
    EXPECT_EQ(res.partition.region_count(), 3u);
    EXPECT_NEAR(res.partition.interface_area(RegionId::from_index(0), RegionId::from_index(1)), 1.0, 1e-14);
    EXPECT_NEAR(res.partition.interface_area(RegionId::from_index(1), RegionId::from_index(2)), 1.0, 1e-14);
    EXPECT_NEAR(res.partition.interface_area(RegionId::from_index(0), RegionId::from_index(2)), 0.0, 1e-14);
    const auto sky = std::ranges::find_if(res.mesh.patches(), [](const auto& p) { return p.name == "sky"; });
    ASSERT_NE(sky, res.mesh.patches().end());
    EXPECT_GT(sky->count, 0u);
}

TEST(MultiRegion3D, InputOrderGivesTheSameMesh) {
    vmm::MeshRequest3D req;
    const auto m = *req.declaration.media().add("m");
    ASSERT_TRUE(req.declaration.add_region("a", m, Cuboid({0, 0, 0}, {1, 1, 1})));
    ASSERT_TRUE(req.declaration.add_region("b", m, vmm::Sphere({0.5, 0.5, 0.5}, 0.35)));
    const auto base = generate(req, {0.12, 0.1});
    // The same sites, reversed, as explicit sources per region.
    std::vector<std::vector<Vec3>> sites(2);
    for (const auto c : base.mesh.cells()) sites[base.mesh.region(c).index()].push_back(base.mesh.site(c));
    req.sources.clear();
    for (std::size_t r = 0; r < 2; ++r) {
        std::ranges::reverse(sites[r]);
        req.sources.push_back(vmm::sites_for_3d(RegionId::from_index(r), vmm::ExplicitSites3D(sites[r])));
    }
    const auto again = vmm::generate_mesh_3d(req);
    ASSERT_TRUE(again) << again.error().message();
    EXPECT_EQ(again->mesh.data().points, base.mesh.data().points);
    EXPECT_EQ(again->mesh.data().face_vertices.values, base.mesh.data().face_vertices.values);
    EXPECT_EQ(again->mesh.data().owner, base.mesh.data().owner);
    EXPECT_EQ(again->mesh.data().neighbour, base.mesh.data().neighbour);
}

/// Largest angle between the area vector and the segment between the two sites,
/// and the area-weighted mean, over the interface faces.
std::pair<double, double> interface_nonortho(const vmm::Mesh3D& m) {
    double worst = 0;
    double weighted = 0;
    double area = 0;
    for (const auto f : m.internal_faces()) {
        if (m.region(m.owner(f)) == m.region(m.neighbour(f))) continue;
        const auto g = vmm::face_geometry(m, f);
        const Vec3 d = m.site(m.neighbour(f)) - m.site(m.owner(f));
        const double a = vmm::norm(g.area_vector);
        const double theta = std::acos(std::clamp(vmm::dot(d, g.area_vector) / (vmm::norm(d) * a), -1.0, 1.0));
        worst = std::max(worst, theta);
        weighted += a * theta;
        area += a;
    }
    return {worst, weighted / area};
}

TEST(MultiRegion3D, MirroredPairsMakeInterfaceFacesOrthogonal) {
    for (const bool ball : {false, true}) {
        vmm::MeshRequest3D req{vmm::Declaration3D({48, 2}), {}, {}, {}};
        const auto m = *req.declaration.media().add("m");
        ASSERT_TRUE(req.declaration.add_region("a", m, Cuboid({0, 0, 0}, {1, 1, 1})));
        if (ball) {
            ASSERT_TRUE(req.declaration.add_region("b", m, vmm::Sphere({0.5, 0.5, 0.5}, 0.3)));
        } else {
            ASSERT_TRUE(req.declaration.add_region("b", m, Cuboid({0, 0, 0.5}, {1, 1, 1})));
        }
        const auto plain = generate(req, {0.1, 0.1});
        req.sites.interface_pairs = vmm::InterfacePairs3D(0.1);
        const auto paired = generate(req, {0.1, 0.1});
        const auto [w0, m0] = interface_nonortho(plain.mesh);
        const auto [w1, m1] = interface_nonortho(paired.mesh);
        std::printf("%s: without pairs max %.3f mean %.3f rad | with pairs max %.3f mean %.3f rad\n",
                    ball ? "sphere" : "flat", w0, m0, w1, m1);
        EXPECT_LT(m1, 0.5 * m0);
    }
}

}  // namespace
