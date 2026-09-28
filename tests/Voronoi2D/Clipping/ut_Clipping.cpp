/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * @file ut_Clipping.cpp
 * @brief Regression tests for half-plane clipping and boundary operations.
 * @ingroup voronoi2d_clipping
 */
#include <vector>

#include <gtest/gtest.h>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/BoundaryCellDetector2D.hpp>
#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/BoundaryShortEdgeCollapse2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/BisectorHalfplane.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCellBuilder2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/HalfplaneClipper2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunaySiteIndex.hpp>

using namespace vmm::s2d;
using namespace vmm::vd2d;
using vmm::error::VMMException;

TEST(Halfplane2D, EvaluatesAndContainsPoints) {
    const Halfplane2D left_of_one{1.0, 0.0, 1.0};

    EXPECT_TRUE(left_of_one.is_valid());
    EXPECT_TRUE(left_of_one.contains(Point2{0.5, 10.0}));
    EXPECT_TRUE(left_of_one.contains(Point2{1.0, -2.0}));
    EXPECT_FALSE(left_of_one.contains(Point2{1.5, 0.0}));
    EXPECT_DOUBLE_EQ(left_of_one.evaluate(Point2{1.5, 0.0}), 0.5);
}

TEST(BisectorHalfplane, KeepsSideCloserToOwnerSite) {
    const auto hp = BisectorHalfplane::between(Point2{0.0, 0.0},
                                               Point2{2.0, 0.0});

    EXPECT_TRUE(hp.contains(Point2{0.25, 5.0}));
    EXPECT_TRUE(hp.contains(Point2{1.0, 0.0}));
    EXPECT_FALSE(hp.contains(Point2{1.25, 0.0}));
    EXPECT_THROW((void)BisectorHalfplane::between(Point2{1.0, 1.0},
                                                  Point2{1.0, 1.0}),
                 VMMException);
}

TEST(HalfplaneClipper2D, ClipsSquareByVerticalHalfplane) {
    const std::vector<Point2> square{
        Point2{0.0, 0.0},
        Point2{2.0, 0.0},
        Point2{2.0, 2.0},
        Point2{0.0, 2.0}
    };

    const auto clipped = HalfplaneClipper2D::clip(
        square,
        Halfplane2D{1.0, 0.0, 1.0});

    ASSERT_EQ(clipped.size(), 4U);
    for (const auto& p : clipped) {
        EXPECT_LE(p.x, 1.0 + 1.0e-12);
    }
}

TEST(HalfplaneClipper2D, ClipsSquareBySeveralHalfplanes) {
    const std::vector<Point2> square{
        Point2{0.0, 0.0},
        Point2{2.0, 0.0},
        Point2{2.0, 2.0},
        Point2{0.0, 2.0}
    };
    const std::vector<Halfplane2D> halfplanes{
        Halfplane2D{1.0, 0.0, 1.5},
        Halfplane2D{-1.0, 0.0, -0.5},
        Halfplane2D{0.0, 1.0, 1.5},
        Halfplane2D{0.0, -1.0, -0.5}
    };
    ClippingWorkspace2D workspace;

    const auto clipped = HalfplaneClipper2D::clip_all(
        square,
        halfplanes,
        workspace);

    ASSERT_EQ(clipped.size(), 4U);
    for (const auto& p : clipped) {
        EXPECT_GE(p.x, 0.5 - 1.0e-12);
        EXPECT_LE(p.x, 1.5 + 1.0e-12);
        EXPECT_GE(p.y, 0.5 - 1.0e-12);
        EXPECT_LE(p.y, 1.5 + 1.0e-12);
    }
}

