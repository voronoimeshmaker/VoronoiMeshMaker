// ============================================================================
// File: ut_Tolerance.cpp
// Description: Tolerance relative to the length scale.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <limits>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/tolerance.hpp>

namespace {

TEST(Tolerance, RejectsInvalidScales) {
    EXPECT_FALSE(vmm::Tolerance::from_length(0.0));
    EXPECT_FALSE(vmm::Tolerance::from_length(-1.0));
    EXPECT_FALSE(vmm::Tolerance::from_length(std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(vmm::Tolerance::from_length(std::numeric_limits<double>::quiet_NaN()));
    EXPECT_FALSE(vmm::Tolerance::from_length(1.0, 0.0));
}

TEST(Tolerance, ScalesWithTheLength) {
    const auto t = vmm::Tolerance::from_length(1e6);
    ASSERT_TRUE(t);
    EXPECT_DOUBLE_EQ(t->length_scale(), 1e6);
    EXPECT_DOUBLE_EQ(t->relative(), 1e-12);
    EXPECT_DOUBLE_EQ(t->point(), 1e-6);
    EXPECT_DOUBLE_EQ(t->measure(2), 1.0);
}

TEST(Tolerance, SamePoint) {
    const auto t = vmm::Tolerance::from_length(1.0, 1e-9);
    ASSERT_TRUE(t);
    EXPECT_TRUE(t->same_point(vmm::Vec2{0, 0}, vmm::Vec2{0, 5e-10}));
    EXPECT_FALSE(t->same_point(vmm::Vec2{0, 0}, vmm::Vec2{0, 2e-9}));
}

}  // namespace
