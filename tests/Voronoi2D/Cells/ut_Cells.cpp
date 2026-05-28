#include <gtest/gtest.h>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCell2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCellBuilder2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunaySiteIndex.hpp>

using namespace vmm::b2d;
using namespace vmm::s2d;
using namespace vmm::vd2d;
using vmm::error::VMMException;

namespace {

SiteSet make_cross_sites() {
    SiteSet sites;
    sites.add(Point2{0.5, 0.5});
    sites.add(Point2{1.5, 0.5});
    sites.add(Point2{0.5, 1.5});
    sites.add(Point2{-0.5, 0.5});
    sites.add(Point2{0.5, -0.5});
    return sites;
}

} // namespace

TEST(VoronoiCell2D, ComputesBasicPolygonMetrics) {
    VoronoiCell2D cell;
    cell.site_id = SiteId{7};
    cell.polygon = {
        Point2{0.0, 0.0},
        Point2{1.0, 0.0},
        Point2{1.0, 1.0},
        Point2{0.0, 1.0}
    };

    EXPECT_FALSE(cell.empty());
    EXPECT_DOUBLE_EQ(cell.area(), 1.0);
    EXPECT_DOUBLE_EQ(cell.perimeter(), 4.0);
    const auto centroid = cell.centroid();
    EXPECT_DOUBLE_EQ(centroid.x, 0.5);
    EXPECT_DOUBLE_EQ(centroid.y, 0.5);
}

TEST(VoronoiCellBuilder2D, BuildsCentralCellFromDelaunayNeighbors) {
    const auto boundary = make_boundary(Rectangle(Point2{-1.0, -1.0}, 3.0, 3.0));
    const auto sites = make_cross_sites();
    auto triangulation = DelaunayBuilder2D::build(sites);
    auto index = DelaunaySiteIndex::from(triangulation);

    const auto cell = VoronoiCellBuilder2D::build(
        sites,
        boundary,
        triangulation,
        index,
        SiteId{0});

    ASSERT_FALSE(cell.empty());
    EXPECT_EQ(cell.site_id, SiteId{0});
    EXPECT_EQ(cell.neighbor_ids.size(), 4U);
    EXPECT_FALSE(cell.is_boundary_cell);
    EXPECT_NEAR(cell.area(), 1.0, 1.0e-12);

    const auto centroid = cell.centroid();
    EXPECT_NEAR(centroid.x, 0.5, 1.0e-12);
    EXPECT_NEAR(centroid.y, 0.5, 1.0e-12);
}

TEST(VoronoiCellBuilder2D, MarksCellsTouchingBoundary) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 1.0, 1.0));
    const auto sites = make_cross_sites();
    auto triangulation = DelaunayBuilder2D::build(sites);
    auto index = DelaunaySiteIndex::from(triangulation);

    const auto cell = VoronoiCellBuilder2D::build(
        sites,
        boundary,
        triangulation,
        index,
        SiteId{0});

    ASSERT_FALSE(cell.empty());
    EXPECT_TRUE(cell.is_boundary_cell);
}

TEST(VoronoiCellBuilder2D, RejectsBoundaryHolesInThisStage) {
    const auto boundary = make_boundary(Ring2D(Point2{0.0, 0.0}, 0.25, 2.0, 64));
    const auto sites = make_cross_sites();
    auto triangulation = DelaunayBuilder2D::build(sites);
    auto index = DelaunaySiteIndex::from(triangulation);

    EXPECT_THROW((void)VoronoiCellBuilder2D::build(
                     sites,
                     boundary,
                     triangulation,
                     index,
                     SiteId{0}),
                 VMMException);
}
