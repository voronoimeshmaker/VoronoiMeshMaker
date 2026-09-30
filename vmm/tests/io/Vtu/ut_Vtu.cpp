// ============================================================================
// File: ut_Vtu.cpp
// Description: VTK XML writer and cell loop reconstruction.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
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

TEST(Vtu, OpenCellIsReported) {
    auto d = vmm::test::two_squares_data();
    d.face_vertices.values[2] = vmm::VertexId{5};  // break face 1 of cell 0
    const auto m = *vmm::Mesh2D::from_data(d);
    EXPECT_EQ(vmm::cell_loops(m).error().code(), vmm::ErrorCode::InvariantViolated);
    std::stringstream s;
    EXPECT_FALSE(vmm::write_vtu(m, s));
}

}  // namespace
