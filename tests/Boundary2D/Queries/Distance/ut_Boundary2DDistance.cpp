#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <span>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DDistance.hpp>

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

TEST(Boundary2DDistance, ComputesPointDistanceToSegment) {
    const auto projection = project_to_segment(Point2{0.5, 2.0},
                                               Point2{0.0, 0.0},
                                               Point2{1.0, 0.0});
    ASSERT_TRUE(projection.valid);
    EXPECT_NEAR(projection.point.x, 0.5, 1.0e-12);
    EXPECT_NEAR(projection.point.y, 0.0, 1.0e-12);
    EXPECT_NEAR(projection.segment_parameter, 0.5, 1.0e-12);
    EXPECT_NEAR(projection.distance, 2.0, 1.0e-12);

    EXPECT_NEAR(distance_to_segment(Point2{0.5, 2.0},
                                    Point2{0.0, 0.0},
                                    Point2{1.0, 0.0}),
                2.0,
                1.0e-12);

    EXPECT_NEAR(distance_to_segment(Point2{2.0, 0.0},
                                    Point2{0.0, 0.0},
                                    Point2{1.0, 0.0}),
                1.0,
                1.0e-12);
}

TEST(Boundary2DDistance, ComputesPointDistanceToRing) {
    const std::vector<Point2> ring{
        Point2{0.0, 0.0},
        Point2{2.0, 0.0},
        Point2{2.0, 2.0},
        Point2{0.0, 2.0}
    };

    EXPECT_NEAR(distance_to_ring(Point2{1.0, 1.0},
                                 std::span<const Point2>(ring.data(), ring.size())),
                1.0,
                1.0e-12);
    EXPECT_NEAR(distance_to_ring(Point2{0.0, 1.0},
                                 std::span<const Point2>(ring.data(), ring.size())),
                0.0,
                1.0e-12);
}

TEST(Boundary2DDistance, ComputesMinimumDistanceAcrossOuterAndHole) {
    const auto boundary = make_square_with_hole();

    const auto projection = project_to_boundary(Point2{0.5, 0.5}, boundary);
    ASSERT_TRUE(projection.valid);
    EXPECT_NEAR(projection.point.x, 0.5, 1.0e-12);
    EXPECT_NEAR(projection.point.y, 0.0, 1.0e-12);
    EXPECT_NEAR(projection.distance, 0.5, 1.0e-12);

    EXPECT_NEAR(distance_to_boundary(Point2{0.5, 0.5}, boundary),
                0.5,
                1.0e-12);
    EXPECT_NEAR(distance_to_boundary(Point2{1.5, 1.5}, boundary),
                0.5,
                1.0e-12);
    EXPECT_NEAR(distance_to_boundary(Point2{1.0, 1.5}, boundary),
                0.0,
                1.0e-12);
}

TEST(Boundary2DDistance, ChecksMinimumDistanceRequirement) {
    const auto boundary = make_square_with_hole();

    EXPECT_TRUE(has_min_distance_to_boundary(Point2{0.5, 0.5}, boundary, 0.25));
    EXPECT_TRUE(has_min_distance_to_boundary(Point2{0.5, 0.5}, boundary, 0.5));
    EXPECT_FALSE(has_min_distance_to_boundary(Point2{0.5, 0.5}, boundary, 0.75));
    EXPECT_FALSE(has_min_distance_to_boundary(Point2{1.0, 1.5}, boundary, 1.0e-12));
    EXPECT_FALSE(has_min_distance_to_boundary(Point2{1.5, 1.5}, boundary, -1.0));
}
