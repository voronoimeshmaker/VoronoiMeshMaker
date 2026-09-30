// ============================================================================
// File: ut_ConfigSection.cpp
// Description: ConfigSection::find.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/config.hpp>

namespace {

TEST(ConfigSection, FindsTheValueOfAKey) {
    vmm::ConfigSection s{"region", "soil", 3, {{"shape", "rectangle", 4}, {"lo", "0 0", 5}}};
    ASSERT_TRUE(s.find("lo"));
    EXPECT_EQ(*s.find("lo"), "0 0");
    EXPECT_EQ(*s.find("shape"), "rectangle");
    EXPECT_FALSE(s.find("hi"));
    EXPECT_FALSE(vmm::ConfigSection{}.find("shape"));
}

}  // namespace
