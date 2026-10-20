// SPDX-License-Identifier: BSD-3-Clause
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
TEST(PolygonMeanSampler, IntegratesAcrossTerrainBreaks) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data()); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,.25,1,2},{0,1},{"bottom","top"},
        {{0,0,0,0,0,0,0,0},{1,2,1,1,1,2,1,1}}); ASSERT_TRUE(grid);
    const vmm::PolygonMeanSampler sample(*base);
    const auto mean=sample(*base,*grid,vmm::CellId::from_index(0),1); ASSERT_TRUE(mean);
    EXPECT_NEAR(*mean,1.5,1e-13);
    const auto second=sample(*base,*grid,vmm::CellId::from_index(1),1); ASSERT_TRUE(second);
    EXPECT_NEAR(*second,1.,1e-13);
    const auto mesh=vmm::generate_layered_mesh(*base,*grid,{},vmm::FlatColumns{sample});
    ASSERT_TRUE(mesh) << mesh.error().message();
    EXPECT_NEAR(mesh->data().column_levels[0].back(),1.5,1e-13);
    EXPECT_FALSE(sample(*base,*grid,{},1));
    EXPECT_FALSE(sample(*base,*grid,vmm::CellId::from_index(0),2));
    const auto copy=*base;
    EXPECT_FALSE(sample(copy,*grid,vmm::CellId::from_index(0),1));
}

TEST(PolygonMeanSampler, DiagonalBreakAndTranslatedCoordinates) {
    for(const double offset:{0.,500000.}) {
        auto data=vmm::test::two_squares_data();
        for(auto& p:data.points) { p[0]+=offset; p[1]+=15*offset; }
        for(auto& p:data.sites) { p[0]+=offset; p[1]+=15*offset; }
        const auto base=vmm::Mesh2D::from_data(std::move(data)); ASSERT_TRUE(base);
        const auto grid=vmm::HorizonGrid::make({offset,offset+1,offset+2},{15*offset,15*offset+1},
            {"bottom","top"},{{0,0,0,0,0,0},{1,1,1,1,2,2}}); ASSERT_TRUE(grid);
        const vmm::PolygonMeanSampler sample(*base);
        const auto mean=sample(*base,*grid,vmm::CellId::from_index(0),1); ASSERT_TRUE(mean);
        EXPECT_NEAR(*mean,4./3.,1e-12);
        const auto small=vmm::HorizonGrid::make({offset,offset+.5},{15*offset,15*offset+1},
            {"bottom","top"},{{0,0,0,0},{1,1,1,1}}); ASSERT_TRUE(small);
        EXPECT_FALSE(sample(*base,*small,vmm::CellId::from_index(0),1));
    }
}

TEST(PolygonMeanSampler, ConcaveAndHoledCellIntegrals) {
    for(const bool hole:{false,true}) for(const double offset:{0.,500000.}) {
        vmm::MeshData<2> data;
        data.points=hole?std::vector<vmm::Vec2>{{0,0},{3,0},{3,3},{0,3},{1,.5},{1,1.5},{2,1.5},{2,.5}}:
                         std::vector<vmm::Vec2>{{0,0},{2,0},{2,1},{1,1},{1,2},{0,2}};
        const auto add=[&](std::size_t a,std::size_t b) {
            const vmm::VertexId vertices[]={vmm::VertexId::from_index(a),vmm::VertexId::from_index(b)};
            data.face_vertices.push_row(vertices); data.owner.push_back(vmm::CellId::from_index(0));
        };
        if(hole) { for(std::size_t i=0;i<4;++i) { add(i,(i+1)%4); add(4+i,4+(i+1)%4); } }
        else for(std::size_t i=0;i<6;++i) add(i,(i+1)%6);
        for(auto& point:data.points) { point[0]+=offset; point[1]+=15*offset; }
        data.patches={{"wall",0,data.owner.size()}};
        data.sites={{offset+.5,15*offset+.5}};
        data.cell_region={vmm::RegionId::from_index(0)};
        data.cell_input_site={vmm::SiteId::from_index(0)};
        data.regions={{"r",vmm::MediumId::from_index(0)}}; data.media={"m"};
        const auto base=vmm::Mesh2D::from_data(std::move(data)); ASSERT_TRUE(base);
        const auto grid=vmm::HorizonGrid::make({offset,offset+1,offset+2,offset+3},
            {15*offset,15*offset+1,15*offset+2,15*offset+3},{"bottom","top"},
            {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4}});
        ASSERT_TRUE(grid);
        const auto mean=vmm::PolygonMeanSampler(*base)(*base,*grid,vmm::CellId::from_index(0),1);
        ASSERT_TRUE(mean) << mean.error().message();
        // L: area 3, integral of y = 5/2. Hole: area 8, integral of y = 25/2.
        EXPECT_NEAR(*mean,hole?41./16.:11./6.,2e-9);
        const auto flat=vmm::generate_layered_mesh(*base,*grid,{},vmm::FlatColumns{vmm::PolygonMeanSampler(*base)});
        ASSERT_TRUE(flat) << flat.error().message();
        const auto metrics=vmm::compute_metrics(flat->mesh());
        EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),hole?20.5:5.5,1e-7);
        if(hole) for(auto face:flat->mesh().faces()) if(flat->data().face_level[face.index()]!=vmm::lateral_face) {
            const auto center=metrics.face_centroid[face.index()];
            EXPECT_FALSE(center[0]>offset+1 && center[0]<offset+2 && center[1]>15*offset+.5 && center[1]<15*offset+1.5);
        }
    }
}
