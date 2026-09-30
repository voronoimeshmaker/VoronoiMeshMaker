// ============================================================================
// File: ut_Anchors.cpp
// Description: P12 end-to-end on the anchor problems (DEC-017): A1, A1 with
//              air (triple junctions) and A2 generated, checked (DEC-011),
//              written and read back through the native format, written as
//              .vtu. Expected interface lengths are analytic where known.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <sstream>
#include <string>
#include <utility>

//==============================================================================
//  External libraries
//==============================================================================
#include <anchors/anchors.hpp>
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/io/native.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/vmm.hpp>

namespace {

vmm::MeshResult2D generate(vmm::anchors::Anchor anchor) {
    vmm::MeshRequest2D request{std::move(anchor.declaration), std::move(anchor.sources), {}, {}, {}};
    auto result = vmm::generate_mesh_2d(request);
    EXPECT_TRUE(result) << (result ? "" : result.error().message());
    return result ? std::move(*result) : vmm::MeshResult2D{};
}

void round_trip(const vmm::Mesh2D& m) {
    std::stringstream native;
    ASSERT_TRUE(vmm::write_native(m, native));
    const auto back = vmm::read_native<2>(native);
    ASSERT_TRUE(back) << back.error().message();
    EXPECT_EQ(back->points(), m.points());
    EXPECT_TRUE(std::ranges::equal(back->owners(), m.owners()));
    std::stringstream vtu;
    EXPECT_TRUE(vmm::write_vtu(m, vtu));
}

TEST(Anchors, A1) {
    const auto r = generate(vmm::anchors::a1(false, 2));
    const auto& p = r.partition;
    EXPECT_NEAR(p.interface_length(vmm::RegionId{1}, vmm::RegionId{2}), 20 + 2 * std::sqrt(125.0), 1e-12);
    EXPECT_NEAR(p.interface_length(vmm::RegionId{0}, vmm::RegionId{1}), 200, 1e-12);
    EXPECT_GT(r.invariants.interface_faces, 0u);
    round_trip(r.mesh);
}

TEST(Anchors, A1WithAirHasTripleJunctions) {
    const auto r = generate(vmm::anchors::a1(true, 2));
    const auto& p = r.partition;
    ASSERT_EQ(p.region_count(), 4u);
    // Water/air and soil/air interfaces: 40 m and 160 m.
    EXPECT_NEAR(p.interface_length(vmm::RegionId{2}, vmm::RegionId{3}), 40, 1e-12);
    EXPECT_NEAR(p.interface_length(vmm::RegionId{1}, vmm::RegionId{3}), 160, 1e-12);
    // Triple junctions: partition vertices shared by segments of three regions.
    std::size_t junctions = 0;
    for (std::size_t v = 0; v < p.vertices().size(); ++v) {
        std::set<std::uint32_t> regions;
        for (const auto& s : p.segments()) {
            if (s.v0 == v || s.v1 == v) {
                for (const auto r2 : {s.left, s.right}) if (r2.valid()) regions.insert(r2.value);
            }
        }
        junctions += regions.size() >= 3 ? 1u : 0u;
    }
    EXPECT_EQ(junctions, 2u);
    round_trip(r.mesh);
}

TEST(Anchors, A2) {
    const auto r = generate(vmm::anchors::a2(2));
    const auto& p = r.partition;
    ASSERT_EQ(p.region_count(), 3u);
    // The island touches only the channel; the plain is split in two by the river.
    EXPECT_EQ(p.interface_length(vmm::RegionId{0}, vmm::RegionId{2}), 0.0);
    EXPECT_GT(p.interface_length(vmm::RegionId{1}, vmm::RegionId{2}), 100.0);
    EXPECT_EQ(p.components(vmm::RegionId{0}).size(), 2u);
    EXPECT_FALSE(r.validation.warnings().empty());
    // No cell of the island touches the plain.
    for (const vmm::FaceId f : r.mesh.internal_faces()) {
        const auto a = r.mesh.region(r.mesh.owner(f)).value;
        const auto b = r.mesh.region(r.mesh.neighbour(f)).value;
        ASSERT_FALSE((a == 0 && b == 2) || (a == 2 && b == 0));
    }
    round_trip(r.mesh);
}

}  // namespace
