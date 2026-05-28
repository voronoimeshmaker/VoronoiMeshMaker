#include <gtest/gtest.h>

#include <cmath>
#include <span>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/ErrorTraits.h>
#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>

using namespace vmm::b2d;
using vmm::error::CoreErr;
using vmm::error::VMMException;

namespace {

double signed_area(std::span<const Point2> pts) {
    if (pts.size() < 3) return 0.0;

    long double area = 0.0L;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        const auto& p = pts[i];
        const auto& q = pts[(i + 1U) % pts.size()];
        area += static_cast<long double>(p.x) * static_cast<long double>(q.y)
              - static_cast<long double>(p.y) * static_cast<long double>(q.x);
    }
    return static_cast<double>(area * 0.5L);
}

double distance(Point2 a, Point2 b) {
    const double dx = static_cast<double>(a.x - b.x);
    const double dy = static_cast<double>(a.y - b.y);
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

TEST(Ring2DConstructors, RejectsInvalidRadiiAndSegments) {
    EXPECT_THROW({
        try {
            Ring2D ring(0.0, 1.0);
        } catch (const VMMException& e) {
            EXPECT_EQ(e.code(), ::vmm::error::error_code(CoreErr::InvalidArgument));
            throw;
        }
    }, VMMException);

    EXPECT_THROW(Ring2D(-1.0, 2.0), VMMException);
    EXPECT_THROW(Ring2D(2.0, 1.0), VMMException);
    EXPECT_THROW(Ring2D(1.0, 1.0), VMMException);
    EXPECT_THROW(Ring2D(0.5, 1.0, 2), VMMException);
}

TEST(Ring2DGeometry, GeneratesOuterAndHoleWithExpectedOrientation) {
    Ring2D ring(Point2{2.0, -1.0}, 1.0, 3.0, 32);

    const auto outer = ring.outer_ring();
    const auto hole = ring.hole_ring();

    ASSERT_EQ(outer.size(), 32u);
    ASSERT_EQ(hole.size(), 32u);

    EXPECT_GT(signed_area(std::span<const Point2>(outer.data(), outer.size())), 0.0);
    EXPECT_LT(signed_area(std::span<const Point2>(hole.data(), hole.size())), 0.0);

    EXPECT_NEAR(distance(outer.front(), ring.center), 3.0, 1e-12);
    EXPECT_NEAR(distance(hole.front(), ring.center), 1.0, 1e-12);
}

TEST(Ring2DGeometry, PolygonizeReturnsOuterRingForShapeConceptCompatibility) {
    Ring2D ring(0.25, 1.0, 16);
    PolygonizePolicy policy{};

    const auto outer = ring.polygonize(policy);

    ASSERT_EQ(outer.size(), 16u);
    EXPECT_GT(signed_area(std::span<const Point2>(outer.data(), outer.size())), 0.0);
}

TEST(Ring2DGeometry, WritesIntoProvidedBuffers) {
    Ring2D ring(0.5, 2.0, 8);

    std::vector<Point2> outer(8);
    std::vector<Point2> hole(8);

    EXPECT_NO_THROW(ring.outer_ring_into(std::span<Point2>(outer.data(), outer.size())));
    EXPECT_NO_THROW(ring.hole_ring_into(std::span<Point2>(hole.data(), hole.size())));

    EXPECT_GT(signed_area(std::span<const Point2>(outer.data(), outer.size())), 0.0);
    EXPECT_LT(signed_area(std::span<const Point2>(hole.data(), hole.size())), 0.0);

    std::vector<Point2> too_small(7);
    EXPECT_THROW(ring.outer_ring_into(std::span<Point2>(too_small.data(), too_small.size())),
                 VMMException);
}

TEST(Ring2DGeometry, BuildsBoundaryDataWithOuterAndHole) {
    Ring2D ring(Point2{1.0, 2.0}, 0.5, 2.0, 12);
    PolygonizePolicy policy{};

    const auto data = ring.boundary_data(policy, RegionId{7});

    ASSERT_TRUE(data.invariant_ok());
    EXPECT_EQ(data.ring_count(), 2);
    EXPECT_EQ(data.vertex_count(), 24);
    EXPECT_EQ(data.kinds[0], LoopKind::Outer);
    EXPECT_EQ(data.kinds[1], LoopKind::Hole);
    EXPECT_EQ(data.regions[0], RegionId{7});
    EXPECT_EQ(data.regions[1], RegionId{7});

    const auto outer = data.ring(0);
    const auto hole = data.ring(1);
    EXPECT_GT(signed_area(outer), 0.0);
    EXPECT_LT(signed_area(hole), 0.0);
}
