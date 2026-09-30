// ============================================================================
// File: ut_IdRange.cpp
// Description: IdRange iteration.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <ranges>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/mesh.hpp>

namespace {

static_assert(std::ranges::forward_range<vmm::IdRange<vmm::FaceTag>>);

TEST(IdRange, IteratesConsecutiveIds) {
    const vmm::IdRange<vmm::FaceTag> r(3, 6);
    std::vector<unsigned> seen;
    for (const vmm::FaceId f : r) seen.push_back(f.value);
    EXPECT_EQ(seen, (std::vector<unsigned>{3, 4, 5}));
    EXPECT_EQ(r.size(), 3u);
    EXPECT_FALSE(r.empty());
    EXPECT_TRUE(r.contains(vmm::FaceId{5}));
    EXPECT_FALSE(r.contains(vmm::FaceId{6}));
    auto it = r.begin();
    EXPECT_EQ((*it++).value, 3u);
    EXPECT_EQ((*it).value, 4u);
    EXPECT_TRUE(vmm::IdRange<vmm::CellTag>().empty());
}

}  // namespace
