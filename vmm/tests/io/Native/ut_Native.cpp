// ============================================================================
// File: ut_Native.cpp
// Description: Native format: exact round trip (anchor A1 and a random
//              mesh), file variants and parse errors.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "random_mesh.hpp"
#include "simple_meshes.hpp"
#include <vmm/io/native.hpp>

namespace {

void expect_identical(const vmm::Mesh2D& a, const vmm::Mesh2D& b) {
    EXPECT_EQ(a.points(), b.points());
    EXPECT_EQ(a.face_vertex_csr().values, b.face_vertex_csr().values);
    EXPECT_EQ(a.face_vertex_csr().offsets, b.face_vertex_csr().offsets);
    EXPECT_TRUE(std::ranges::equal(a.owners(), b.owners()));
    EXPECT_TRUE(std::ranges::equal(a.neighbours(), b.neighbours()));
    EXPECT_TRUE(std::ranges::equal(a.sites(), b.sites()));
    EXPECT_TRUE(std::ranges::equal(a.cell_regions(), b.cell_regions()));
    EXPECT_TRUE(std::ranges::equal(a.cell_input_sites(), b.cell_input_sites()));
    ASSERT_EQ(a.patches().size(), b.patches().size());
    for (std::size_t k = 0; k < a.patches().size(); ++k) {
        EXPECT_EQ(a.patches()[k].name, b.patches()[k].name);
        EXPECT_EQ(a.patches()[k].count, b.patches()[k].count);
    }
    EXPECT_EQ(a.media(), b.media());
    EXPECT_EQ(a.regions()[0].name, b.regions()[0].name);
}

TEST(Native, RoundTripIsExact) {
    const auto b = vmm::test::random_square_mesh(500, 9);
    std::stringstream s;
    ASSERT_TRUE(vmm::write_native(b.mesh, s, {"unit test"}));
    EXPECT_NE(s.str().find("# generator unit test"), std::string::npos);
    const auto r = vmm::read_native<2>(s);
    ASSERT_TRUE(r) << r.error().message();
    expect_identical(b.mesh, *r);
}

TEST(Native, FileVariants) {
    const auto m = *vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    const auto path = std::filesystem::temp_directory_path() / "vmm_ut_native.vmesh";
    ASSERT_TRUE(vmm::write_native(m, path));
    const auto r = vmm::read_native<2>(path);
    ASSERT_TRUE(r);
    expect_identical(m, *r);
    std::filesystem::remove(path);
    EXPECT_EQ(vmm::read_native<2>(path).error().code(), vmm::ErrorCode::FileOpenFailed);
    EXPECT_EQ(vmm::write_native(m, std::filesystem::path("/nonexistent/dir/x.vmesh")).error().code(),
              vmm::ErrorCode::FileOpenFailed);
}

TEST(Native, RejectsNamesWithSpaces) {
    auto d = vmm::test::two_squares_data();
    d.regions[0].name = "two words";
    std::stringstream s;
    EXPECT_EQ(vmm::write_native(*vmm::Mesh2D::from_data(d), s).error().code(), vmm::ErrorCode::InvalidArgument);
    d = vmm::test::two_squares_data();
    d.media[0] = "";
    EXPECT_FALSE(vmm::write_native(*vmm::Mesh2D::from_data(d), s));
    d = vmm::test::two_squares_data();
    d.patches[0].name = "a b";
    EXPECT_FALSE(vmm::write_native(*vmm::Mesh2D::from_data(d), s));
}

std::string valid_text() {
    std::stringstream s;
    (void)vmm::write_native(*vmm::Mesh2D::from_data(vmm::test::two_squares_data()), s);
    return s.str();
}

vmm::ErrorCode read_error(std::string text) {
    std::stringstream s(text);
    const auto r = vmm::read_native<2>(s);
    return r ? vmm::ErrorCode{} : r.error().code();
}

std::string replace(std::string text, const std::string& from, const std::string& to) {
    const auto p = text.find(from);
    if (p != std::string::npos) text.replace(p, from.size(), to);
    return text;
}

TEST(Native, ParseErrors) {
    using vmm::ErrorCode;
    const std::string ok = valid_text();
    EXPECT_EQ(read_error(ok), ErrorCode{});
    EXPECT_EQ(read_error("hello\n"), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "vmm-mesh 1", "vmm-mesh 2")), ErrorCode::UnsupportedVersion);
    EXPECT_EQ(read_error(replace(ok, "dimension 2", "dimension 3")), ErrorCode::InconsistentData);
    EXPECT_EQ(read_error(replace(ok, "dimension 2", "dimension x")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "points 6", "points 7")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "\n2 1\n", "\n2 abc\n")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "r 0", "r 5")), ErrorCode::InconsistentData);
    EXPECT_EQ(read_error(replace(ok, "faces 7 1", "faces 7 9")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "2 1 4 0 1", "2 1 99 0 1")), ErrorCode::InconsistentData);
    EXPECT_EQ(read_error(replace(ok, "2 1 4 0 1", "2 1 4 0 7")), ErrorCode::InconsistentData);
    EXPECT_EQ(read_error(replace(ok, "2 1 4 0 1", "2 1 4 9 1")), ErrorCode::InconsistentData);
    EXPECT_EQ(read_error(replace(ok, "2 1 4 0 1", "2 1 4 0")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "2 1 4 0 1", "x 1 4 0 1")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "end", "fin")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "wall 1 6", "wall 1 5")), ErrorCode::InconsistentData);
    EXPECT_EQ(read_error(replace(ok, "wall 1 6", "wall x 6")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "wall 1 6", "wall 1 y")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "cells 2\n0 1", "cells 2\n9 1")), ErrorCode::InconsistentData);
    EXPECT_EQ(read_error(replace(ok, "cells 2\n0 1", "cells 2\n0 z")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "0 1 0.5 0.5", "0 1 0.5 q")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "media 1\nm", "media 1\n")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "regions 1\nr 0", "regions 1\nr")), ErrorCode::ParseError);
    EXPECT_EQ(read_error(replace(ok, "vmm-mesh 1\n", "vmm-mesh 1\n# a comment\n\r\n")), ErrorCode{});
}

TEST(Native, ThreeDimensionalHeader) {
    std::stringstream s(valid_text());
    EXPECT_EQ(vmm::read_native<3>(s).error().code(), vmm::ErrorCode::InconsistentData);
}

}  // namespace
