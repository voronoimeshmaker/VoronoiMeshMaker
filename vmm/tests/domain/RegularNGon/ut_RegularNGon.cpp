// ============================================================================
// File: ut_RegularNGon.cpp
// Description: Regular polygon.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes.hpp>

namespace {

TEST(RegularNGon, SquareAsFourGon) {
    const auto o = vmm::RegularNGon(vmm::Vec2{0, 0}, 1, 4).outline({});
    ASSERT_TRUE(o);
    EXPECT_NEAR(o->area(), 2.0, 1e-15);
    EXPECT_FALSE(vmm::RegularNGon(vmm::Vec2{0, 0}, 1, 2).outline({}));
    EXPECT_FALSE(vmm::RegularNGon(vmm::Vec2{0, 0}, -1, 5).outline({}));
}

}  // namespace
