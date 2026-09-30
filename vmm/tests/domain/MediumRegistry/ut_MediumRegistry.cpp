// ============================================================================
// File: ut_MediumRegistry.cpp
// Description: Open medium registry.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/declaration.hpp>

namespace {

TEST(MediumRegistry, AddFindNames) {
    vmm::MediumRegistry m;
    const auto water = m.add("water");
    const auto soil = m.add("soil");
    ASSERT_TRUE(water && soil);
    EXPECT_EQ(water->index(), 0u);
    EXPECT_EQ(m.find("soil"), soil);
    EXPECT_FALSE(m.find("air"));
    EXPECT_EQ(m.name(*soil), "soil");
    EXPECT_EQ(m.size(), 2u);
    EXPECT_EQ(m.names().front(), "water");
    EXPECT_EQ(m.add("water").error().code(), vmm::ErrorCode::DuplicateName);
    EXPECT_EQ(m.add("").error().code(), vmm::ErrorCode::InvalidArgument);
}

}  // namespace
