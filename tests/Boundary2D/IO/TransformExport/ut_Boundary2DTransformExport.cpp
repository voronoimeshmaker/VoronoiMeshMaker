#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/Boundary2D/Transforms/Boundary2DTransform.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/IO/Boundary2D/Boundary2DExport.hpp>

using namespace vmm::b2d;
using namespace vmm::io;

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

std::filesystem::path temp_output_dir() {
    const auto dir = std::filesystem::temp_directory_path() / "vmm_boundary2d_transform_export";
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

TEST(Boundary2DTransformExport, RotatedAndTranslatedRingCanBeWrittenToVtk) {
    auto boundary = make_boundary(
        Ring2D(Point2{0.0, 0.0}, 0.5, 1.5, 32),
        PolygonizePolicy{},
        RegionId{33});

    rotate_in_place(boundary, ::vmm::constants::kPi / 6.0, Point2{0.25, -0.15});
    translate_in_place(boundary, 2.0, -1.0);

    const auto path = write_boundary_vtk_legacy(
        boundary,
        temp_output_dir().string(),
        "ring2d_rotated_translated",
        VtkOptions{});

    ASSERT_TRUE(std::filesystem::exists(path));
    const auto vtk = read_file(path);

    EXPECT_NE(vtk.find("DATASET POLYDATA"), std::string::npos);
    EXPECT_NE(vtk.find("POINTS 64 double"), std::string::npos);
    EXPECT_NE(vtk.find("LINES 2 "), std::string::npos);
    EXPECT_NE(vtk.find("SCALARS loop_kind int 1"), std::string::npos);
    EXPECT_NE(vtk.find("\n33\n"), std::string::npos);
    EXPECT_EQ(vtk.find("POLYLINES"), std::string::npos);
}