TEST(HalfplaneClipper2D, ResolvesCrossingsBelowLegacyAbsoluteTolerance) {
    const Halfplane2D plane{Real{1}, Real{0}, Real{0}};
    const Point2 p{Real{-1e-8}, Real{0}};
    const Point2 q{Real{1e-8}, Real{1}};
    const auto crossing = HalfplaneClipper2D::intersection(p, q, plane);
    EXPECT_EQ(crossing.x, Real{0});
    EXPECT_EQ(crossing.y, Real{0.5});

    const std::vector<Point2> rectangle{
        {Real{-1e-8}, Real{0}}, {Real{1e-8}, Real{0}},
        {Real{1e-8}, Real{1}}, {Real{-1e-8}, Real{1}}};
    const auto clipped = HalfplaneClipper2D::clip(rectangle, plane);
    ASSERT_EQ(clipped.size(), 4U);
    for (const auto& point : clipped) EXPECT_LE(point.x, Real{0});
}

TEST(HalfplaneClipper2D, IsInvariantUnderPositivePlaneScaling) {
    const std::vector<Point2> square{
        {Real{0}, Real{0}}, {Real{2}, Real{0}},
        {Real{2}, Real{2}}, {Real{0}, Real{2}}};
    const auto reference = HalfplaneClipper2D::clip(
        square, Halfplane2D{Real{1}, Real{0}, Real{1}});
    for (const Real scale : {Real{1e-12}, Real{1}, Real{1e12}}) {
        const auto clipped = HalfplaneClipper2D::clip(
            square, Halfplane2D{scale, Real{0}, scale});
        ASSERT_EQ(clipped.size(), reference.size());
        for (std::size_t i = 0; i < clipped.size(); ++i) {
            EXPECT_EQ(clipped[i].x, reference[i].x);
            EXPECT_EQ(clipped[i].y, reference[i].y);
        }
    }
}

TEST(BisectorHalfplane, PreservesTranslatedAxisAlignedBisector) {
    const Point2 owner{Real{1000000}, Real{-2000000}};
    const Point2 neighbour{Real{1000000.125}, Real{-2000000}};
    const auto plane = BisectorHalfplane::between(owner, neighbour);
    const auto reverse = BisectorHalfplane::between(neighbour, owner);
    const Point2 midpoint{Real{1000000.0625}, owner.y};
    EXPECT_EQ(plane.evaluate(midpoint), Real{0});
    EXPECT_EQ(plane.a, -reverse.a);
    EXPECT_EQ(plane.b, -reverse.b);
    EXPECT_EQ(plane.c, -reverse.c);
}

TEST(VoronoiCellBuilder2D, RetainsGenuineSubmicrometreFace) {
    const auto boundary = vmm::b2d::make_boundary(
        vmm::b2d::Rectangle(Point2{Real{0}, Real{0}}, Real{1}, Real{1}));
    constexpr Real cut{1e-7};
    SiteSet sites;
    sites.add(Point2{Real{0.25}, Real{0.25}});
    sites.add(Point2{Real{0.75}, Real{0.25}});
    sites.add(Point2{Real{0.25}, Real{0.75}});
    sites.add(Point2{Real{0.75} - cut, Real{0.75} - cut});
    auto triangulation = DelaunayBuilder2D::build(sites);
    const auto index = DelaunaySiteIndex::from(triangulation);
    const auto cell = VoronoiCellBuilder2D::build(
        sites, boundary, triangulation, index, SiteId{0});
    ASSERT_EQ(cell.polygon.size(), 5U);
    std::size_t short_faces = 0;
    for (std::size_t i = 0; i < cell.polygon.size(); ++i) {
        const auto p = cell.polygon[i];
        const auto q = cell.polygon[(i + 1U) % cell.polygon.size()];
        const auto length = std::hypot(q.x - p.x, q.y - p.y);
        if (length < Real{1e-6}) {
            ++short_faces;
            EXPECT_NEAR(length, std::sqrt(Real{2}) * cut, Real{1e-14});
            EXPECT_NEAR(p.x + p.y, Real{1} - cut, Real{1e-14});
            EXPECT_NEAR(q.x + q.y, Real{1} - cut, Real{1e-14});
        }
    }
    EXPECT_EQ(short_faces, 1U);
}

