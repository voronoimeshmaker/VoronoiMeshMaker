#include <gtest/gtest.h>

#include <array>
#include <span>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Validation/Boundary2DValidation.hpp>

using namespace vmm::b2d;

namespace {

std::vector<Point2> square_ccw() {
    return {
        Point2{0.0, 0.0},
        Point2{4.0, 0.0},
        Point2{4.0, 4.0},
        Point2{0.0, 4.0}
    };
}

std::vector<Point2> square_cw() {
    return {
        Point2{0.0, 0.0},
        Point2{0.0, 4.0},
        Point2{4.0, 4.0},
        Point2{4.0, 0.0}
    };
}

std::vector<Point2> hole_cw() {
    return {
        Point2{1.0, 1.0},
        Point2{1.0, 2.0},
        Point2{2.0, 2.0},
        Point2{2.0, 1.0}
    };
}

Boundary2DData boundary_with_hole() {
    const auto outer = square_ccw();
    const auto hole = hole_cw();
    const std::array<std::span<const Point2>, 1> holes{
        std::span<const Point2>(hole.data(), hole.size())
    };

    return make_boundary_from_rings(
        std::span<const Point2>(outer.data(), outer.size()),
        std::span<const std::span<const Point2>>(holes.data(), holes.size()));
}

} // namespace

TEST(Boundary2DValidation, ComputesSignedAndAbsoluteRingArea) {
    const auto ccw = square_ccw();
    const auto cw = square_cw();

    EXPECT_NEAR(signed_area(std::span<const Point2>(ccw.data(), ccw.size())),
                16.0,
                1.0e-12);
    EXPECT_NEAR(signed_area(std::span<const Point2>(cw.data(), cw.size())),
                -16.0,
                1.0e-12);
    EXPECT_NEAR(ring_area(std::span<const Point2>(cw.data(), cw.size())),
                16.0,
                1.0e-12);
}

TEST(Boundary2DValidation, DetectsRingOrientation) {
    const auto ccw = square_ccw();
    const auto cw = square_cw();

    EXPECT_TRUE(is_counter_clockwise(std::span<const Point2>(ccw.data(), ccw.size())));
    EXPECT_FALSE(is_clockwise(std::span<const Point2>(ccw.data(), ccw.size())));
    EXPECT_TRUE(is_clockwise(std::span<const Point2>(cw.data(), cw.size())));
    EXPECT_FALSE(is_counter_clockwise(std::span<const Point2>(cw.data(), cw.size())));
}

TEST(Boundary2DValidation, ComputesBoundaryAreaWithHoles) {
    const auto boundary = boundary_with_hole();

    EXPECT_NEAR(boundary_area(boundary), 15.0, 1.0e-12);
}

TEST(Boundary2DValidation, ComputesBoundingBox) {
    const auto boundary = boundary_with_hole();

    const auto box = bounding_box(boundary);

    EXPECT_FALSE(box.is_empty());
    EXPECT_NEAR(box.min.x, 0.0, 1.0e-12);
    EXPECT_NEAR(box.min.y, 0.0, 1.0e-12);
    EXPECT_NEAR(box.max.x, 4.0, 1.0e-12);
    EXPECT_NEAR(box.max.y, 4.0, 1.0e-12);
}

TEST(Boundary2DValidation, ValidatesMinimalPolygonWithHole) {
    const auto boundary = boundary_with_hole();

    EXPECT_TRUE(has_expected_orientation(boundary));
    EXPECT_TRUE(is_valid_boundary_minimal(boundary));
}

TEST(Boundary2DValidation, RejectsUnexpectedOuterOrientation) {
    const auto outer = square_cw();
    const auto boundary = make_boundary_from_outer(
        std::span<const Point2>(outer.data(), outer.size()));

    EXPECT_FALSE(has_expected_orientation(boundary));
    EXPECT_FALSE(is_valid_boundary_minimal(boundary));
}
