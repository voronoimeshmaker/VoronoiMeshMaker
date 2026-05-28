#include <gtest/gtest.h>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>

using namespace vmm::b2d;
using namespace vmm::s2d;
using vmm::error::VMMException;

TEST(SiteFactory, CartesianGridCreatesSequentialSitesInsideRectangle) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 3.0),
                                        PolygonizePolicy{},
                                        RegionId{2});

    const auto sites = make_sites(boundary,
                                  CartesianGrid2D{1.0},
                                  SiteValidationOptions{},
                                  RegionId{2});

    ASSERT_EQ(sites.size(), 12U);
    EXPECT_TRUE(sites.ids_are_sequential());
    EXPECT_TRUE(validate_sites(sites, boundary));

    EXPECT_EQ(sites[0].id, SiteId{0});
    EXPECT_EQ(sites[11].id, SiteId{11});
    EXPECT_DOUBLE_EQ(sites[0].point.x, 0.5);
    EXPECT_DOUBLE_EQ(sites[0].point.y, 0.5);
    EXPECT_EQ(sites[0].region, RegionId{2});
}

TEST(SiteFactory, CartesianGridRespectsMinimumBoundaryDistance) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 3.0));

    const auto sites = make_sites(
        boundary,
        CartesianGrid2D{0.5},
        SiteValidationOptions{.min_distance_to_boundary = 0.5});

    ASSERT_FALSE(sites.empty());
    EXPECT_TRUE(validate_sites(
        sites,
        boundary,
        SiteValidationOptions{.min_distance_to_boundary = 0.5}));

    for (const auto& site : sites) {
        EXPECT_GE(site.point.x, 0.5);
        EXPECT_LE(site.point.x, 3.5);
        EXPECT_GE(site.point.y, 0.5);
        EXPECT_LE(site.point.y, 2.5);
    }
}

TEST(SiteFactory, CartesianGridCanUseExplicitOrigin) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 3.0));

    const auto sites = make_sites(
        boundary,
        CartesianGrid2D{1.0}.with_origin(Point2{1.0, 1.0}));

    ASSERT_FALSE(sites.empty());
    EXPECT_DOUBLE_EQ(sites[0].point.x, 1.0);
    EXPECT_DOUBLE_EQ(sites[0].point.y, 1.0);
    EXPECT_TRUE(sites.ids_are_sequential());
}

TEST(SiteFactory, CartesianGridEpsilonUsesPaperFormulaForThetaZero) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 4.0));

    const auto centered = make_sites(
        boundary,
        CartesianGrid2D{1.0}.with_epsilon(0.0));
    const auto eccentric = make_sites(
        boundary,
        CartesianGrid2D{1.0}.with_epsilon(0.5));

    ASSERT_EQ(centered.size(), 16U);
    ASSERT_EQ(eccentric.size(), 16U);

    EXPECT_DOUBLE_EQ(centered[0].point.x, 0.5);
    EXPECT_DOUBLE_EQ(centered[0].point.y, 0.5);

    // theta = 0: x = xc + e*h/2 and y = yc - e*h/2.
    EXPECT_DOUBLE_EQ(eccentric[0].point.x, 0.75);
    EXPECT_DOUBLE_EQ(eccentric[0].point.y, 0.25);
    EXPECT_TRUE(eccentric.ids_are_sequential());
    EXPECT_TRUE(validate_sites(eccentric, boundary));
}

TEST(SiteFactory, CartesianGridRejectsInvalidEpsilon) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 4.0));

    EXPECT_THROW((void)make_sites(
                     boundary,
                     CartesianGrid2D{1.0}.with_epsilon(1.0)),
                 VMMException);
    EXPECT_THROW((void)make_sites(
                     boundary,
                     CartesianGrid2D{1.0}.with_epsilon(-1.0)),
                 VMMException);
}

TEST(SiteFactory, CartesianGridCanBeCenteredInBoundingBox) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 3.0));

    const auto sites = make_sites(
        boundary,
        CartesianGrid2D{1.5}.centered_in_box());

    ASSERT_FALSE(sites.empty());
    EXPECT_NEAR(sites[0].point.x, 0.5, 1.0e-12);
    EXPECT_NEAR(sites[0].point.y, 1.5, 1.0e-12);
    EXPECT_TRUE(sites.ids_are_sequential());
}

