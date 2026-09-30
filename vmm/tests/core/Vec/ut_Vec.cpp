// ============================================================================
// File: ut_Vec.cpp
// Description: Vector operations and Id of core/types.hpp.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <numbers>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>

namespace {

TEST(Vec, Arithmetic) {
    const vmm::Vec3 a{1, 2, 3};
    const vmm::Vec3 b{4, 5, 6};
    EXPECT_EQ(a + b, (vmm::Vec3{5, 7, 9}));
    EXPECT_EQ(b - a, (vmm::Vec3{3, 3, 3}));
    EXPECT_EQ(2.0 * a, (vmm::Vec3{2, 4, 6}));
    EXPECT_DOUBLE_EQ(vmm::dot(a, b), 32.0);
    EXPECT_DOUBLE_EQ(vmm::norm(vmm::Vec2{3, 4}), 5.0);
    EXPECT_EQ(vmm::cross(vmm::Vec3{1, 0, 0}, vmm::Vec3{0, 1, 0}), (vmm::Vec3{0, 0, 1}));
    EXPECT_DOUBLE_EQ(vmm::cross(vmm::Vec2{1, 0}, vmm::Vec2{0, 2}), 2.0);
}

TEST(Vec, AngleBetween) {
    EXPECT_DOUBLE_EQ(vmm::angle_between(vmm::Vec2{1, 0}, vmm::Vec2{0, 1}), std::numbers::pi / 2);
    EXPECT_DOUBLE_EQ(vmm::angle_between(vmm::Vec3{1, 0, 0}, vmm::Vec3{-1, 0, 0}), std::numbers::pi);
    EXPECT_NEAR(vmm::angle_between(vmm::Vec2{1, 1e-12}, vmm::Vec2{1, 0}), 1e-12, 1e-24);
}

TEST(Vec, IdValidityAndOrder) {
    const vmm::CellId invalid;
    EXPECT_FALSE(invalid.valid());
    EXPECT_EQ(invalid, vmm::CellId::invalid());
    const auto a = vmm::CellId::from_index(3);
    EXPECT_TRUE(a.valid());
    EXPECT_EQ(a.index(), 3u);
    EXPECT_LT(a, vmm::CellId::from_index(4));
}

}  // namespace
