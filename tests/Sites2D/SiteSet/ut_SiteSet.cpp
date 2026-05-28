#include <gtest/gtest.h>

#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>
#include <VoronoiMeshMaker/Site2DTransform.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>

using namespace vmm::s2d;
using vmm::error::VMMException;

TEST(SiteSet, AddsSitesWithSequentialIds) {
    SiteSet sites;
    sites.reserve(2);

    const auto& first = sites.add(Point2{0.0, 0.0}, RegionId{4});
    const auto& second = sites.add(Point2{1.0, 0.0}, RegionId{5}, 0.5);

    ASSERT_EQ(sites.size(), 2U);
    EXPECT_EQ(first.id, SiteId{0});
    EXPECT_EQ(second.id, SiteId{1});
    EXPECT_EQ(sites[0].region, RegionId{4});
    EXPECT_EQ(sites[1].region, RegionId{5});
    EXPECT_DOUBLE_EQ(sites[1].weight, 0.5);
    EXPECT_TRUE(sites.ids_are_sequential());
}

TEST(SiteSet, PushBackOwnsAndNormalizesSequentialIds) {
    SiteSet sites;
    sites.push_back(Site2D(Point2{0.0, 0.0}, SiteId{99}, RegionId{1}));
    sites.push_back(Site2D(Point2{1.0, 0.0}, SiteId{77}, RegionId{2}));

    ASSERT_EQ(sites.size(), 2U);
    EXPECT_EQ(sites[0].id, SiteId{0});
    EXPECT_EQ(sites[1].id, SiteId{1});
    EXPECT_TRUE(sites.ids_are_sequential());
}

TEST(SiteSet, CanRenumberExistingSitesSequentially) {
    SiteSet sites;
    sites.add(Point2{0.0, 0.0});
    sites.add(Point2{1.0, 0.0});
    sites[1].id = SiteId{42};

    EXPECT_FALSE(sites.ids_are_sequential());

    sites.renumber_sequential();

    EXPECT_TRUE(sites.ids_are_sequential());
    EXPECT_EQ(sites[1].id, SiteId{1});
}

TEST(SiteSet, ProvidesSpanAndCheckedAccess) {
    SiteSet sites;
    sites.add(Point2{0.0, 0.0});
    sites.add(Point2{1.0, 0.0});

    auto view = sites.span();
    ASSERT_EQ(view.size(), 2U);
    EXPECT_DOUBLE_EQ(view[1].point.x, 1.0);

    EXPECT_NO_THROW((void)sites.at(1));
    EXPECT_THROW((void)sites.at(2), VMMException);
}

TEST(SiteSet, CanRotateSitesAroundArbitraryCenter) {
    SiteSet sites;
    sites.add(Point2{2.0, 1.0}, RegionId{3});

    rotate_in_place(sites, vmm::constants::kPi / 2.0, Point2{1.0, 1.0});

    ASSERT_EQ(sites.size(), 1U);
    EXPECT_NEAR(sites[0].point.x, 1.0, 1.0e-12);
    EXPECT_NEAR(sites[0].point.y, 2.0, 1.0e-12);
    EXPECT_EQ(sites[0].id, SiteId{0});
    EXPECT_EQ(sites[0].region, RegionId{3});
}
