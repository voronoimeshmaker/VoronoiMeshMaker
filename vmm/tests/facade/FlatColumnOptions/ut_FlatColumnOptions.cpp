// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <limits>
#include <numeric>
#include <sstream>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "simple_meshes.hpp"
#include <vmm/io/layered.hpp>
#include <vmm/layered.hpp>
#include <vmm/mesh/metrics.hpp>
TEST(FlatColumnOptions, AbsoluteRelativeAndZeroThickness) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data()); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,2},{0,1},{"bottom","top"},{{0,0,0,0},{4,4,4,4}});
    ASSERT_TRUE(grid);
    vmm::FlatColumns<> policy;
    policy.options.min_thickness_absolute=1.;
    EXPECT_TRUE(vmm::generate_layered_mesh(*base,*grid,{{0,.25,1}},policy));
    const auto thin=vmm::generate_layered_mesh(*base,*grid,{{0,.125,1}},policy);
    ASSERT_FALSE(thin); EXPECT_NE(thin.error().context().find("column=0 layer=0"),std::string::npos);
    policy.options={0,.25};
    EXPECT_TRUE(vmm::generate_layered_mesh(*base,*grid,{{0,.25,1}},policy));
    EXPECT_FALSE(vmm::generate_layered_mesh(*base,*grid,{{0,.125,1}},policy));
    for(const double bad:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
        policy.options={bad,0}; EXPECT_FALSE(vmm::generate_layered_mesh(*base,*grid,{},policy));
        policy.options={0,bad}; EXPECT_FALSE(vmm::generate_layered_mesh(*base,*grid,{},policy));
    }
    policy.options={0,1.01}; EXPECT_FALSE(vmm::generate_layered_mesh(*base,*grid,{},policy));
    const auto pinch=vmm::HorizonGrid::make({0,2},{0,1},{"a","b","c"},
        {{0,0,0,0},{0,0,0,0},{4,4,4,4}}); ASSERT_TRUE(pinch);
    policy.options={4,1};
    const auto omitted=vmm::generate_layered_mesh(*base,*pinch,{},policy); ASSERT_TRUE(omitted);
    EXPECT_EQ(omitted->data().kfloor,(std::vector<std::size_t>{1,1}));
}

TEST(FlatColumnOptions, MergePreservesVolumeAndRegionalInterfaces) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data()); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,2},{0,1},{"bottom","top"},{{0,0,0,0},{4,4,4,4}}); ASSERT_TRUE(grid);
    vmm::FlatColumns<> policy; policy.options={1,0,vmm::merge_thin_layers};
    const auto mesh=vmm::generate_layered_mesh(*base,*grid,{{0,.125,.875,1}},policy);
    ASSERT_TRUE(mesh) << mesh.error().message();
    EXPECT_EQ(mesh->mesh().cell_count(),2u);
    EXPECT_EQ(mesh->data().column_levels[0],(std::vector<double>{0,0,4,4}));
    EXPECT_EQ(mesh->data().merged_count,(std::vector<std::size_t>{2,2}));
    const auto metrics=vmm::compute_metrics(mesh->mesh());
    EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),8.,1e-12);
    const auto regions=vmm::HorizonGrid::make({0,2},{0,1},{"a","b","c"},
        {{0,0,0,0},{.5,.5,.5,.5},{4,4,4,4}}); ASSERT_TRUE(regions);
    EXPECT_FALSE(vmm::generate_layered_mesh(*base,*regions,{},policy));
    std::vector<double> levels{0,.25,.5}; const auto original=levels;
    const std::vector<std::size_t> same{0,0};
    EXPECT_FALSE(vmm::merge_thin_layers(levels,same,1)); EXPECT_EQ(levels,original);
    policy.options.thickness_policy=[](auto& z,auto,double)->vmm::Result<void> { z.clear(); return {}; };
    EXPECT_FALSE(vmm::generate_layered_mesh(*base,*grid,{},policy));
    policy.options.thickness_policy=[](auto& z,auto,double)->vmm::Result<void> { z[1]=2; return {}; };
    EXPECT_FALSE(vmm::generate_layered_mesh(*base,*regions,{},policy));
}

