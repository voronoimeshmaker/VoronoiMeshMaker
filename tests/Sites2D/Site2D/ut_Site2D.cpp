#include <gtest/gtest.h>

#include <VoronoiMeshMaker/Sites2D/Site2D.hpp>

using namespace vmm::s2d;

TEST(Site2D, StoresPointIdRegionAndWeight) {
    const Site2D site(Point2{1.0, 2.0}, SiteId{7}, RegionId{3}, 0.25);

    EXPECT_DOUBLE_EQ(site.point.x, 1.0);
    EXPECT_DOUBLE_EQ(site.point.y, 2.0);
    EXPECT_EQ(site.id, SiteId{7});
    EXPECT_EQ(site.region, RegionId{3});
    EXPECT_DOUBLE_EQ(site.weight, 0.25);
}

TEST(SiteId, IsStrongComparableId) {
    EXPECT_EQ(SiteId{1}, SiteId{1});
    EXPECT_NE(SiteId{1}, SiteId{2});
    EXPECT_EQ(static_cast<Index>(SiteId{9}), 9);
}
