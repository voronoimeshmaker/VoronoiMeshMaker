#include <gtest/gtest.h>

#include <limits>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>
#include <VoronoiMeshMaker/Sites2D/SiteValidation.hpp>

using namespace vmm::b2d;
using namespace vmm::s2d;
using vmm::error::VMMException;

namespace {

Boundary2DData rectangle_domain() {
    return make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 3.0),
                         PolygonizePolicy{},
                         RegionId{1});
}

} // namespace

TEST(SiteValidation, AcceptsStrictlyInteriorSiteWithMinimumBoundaryDistance) {
    const auto boundary = rectangle_domain();
    const Site2D site(Point2{2.0, 1.5});
    const SiteValidationOptions options{.min_distance_to_boundary = 1.0};

    const auto report = validate_site(site, boundary, options);

    EXPECT_TRUE(report);
}

TEST(SiteValidation, RejectsSiteOnBoundaryEvenWhenMinimumDistanceIsZero) {
    const auto boundary = rectangle_domain();
    const Site2D site(Point2{0.0, 1.5});

    const auto report = validate_site(site, boundary);

    EXPECT_FALSE(report);
    EXPECT_EQ(report.error, SiteValidationError::OutsideDomainOrOnBoundary);
}

TEST(SiteValidation, RejectsSiteTooCloseToBoundary) {
    const auto boundary = rectangle_domain();
    const Site2D site(Point2{0.25, 1.5});
    const SiteValidationOptions options{.min_distance_to_boundary = 0.5};

    const auto report = validate_site(site, boundary, options);

    EXPECT_FALSE(report);
    EXPECT_EQ(report.error, SiteValidationError::TooCloseToBoundary);
}

TEST(SiteValidation, RejectsSiteInsideHole) {
    const auto boundary = make_boundary(Ring2D(Point2{0.0, 0.0}, 0.5, 2.0, 48));
    const Site2D site(Point2{0.0, 0.0});

    const auto report = validate_site(site, boundary);

    EXPECT_FALSE(report);
    EXPECT_EQ(report.error, SiteValidationError::OutsideDomainOrOnBoundary);
}

TEST(SiteValidation, RejectsSiteTooCloseToHoleBoundary) {
    const auto boundary = make_boundary(Ring2D(Point2{0.0, 0.0}, 1.0, 3.0, 96));
    const Site2D site(Point2{1.1, 0.0});
    const SiteValidationOptions options{.min_distance_to_boundary = 0.25};

    const auto report = validate_site(site, boundary, options);

    EXPECT_FALSE(report);
    EXPECT_EQ(report.error, SiteValidationError::TooCloseToBoundary);
}

TEST(SiteValidation, RejectsNonFiniteCoordinatesAndInvalidOptions) {
    const auto boundary = rectangle_domain();

    EXPECT_EQ(validate_site(Site2D(Point2{std::numeric_limits<Real>::infinity(), 0.0}),
                            boundary).error,
              SiteValidationError::NonFiniteCoordinate);

    EXPECT_EQ(validate_site(Site2D(Point2{1.0, 1.0}),
                            boundary,
                            SiteValidationOptions{.min_distance_to_boundary = -1.0}).error,
              SiteValidationError::InvalidOptions);
}

TEST(SiteValidation, RejectsNonSequentialSiteIds) {
    const auto boundary = rectangle_domain();
    SiteSet sites;
    sites.add(Point2{1.0, 1.0});
    sites.add(Point2{2.0, 1.0});
    sites[1].id = SiteId{42};

    const auto report = validate_sites(sites, boundary);

    EXPECT_FALSE(report);
    EXPECT_EQ(report.error, SiteValidationError::NonSequentialSiteId);
    EXPECT_EQ(report.index, 1U);
}

TEST(SiteValidation, CanValidateGeometryBeforeFinalRenumbering) {
    const auto boundary = rectangle_domain();
    SiteSet sites;
    sites.add(Point2{1.0, 1.0});
    sites.add(Point2{2.0, 1.0});
    sites[1].id = SiteId{42};

    const auto report = validate_sites(
        sites,
        boundary,
        SiteValidationOptions{.require_sequential_ids = false});

    EXPECT_TRUE(report);
}

TEST(SiteValidation, ValidatesSiteSetsAndSpacingBetweenSites) {
    const auto boundary = rectangle_domain();
    SiteSet sites;
    sites.add(Point2{1.0, 1.0});
    sites.add(Point2{1.1, 1.0});

    const SiteValidationOptions options{
        .min_distance_to_boundary = 0.2,
        .min_distance_between_sites = 0.25
    };

    const auto report = validate_sites(sites, boundary, options);

    EXPECT_FALSE(report);
    EXPECT_EQ(report.error, SiteValidationError::TooCloseToAnotherSite);
    EXPECT_EQ(report.index, 0U);
    EXPECT_EQ(report.other_index, 1U);
}

TEST(SiteValidation, ThrowingValidationReportsInvalidSets) {
    const auto boundary = rectangle_domain();
    SiteSet sites;
    sites.add(Point2{0.0, 0.0});

    EXPECT_THROW(validate_sites_or_throw(sites, boundary), VMMException);
}
