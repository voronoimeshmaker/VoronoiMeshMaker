// ============================================================================
// File: ut_Vtu.cpp
// Description: VTK XML writer and cell loop reconstruction.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstdint>
#include <filesystem>
#include <format>
#include <sstream>
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "random_mesh.hpp"
#include "simple_meshes.hpp"
#include <vmm/geometry/polygon.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/mesh/metrics.hpp>

namespace {

std::size_t count(const std::string& text, const std::string& what) {
    std::size_t n = 0;
    for (auto p = text.find(what); p != std::string::npos; p = text.find(what, p + 1)) ++n;
    return n;
}

TEST(Vtu, CellLoopsMatchCellAreas) {
    const auto b = vmm::test::random_square_mesh(400, 12);
    const auto loops = vmm::cell_loops(b.mesh);
    ASSERT_TRUE(loops);
    const auto x = vmm::compute_metrics(b.mesh);
    for (const vmm::CellId c : b.mesh.cells()) {
        ASSERT_EQ((*loops)[c.index()].size(), 1u);
        std::vector<vmm::Vec2> pts;
        for (const auto v : (*loops)[c.index()][0]) pts.push_back(b.mesh.point(v));
        ASSERT_NEAR(vmm::signed_area(pts), x.cell_measure[c.index()], 1e-15);
    }
}

TEST(Vtu, WritesAWellFormedFile) {
    const auto b = vmm::test::random_square_mesh(100, 13);
    std::stringstream s;
    ASSERT_TRUE(vmm::write_vtu(b.mesh, s));
    const std::string t = s.str();
    EXPECT_NE(t.find(std::format("NumberOfPoints=\"{}\" NumberOfCells=\"{}\"", b.mesh.point_count(), b.mesh.cell_count())),
              std::string::npos);
    for (const char* name : {"region", "medium", "input_site", "loops", "area", "aspect_ratio", "max_nonorthogonality"}) {
        EXPECT_EQ(count(t, std::format("Name=\"{}\"", name)), 1u) << name;
    }
    std::stringstream plain;
    ASSERT_TRUE(vmm::write_vtu(b.mesh, plain, {false}));
    EXPECT_EQ(count(plain.str(), "Name=\"area\""), 0u);
    const auto path = std::filesystem::temp_directory_path() / "vmm_ut.vtu";
    ASSERT_TRUE(vmm::write_vtu(b.mesh, path));
    EXPECT_GT(std::filesystem::file_size(path), 1000u);
    std::filesystem::remove(path);
    EXPECT_FALSE(vmm::write_vtu(b.mesh, std::filesystem::path("/nonexistent/dir/x.vtu")));
}

/// Two unit cubes along x sharing the face x = 1: one internal face and ten
/// boundary faces, each counter-clockwise seen from outside its owner.
vmm::Mesh3D two_cubes() {
    vmm::MeshData<3> d;
    for (int k = 0; k < 12; ++k) {
        d.points.push_back({static_cast<double>(k % 3), static_cast<double>((k / 3) % 2), static_cast<double>(k / 6)});
    }
    const auto id = [](int x, int y, int z) { return vmm::VertexId{static_cast<std::uint32_t>(x + 3 * y + 6 * z)}; };
    const auto face = [&](std::vector<vmm::VertexId> v, std::uint32_t owner) {
        d.face_vertices.push_row(v);
        d.owner.push_back(vmm::CellId{owner});
    };
    face({id(1, 0, 0), id(1, 1, 0), id(1, 1, 1), id(1, 0, 1)}, 0);  // internal, towards cell 1
    d.neighbour.push_back(vmm::CellId{1});
    face({id(0, 0, 0), id(0, 0, 1), id(0, 1, 1), id(0, 1, 0)}, 0);
    face({id(2, 0, 0), id(2, 1, 0), id(2, 1, 1), id(2, 0, 1)}, 1);
    for (std::uint32_t c = 0; c < 2; ++c) {
        const int x0 = static_cast<int>(c);
        const int x1 = x0 + 1;
        face({id(x0, 0, 0), id(x1, 0, 0), id(x1, 0, 1), id(x0, 0, 1)}, c);
        face({id(x0, 1, 0), id(x0, 1, 1), id(x1, 1, 1), id(x1, 1, 0)}, c);
        face({id(x0, 0, 0), id(x0, 1, 0), id(x1, 1, 0), id(x1, 0, 0)}, c);
        face({id(x0, 0, 1), id(x1, 0, 1), id(x1, 1, 1), id(x0, 1, 1)}, c);
    }
    d.patches = {{"wall", 1, 10}};
    d.sites = {{0.5, 0.5, 0.5}, {1.5, 0.5, 0.5}};
    d.cell_region = {vmm::RegionId{0}, vmm::RegionId{0}};
    d.cell_input_site = {vmm::SiteId{0}, vmm::SiteId{1}};
    d.regions = {{"r", vmm::MediumId{0}}};
    d.media = {"m"};
    return *vmm::Mesh3D::from_data(d);
}

TEST(Vtu, WritesPolyhedra) {
    const auto m = two_cubes();
    std::stringstream s;
    ASSERT_TRUE(vmm::write_vtu(m, s));
    const std::string t = s.str();
    EXPECT_NE(t.find("NumberOfPoints=\"12\" NumberOfCells=\"2\""), std::string::npos);
    EXPECT_EQ(count(t, "          42\n"), 2u);
    for (const char* name : {"faces", "faceoffsets", "region", "medium", "input_site", "volume", "aspect_ratio",
                             "max_nonorthogonality"}) {
        EXPECT_EQ(count(t, std::format("Name=\"{}\"", name)), 1u) << name;
    }
    // The shared face (vertices 1 4 10 7) as stored for cell 0 and reversed for cell 1.
    EXPECT_NE(t.find(" 4 1 4 10 7"), std::string::npos);
    EXPECT_NE(t.find(" 4 7 10 4 1"), std::string::npos);
    std::stringstream plain;
    ASSERT_TRUE(vmm::write_vtu(m, plain, {false}));
    EXPECT_EQ(count(plain.str(), "Name=\"volume\""), 0u);
    const auto path = std::filesystem::temp_directory_path() / "vmm_ut3d.vtu";
    ASSERT_TRUE(vmm::write_vtu(m, path));
    EXPECT_GT(std::filesystem::file_size(path), 500u);
    std::filesystem::remove(path);
    EXPECT_FALSE(vmm::write_vtu(m, std::filesystem::path("/nonexistent/dir/x.vtu")));
}

TEST(Vtu, OpenCellIsReported) {
    auto d = vmm::test::two_squares_data();
    d.face_vertices.values[2] = vmm::VertexId{5};  // break face 1 of cell 0
    const auto m = *vmm::Mesh2D::from_data(d);
    EXPECT_EQ(vmm::cell_loops(m).error().code(), vmm::ErrorCode::InvariantViolated);
    std::stringstream s;
    EXPECT_FALSE(vmm::write_vtu(m, s));
}

}  // namespace