TEST(FlatColumnOptions, BottomMinimumConsumesOnlyItsOwnInterval) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data()); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,2},{0,1},{"a","b","c"},
        {{0,0,0,0},{0,0,0,0},{4,4,4,4}}); ASSERT_TRUE(grid);
    vmm::FlatColumns<> policy; policy.options.min_bottom_thickness=2;
    const auto mesh=vmm::generate_layered_mesh(*base,*grid,{{0,1},{0,.125,.25,.75,1}},policy);
    ASSERT_TRUE(mesh) << mesh.error().message();
    EXPECT_EQ(mesh->data().column_levels[0],(std::vector<double>{0,0,2,2,3,4}));
    EXPECT_EQ(mesh->data().kfloor[0],1u);
    EXPECT_EQ(mesh->data().bottom_adjusted_count,(std::vector<std::size_t>{1,1}));
    const auto metrics=vmm::compute_metrics(mesh->mesh());
    EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),8.,1e-12);
    const auto separated=vmm::HorizonGrid::make({0,2},{0,1},{"a","b","c"},
        {{0,0,0,0},{1,1,1,1},{4,4,4,4}}); ASSERT_TRUE(separated);
    EXPECT_FALSE(vmm::generate_layered_mesh(*base,*separated,{},policy));
    for(const double bad:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
        policy.options.min_bottom_thickness=bad;
        EXPECT_FALSE(vmm::generate_layered_mesh(*base,*grid,{},policy));
    }
}

TEST(FlatColumnOptions, MergeValidationAndAbsentNeighbours) {
    const std::vector<std::size_t> interval{0,0,0};
    for(auto values: {std::vector<double>{},std::vector<double>{0,1},
                     std::vector<double>{0,2,1,3},
                     std::vector<double>{0,1,2,std::numeric_limits<double>::infinity()}})
        EXPECT_FALSE(vmm::merge_thin_layers(values,interval,1));
    for(const double minimum:{-1.,std::numeric_limits<double>::quiet_NaN()}) {
        std::vector<double> values{0,1,2,3};
        EXPECT_FALSE(vmm::merge_thin_layers(values,interval,minimum));
    }
    std::vector<double> upward{0,.25,.25,3};
    ASSERT_TRUE(vmm::merge_thin_layers(upward,interval,1));
    EXPECT_EQ(upward,(std::vector<double>{0,0,0,3}));
    std::vector<double> downward{0,2,2,2.25};
    ASSERT_TRUE(vmm::merge_thin_layers(downward,interval,1));
    EXPECT_EQ(downward,(std::vector<double>{0,2.25,2.25,2.25}));
}

TEST(FlatColumnOptions, OmissionKeepsGapsAndHistoryRoundtrips) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data());ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,2},{0,1},{"bottom","top"},{{0,0,0,0},{4,4,4,4}});ASSERT_TRUE(grid);
    vmm::FlatColumns<> policy;policy.options.min_thickness_absolute=1;policy.options.omit_layer=vmm::omit_thin_layer;
    const auto mesh=vmm::generate_layered_mesh(*base,*grid,{{0,.375,.5,1}},policy);
    ASSERT_TRUE(mesh) << mesh.error().message();
    EXPECT_EQ(mesh->mesh().cell_count(),4u);
    const auto metrics=vmm::compute_metrics(mesh->mesh());
    EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),7.,1e-12);
    for(auto cell:mesh->mesh().cells()) EXPECT_TRUE(mesh->vertical_neighbours_view(cell).empty());
    const auto quality=vmm::column_quality_report(*mesh);
    for(const auto& q:quality) { EXPECT_TRUE(q.history_available);EXPECT_EQ(q.omitted,1u);EXPECT_EQ(q.merged,0u); }
    std::stringstream stream;ASSERT_TRUE(vmm::write_layered(*mesh,stream));
    EXPECT_TRUE(stream.str().starts_with("VMM_LAYERS 4"));
    const auto restored=vmm::read_layered(stream);ASSERT_TRUE(restored);
    EXPECT_EQ(restored->data().omitted_count,mesh->data().omitted_count);
    auto bad=mesh->data();bad.omitted_count[0]=99;EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
    bad=mesh->data();bad.merged_count.clear();EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
    const auto bottom=vmm::generate_layered_mesh(*base,*grid,{{0,.125,1}},policy);ASSERT_TRUE(bottom);
    EXPECT_EQ(bottom->data().kfloor,(std::vector<std::size_t>{1,1}));
}
