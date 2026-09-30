// ============================================================================
// File: ut_Partition2D.cpp
// Description: Partition2D queries on a hand-built partition: unit square
//              split by x = 0.5 into regions 0 (left) and 1 (right).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <stdexcept>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/partition.hpp>

namespace {

using vmm::PatchId;
using vmm::RegionId;
using vmm::SegmentUse;
using vmm::Vec2;

vmm::Partition2D split_square() {
    // v0 (0,0) v1 (0,1) v2 (0.5,0) v3 (0.5,1) v4 (1,0) v5 (1,1)
    std::vector<Vec2> v{{0, 0}, {0, 1}, {0.5, 0}, {0.5, 1}, {1, 0}, {1, 1}};
    const RegionId r0 = RegionId::from_index(0);
    const RegionId r1 = RegionId::from_index(1);
    const RegionId out = RegionId::invalid();
    const PatchId p = PatchId::from_index(0);
    std::vector<vmm::PartitionSegment> s{
        {0, 2, r0, out, p}, {2, 4, r1, out, p}, {4, 5, r1, out, p}, {3, 5, out, r1, p},
        {1, 3, out, r0, p}, {0, 1, out, r0, p}, {2, 3, r0, r1, PatchId::invalid()}};
    vmm::RegionComponent left{{{SegmentUse{0, false}, SegmentUse{6, false}, SegmentUse{4, true}, SegmentUse{5, true}}}};
    vmm::RegionComponent right{{{SegmentUse{1, false}, SegmentUse{2, false}, SegmentUse{3, true}, SegmentUse{6, true}}}};
    return vmm::Partition2D(v, s, {{"left", vmm::MediumId::from_index(0)}, {"right", vmm::MediumId::from_index(0)}},
                            {"m"}, {"boundary"}, {{left}, {right}}, {});
}

TEST(Partition2D, Classification) {
    const auto p = split_square();
    EXPECT_EQ(p.region_count(), 2u);
    EXPECT_TRUE(p.is_interface(6));
    EXPECT_FALSE(p.is_boundary(6));
    EXPECT_TRUE(p.is_boundary(0));
    EXPECT_FALSE(p.is_interface(0));
    EXPECT_EQ(p.patches().front(), "boundary");
    EXPECT_EQ(p.media().front(), "m");
    EXPECT_TRUE(p.voids().empty());
}

TEST(Partition2D, Measures) {
    const auto p = split_square();
    EXPECT_DOUBLE_EQ(p.region_area(RegionId::from_index(0)), 0.5);
    EXPECT_DOUBLE_EQ(p.total_area(), 1.0);
    EXPECT_DOUBLE_EQ(p.boundary_length(), 4.0);
    EXPECT_DOUBLE_EQ(p.interface_length(), 1.0);
    EXPECT_DOUBLE_EQ(p.interface_length(RegionId::from_index(1), RegionId::from_index(0)), 1.0);
    EXPECT_DOUBLE_EQ(p.interface_length(RegionId::from_index(0), RegionId::from_index(0)), 0.0);
    EXPECT_DOUBLE_EQ(p.length_scale(), std::sqrt(2.0));
    EXPECT_DOUBLE_EQ(p.segment_length(6), 1.0);
}

TEST(Partition2D, LoopsAndPolygons) {
    const auto p = split_square();
    const auto& loop = p.components(RegionId::from_index(1))[0].loops[0];
    EXPECT_EQ(p.start(loop[2]), (Vec2{1, 1}));
    EXPECT_EQ(p.end(loop[2]), (Vec2{0.5, 1}));
    EXPECT_EQ(p.loop_points(loop).size(), 4u);
    EXPECT_DOUBLE_EQ(p.component_polygon(RegionId::from_index(1), 0).area(), 0.5);
    EXPECT_THROW((void)p.components(RegionId::from_index(5)), std::out_of_range);
}

TEST(Partition2D, DefaultIsEmpty) {
    const vmm::Partition2D p;
    EXPECT_EQ(p.region_count(), 0u);
    EXPECT_EQ(p.total_area(), 0.0);
    EXPECT_EQ(p.length_scale(), 0.0);
}

}  // namespace
