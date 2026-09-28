#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/LloydOptimizer2D.hpp>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>

using namespace vmm::b2d;
using namespace vmm::s2d;
using namespace vmm::vd2d;

namespace {

[[nodiscard]] double dist(Point2 a, Point2 b) noexcept {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

TEST(CVT, LloydStepMovesGeneratorsToClippedCellCentroids) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 1.0, 1.0));
    SiteSet sites;
    sites.add(Point2{0.18, 0.22});
    sites.add(Point2{0.82, 0.18});
    sites.add(Point2{0.76, 0.81});
    sites.add(Point2{0.24, 0.78});

    const auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);
    const auto moved = lloyd_step(sites, boundary);

    ASSERT_EQ(moved.size(), sites.size());
    for (std::size_t i = 0; i < moved.size(); ++i) {
        const auto centroid = diagram.cell(SiteId{static_cast<Index>(i)}).centroid();
        EXPECT_NEAR(moved[i].point.x, centroid.x, 1.0e-12);
        EXPECT_NEAR(moved[i].point.y, centroid.y, 1.0e-12);
        EXPECT_EQ(moved[i].id, SiteId{static_cast<Index>(i)});
    }
}

TEST(CVT, LloydRelaxKeepsCartesianCentroidalMeshStationary) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 1.0, 1.0));
    const auto sites = make_sites(
        boundary,
        CartesianGridCount2D{2, 2},
        SiteValidationOptions{});

    const LloydOptions2D options{
        .max_iterations = 5U,
        .tolerance = 1.0e-12,
        .stop_on_tolerance = true
    };

    const auto result = lloyd_relax(sites, boundary, options);

    EXPECT_TRUE(result.converged);
    ASSERT_EQ(result.iterations(), 1U);
    ASSERT_EQ(result.sites.size(), sites.size());
    ASSERT_FALSE(result.history.empty());
    EXPECT_LE(result.history.front().max_displacement, 1.0e-12);
    EXPECT_NEAR(result.diagram.total_volume_area(), 1.0, 1.0e-12);

    for (std::size_t i = 0; i < sites.size(); ++i) {
        EXPECT_NEAR(result.sites[i].point.x, sites[i].point.x, 1.0e-12);
        EXPECT_NEAR(result.sites[i].point.y, sites[i].point.y, 1.0e-12);
    }
}

TEST(CVT, LloydRelaxReducesCvtEnergyForPerturbedSites) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 1.0, 1.0));
    SiteSet sites;
    sites.add(Point2{0.16, 0.28});
    sites.add(Point2{0.72, 0.15});
    sites.add(Point2{0.88, 0.72});
    sites.add(Point2{0.33, 0.86});
    sites.add(Point2{0.51, 0.47});

    const LloydOptions2D options{
        .max_iterations = 4U,
        .tolerance = 0.0,
        .stop_on_tolerance = false
    };

    const auto result = lloyd_relax(sites, boundary, options);

    ASSERT_EQ(result.history.size(), 4U);
    EXPECT_FALSE(result.converged);
    EXPECT_GT(result.history.front().max_displacement, 0.0);
    EXPECT_LT(result.history.back().cvt_energy, result.history.front().cvt_energy);
    EXPECT_NEAR(result.diagram.total_volume_area(), 1.0, 1.0e-12);

    for (const auto& site : result.sites) {
        EXPECT_TRUE(contains(site.point, boundary, true));
    }
}

TEST(CVT, LloydRelaxRejectsEmptySiteSet) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 1.0, 1.0));
    const SiteSet sites;

    EXPECT_THROW((void)lloyd_relax(sites, boundary), vmm::error::VMMException);
}