TEST(SiteFactory, CartesianGridCountUsesRequestedCandidateLayout) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 2.0));

    const auto sites = make_sites(boundary, CartesianGridCount2D{4, 2});

    ASSERT_EQ(sites.size(), 8U);
    EXPECT_DOUBLE_EQ(sites[0].point.x, 0.5);
    EXPECT_DOUBLE_EQ(sites[0].point.y, 0.5);
    EXPECT_DOUBLE_EQ(sites[7].point.x, 3.5);
    EXPECT_DOUBLE_EQ(sites[7].point.y, 1.5);
    EXPECT_TRUE(validate_sites(sites, boundary));
}

TEST(SiteFactory, TriangularIIRawSitesFollowLegacyPatternAtZeroEpsilon) {
    const SiteGenerationBox2D box{Point2{0.0, 0.0}, Point2{2.0, 2.0}};

    const auto sites = make_raw_sites(box, TriangularIIGrid2D{1.0}.with_epsilon(0.0));

    ASSERT_EQ(sites.size(), 8U);
    EXPECT_TRUE(sites.ids_are_sequential());

    EXPECT_NEAR(sites[0].point.x, 1.0 / 3.0, 1.0e-12);
    EXPECT_NEAR(sites[0].point.y, 2.0 / 3.0, 1.0e-12);
    EXPECT_NEAR(sites[1].point.x, 2.0 / 3.0, 1.0e-12);
    EXPECT_NEAR(sites[1].point.y, 1.0 / 3.0, 1.0e-12);
    EXPECT_NEAR(sites[7].point.x, 5.0 / 3.0, 1.0e-12);
    EXPECT_NEAR(sites[7].point.y, 4.0 / 3.0, 1.0e-12);
}

TEST(SiteFactory, TriangularIIRawSitesUseEpsilon) {
    const SiteGenerationBox2D box{Point2{0.0, 0.0}, Point2{2.0, 2.0}};

    const auto positive = make_raw_sites(
        box,
        TriangularIIGrid2D{1.0}.with_epsilon(0.6));
    const auto negative = make_raw_sites(
        box,
        TriangularIIGrid2D{1.0}.with_epsilon(-0.6));

    ASSERT_EQ(positive.size(), 8U);
    ASSERT_EQ(negative.size(), 8U);

    EXPECT_NEAR(positive[0].point.x, 13.0 / 30.0, 1.0e-12);
    EXPECT_NEAR(positive[0].point.y, 17.0 / 30.0, 1.0e-12);
    EXPECT_NEAR(negative[0].point.x, 2.0 / 15.0, 1.0e-12);
    EXPECT_NEAR(negative[0].point.y, 13.0 / 15.0, 1.0e-12);
}

TEST(SiteFactory, TriangularIVRawSitesFollowLegacyPatternAtZeroEpsilon) {
    const SiteGenerationBox2D box{Point2{0.0, 0.0}, Point2{1.0, 1.0}};

    const auto sites = make_raw_sites(box, TriangularIVGrid2D{1.0}.with_epsilon(0.0));

    ASSERT_EQ(sites.size(), 4U);
    EXPECT_TRUE(sites.ids_are_sequential());

    EXPECT_NEAR(sites[0].point.x, 1.0 / 3.0, 1.0e-12);
    EXPECT_NEAR(sites[0].point.y, 0.5, 1.0e-12);
    EXPECT_NEAR(sites[1].point.x, 0.5, 1.0e-12);
    EXPECT_NEAR(sites[1].point.y, 1.0 / 3.0, 1.0e-12);
    EXPECT_NEAR(sites[2].point.x, 2.0 / 3.0, 1.0e-12);
    EXPECT_NEAR(sites[2].point.y, 0.5, 1.0e-12);
    EXPECT_NEAR(sites[3].point.x, 0.5, 1.0e-12);
    EXPECT_NEAR(sites[3].point.y, 2.0 / 3.0, 1.0e-12);
}

TEST(SiteFactory, TriangularIVRawSitesUseEpsilon) {
    const SiteGenerationBox2D box{Point2{0.0, 0.0}, Point2{1.0, 1.0}};

    const auto positive = make_raw_sites(
        box,
        TriangularIVGrid2D{1.0}.with_epsilon(0.6));
    const auto negative = make_raw_sites(
        box,
        TriangularIVGrid2D{1.0}.with_epsilon(-0.6));

    ASSERT_EQ(positive.size(), 4U);
    ASSERT_EQ(negative.size(), 4U);

    EXPECT_NEAR(positive[0].point.x, 13.0 / 30.0, 1.0e-12);
    EXPECT_NEAR(positive[2].point.x, 17.0 / 30.0, 1.0e-12);
    EXPECT_NEAR(negative[0].point.x, 2.0 / 15.0, 1.0e-12);
    EXPECT_NEAR(negative[2].point.x, 13.0 / 15.0, 1.0e-12);
}

