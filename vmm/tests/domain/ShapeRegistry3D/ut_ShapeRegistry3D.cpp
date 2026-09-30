// ============================================================================
// File: ut_ShapeRegistry3D.cpp
// Description: ShapeRegistry3D: built-in shapes by name, user shapes, errors.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <string>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes3d.hpp>

namespace {

using vmm::ShapeParameters;
using vmm::ShapeRegistry3D;

TEST(ShapeRegistry3D, BuiltinShapes) {
    const auto r = ShapeRegistry3D::with_builtin_shapes();
    EXPECT_TRUE(r.contains("cuboid"));
    ShapeParameters box;
    box.numbers = {{"lo", {0, 0, 0}}, {"hi", {1, 2, 3}}};
    box.texts = {{"z+", "top"}};
    const auto b = r.make("cuboid", box);
    ASSERT_TRUE(b) << b.error().message();
    EXPECT_DOUBLE_EQ(b->volume(), 6.0);
    ShapeParameters ball;
    ball.numbers = {{"center", {0, 0, 0}}, {"radius", {1}}};
    EXPECT_TRUE(r.make("sphere", ball));
    ShapeParameters tube;
    tube.numbers = {{"base", {0, 0, 0}}, {"radius", {1}}, {"height", {2}}};
    EXPECT_TRUE(r.make("cylinder", tube));
}

TEST(ShapeRegistry3D, MissingParametersAndUnknownNames) {
    const auto r = ShapeRegistry3D::with_builtin_shapes();
    ShapeParameters none;
    EXPECT_EQ(r.make("cuboid", none).error().code(), vmm::ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(r.make("sphere", none).error().code(), vmm::ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(r.make("cylinder", none).error().code(), vmm::ErrorCode::InvalidShapeParameter);
    ShapeParameters half;
    half.numbers = {{"lo", {0, 0, 0}}, {"center", {0, 0, 0}}, {"base", {0, 0, 0}}, {"radius", {1}}};
    EXPECT_EQ(r.make("cuboid", half).error().code(), vmm::ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(r.make("sphere", ShapeParameters{{{"center", {0, 0, 0}}}, {}}).error().code(),
              vmm::ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(r.make("cylinder", half).error().code(), vmm::ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(r.make("cylinder", ShapeParameters{{{"base", {0, 0, 0}}}, {}}).error().code(),
              vmm::ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(r.make("torus", none).error().code(), vmm::ErrorCode::UnknownShape);
}

TEST(ShapeRegistry3D, UserShapes) {
    auto r = ShapeRegistry3D::with_builtin_shapes();
    const auto unit = [](const ShapeParameters&, const vmm::PolygonizeOptions3& o) {
        return vmm::Cuboid({0, 0, 0}, {1, 1, 1}).surface(o);
    };
    EXPECT_TRUE(r.add("unit", unit));
    EXPECT_EQ(r.add("unit", unit).error().code(), vmm::ErrorCode::DuplicateName);
    EXPECT_EQ(r.add("", unit).error().code(), vmm::ErrorCode::InvalidArgument);
    EXPECT_EQ(r.add("none", {}).error().code(), vmm::ErrorCode::InvalidArgument);
    EXPECT_TRUE(r.make("unit", {}));
}

}  // namespace
