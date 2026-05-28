#include <gtest/gtest.h>

#include <cmath>
#include <execution>
#include <span>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Transforms/Boundary2DTransform.hpp>

using namespace vmm::b2d;

namespace {

constexpr double kTol = 1.0e-12;
constexpr double kPi = 3.141592653589793238462643383279502884;

void expect_near(Point2 p, Point2 expected) {
    EXPECT_NEAR(p.x, expected.x, kTol);
    EXPECT_NEAR(p.y, expected.y, kTol);
}

Boundary2DData make_unit_square() {
    const std::vector<Point2> outer{
        Point2{0.0, 0.0},
        Point2{1.0, 0.0},
        Point2{1.0, 1.0},
        Point2{0.0, 1.0}
    };

    return make_boundary_from_outer(
        std::span<const Point2>(outer.data(), outer.size()),
        RegionId{3});
}

} // namespace

TEST(Boundary2DTransform, AppliesAffineTransformToPoint) {
    const Affine2 transform = compose(translation_transform(2.0, -1.0),
                                      rotation_transform(kPi / 2.0));

    expect_near(apply_transform(Point2{1.0, 0.0}, transform),
                Point2{2.0, 0.0});
}

TEST(Boundary2DTransform, TranslatesBoundaryInPlacePreservingTopology) {
    auto boundary = make_unit_square();

    translate_in_place(boundary, 2.0, -3.0);

    ASSERT_TRUE(boundary.invariant_ok());
    EXPECT_EQ(boundary.ring_count(), 1);
    EXPECT_EQ(boundary.regions[0], RegionId{3});

    const auto ring = boundary.ring(0);
    expect_near(ring[0], Point2{2.0, -3.0});
    expect_near(ring[1], Point2{3.0, -3.0});
    expect_near(ring[2], Point2{3.0, -2.0});
    expect_near(ring[3], Point2{2.0, -2.0});
}

TEST(Boundary2DTransform, RotatesBoundaryAroundOrigin) {
    auto boundary = make_unit_square();

    rotate_in_place(boundary, kPi / 2.0);

    const auto ring = boundary.ring(0);
    expect_near(ring[0], Point2{0.0, 0.0});
    expect_near(ring[1], Point2{0.0, 1.0});
    expect_near(ring[2], Point2{-1.0, 1.0});
    expect_near(ring[3], Point2{-1.0, 0.0});
}

TEST(Boundary2DTransform, RotatesBoundaryAroundArbitraryCenter) {
    auto boundary = make_unit_square();

    rotate_in_place(boundary, kPi, Point2{0.5, 0.5});

    const auto ring = boundary.ring(0);
    expect_near(ring[0], Point2{1.0, 1.0});
    expect_near(ring[1], Point2{0.0, 1.0});
    expect_near(ring[2], Point2{0.0, 0.0});
    expect_near(ring[3], Point2{1.0, 0.0});
}

TEST(Boundary2DTransform, CopyReturningTransformLeavesInputUnchanged) {
    auto boundary = make_unit_square();

    const auto moved = translated(boundary, -1.0, 2.0);

    expect_near(boundary.ring(0)[0], Point2{0.0, 0.0});
    expect_near(moved.ring(0)[0], Point2{-1.0, 2.0});
}

TEST(Boundary2DTransform, ParallelPolicyOverloadTransformsPointSpan) {
    std::vector<Point2> points(1024, Point2{1.0, 2.0});

    transform_in_place(std::execution::par,
                       std::span<Point2>(points.data(), points.size()),
                       translation_transform(3.0, 4.0));

    for (const auto point : points) {
        expect_near(point, Point2{4.0, 6.0});
    }
}
