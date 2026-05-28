#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/IO/Sites2D/Sites2DExport.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteValidation.hpp>

using namespace vmm::b2d;
using namespace vmm::s2d;
using namespace vmm::io;

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

std::filesystem::path temp_folder() {
    return std::filesystem::temp_directory_path()
         / ("vmm_site2d_io_ut_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
}

} // namespace

TEST(Site2DIO, WritesSitesAndBoundaryInTheSameVtkFile) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 3.0),
                                        PolygonizePolicy{},
                                        RegionId{7});

    SiteSet sites;
    sites.add(Point2{1.0, 1.0}, RegionId{11}, 0.25);
    sites.add(Point2{3.0, 2.0}, RegionId{12}, 0.50);

    const auto validation = validate_sites(
        sites,
        boundary,
        SiteValidationOptions{.min_distance_to_boundary = 0.5});
    ASSERT_TRUE(validation);

    VtkOptions options{};
    options.precision = 8;
    options.cell_data = true;

    const auto path = write_sites_vtk_legacy(
        sites,
        boundary,
        temp_folder().string(),
        "sites_with_boundary",
        options);

    ASSERT_TRUE(std::filesystem::exists(path));
    ASSERT_GT(std::filesystem::file_size(path), 0u);

    const std::string content = read_file(path);

    EXPECT_NE(content.find("VMM Site2D with Boundary2D"), std::string::npos);
    EXPECT_NE(content.find("DATASET POLYDATA"), std::string::npos);
    EXPECT_NE(content.find("POINTS 6 double"), std::string::npos);
    EXPECT_NE(content.find("LINES 1 6"), std::string::npos);
    EXPECT_NE(content.find("VERTICES 2 4"), std::string::npos);
    EXPECT_NE(content.find("CELL_DATA 3"), std::string::npos);
    EXPECT_NE(content.find("SCALARS entity_kind int 1"), std::string::npos);
    EXPECT_NE(content.find("SCALARS site_id int 1"), std::string::npos);
    EXPECT_NE(content.find("SCALARS region_id int 1"), std::string::npos);
    EXPECT_NE(content.find("SCALARS site_weight double 1"), std::string::npos);
    EXPECT_NE(content.find("POINT_DATA 6"), std::string::npos);
    EXPECT_NE(content.find("SCALARS point_entity_kind int 1"), std::string::npos);
    EXPECT_NE(content.find("SCALARS point_site_id int 1"), std::string::npos);
    EXPECT_NE(content.find("SCALARS point_region_id int 1"), std::string::npos);
    EXPECT_NE(content.find("SCALARS point_site_weight double 1"), std::string::npos);
}
