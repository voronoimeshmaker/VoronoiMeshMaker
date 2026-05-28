#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>
#include <VoronoiMeshMaker/IO/Voronoi2D/Delaunay2DExport.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayNeighborProvider2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunaySiteIndex.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Traits/CgalKernelTraits2D.hpp>

using namespace vmm::s2d;
using namespace vmm::vd2d;
using vmm::error::VMMException;

namespace {

SiteSet make_square_with_center_sites() {
    SiteSet sites;
    sites.add(Point2{0.0, 0.0});
    sites.add(Point2{1.0, 0.0});
    sites.add(Point2{1.0, 1.0});
    sites.add(Point2{0.0, 1.0});
    sites.add(Point2{0.5, 0.5});
    return sites;
}

[[nodiscard]] bool contains_id(const std::vector<SiteId>& ids, SiteId id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

std::filesystem::path temp_folder() {
    return std::filesystem::temp_directory_path()
         / ("vmm_delaunay2d_io_ut_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
}

} // namespace

TEST(CgalKernelTraits2D, ConvertsBetweenVmmAndCgalPoints) {
    const Point2 point{1.25, -2.5};

    const auto cgal_point = CgalKernelTraits2D::to_cgal(point);
    const auto round_trip = CgalKernelTraits2D::from_cgal(cgal_point);

    EXPECT_DOUBLE_EQ(round_trip.x, point.x);
    EXPECT_DOUBLE_EQ(round_trip.y, point.y);
}

TEST(DelaunayBuilder2D, BuildsTwoDimensionalTriangulationWithSiteIds) {
    const auto sites = make_square_with_center_sites();

    auto triangulation = DelaunayBuilder2D::build(sites);

    EXPECT_EQ(triangulation.number_of_vertices(), sites.size());
    EXPECT_EQ(triangulation.dimension(), 2);

    for (auto vertex = triangulation.finite_vertices_begin();
         vertex != triangulation.finite_vertices_end();
         ++vertex) {
        EXPECT_GE(vertex->info().value, 0);
        EXPECT_LT(static_cast<std::size_t>(vertex->info().value), sites.size());
    }
}

TEST(DelaunaySiteIndex, MapsSequentialSiteIdsToVertexHandles) {
    const auto sites = make_square_with_center_sites();
    auto triangulation = DelaunayBuilder2D::build(sites);

    const auto index = DelaunaySiteIndex::from(triangulation);

    ASSERT_EQ(index.size(), sites.size());
    for (std::size_t i = 0; i < sites.size(); ++i) {
        const SiteId id{static_cast<Index>(i)};
        ASSERT_TRUE(index.contains(id));
        EXPECT_EQ(index.at(id)->info(), id);
    }
    EXPECT_THROW((void)index.at(SiteId{99}), VMMException);
}

TEST(DelaunayNeighborProvider2D, FindsDelaunayAdjacentSiteIds) {
    const auto sites = make_square_with_center_sites();
    auto triangulation = DelaunayBuilder2D::build(sites);
    const auto index = DelaunaySiteIndex::from(triangulation);

    const auto center_neighbors = DelaunayNeighborProvider2D::neighbor_site_ids(
        triangulation,
        index,
        SiteId{4});

    ASSERT_EQ(center_neighbors.size(), 4U);
    EXPECT_TRUE(contains_id(center_neighbors, SiteId{0}));
    EXPECT_TRUE(contains_id(center_neighbors, SiteId{1}));
    EXPECT_TRUE(contains_id(center_neighbors, SiteId{2}));
    EXPECT_TRUE(contains_id(center_neighbors, SiteId{3}));
}

TEST(DelaunayBuilder2D, RejectsDuplicatePoints) {
    SiteSet sites;
    sites.add(Point2{0.0, 0.0});
    sites.add(Point2{1.0, 0.0});
    sites.add(Point2{1.0, 0.0});
    sites.add(Point2{0.0, 1.0});

    EXPECT_THROW((void)DelaunayBuilder2D::build(sites), VMMException);
}

TEST(DelaunayBuilder2D, RejectsCollinearPointSetsByDefault) {
    SiteSet sites;
    sites.add(Point2{0.0, 0.0});
    sites.add(Point2{1.0, 0.0});
    sites.add(Point2{2.0, 0.0});

    EXPECT_THROW((void)DelaunayBuilder2D::build(sites), VMMException);
}

TEST(DelaunayBuilder2D, AllowsNonTwoDimensionalPointSetsWhenRequested) {
    SiteSet sites;
    sites.add(Point2{0.0, 0.0});
    sites.add(Point2{1.0, 0.0});
    sites.add(Point2{2.0, 0.0});

    const DelaunayBuildOptions2D options{
        .require_sequential_ids = true,
        .require_unique_points = true,
        .require_two_dimensional = false
    };

    const auto triangulation = DelaunayBuilder2D::build(sites, options);

    EXPECT_EQ(triangulation.number_of_vertices(), 3U);
    EXPECT_EQ(triangulation.dimension(), 1);
}

TEST(Delaunay2DIO, WritesBoundarySitesAndDelaunayInSameVtkFile) {
    namespace b2d = ::vmm::b2d;
    namespace io = ::vmm::io;

    const auto boundary = b2d::make_boundary(
        b2d::Rectangle(b2d::Point2{0.0, 0.0}, 1.0, 1.0),
        b2d::PolygonizePolicy{},
        b2d::RegionId{7});
    const auto sites = make_square_with_center_sites();
    const auto triangulation = DelaunayBuilder2D::build(sites);

    io::VtkOptions options{};
    options.precision = 8;
    options.cell_data = true;

    const auto path = io::write_delaunay_vtk_legacy(
        sites,
        boundary,
        triangulation,
        temp_folder().string(),
        "delaunay_with_boundary_and_sites",
        options);

    ASSERT_TRUE(std::filesystem::exists(path));
    ASSERT_GT(std::filesystem::file_size(path), 0U);

    const std::string content = read_file(path);

    EXPECT_NE(content.find("VMM Boundary2D + Site2D + Delaunay2D"),
              std::string::npos);
    EXPECT_NE(content.find("DATASET POLYDATA"), std::string::npos);
    EXPECT_NE(content.find("POINTS 9 double"), std::string::npos);
    EXPECT_NE(content.find("LINES 9 30"), std::string::npos);
    EXPECT_NE(content.find("VERTICES 5 10"), std::string::npos);
    EXPECT_NE(content.find("SCALARS entity_kind int 1"), std::string::npos);
    EXPECT_NE(content.find("SCALARS delaunay_edge int 1"), std::string::npos);
    EXPECT_NE(content.find("POINT_DATA 9"), std::string::npos);
    EXPECT_NE(content.find("SCALARS point_site_id int 1"), std::string::npos);
}
