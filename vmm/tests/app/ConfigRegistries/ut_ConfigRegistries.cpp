// ============================================================================
// File: ut_ConfigRegistries.cpp
// Description: ConfigRegistries: built-in shapes and sources, plus the
//              configuration-only 3D shapes stl and extrusion.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <filesystem>
#include <string>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/registries.hpp>
#include <vmm/io/stl.hpp>

namespace {

using vmm::ShapeParameters;

TEST(ConfigRegistries, BuiltinsOfEveryRegistry) {
    const auto r = vmm::ConfigRegistries::with_builtins();
    EXPECT_TRUE(r.shapes_2d.contains("rectangle"));
    EXPECT_TRUE(r.shapes_3d.contains("cuboid"));
    EXPECT_TRUE(r.shapes_3d.contains("stl"));
    EXPECT_TRUE(r.shapes_3d.contains("extrusion"));
    EXPECT_TRUE(r.sites_2d.contains("hexagonal"));
    EXPECT_TRUE(r.sites_3d.contains("uniform"));
}

TEST(ConfigRegistries, StlShapeReadsAFile) {
    const auto r = vmm::ConfigRegistries::with_builtins();
    const auto cube = vmm::Cuboid({0, 0, 0}, {2, 1, 1}).surface({});
    ASSERT_TRUE(cube);
    const auto path = std::filesystem::temp_directory_path() / "vmm_ut_configregistries.stl";
    ASSERT_TRUE(vmm::write_stl(*cube, path, {true}));
    const auto s = r.shapes_3d.make("stl", ShapeParameters{{}, {{"file", path.string()}, {"patch", "walls"}}});
    std::filesystem::remove(path);
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_NEAR(s->volume(), 2.0, 1e-6);
    ASSERT_EQ(s->patches().size(), 1u);
    EXPECT_EQ(s->patches()[0], "walls");
    EXPECT_EQ(r.shapes_3d.make("stl", {}).error().code(), vmm::ErrorCode::InvalidShapeParameter);
    EXPECT_EQ(r.shapes_3d.make("stl", ShapeParameters{{}, {{"file", path.string()}}}).error().code(),
              vmm::ErrorCode::FileOpenFailed);
}

TEST(ConfigRegistries, ExtrusionShape) {
    const auto r = vmm::ConfigRegistries::with_builtins();
    ShapeParameters p{{{"xy", {0, 0, 2, 0, 2, 1, 0, 1}}, {"z", {0, 3}}}, {{"tag", "side"}, {"top", "roof"}}};
    const auto s = r.shapes_3d.make("extrusion", p);
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_NEAR(s->volume(), 6.0, 1e-12);
    p.texts.clear();
    EXPECT_TRUE(r.shapes_3d.make("extrusion", p));
    const auto bad = vmm::ErrorCode::InvalidShapeParameter;
    EXPECT_EQ(r.shapes_3d.make("extrusion", ShapeParameters{{{"z", {0, 1}}}, {}}).error().code(), bad);
    EXPECT_EQ(r.shapes_3d.make("extrusion", ShapeParameters{{{"xy", {0, 0, 1, 0, 1, 1}}}, {}}).error().code(), bad);
    EXPECT_FALSE(r.shapes_3d.make("extrusion", ShapeParameters{{{"xy", {0, 0, 1, 0}}, {"z", {0, 1}}}, {}}));
}

}  // namespace