TEST(SiteFactory, HexagonalRawSitesCreateStaggeredRows) {
    const SiteGenerationBox2D box{Point2{0.0, 0.0}, Point2{2.0, 2.0}};

    const auto sites = make_raw_sites(box, HexagonalGrid2D{2});

    ASSERT_EQ(sites.size(), 5U);
    EXPECT_TRUE(sites.ids_are_sequential());

    const double row_spacing = std::sqrt(3.0) * 0.5;
    const double y0 = (2.0 - row_spacing) * 0.5;
    EXPECT_DOUBLE_EQ(sites[0].point.x, 0.0);
    EXPECT_NEAR(sites[0].point.y, y0, 1.0e-12);
    EXPECT_DOUBLE_EQ(sites[1].point.x, 1.0);
    EXPECT_NEAR(sites[1].point.y, y0, 1.0e-12);
    EXPECT_DOUBLE_EQ(sites[2].point.x, 2.0);
    EXPECT_NEAR(sites[2].point.y, y0, 1.0e-12);
    EXPECT_DOUBLE_EQ(sites[3].point.x, 0.5);
    EXPECT_NEAR(sites[3].point.y, y0 + row_spacing, 1.0e-12);
    EXPECT_DOUBLE_EQ(sites[4].point.x, 1.5);
    EXPECT_NEAR(sites[4].point.y, y0 + row_spacing, 1.0e-12);
    EXPECT_NEAR(sites[0].point.y - box.min.y,
                box.max.y - sites[3].point.y,
                1.0e-12);
    for (const auto& site : sites) {
        EXPECT_GE(site.point.x, box.min.x);
        EXPECT_LE(site.point.x, box.max.x);
        EXPECT_GT(site.point.y, box.min.y);
        EXPECT_LT(site.point.y, box.max.y);
    }
}

TEST(SiteFactory, StructuredPatternsAreClippedAndValidatedByBoundary) {
    const auto boundary = make_boundary(Ring2D(Point2{0.0, 0.0}, 1.0, 3.0, 128));
    const SiteValidationOptions options{.min_distance_to_boundary = 0.2};

    const auto tri2 = make_sites(boundary, TriangularIIGrid2D{0.55}, options);
    const auto tri4 = make_sites(boundary, TriangularIVGrid2D{0.70}, options);
    const auto hex = make_sites(boundary, HexagonalGrid2D{8}, options);

    ASSERT_FALSE(tri2.empty());
    ASSERT_FALSE(tri4.empty());
    ASSERT_FALSE(hex.empty());
    EXPECT_TRUE(validate_sites(tri2, boundary, options));
    EXPECT_TRUE(validate_sites(tri4, boundary, options));
    EXPECT_TRUE(validate_sites(hex, boundary, options));
}

TEST(SiteFactory, CartesianGridSkipsRingHole) {
    const auto boundary = make_boundary(Ring2D(Point2{0.0, 0.0}, 1.0, 3.0, 96));

    const auto sites = make_sites(
        boundary,
        CartesianGrid2D{0.5},
        SiteValidationOptions{.min_distance_to_boundary = 0.2});

    ASSERT_FALSE(sites.empty());
    EXPECT_TRUE(validate_sites(
        sites,
        boundary,
        SiteValidationOptions{.min_distance_to_boundary = 0.2}));

    for (const auto& site : sites) {
        const auto r2 = site.point.x * site.point.x + site.point.y * site.point.y;
        EXPECT_GT(r2, 1.0);
        EXPECT_LT(r2, 9.0);
    }
}

