// ============================================================================
// File: ut_ConfigRun.cpp
// Description: DEC-040 end to end: configuration text -> facade request ->
//              mesh on disk. The request equals the one written with the
//              C++ API; errors name the line and section; user registries.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/config.hpp>
#include <vmm/app/run.hpp>
#include <vmm/io/native.hpp>
#include <vmm/vmm.hpp>

namespace {

using vmm::MeshConfig;

constexpr std::string_view kSoil = R"(dimension = 2
seed = 9
[region soil]
shape = rectangle
lo = 0 0
hi = 2 1
top = surface
sites = uniform
sites.spacing = 0.1
[hole]
shape = circle
center = 1, 0.5
radius = 0.2
tag = well
)";

MeshConfig parse(std::string_view text, std::filesystem::path base = {}) {
    auto c = MeshConfig::parse(text, std::move(base));
    EXPECT_TRUE(c) << c.error().message();
    return *c;
}

/// Error of make_request_2d/3d (by the dimension of the text), expecting `code` and `context` in it.
void expect_error(std::string_view text, vmm::ErrorCode code, std::string_view context) {
    const auto c = parse(text);
    const auto e = c.dimension() == 2 ? vmm::make_request_2d(c).error() : vmm::make_request_3d(c).error();
    EXPECT_EQ(e.code(), code) << e.message();
    EXPECT_NE(e.context().find(context), std::string::npos) << e.context();
}

TEST(ConfigRun, RequestEqualsTheCppApi) {
    const auto request = vmm::make_request_2d(parse(kSoil));
    ASSERT_TRUE(request) << request.error().message();
    const auto from_config = vmm::generate_mesh_2d(*request);
    ASSERT_TRUE(from_config) << from_config.error().message();

    vmm::MeshRequest2D direct;
    auto& d = direct.declaration;
    const auto soil = *d.media().add("soil");
    const auto region = *d.add_region("soil", soil, vmm::Rectangle({0, 0}, {2, 1}, {"", "", "surface", ""}));
    ASSERT_TRUE(d.add_hole(vmm::Circle({1, 0.5}, 0.2, "well")));
    direct.sources = {vmm::sites_for(region, vmm::UniformRandomSource(0.1))};
    direct.sites.seed = 9;
    const auto from_api = vmm::generate_mesh_2d(direct);
    ASSERT_TRUE(from_api);

    EXPECT_EQ(from_config->mesh.data().points, from_api->mesh.data().points);
    EXPECT_EQ(from_config->mesh.data().owner, from_api->mesh.data().owner);
    EXPECT_EQ(from_config->mesh.data().neighbour, from_api->mesh.data().neighbour);
    ASSERT_EQ(from_config->mesh.patches().size(), from_api->mesh.patches().size());
    for (std::size_t i = 0; i < from_api->mesh.patches().size(); ++i) {
        EXPECT_EQ(from_config->mesh.patches()[i].name, from_api->mesh.patches()[i].name);
    }
}

TEST(ConfigRun, GlobalOptionsReachTheRequest) {
    const auto r2 = vmm::make_request_2d(parse("dimension = 2\nseed = 4\ninterface_pairs = 0.05\ntolerance = 1e-11\n"
                                               "[region a]\nmedium = sand\nshape = rectangle\nlo = 0 0\nhi = 1 1\n"
                                               "sites = count\nsites.count = 5\n"));
    ASSERT_TRUE(r2) << r2.error().message();
    EXPECT_EQ(r2->sites.seed, 4u);
    ASSERT_TRUE(r2->sites.interface_pairs);
    EXPECT_EQ(r2->sites.interface_pairs->spacing(), 0.05);
    EXPECT_EQ(r2->build.relative_tolerance, 1e-11);
    EXPECT_EQ(r2->declaration.media().names(), std::vector<std::string>{"sand"});
    const auto r3 = vmm::make_request_3d(parse("dimension = 3\ninterface_pairs = 0.2\ntolerance = 1e-11\n"
                                               "[region a]\nshape = cuboid\nlo = 0 0 0\nhi = 1 1 1\n"
                                               "sites = count\nsites.count = 5\n"
                                               "[region b]\nmedium = a\nshape = sphere\ncenter = 0.5 0.5 0.5\n"
                                               "radius = 0.2\nsites = count\nsites.count = 3\n"));
    ASSERT_TRUE(r3) << r3.error().message();
    ASSERT_TRUE(r3->sites.interface_pairs);
    EXPECT_EQ(r3->sites.interface_pairs->spacing(), 0.2);
    EXPECT_EQ(r3->build.relative_tolerance, 1e-11);
    EXPECT_EQ(r3->declaration.media().names(), std::vector<std::string>{"a"});  // medium shared by name
    EXPECT_EQ(r3->declaration.regions().size(), 2u);
    EXPECT_EQ(r3->sources.size(), 2u);
}

