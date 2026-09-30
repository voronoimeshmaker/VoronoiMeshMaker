// ============================================================================
// File: ut_ShapeRegistry.cpp
// Description: Open shape registry.
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

using vmm::ErrorCode;
using vmm::ShapeParameters;

TEST(ShapeRegistry, BuiltinShapes) {
    const auto r = vmm::ShapeRegistry::with_builtin_shapes();
    ShapeParameters rect{{{"lo", {0, 0}}, {"hi", {2, 3}}}, {{"top", "sky"}}};
    const auto a = r.make("rectangle", rect);
    ASSERT_TRUE(a);
    EXPECT_DOUBLE_EQ(a->area(), 6.0);
    EXPECT_EQ(a->outer_tags()[2], "sky");
    EXPECT_TRUE(r.make("polygon", {{{"xy", {0, 0, 1, 0, 0, 1}}}, {}}));
    EXPECT_TRUE(r.make("circle", {{{"center", {0, 0}}, {"radius", {1}}}, {}}));
    EXPECT_TRUE(r.make("ellipse", {{{"center", {0, 0}}, {"axes", {2, 1}}, {"angle", {0.5}}}, {}}));
    EXPECT_TRUE(r.make("regular_ngon", {{{"center", {0, 0}}, {"radius", {1}}, {"n", {6}}}, {}}));
}

TEST(ShapeRegistry, MissingOrBadParameters) {
    const auto r = vmm::ShapeRegistry::with_builtin_shapes();
    EXPECT_EQ(r.make("hexagon", {}).error().code(), ErrorCode::UnknownShape);
    EXPECT_EQ(r.make("rectangle", {{{"lo", {0}}}, {}}).error().code(), ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(r.make("rectangle", {{{"lo", {0, 0}}}, {}}).error().code(), ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(r.make("polygon", {{{"xy", {0, 0, 1}}}, {}}).error().code(), ErrorCode::InvalidShapeParameter);
    EXPECT_FALSE(r.make("polygon", {}));
    EXPECT_FALSE(r.make("circle", {{{"center", {0, 0}}}, {}}));
    EXPECT_FALSE(r.make("circle", {}));
    EXPECT_FALSE(r.make("ellipse", {{{"center", {0, 0}}}, {}}));
    EXPECT_FALSE(r.make("ellipse", {}));
    EXPECT_FALSE(r.make("regular_ngon", {{{"center", {0, 0}}, {"radius", {1}}}, {}}));
    EXPECT_FALSE(r.make("regular_ngon", {{{"center", {0, 0}}}, {}}));
    EXPECT_FALSE(r.make("regular_ngon", {}));
}

TEST(ShapeRegistry, UserShapesCanBeAdded) {
    auto r = vmm::ShapeRegistry::with_builtin_shapes();
    const auto unit = [](const ShapeParameters&, const vmm::PolygonizeOptions& o) {
        return vmm::Rectangle(vmm::Vec2{0, 0}, vmm::Vec2{1, 1}).outline(o);
    };
    EXPECT_TRUE(r.add("unit", unit));
    EXPECT_TRUE(r.contains("unit"));
    EXPECT_EQ(r.add("unit", unit).error().code(), ErrorCode::DuplicateName);
    EXPECT_EQ(r.add("", unit).error().code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(r.add("none", {}).error().code(), ErrorCode::InvalidArgument);
    EXPECT_DOUBLE_EQ(r.make("unit", {})->area(), 1.0);
}

}  // namespace
