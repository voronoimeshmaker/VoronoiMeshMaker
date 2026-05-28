#include <gtest/gtest.h>

#include <array>
#include <span>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>

using namespace vmm::b2d;
using vmm::error::VMMException;

namespace {

std::vector<Point2> square_ccw() {
    return {
        Point2{0.0, 0.0},
        Point2{4.0, 0.0},
        Point2{4.0, 4.0},
        Point2{0.0, 4.0}
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

} // namespace

TEST(Boundary2DBuilder, BuildsBoundaryFromSingleOuterRing) {
    const auto outer = square_ccw();

    const auto boundary = make_boundary_from_outer(
        std::span<const Point2>(outer.data(), outer.size()),
        RegionId{9});

    ASSERT_TRUE(boundary.invariant_ok());
    EXPECT_EQ(boundary.ring_count(), 1);
    EXPECT_EQ(boundary.vertex_count(), 4);
    EXPECT_EQ(boundary.kinds[0], LoopKind::Outer);
    EXPECT_EQ(boundary.regions[0], RegionId{9});
}

TEST(Boundary2DBuilder, BuildsBoundaryFromOuterAndHoles) {
    const auto outer = square_ccw();
    const auto hole = hole_cw();
    const std::array<std::span<const Point2>, 1> holes{
        std::span<const Point2>(hole.data(), hole.size())
    };

    const auto boundary = make_boundary_from_rings(
        std::span<const Point2>(outer.data(), outer.size()),
        std::span<const std::span<const Point2>>(holes.data(), holes.size()),
        RegionId{4});

    ASSERT_TRUE(boundary.invariant_ok());
    EXPECT_EQ(boundary.ring_count(), 2);
    EXPECT_EQ(boundary.vertex_count(), 8);
    EXPECT_EQ(boundary.kinds[0], LoopKind::Outer);
    EXPECT_EQ(boundary.kinds[1], LoopKind::Hole);
    EXPECT_EQ(boundary.regions[1], RegionId{4});
}

TEST(Boundary2DBuilder, RejectsRingsWithTooFewVertices) {
    const std::array<Point2, 2> invalid{
        Point2{0.0, 0.0},
        Point2{1.0, 0.0}
    };

    EXPECT_THROW(
        make_boundary_from_outer(
            std::span<const Point2>(invalid.data(), invalid.size())),
        VMMException);
}

TEST(Boundary2DBuilder, UsesPolygonizeForSimpleShapes) {
    const Rectangle rectangle(2.0, 3.0);

    const auto boundary = make_boundary(rectangle, PolygonizePolicy{}, RegionId{8});

    ASSERT_TRUE(boundary.invariant_ok());
    EXPECT_EQ(boundary.ring_count(), 1);
    EXPECT_EQ(boundary.vertex_count(), 4);
    EXPECT_EQ(boundary.kinds[0], LoopKind::Outer);
    EXPECT_EQ(boundary.regions[0], RegionId{8});
}

TEST(Boundary2DBuilder, PreservesHolesWhenShapeProvidesBoundaryData) {
    const Ring2D ring(Point2{0.0, 0.0}, 1.0, 2.0, 16);

    const auto boundary = make_boundary(ring, PolygonizePolicy{}, RegionId{5});

    ASSERT_TRUE(boundary.invariant_ok());
    EXPECT_EQ(boundary.ring_count(), 2);
    EXPECT_EQ(boundary.vertex_count(), 32);
    EXPECT_EQ(boundary.kinds[0], LoopKind::Outer);
    EXPECT_EQ(boundary.kinds[1], LoopKind::Hole);
    EXPECT_EQ(boundary.regions[0], RegionId{5});
    EXPECT_EQ(boundary.regions[1], RegionId{5});
}