TEST(BoundaryShortEdgeCollapse2D, CollapsesShortEdgeOnBoundarySegment) {
    const auto boundary = vmm::b2d::make_boundary(
        vmm::b2d::Rectangle(Point2{0.0, 0.0}, 1.0, 1.0));

    std::vector<Point2> polygon{
        Point2{0.0, 0.0},
        Point2{0.01, 0.0},
        Point2{1.0, 0.0},
        Point2{1.0, 1.0},
        Point2{0.0, 1.0}
    };

    const auto result = collapse_short_boundary_edges(
        polygon,
        boundary,
        BoundaryShortEdgeCollapseOptions2D{
            0.05,
            BoundaryShortEdgePolicy2D::CollapseToBoundaryProjection});

    EXPECT_EQ(result.collapsed_count, 1U);
    ASSERT_EQ(polygon.size(), 4U);
    EXPECT_NEAR(polygon.front().x, 0.005, 1.0e-12);
    EXPECT_NEAR(polygon.front().y, 0.0, 1.0e-12);
    EXPECT_LT(result.minimum_length_before, 0.05);
    EXPECT_GT(result.minimum_length_after, 0.05);
}

TEST(BoundaryShortEdgeCollapse2D, KeepsShortInteriorEdge) {
    const auto boundary = vmm::b2d::make_boundary(
        vmm::b2d::Rectangle(Point2{0.0, 0.0}, 1.0, 1.0));

    std::vector<Point2> polygon{
        Point2{0.0, 0.0},
        Point2{0.01, 0.01},
        Point2{1.0, 0.0},
        Point2{1.0, 1.0},
        Point2{0.0, 1.0}
    };

    const auto result = collapse_short_boundary_edges(
        polygon,
        boundary,
        BoundaryShortEdgeCollapseOptions2D{
            0.05,
            BoundaryShortEdgePolicy2D::CollapseToBoundaryProjection});

    EXPECT_EQ(result.collapsed_count, 0U);
    EXPECT_EQ(polygon.size(), 5U);
    EXPECT_LT(result.minimum_length_after, 0.05);
}

TEST(BoundaryShortEdgeCollapse2D, RejectsNegativeMinimumLength) {
    const auto boundary = vmm::b2d::make_boundary(
        vmm::b2d::Rectangle(Point2{0.0, 0.0}, 1.0, 1.0));
    std::vector<Point2> polygon{
        Point2{0.0, 0.0},
        Point2{1.0, 0.0},
        Point2{1.0, 1.0},
        Point2{0.0, 1.0}
    };

    EXPECT_THROW(
        (void)collapse_short_boundary_edges(
            polygon,
            boundary,
            BoundaryShortEdgeCollapseOptions2D{
                -0.01,
                BoundaryShortEdgePolicy2D::CollapseToBoundaryProjection}),
        VMMException);
}

TEST(BoundaryCellDetector2D, DetectsBoundaryCellsByEdgePropagation) {
    const auto boundary = vmm::b2d::make_boundary(
        vmm::b2d::Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));

    SiteSet sites;
    sites.add(Point2{1.0, 1.0});
    sites.add(Point2{0.5, 1.0});
    sites.add(Point2{1.5, 1.0});
    sites.add(Point2{1.0, 0.5});
    sites.add(Point2{1.0, 1.5});

    auto triangulation = DelaunayBuilder2D::build(sites);
    auto index = DelaunaySiteIndex::from(triangulation);

    const auto detection = BoundaryCellDetector2D::detect(
        sites,
        boundary,
        triangulation,
        index);

    ASSERT_EQ(detection.is_boundary_site.size(), sites.size());
    EXPECT_FALSE(detection.is_boundary(SiteId{0}));
    EXPECT_TRUE(detection.is_boundary(SiteId{1}));
    EXPECT_TRUE(detection.is_boundary(SiteId{2}));
    EXPECT_TRUE(detection.is_boundary(SiteId{3}));
    EXPECT_TRUE(detection.is_boundary(SiteId{4}));
    EXPECT_EQ(detection.boundary_site_indices.size(), 4U);
    EXPECT_FALSE(detection.incident_edges.empty());
}