TEST(ConfigRun, ErrorsNameTheLineAndSection) {
    const std::string head = "dimension = 2\n[region a]\n";
    expect_error(head + "shape = square\nsites = count\nsites.count = 3\n", vmm::ErrorCode::UnknownShape,
                 "line 3 [region a]");
    expect_error(head + "lo = 0 0\n", vmm::ErrorCode::ParseError, "line 2 [region a]: 'shape = <name>' is required");
    expect_error(head + "shape = rectangle\nlo = 0 0\n", vmm::ErrorCode::InvalidShapeParameter, "line 3 [region a]");
    expect_error(head + "shape = rectangle\nlo = 0 0\nhi = 1 1\n", vmm::ErrorCode::ParseError,
                 "line 2 [region a]: 'sites = <source>' is required");
    expect_error(head + "shape = rectangle\nlo = 0 0\nhi = 1 1\nsites = poisson\n", vmm::ErrorCode::InvalidArgument,
                 "line 6 [region a]: unknown site source 'poisson'");
    expect_error(head + "shape = rectangle\nlo = 0 0\nhi = 1 1\nsite.spacing = 1\n", vmm::ErrorCode::ParseError,
                 "line 6 [region a]: unknown key 'site.spacing'");
    expect_error(head + "shape = rectangle\nlo = 0 0\nhi = 1 1\nsites = count\nsites.count = 2\n"
                        "[region a]\nshape = rectangle\nlo = 0 0\nhi = 1 1\nsites = count\nsites.count = 2\n",
                 vmm::ErrorCode::DuplicateName, "line 8 [region a]");
    expect_error(head + "shape = rectangle\nlo = 0 0\nhi = 1 1\nsites = count\nsites.count = 2\n"
                        "[hole]\nshape = circle\ncenter = 0.5 0.5\nradius = 0.1\nsites = count\n",
                 vmm::ErrorCode::ParseError, "[hole]: a hole takes no medium and no sites");
    expect_error(head + "shape = rectangle\nlo = 0 0\nhi = 1 1\nsites = count\nsites.count = 2\n"
                        "[background b]\n",
                 vmm::ErrorCode::ParseError, "[background b]: 'sites = <source>' is required");
    expect_error("dimension = 3\n[region a]\nshape = cuboid\nlo = 0 0 0\nhi = 1 1 1\nsites = count\n"
                 "sites.count = 2\n[background b]\n",
                 vmm::ErrorCode::ParseError, "[background b]: [background] is 2D only");
    expect_error("dimension = 3\n[region a]\nshape = cuboid\nlo = 0 0 0\nhi = 1 1 1\nsites = count\n"
                 "sites.count = 2\n[hole]\nshape = sphere\n",
                 vmm::ErrorCode::InvalidShapeParameter, "line 9 [hole]");
}

TEST(ConfigRun, DimensionMustMatch) {
    const auto c = parse(kSoil);
    EXPECT_EQ(vmm::make_request_3d(c).error().code(), vmm::ErrorCode::InvalidArgument);
    const auto c3 = parse("dimension = 3\n[region a]\n");
    EXPECT_EQ(vmm::make_request_2d(c3).error().code(), vmm::ErrorCode::InvalidArgument);
}

TEST(ConfigRun, BackgroundFillsTheVoid2D) {
    const auto c = parse(R"(dimension = 2
[region s]
medium = rock
shape = rectangle
lo = 0 0
hi = 3 1
sites = grid
sites.spacing = 0.25
[region w]
medium = rock
shape = rectangle
lo = 0 0
hi = 1 3
sites = grid
sites.spacing = 0.25
[region e]
medium = rock
shape = rectangle
lo = 2 0
hi = 3 3
sites = grid
sites.spacing = 0.25
[region n]
medium = rock
shape = rectangle
lo = 0 2
hi = 3 3
sites = grid
sites.spacing = 0.25
[background fill]
medium = water
sites = explicit
sites.xy = 1.5 1.5
)");
    const auto request = vmm::make_request_2d(c);
    ASSERT_TRUE(request) << request.error().message();
    EXPECT_EQ(request->declaration.media().names(), (std::vector<std::string>{"rock", "water"}));
    const auto mesh = vmm::generate_mesh_2d(*request);
    ASSERT_TRUE(mesh) << mesh.error().message();
    EXPECT_NEAR(mesh->partition.region_area(request->declaration.background()), 1.0, 1e-12);
}

