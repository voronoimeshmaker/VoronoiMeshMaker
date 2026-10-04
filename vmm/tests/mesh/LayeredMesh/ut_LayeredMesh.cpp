// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <numeric>
#include <vector>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "simple_meshes.hpp"
#include <vmm/layered.hpp>
#include <vmm/mesh/metrics.hpp>
namespace {
auto build() {
    auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    auto h=vmm::HorizonGrid::make({0,1,2},{0,1},{"bottom","middle","top"},
        {{0,0,0,0,0,0},{1,1,1,1,1,1},{3,3,3,3,3,3}});
    return vmm::generate_layered_mesh(*base,*h,{{0,0.25,1},{0,1}});
}
TEST(LayeredMesh, GeometryAndQueries) {
    auto m=build();
    ASSERT_TRUE(m) << m.error().message();
    EXPECT_EQ(m->mesh().cell_count(),6u);
    auto metrics=vmm::compute_metrics(m->mesh());
    EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),6.,1e-12);
    EXPECT_EQ(m->cells_in_column(0).size(),3u);
    EXPECT_TRUE(m->cells_in_column(99).empty());
    EXPECT_EQ(m->vertical_neighbours(vmm::CellId{1}).size(),2u);
    EXPECT_TRUE(m->vertical_neighbours(vmm::CellId{99}).empty());
    EXPECT_EQ(m->data().fractions[0].size(),3u);
}
TEST(LayeredMesh, RejectsInvalidMetadata) {
    auto m=build(); ASSERT_TRUE(m) << m.error().message();
    auto d=m->data(); d.column.pop_back();
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
    d=m->data(); d.layer[0]=999;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
    d=m->data(); d.column_count=0;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
    d=m->data(); d.fractions[0]={0,0,1};
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
    d=m->data(); d.face_level[0]=999;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
    d=m->data(); d.column[1]=d.column[0]; d.layer[1]=d.layer[0];
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
}
TEST(LayeredMesh, PinchOutInsideAColumn) {
    auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    // The intermediate region starts at x=0.5, inside the first base column.
    auto h=vmm::HorizonGrid::make({0,0.5,1,2},{0,1},{"bottom","pinch","top"},
        {{0,0,0,0,0,0,0,0},{0,0,1,1,0,0,1,1},{2,2,2,2,2,2,2,2}});
    auto m=vmm::generate_layered_mesh(*base,*h);
    ASSERT_TRUE(m) << m.error().message();
    auto metrics=vmm::compute_metrics(m->mesh());
    EXPECT_NEAR(metrics.cell_measure[0],0.25,1e-12);
    EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),4.,1e-12);
    for(double v:metrics.cell_measure) EXPECT_GT(v,0);
}
TEST(LayeredMesh, EmptyIntervalsAndErrors) {
    auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    auto h=vmm::HorizonGrid::make({0,2},{0,1},{"a","b","c"},
        {{0,0,0,0},{0,0,0,0},{1,1,1,1}});
    auto m=vmm::generate_layered_mesh(*base,*h);
    ASSERT_TRUE(m) << m.error().message();
    EXPECT_EQ(m->mesh().cell_count(),2u);
    EXPECT_EQ(m->data().layer[0],1u);
    EXPECT_FALSE(vmm::generate_layered_mesh(*base,*h,{{0,1}}));
    EXPECT_FALSE(vmm::generate_layered_mesh(*base,*h,{{0,1},{0,0,1}}));
    auto small=vmm::HorizonGrid::make({0,1},{0,1},{"a","b"},{{0,0,0,0},{1,1,1,1}});
    EXPECT_FALSE(vmm::generate_layered_mesh(*base,*small));
    EXPECT_FALSE(vmm::generate_layered_mesh(vmm::Mesh2D{},*h));
}
}
