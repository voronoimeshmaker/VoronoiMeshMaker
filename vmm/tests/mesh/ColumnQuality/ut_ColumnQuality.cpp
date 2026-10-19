// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "simple_meshes.hpp"
#include <vmm/layered.hpp>
TEST(ColumnQuality, DistinguishesSlopesAndMissingLayers) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data()); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,1,2},{0,1},{"bottom","middle","top"},
        {{0,0,0,0,0,0},{1,2,3,1,2,3},{4,4,4,4,4,4}}); ASSERT_TRUE(grid);
    const auto draped=vmm::generate_layered_mesh(*base,*grid); ASSERT_TRUE(draped);
    const auto quality=vmm::column_quality_report(*draped); ASSERT_EQ(quality.size(),2u);
    for(const auto& q:quality) {
        EXPECT_NEAR(q.min_thickness,1.,1e-10);
        EXPECT_NEAR(q.max_interface_slope,std::atan(1.),1e-10);
        EXPECT_EQ(q.absent_layers,0u);
    }
    const auto sample=[](const auto&,const auto&,vmm::CellId column,std::size_t h)->vmm::Result<double> {
        const double levels[2][3]={{0,0,4},{0,1,4}}; return levels[column.index()][h];
    };
    const auto flat=vmm::generate_layered_mesh(*base,*grid,{},vmm::FlatColumns<decltype(sample)>{sample});
    ASSERT_TRUE(flat);
    const auto fq=vmm::column_quality_report(*flat); ASSERT_EQ(fq.size(),2u);
    EXPECT_DOUBLE_EQ(fq[0].min_thickness,4.); EXPECT_EQ(fq[0].absent_layers,1u);
    EXPECT_DOUBLE_EQ(fq[1].min_thickness,1.); EXPECT_EQ(fq[1].absent_layers,0u);
    for(const auto& q:fq) EXPECT_DOUBLE_EQ(q.max_interface_slope,0.);
    EXPECT_GT(fq[0].max_vertical_skew,0.);
}
