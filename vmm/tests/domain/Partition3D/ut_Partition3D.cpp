// ============================================================================
// File: ut_Partition3D.cpp
// Description: Partition3D: measures, region surfaces and interfaces of a two-region partition.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/partition3d.hpp>
#include <vmm/domain/shapes3d.hpp>

namespace {

using vmm::Partition3D;
using vmm::PartitionTriangle;
using vmm::RegionId;
using vmm::Vec3;

/// Two unit cubes side by side along x, sharing the square x = 1 (two triangles).
Partition3D two_cubes() {
    std::vector<Vec3> v;
    for (int k = 0; k < 12; ++k) v.push_back({static_cast<double>(k % 3), static_cast<double>((k / 3) % 2), static_cast<double>(k / 6)});
    const auto id = [](int x, int y, int z) { return static_cast<std::uint32_t>(x + 3 * y + 6 * z); };
    const RegionId a = RegionId::from_index(0);
    const RegionId b = RegionId::from_index(1);
    const RegionId none = RegionId::invalid();
    const vmm::PatchId p = vmm::PatchId::from_index(0);
    std::vector<PartitionTriangle> t;
    const auto quad = [&](std::uint32_t q0, std::uint32_t q1, std::uint32_t q2, std::uint32_t q3, RegionId in, RegionId out) {
        t.push_back({{q0, q1, q2}, in, out, out.valid() ? vmm::PatchId::invalid() : p});
        t.push_back({{q0, q2, q3}, in, out, out.valid() ? vmm::PatchId::invalid() : p});
    };
    for (int c = 0; c < 2; ++c) {
        const RegionId r = c == 0 ? a : b;
        const int x0 = c;
        const int x1 = c + 1;
        if (c == 0) quad(id(x0, 0, 0), id(x0, 0, 1), id(x0, 1, 1), id(x0, 1, 0), r, none);   // x-
        if (c == 1) quad(id(x1, 0, 0), id(x1, 1, 0), id(x1, 1, 1), id(x1, 0, 1), r, none);   // x+
        quad(id(x0, 0, 0), id(x1, 0, 0), id(x1, 0, 1), id(x0, 0, 1), r, none);              // y-
        quad(id(x0, 1, 0), id(x0, 1, 1), id(x1, 1, 1), id(x1, 1, 0), r, none);              // y+
        quad(id(x0, 0, 0), id(x0, 1, 0), id(x1, 1, 0), id(x1, 0, 0), r, none);              // z-
        quad(id(x0, 0, 1), id(x1, 0, 1), id(x1, 1, 1), id(x0, 1, 1), r, none);              // z+
    }
    quad(id(1, 0, 0), id(1, 1, 0), id(1, 1, 1), id(1, 0, 1), a, b);  // interface, outward from a
    return Partition3D(v, t, {{"a", vmm::MediumId::from_index(0)}, {"b", vmm::MediumId::from_index(0)}}, {"m"}, {"wall"});
}

TEST(Partition3D, Measures) {
    const auto p = two_cubes();
    EXPECT_EQ(p.region_count(), 2u);
    EXPECT_DOUBLE_EQ(p.region_volume(RegionId::from_index(0)), 1.0);
    EXPECT_DOUBLE_EQ(p.region_volume(RegionId::from_index(1)), 1.0);
    EXPECT_DOUBLE_EQ(p.total_volume(), 2.0);
    EXPECT_DOUBLE_EQ(p.boundary_area(), 10.0);
    EXPECT_DOUBLE_EQ(p.interface_area(RegionId::from_index(1), RegionId::from_index(0)), 1.0);
    EXPECT_DOUBLE_EQ(p.length_scale(), std::sqrt(6.0));
    EXPECT_EQ(p.patches().front(), "wall");
    EXPECT_EQ(p.media().front(), "m");
    EXPECT_EQ(p.vertices().size(), 12u);
    EXPECT_EQ(p.regions()[1].name, "b");
}

TEST(Partition3D, RegionSurfaces) {
    const auto p = two_cubes();
    for (std::size_t r = 0; r < 2; ++r) {
        const auto s = p.region_surface(RegionId::from_index(r));
        ASSERT_TRUE(s) << s.error().message();
        EXPECT_DOUBLE_EQ(s->volume(), 1.0);
        EXPECT_EQ(s->triangle_count(), 12u);
        EXPECT_EQ(s->patches().back(), "interface");
    }
    EXPECT_EQ(p.region_surface(RegionId::from_index(2)).error().code(), vmm::ErrorCode::InvalidArgument);
    EXPECT_EQ(p.region_surface(RegionId::invalid()).error().code(), vmm::ErrorCode::InvalidArgument);
}

TEST(Partition3D, FromAShape) {
    const auto s = vmm::Cuboid({0, 0, 0}, {2, 1, 1}).surface({});
    ASSERT_TRUE(s);
    std::vector<PartitionTriangle> t;
    for (std::size_t k = 0; k < s->triangle_count(); ++k) {
        t.push_back({s->triangles()[k], RegionId::from_index(0), RegionId::invalid(), vmm::PatchId::from_index(0)});
    }
    const Partition3D p(s->points(), t, {{"r", vmm::MediumId::from_index(0)}}, {"m"}, s->patches());
    EXPECT_DOUBLE_EQ(p.total_volume(), 2.0);
    EXPECT_DOUBLE_EQ(p.boundary_area(), 10.0);
    EXPECT_DOUBLE_EQ(p.interface_area(RegionId::from_index(0), RegionId::from_index(1)), 0.0);
}

}  // namespace
