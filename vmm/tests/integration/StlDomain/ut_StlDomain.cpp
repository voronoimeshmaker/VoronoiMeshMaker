// ============================================================================
// File: ut_StlDomain.cpp
// Description: P17 end to end: a terrain block written as binary and ASCII
//              STL, read back, repaired and meshed; invariants against the
//              surface read from the file; the local clipping is used.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <filesystem>
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "anchors/anchors.hpp"
#include <vmm/io/stl.hpp>
#include <vmm/vmm.hpp>

namespace {

void mesh_from_stl(bool binary) {
    const auto terrain = vmm::anchors::terrain_block(12);
    const auto path = std::filesystem::temp_directory_path() / (binary ? "vmm_ut_terrain_b.stl" : "vmm_ut_terrain_a.stl");
    ASSERT_TRUE(vmm::write_stl(terrain, path, {binary}));
    vmm::SurfaceRepairReport report;
    const auto surface = vmm::read_stl_surface(path, {"ground"}, {}, &report);
    std::filesystem::remove(path);
    ASSERT_TRUE(surface) << surface.error().message();
    EXPECT_EQ(surface->points().size(), terrain.points().size());
    EXPECT_EQ(report.components, 1u);
    EXPECT_NEAR(surface->volume(), terrain.volume(), 1e-6);  // binary STL rounds to float
    if (!binary) {
        EXPECT_EQ(surface->patches(), (std::vector<std::string>{"terrain", "rock"}));
    }

    vmm::MeshRequest3D req;
    const auto m = *req.declaration.media().add("soil");
    const auto r = req.declaration.add_region("ground", m, vmm::SurfaceShape(*surface));
    ASSERT_TRUE(r);
    req.sources = {vmm::sites_for_3d(*r, vmm::UniformRandomSource3D(0.08))};
    const auto res = vmm::generate_mesh_3d(req);
    ASSERT_TRUE(res) << res.error().message();
    EXPECT_GT(res->stats.clipped_cells, 0u);
    EXPECT_GT(res->stats.local_clips, res->stats.clipped_cells / 2);
    EXPECT_EQ(res->mesh.patches().size(), binary ? 1u : 2u);
}

TEST(StlDomain, BinaryTerrain) { mesh_from_stl(true); }

TEST(StlDomain, AsciiTerrainKeepsPatches) { mesh_from_stl(false); }

}  // namespace
