// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <limits>
#include <vector>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/geometry/horizons.hpp>
TEST(HorizonGrid, FrozenSamplingAndAccessors) {
    std::vector<vmm::HorizonGrid::Surface> f{
        [](auto p){return p[0];},[](auto p){return p[0]+1;}};
    auto g=vmm::HorizonGrid::sample({0,1},{0,1},{"bottom","top"},f);
    ASSERT_TRUE(g);
    EXPECT_EQ(g->x(),(std::vector<double>{0,1}));
    EXPECT_EQ(g->y(),(std::vector<double>{0,1}));
    EXPECT_EQ(g->names()[1],"top");
    EXPECT_EQ(g->horizon_count(),2u);
    EXPECT_EQ(g->elevations()[1],(std::vector<double>{1,2,1,2}));
    f[1]=[](auto){return 99.;};
    EXPECT_EQ(g->elevations()[1][0],1);
}
TEST(HorizonGrid, RejectsInvalidGeometry) {
    using vmm::HorizonGrid;
    EXPECT_FALSE(HorizonGrid::make({0},{0,1},{"a","b"},{{0,0},{1,1}}));
    EXPECT_FALSE(HorizonGrid::make({1,0},{0,1},{"a","b"},{{0,0,0,0},{1,1,1,1}}));
    EXPECT_FALSE(HorizonGrid::make({0,1},{0,1},{"a","a"},{{0,0,0,0},{1,1,1,1}}));
    EXPECT_FALSE(HorizonGrid::make({0,1},{0,1},{"a","b"},{{0,0,0,0},{1}}));
    EXPECT_FALSE(HorizonGrid::make({0,1},{0,1},{"a","b"},{{0,0,0,0},{1,-1,1,1}}));
    EXPECT_FALSE(HorizonGrid::make({0,1},{0,1},{"a","b"},{{0,0,0,0},{1,1,1,std::numeric_limits<double>::infinity()}}));
    EXPECT_TRUE(HorizonGrid::make({0,1},{0,1},{"a","b"},{{0,0,0,0},{0,0,1,1}}));
    std::vector<HorizonGrid::Surface> empty(2);
    EXPECT_FALSE(HorizonGrid::sample({0,1},{0,1},{"a","b"},empty));
    EXPECT_FALSE(HorizonGrid::sample({0},{0,1},{"a","b"},empty));
}

TEST(HorizonGrid, PiecewiseAffineElevation) {
    auto g=vmm::HorizonGrid::make({0,2},{0,2},{"a","b"},{{0,2,4,8},{10,12,14,18}});
    ASSERT_TRUE(g);
    EXPECT_DOUBLE_EQ(*g->elevation(0,{1.5,0.5}),3.);
    EXPECT_DOUBLE_EQ(*g->elevation(0,{0.5,1.5}),4.);
    EXPECT_DOUBLE_EQ(*g->elevation(0,{1,1}),4.);
    EXPECT_DOUBLE_EQ(*g->elevation(1,{2,2}),18.);
    EXPECT_DOUBLE_EQ(*g->elevation(0,{0,0}),0.);
    EXPECT_FALSE(g->elevation(2,{0,0}));
    EXPECT_FALSE(g->elevation(0,{-0.1,0}));
    EXPECT_FALSE(g->elevation(0,{0,2.1}));
    EXPECT_FALSE(g->elevation(0,{std::numeric_limits<double>::quiet_NaN(),0}));
    EXPECT_FALSE(vmm::HorizonGrid{}.elevation(0,{0,0}));
}