TEST(SiteFactory, CartesianGridRejectsInvalidPatternAndValidationOptions) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 3.0));

    EXPECT_THROW((void)make_sites(boundary, CartesianGrid2D{0.0}), VMMException);
    EXPECT_THROW((void)make_sites(boundary, CartesianGridCount2D{0, 3}), VMMException);
    EXPECT_THROW((void)make_sites(boundary, TriangularIIGrid2D{0.0}), VMMException);
    EXPECT_THROW((void)make_sites(boundary, TriangularIVGrid2D{0.0}), VMMException);
    EXPECT_THROW((void)make_sites(boundary, HexagonalGrid2D{0}), VMMException);
    EXPECT_THROW((void)make_sites(
                     boundary,
                     TriangularIIGrid2D{1.0}.with_epsilon(1.0)),
                 VMMException);
    EXPECT_THROW((void)make_sites(
                     boundary,
                     TriangularIVGrid2D{1.0}.with_epsilon(-1.0)),
                 VMMException);
    EXPECT_THROW((void)make_sites(boundary, UniformRandom2D{0}), VMMException);
    EXPECT_THROW((void)make_sites(
                     boundary,
                     CartesianGrid2D{1.0},
                     SiteValidationOptions{.min_distance_to_boundary = -1.0}),
                 VMMException);
}

TEST(SiteFactory, UniformRandomIsReproducibleAndValidated) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 3.0));
    const SiteValidationOptions options{
        .min_distance_to_boundary = 0.1,
        .min_distance_between_sites = 0.1
    };

    const auto a = make_sites(boundary, UniformRandom2D{12, 1234U}, options);
    const auto b = make_sites(boundary, UniformRandom2D{12, 1234U}, options);

    ASSERT_EQ(a.size(), 12U);
    ASSERT_EQ(b.size(), 12U);
    EXPECT_TRUE(a.ids_are_sequential());
    EXPECT_TRUE(validate_sites(a, boundary, options));

    for (std::size_t i = 0; i < a.size(); ++i) {
        EXPECT_DOUBLE_EQ(a[i].point.x, b[i].point.x);
        EXPECT_DOUBLE_EQ(a[i].point.y, b[i].point.y);
    }
}

TEST(SiteFactory, UniformRandomScalesBeyondTenThousandSites) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 100.0, 100.0));
    const SiteValidationOptions options{
        .min_distance_to_boundary = 0.1,
        .min_distance_between_sites = 0.05
    };

    const auto sites = make_sites(boundary, UniformRandom2D{12000, 5678U}, options);

    ASSERT_EQ(sites.size(), 12000U);
    EXPECT_TRUE(sites.ids_are_sequential());
    EXPECT_TRUE(validate_sites(sites, boundary, options));
}

TEST(SiteFactory, CanAppendCartesianAndRandomLayersInSameBoundary) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 5.0, 4.0));
    const SiteValidationOptions options{
        .min_distance_to_boundary = 0.25,
        .min_distance_between_sites = 0.25
    };

    auto sites = make_sites(boundary, CartesianGridCount2D{3, 2}, options, RegionId{1});
    const auto cartesian_count = sites.size();

    append_sites(sites,
                 boundary,
                 UniformRandom2D{10, 42U},
                 options,
                 RegionId{2});

    ASSERT_EQ(sites.size(), cartesian_count + 10U);
    EXPECT_TRUE(sites.ids_are_sequential());
    EXPECT_TRUE(validate_sites(sites, boundary, options));
    EXPECT_EQ(sites[0].region, RegionId{1});
    EXPECT_EQ(sites[sites.size() - 1U].region, RegionId{2});
}

TEST(SiteFactory, CanAppendAllStructuredPatternsInSameBoundary) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 5.0, 4.0));
    const SiteValidationOptions options{
        .min_distance_to_boundary = 0.20,
        .min_distance_between_sites = 0.20
    };

    auto sites = make_sites(boundary, CartesianGridCount2D{3, 2}, options, RegionId{1});
    const auto first_count = sites.size();

    append_sites(sites, boundary, TriangularIIGrid2D{0.75}, options, RegionId{2});
    append_sites(sites, boundary, TriangularIVGrid2D{0.90}, options, RegionId{3});
    append_sites(sites, boundary, HexagonalGrid2D{4}, options, RegionId{4});

    ASSERT_GT(sites.size(), first_count);
    EXPECT_TRUE(sites.ids_are_sequential());
    EXPECT_TRUE(validate_sites(sites, boundary, options));
    EXPECT_EQ(sites[0].region, RegionId{1});
}

TEST(SiteFactory, CartesianGridFiltersCandidatesTooCloseToExistingSites) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 4.0, 3.0));

    const SiteValidationOptions options{.min_distance_between_sites = 0.75};
    const auto sites = make_sites(boundary, CartesianGrid2D{0.5}, options);

    EXPECT_FALSE(sites.empty());
    EXPECT_LT(sites.size(), 48U);
    EXPECT_TRUE(validate_sites(sites, boundary, options));
}