TEST(ConfigRun, RunWritesTheRequestedFiles) {
    const auto dir = std::filesystem::temp_directory_path() / "vmm_ut_configrun";
    std::filesystem::remove_all(dir);
    auto c = parse(kSoil, dir);
    c.set_output("out/soil");
    const auto report = vmm::run_config(c);
    ASSERT_TRUE(report) << report.error().message();
    EXPECT_EQ(report->dimension, 2);
    ASSERT_EQ(report->written.size(), 2u);
    EXPECT_EQ(report->written[0], dir / "out/soil.vmesh");
    EXPECT_EQ(report->written[1], dir / "out/soil.vtu");
    EXPECT_TRUE(std::filesystem::exists(report->written[1]));
    const auto back = vmm::read_native<2>(report->written[0]);
    ASSERT_TRUE(back) << back.error().message();
    EXPECT_EQ(back->cell_count(), report->cells);
    EXPECT_EQ(back->internal_face_count(), report->internal_faces);
    EXPECT_EQ(back->boundary_faces().size(), report->boundary_faces);
    EXPECT_LT(report->invariants.total_relative_error, 1e-12);

    const auto c3 = parse("dimension = 3\nformats = vmesh\noutput = cube\n[region a]\nshape = extrusion\n"
                          "xy = 0 0 1 0 1 1 0 1\nz = 0 1\nsites = grid\nsites.spacing = 0.5\n"
                          "sites.origin = 0.25 0.25 0.25\n",
                          dir);
    const auto report3 = vmm::run_config(c3);
    ASSERT_TRUE(report3) << report3.error().message();
    EXPECT_EQ(report3->dimension, 3);
    EXPECT_EQ(report3->cells, 8u);
    ASSERT_EQ(report3->written.size(), 1u);
    EXPECT_EQ(vmm::read_native<3>(report3->written[0])->cell_count(), 8u);
    std::filesystem::remove_all(dir);
}

TEST(ConfigRun, RunErrors) {
    auto c = parse("dimension = 2\nformats = vtu obj\n[region a]\n");
    EXPECT_EQ(vmm::run_config(c).error().code(), vmm::ErrorCode::ParseError);  // before any meshing
    EXPECT_EQ(vmm::run_config(parse("dimension = 2\n[region a]\n")).error().code(), vmm::ErrorCode::ParseError);
    EXPECT_EQ(vmm::run_config(parse("dimension = 3\n[region a]\n")).error().code(), vmm::ErrorCode::ParseError);
    // Two regions of one medium where the second empties the first: a meshing (validation) error.
    const auto emptied = parse("dimension = 2\n[region small]\nshape = rectangle\nlo = 0.2 0.2\nhi = 0.4 0.4\n"
                               "sites = count\nsites.count = 2\n[region big]\nshape = rectangle\nlo = 0 0\n"
                               "hi = 1 1\nsites = count\nsites.count = 4\n");
    EXPECT_EQ(vmm::run_config(emptied).error().code(), vmm::ErrorCode::RegionEmptied);
    const auto no_sites3 = parse("dimension = 3\n[region a]\nshape = cuboid\nlo = 0 0 0\nhi = 1 1 1\n"
                                 "sites = explicit\nsites.xyz = 5 5 5\n");
    EXPECT_FALSE(vmm::run_config(no_sites3));
    // An output path under a file cannot be created: the writer reports it.
    const auto file = std::filesystem::temp_directory_path() / "vmm_ut_configrun_file";
    { std::ofstream(file) << "x"; }
    auto blocked = parse("dimension = 2\n[region a]\nshape = rectangle\nlo = 0 0\nhi = 1 1\nsites = count\n"
                         "sites.count = 3\n");
    blocked.set_output(file / "mesh");
    EXPECT_EQ(vmm::run_config(blocked).error().code(), vmm::ErrorCode::FileOpenFailed);
    std::filesystem::remove(file);
}

TEST(ConfigRun, UserRegistries) {
    auto registries = vmm::ConfigRegistries::with_builtins();
    ASSERT_TRUE(registries.shapes_2d.add(
        "unit_square", [](const vmm::ShapeParameters&, const vmm::PolygonizeOptions& o) {
            return vmm::Rectangle({0, 0}, {1, 1}).outline(o);
        }));
    const auto request = vmm::make_request_2d(
        parse("dimension = 2\n[region a]\nshape = unit_square\nsites = count\nsites.count = 6\n"), registries);
    ASSERT_TRUE(request) << request.error().message();
    const auto mesh = vmm::generate_mesh_2d(*request);
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->mesh.cell_count(), 6u);
}

}  // namespace
