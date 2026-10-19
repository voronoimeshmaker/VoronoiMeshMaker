// ============================================================================
// File: ut_FlatColumns.cpp
// Description: Open flat-column sampling policy and error propagation.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================
//==============================================================================
//  C++ standard library
//==============================================================================
#include <numeric>
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
TEST(FlatColumns, DefaultSamplingAndCustomFailure) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data()); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,2},{0,1},{"bottom","top"},{{0,0,0,0},{1,1,1,1}});
    ASSERT_TRUE(grid);
    const auto flat=vmm::generate_layered_mesh(*base,*grid,{},vmm::FlatColumns<>{});
    ASSERT_TRUE(flat);
    EXPECT_EQ(flat->mesh().cell_count(),2u);
    const auto bad=[](const auto&,const auto&,vmm::CellId,std::size_t)->vmm::Result<vmm::Real> {
        return vmm::fail(vmm::ErrorCode::InvalidArgument,"custom sampler error");
    };
    const auto failed=vmm::generate_layered_mesh(*base,*grid,{},vmm::FlatColumns<decltype(bad)>{bad});
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error().context(),"custom sampler error");
    EXPECT_FALSE(vmm::generate_layered_mesh(*base,*grid,{{0,0,1}},vmm::FlatColumns<>{}));
}

TEST(FlatColumns, AbsentBottomAndInteriorLayersPreserveVolume) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data()); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,2},{0,1},{"a","b","c","d"},
        {{0,0,0,0},{1,1,1,1},{2,2,2,2},{3,3,3,3}}); ASSERT_TRUE(grid);
    const auto sample=[](const auto&,const auto&,vmm::CellId column,std::size_t h)->vmm::Result<double> {
        const double levels[2][4]={{0,0,1,3},{0,1,1,3}};
        return levels[column.index()][h];
    };
    const auto flat=vmm::generate_layered_mesh(*base,*grid,{},vmm::FlatColumns<decltype(sample)>{sample});
    ASSERT_TRUE(flat) << flat.error().message();
    EXPECT_EQ(flat->mesh().cell_count(),4u);
    EXPECT_EQ(flat->data().kfloor,(std::vector<std::size_t>{1,0}));
    const auto metrics=vmm::compute_metrics(flat->mesh());
    EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),6.,6e-12);
    for(std::size_t c=0;c<2;++c) {
        const auto cells=flat->cells_in_column_view(c); ASSERT_EQ(cells.size(),2u);
        EXPECT_EQ(flat->vertical_neighbours_view(cells.front()).size(),1u);
        EXPECT_EQ(flat->vertical_neighbours_view(cells.back()).size(),1u);
    }
}
