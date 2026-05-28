#include <gtest/gtest.h>

#include <array>
#include <span>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DContains.hpp>

using namespace vmm::b2d;

namespace {

Boundary2DData make_square_with_hole() {
    const std::vector<Point2> outer{
        Point2{0.0, 0.0},
        Point2{4.0, 0.0},
        Point2{4.0, 4.0},
        Point2{0.0, 4.0}
    };

    const std::vector<Point2> hole{
        Point2{1.0, 1.0},
        Point2{1.0, 2.0},
        Point2{2.0, 2.0},
        Point2{2.0, 1.0}
    };

    const std::array<std::span<const Point2>, 1> holes{
        std::span<const Point2>(hole.data(), hole.size())
    };

    return make_boundary_from_rings(
        std::span<const Point2>(outer.data(), outer.size()),
        std::span<const std::span<const Point2>>(holes.data(), holes.size()));
}

} // namespace

TEST(Boundary2DContains, DetectsPointOnSegment) {
    EXPECT_TRUE(point_on_segment(Point2{0.5, 0.0},
                                 Point2{0.0, 0.0},
                                 Point2{1.0, 0.0}));
    EXPECT_FALSE(point_on_segment(Point2{1.5, 0.0},
                                  Point2{0.0, 0.0},
                                  Point2{1.0, 0.0}));
    EXPECT_FALSE(point_on_segment(Point2{0.5, 0.1},
                                  Point2{0.0, 0.0},
                                  Point2{1.0, 0.0}));
}

TEST(Boundary2DContains, DetectsPointOnRing) {
    const std::vector<Point2> ring{
        Point2{0.0, 0.0},
        Point2{1.0, 0.0},
        Point2{1.0, 1.0},
        Point2{0.0, 1.0}
    };

    EXPECT_TRUE(point_on_ring(Point2{0.5, 0.0},
                              std::span<const Point2>(ring.data(), ring.size())));
    EXPECT_FALSE(point_on_ring(Point2{0.5, 0.5},
                               std::span<const Point2>(ring.data(), ring.size())));
}

TEST(Boundary2DContains, StrictRingQueryExcludesBoundary) {
    const std::vector<Point2> ring{
        Point2{0.0, 0.0},
        Point2{2.0, 0.0},
        Point2{2.0, 2.0},
        Point2{0.0, 2.0}
    };

    EXPECT_TRUE(point_in_ring_strict(Point2{1.0, 1.0},
                                     std::span<const Point2>(ring.data(), ring.size())));
    EXPECT_FALSE(point_in_ring_strict(Point2{0.0, 1.0},
                                      std::span<const Point2>(ring.data(), ring.size())));
    EXPECT_FALSE(point_in_ring_strict(Point2{3.0, 1.0},
                                      std::span<const Point2>(ring.data(), ring.size())));
}

TEST(Boundary2DContains, HandlesPolygonWithHole) {
    const auto boundary = make_square_with_hole();

    EXPECT_TRUE(contains(Point2{0.5, 0.5}, boundary));
    EXPECT_FALSE(contains(Point2{1.5, 1.5}, boundary));
    EXPECT_FALSE(contains(Point2{5.0, 5.0}, boundary));
}

TEST(Boundary2DContains, HonorsBoundaryInclusionFlag) {
    const auto boundary = make_square_with_hole();

    EXPECT_TRUE(contains(Point2{0.0, 2.0}, boundary, true));
    EXPECT_FALSE(contains(Point2{0.0, 2.0}, boundary, false));

    EXPECT_TRUE(contains(Point2{1.0, 1.5}, boundary, true));
    EXPECT_FALSE(contains(Point2{1.0, 1.5}, boundary, false));
}

TEST(Boundary2DContains, RejectsInvalidBoundaryData) {
    Boundary2DData boundary;
    boundary.points.push_back(Point2{0.0, 0.0});

    EXPECT_FALSE(contains(Point2{0.0, 0.0}, boundary));
}
