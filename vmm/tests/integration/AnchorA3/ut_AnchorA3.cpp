// ============================================================================
// File: ut_AnchorA3.cpp
// Description: Anchor A3 (P04 §2.3, P18): soil block with a curved river,
//              four regions by precedence, interfaces that meet along triple
//              lines; analytic channel volume and water/air interface area.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "anchors/anchors.hpp"
#include <vmm/vmm.hpp>

namespace {

using vmm::RegionId;

TEST(AnchorA3, SoilBlockWithRiver) {
    auto a = vmm::anchors::a3();
    vmm::MeshRequest3D req{std::move(a.declaration), std::move(a.sources), {}, {}};
    const auto res = vmm::generate_mesh_3d(req);
    ASSERT_TRUE(res) << res.error().message();
    const auto& p = res->partition;
    ASSERT_EQ(p.region_count(), 4u);
    const RegionId inf{0}, sup{1}, air{2}, canal{3};
    EXPECT_NEAR(p.region_volume(canal), 69000.0, 1e-9 * 69000);       // integral of (40 - 2d) d dx
    EXPECT_NEAR(p.interface_area(canal, air), 20000.0, 1e-9 * 20000);  // 40 m wide along 500 m
    EXPECT_NEAR(p.total_volume(), 500.0 * 200 * 30, 1e-9 * 3e6);
    EXPECT_GT(p.interface_area(canal, sup), 0.0);
    EXPECT_GT(p.interface_area(canal, inf), 0.0);  // downstream the bed cuts the lower soil
    EXPECT_GT(p.interface_area(sup, air), 0.0);
    EXPECT_GT(p.interface_area(inf, sup), 0.0);
    // Triple lines: some channel cell touches both soil and air.
    std::vector<std::set<std::uint32_t>> touches(res->mesh.cell_count());
    for (const auto f : res->mesh.internal_faces()) {
        const auto o = res->mesh.owner(f);
        const auto n = res->mesh.neighbour(f);
        touches[o.index()].insert(res->mesh.region(n).value);
        touches[n.index()].insert(res->mesh.region(o).value);
    }
    bool triple = false;
    for (const auto c : res->mesh.cells()) {
        if (res->mesh.region(c) != canal) continue;
        const auto& t = touches[c.index()];
        triple = triple || (t.contains(air.value) && (t.contains(sup.value) || t.contains(inf.value)));
    }
    EXPECT_TRUE(triple);
    std::set<std::string> patches;
    for (const auto& pr : res->mesh.patches()) {
        if (pr.count > 0) patches.insert(pr.name);
    }
    EXPECT_EQ(patches, (std::set<std::string>{"base", "jusante", "margem_dir", "margem_esq", "montante", "topo_ar"}));
    EXPECT_GT(res->stats.interface_faces, 0u);
}

}  // namespace
