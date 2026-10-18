// ============================================================================
// File: ut_Stl.cpp
// Description: STL reader and writer: ASCII with named solids (patches),
//              binary, round trips through repair_surface, malformed input.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstring>
#include <filesystem>
#include <limits>
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
#include "allocation_failure.hpp"
#include <vmm/domain/shapes3d.hpp>
#include <vmm/io/stl.hpp>

namespace {

using vmm::ErrorCode;

vmm::TriangleSurface tagged_cube() {
    return *vmm::Cuboid({0, 0, 0}, {1, 2, 3}, {"w", "e", "s", "n", "floor", "top"}).surface({});
}

TEST(Stl, AsciiRoundTripKeepsPatches) {
    const auto cube = tagged_cube();
    std::stringstream s;
    ASSERT_TRUE(vmm::write_stl(cube, s));
    EXPECT_NE(s.str().find("solid floor"), std::string::npos);
    const auto soup = vmm::read_stl(s);
    ASSERT_TRUE(soup) << soup.error().message();
    EXPECT_EQ(soup->triangles.size(), 12u);
    EXPECT_EQ(soup->points.size(), 36u);
    EXPECT_EQ(soup->patches, cube.patches());
    const auto back = vmm::repair_surface(*soup);
    ASSERT_TRUE(back);
    EXPECT_DOUBLE_EQ(back->volume(), 6.0);
}

TEST(Stl, BinaryRoundTripAndFiles) {
    const auto cube = tagged_cube();
    const auto path = std::filesystem::temp_directory_path() / "vmm_ut_cube.stl";
    ASSERT_TRUE(vmm::write_stl(cube, path, {true}));
    EXPECT_EQ(std::filesystem::file_size(path), 84u + 50u * 12u);
    vmm::SurfaceRepairReport rep;
    const auto s = vmm::read_stl_surface(path, {"block"}, {}, &rep);
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_EQ(s->patches(), (std::vector<std::string>{"block"}));
    EXPECT_DOUBLE_EQ(s->volume(), 6.0);
    EXPECT_EQ(rep.welded_points, 28u);
    std::filesystem::remove(path);
    EXPECT_EQ(vmm::read_stl(std::filesystem::path("/nonexistent/x.stl")).error().code(), ErrorCode::FileOpenFailed);
    EXPECT_EQ(vmm::read_stl_surface("/nonexistent/x.stl").error().code(), ErrorCode::FileOpenFailed);
    EXPECT_FALSE(vmm::write_stl(cube, std::filesystem::path("/nonexistent/dir/x.stl")));
}

TEST(Stl, UnnamedSolidsTakeTheDefaultPatch) {
    std::stringstream s("solid\n facet normal 0 0 1\n  outer loop\n   vertex 0 0 0\n   vertex 1 0 0\n   vertex 0 1 0\n"
                        "  endloop\n endfacet\nendsolid\n");
    const auto soup = vmm::read_stl(s, {"terrain"});
    ASSERT_TRUE(soup) << soup.error().message();
    EXPECT_EQ(soup->patches, (std::vector<std::string>{"terrain"}));
}

TEST(Stl, MalformedInput) {
    const auto parse = [](const std::string& text) {
        std::stringstream s(text);
        return vmm::read_stl(s).error().code();
    };
    EXPECT_EQ(parse("hello"), ErrorCode::ParseError);
    EXPECT_EQ(parse("solid a\nsolid b\n"), ErrorCode::ParseError);
    EXPECT_EQ(parse("endsolid a\n"), ErrorCode::ParseError);
    EXPECT_EQ(parse("solid a\n facet normal 0 0 1\n"), ErrorCode::ParseError);  // missing endsolid
    EXPECT_EQ(parse("facet normal 0 0 1\n"), ErrorCode::ParseError);
    EXPECT_EQ(parse("solid a\n vertex 0 0\nendsolid a\n"), ErrorCode::ParseError);
    EXPECT_EQ(parse("solid a\n vertex 0 0 0\n endfacet\nendsolid a\n"), ErrorCode::ParseError);
    EXPECT_EQ(parse("solid a\n polygon\nendsolid a\n"), ErrorCode::ParseError);
    EXPECT_EQ(parse("solid a\nendsolid a\n"), ErrorCode::ParseError);  // no facet
}


TEST(Stl, RejectsTruncatedBinaryCountsAndNonFiniteValues) {
    std::ostringstream out;
    ASSERT_TRUE(vmm::write_stl(tagged_cube(), out, {true}));
    const auto valid = out.str();
    for (std::size_t size = 0; size < valid.size(); ++size) {
        std::istringstream in(valid.substr(0, size));
        EXPECT_FALSE(vmm::read_stl(in)) << size;
    }
    auto bytes = valid;
    const std::uint32_t count = std::numeric_limits<std::uint32_t>::max();
    std::memcpy(bytes.data() + 80, &count, sizeof(count));
    std::istringstream huge(bytes);
    EXPECT_FALSE(vmm::read_stl(huge));
    for (const auto value : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        bytes = valid;
        std::memcpy(bytes.data() + 84 + 12, &value, sizeof(value));
        std::istringstream nonfinite(bytes);
        EXPECT_FALSE(vmm::read_stl(nonfinite));
    }
    std::istringstream trailing(valid + "extra");
    EXPECT_FALSE(vmm::read_stl(trailing));
}

TEST(Stl, RequiresCompleteAsciiFacets) {
    for (const auto* body : {"vertex 0 0 0\n", "facet normal 0 0 1\n", "endloop\n", "endfacet\n",
                             "facet normal nan 0 1\n", "outer loop\n"}) {
        std::istringstream in(std::string("solid a\n") + body + "endsolid a\n");
        EXPECT_FALSE(vmm::read_stl(in));
    }
    std::ostringstream out;
    ASSERT_TRUE(vmm::write_stl(tagged_cube(), out));
    std::istringstream extra(out.str() + "unexpected\n");
    EXPECT_FALSE(vmm::read_stl(extra));
}


TEST(Stl, AllocationFailuresStayInsideTheReader) {
    std::ostringstream out;
    ASSERT_TRUE(vmm::write_stl(tagged_cube(), out));
    const auto text = out.str();
    for (std::size_t attempt = 0; attempt < 80; ++attempt) {
        std::istringstream in(text);
        vmm::test::fail_allocation_after(attempt);
        const auto result = vmm::read_stl(in);
        vmm::test::stop_allocation_failure();
        if (vmm::test::allocation_failed) { EXPECT_FALSE(result); }
    }
}

}  // namespace

TEST(Stl, AllocationFailuresInFileReaders) {
    const auto path=std::filesystem::temp_directory_path()/"vmm_p30_allocation.stl";
    ASSERT_TRUE(vmm::write_stl(tagged_cube(),path));
    for(std::size_t attempt=0;attempt<120;++attempt) {
        vmm::test::fail_allocation_after(attempt);
        const auto soup=vmm::read_stl(path);
        vmm::test::stop_allocation_failure();
        if(vmm::test::allocation_failed) { EXPECT_FALSE(soup); }
        vmm::test::fail_allocation_after(attempt);
        const auto surface=vmm::read_stl_surface(path);
        vmm::test::stop_allocation_failure();
        if(vmm::test::allocation_failed) { EXPECT_FALSE(surface); }
    }
    std::filesystem::remove(path);
    std::istringstream broken;
    broken.setstate(std::ios::badbit);
    EXPECT_FALSE(vmm::read_stl(broken));
}
