// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <vector>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "simple_meshes.hpp"
#include <vmm/cvt.hpp>
#include <vmm/io/layered.hpp>
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
    EXPECT_EQ(m->data().site_is_centroid[0],1);
    EXPECT_LE(vmm::norm(m->mesh().site(vmm::CellId{0})-metrics.cell_centroid[0]),
        1e-12*vmm::norm(m->mesh().points().back()-m->mesh().points().front()));
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

TEST(LayeredMesh, RejectsShiftedSupportAndWrongRegion) {
    auto m=build(); ASSERT_TRUE(m);
    auto d=m->data();
    auto raw=d.mesh.data();
    // A rigid vertical translation preserves all volumes and closure.
    for(auto& p:raw.points) p[2]+=0.125;
    for(auto& p:raw.sites) p[2]+=0.125;
    auto shifted=vmm::Mesh3D::from_data(raw); ASSERT_TRUE(shifted);
    d.mesh=*shifted;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
    d=m->data(); raw=d.mesh.data();
    raw.cell_region[0]=vmm::RegionId{1};
    auto mislabeled=vmm::Mesh3D::from_data(raw); ASSERT_TRUE(mislabeled);
    d.mesh=*mislabeled;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
}

TEST(LayeredMesh, RejectsDuplicateFaceAndGap) {
    auto m=build(); ASSERT_TRUE(m);
    auto d=m->data(); auto raw=d.mesh.data();
    const auto last=raw.owner.size()-1;
    const auto vertices=raw.face_vertices.row(last);
    const std::vector<vmm::VertexId> copy(vertices.begin(),vertices.end());
    raw.face_vertices.push_row(copy);
    raw.owner.push_back(raw.owner.back());
    ++raw.patches.back().count;
    d.face_level.push_back(d.face_level.back());
    auto duplicate=vmm::Mesh3D::from_data(raw); ASSERT_TRUE(duplicate);
    d.mesh=*duplicate;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
    d=m->data(); raw=d.mesh.data();
    raw.face_vertices.offsets.pop_back();
    raw.face_vertices.values.resize(raw.face_vertices.offsets.back());
    raw.owner.pop_back();
    --raw.patches.back().count;
    d.face_level.pop_back();
    auto gap=vmm::Mesh3D::from_data(raw); ASSERT_TRUE(gap);
    d.mesh=*gap;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(d));
}

TEST(LayeredMesh, IndexedViewsMatchGeometryAndSurviveCopies) {
    auto mesh=build(); ASSERT_TRUE(mesh);
    const auto copy=*mesh;
    for(std::size_t column=0;column<mesh->data().column_count;++column) {
        const auto view=copy.cells_in_column_view(column);
        EXPECT_EQ(std::vector<vmm::CellId>(view.begin(),view.end()),mesh->cells_in_column(column));
        for(auto cell:view) EXPECT_EQ(copy.data().column[cell.index()],column);
        for(std::size_t i=1;i<view.size();++i)
            EXPECT_LT(copy.data().layer[view[i-1].index()],copy.data().layer[view[i].index()]);
    }
    for(auto cell:mesh->mesh().cells()) {
        std::vector<vmm::CellId> expected;
        for(auto face:mesh->mesh().internal_faces()) if(mesh->data().face_level[face.index()]!=vmm::lateral_face) {
            if(mesh->mesh().owner(face)==cell) expected.push_back(mesh->mesh().neighbour(face));
            if(mesh->mesh().neighbour(face)==cell) expected.push_back(mesh->mesh().owner(face));
        }
        std::sort(expected.begin(),expected.end());
        expected.erase(std::unique(expected.begin(),expected.end()),expected.end());
        const auto view=copy.vertical_neighbours_view(cell);
        EXPECT_EQ(std::vector<vmm::CellId>(view.begin(),view.end()),expected);
    }
    EXPECT_TRUE(copy.cells_in_column_view(99).empty());
    EXPECT_TRUE(copy.vertical_neighbours_view(vmm::CellId{}).empty());
    EXPECT_TRUE(copy.vertical_neighbours_view(vmm::CellId{99}).empty());
    EXPECT_TRUE(vmm::LayeredMesh{}.cells_in_column_view(0).empty());
}

TEST(LayeredMesh, BaseEdgeOwnerIsOnTheLeft) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    ASSERT_TRUE(base);
    for(auto face:base->faces()) {
        const auto vertices=base->face_vertices(face);
        ASSERT_EQ(vertices.size(),2u);
        const auto a=base->point(vertices[0]),b=base->point(vertices[1]);
        const auto site=base->site(base->owner(face));
        EXPECT_GT((b[0]-a[0])*(site[1]-a[1])-(b[1]-a[1])*(site[0]-a[0]),0);
    }
}

TEST(LayeredMesh, SitesRetainBaseGenerators) {
    auto mesh=build(); ASSERT_TRUE(mesh);
    ASSERT_EQ(mesh->data().base_generator.size(),mesh->data().column_count);
    ASSERT_EQ(mesh->data().site_is_centroid.size(),mesh->mesh().cell_count());
    for(auto cell:mesh->mesh().cells()) {
        const auto generator=mesh->data().base_generator[mesh->data().column[cell.index()]];
        EXPECT_DOUBLE_EQ(mesh->mesh().site(cell)[0],generator[0]);
        EXPECT_DOUBLE_EQ(mesh->mesh().site(cell)[1],generator[1]);
        EXPECT_EQ(mesh->data().site_is_centroid[cell.index()],0);
    }
    auto bad=mesh->data(); bad.base_generator.pop_back();
    EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
    bad=mesh->data(); bad.site_is_centroid.pop_back();
    EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
    bad=mesh->data(); bad.site_is_centroid[0]=2;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
}

TEST(LayeredMesh, HorizontalOrthogonalityAndVerticalSkewAreSeparate) {
    auto mesh=build(); ASSERT_TRUE(mesh);
    const auto metrics=vmm::compute_metrics(mesh->mesh());
    for(auto face:mesh->mesh().internal_faces()) {
        const auto delta=mesh->mesh().site(mesh->mesh().neighbour(face))-mesh->mesh().site(mesh->mesh().owner(face));
        EXPECT_DOUBLE_EQ(metrics.vertical_skew[face.index()],std::atan2(std::abs(delta[2]),std::hypot(delta[0],delta[1])));
        if(mesh->data().face_level[face.index()]==vmm::lateral_face) {
            const auto area=metrics.face_area_vector[face.index()];
            const auto angle=std::atan2(std::abs(area[0]*delta[1]-area[1]*delta[0]),area[0]*delta[0]+area[1]*delta[1]);
            EXPECT_LE(angle,1e-8);
        }
    }
    const auto energy=vmm::cvt_energy(*mesh);
    ASSERT_FALSE(energy);
    EXPECT_EQ(energy.error().code(),vmm::ErrorCode::InvalidArgument);
}

TEST(LayeredMesh, PlanarInterfacesMergeIntoSingleFaces) {
    auto mesh=build(); ASSERT_TRUE(mesh);
    for(auto cell:mesh->mesh().cells()) {
        std::size_t horizontal=0;
        for(auto face:mesh->mesh().faces())
            if(mesh->data().face_level[face.index()]!=vmm::lateral_face &&
               (mesh->mesh().owner(face)==cell || mesh->mesh().neighbour(face)==cell)) ++horizontal;
        EXPECT_EQ(horizontal,2u);
    }
    EXPECT_EQ(mesh->mesh().face_count(),29u);
}

TEST(LayeredMesh, FlatColumnLevelsAreValidated) {
    const auto original=build(); ASSERT_TRUE(original);
    auto data=original->data();
    data.column_levels={{0,0.25,1,3},{0,0.25,1,3}};
    data.kfloor={0,0};
    const auto flat=vmm::LayeredMesh::from_data(data);
    ASSERT_TRUE(flat) << flat.error().message();
    std::stringstream stream;
    ASSERT_TRUE(vmm::write_layered(*flat,stream));
    EXPECT_EQ(stream.str().find("VMM_LAYERS 3"),0u);
    const auto restored=vmm::read_layered(stream);
    ASSERT_TRUE(restored) << restored.error().message();
    EXPECT_EQ(restored->data().column_levels,data.column_levels);
    EXPECT_EQ(restored->data().kfloor,data.kfloor);
    auto bad=data; bad.kfloor[0]=1;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
    bad=data; bad.column_levels[0][1]=-1;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
    bad=data; bad.column_levels[0][1]=0.5;
    EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
    bad=data; bad.column_levels.pop_back();
    EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
    bad=data; bad.kfloor.clear();
    EXPECT_FALSE(vmm::LayeredMesh::from_data(bad));
}

TEST(LayeredMesh, DirectFlatColumnsWithSteps) {
    const auto base=vmm::Mesh2D::from_data(vmm::test::two_squares_data()); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,1,2},{0,1},{"bottom","middle","top"},
        {{0,0,0,0,0,0},{1,1.5,2,1,1.5,2},{3,3,3,3,3,3}});
    ASSERT_TRUE(grid);
    const auto flat=vmm::generate_layered_mesh(*base,*grid,{{0,1},{0,1}},vmm::FlatColumns<>{});
    ASSERT_TRUE(flat) << flat.error().message();
    EXPECT_EQ(flat->mesh().cell_count(),4u);
    EXPECT_EQ(flat->data().column_levels[0],(std::vector<double>{0,1.25,3}));
    EXPECT_EQ(flat->data().column_levels[1],(std::vector<double>{0,1.75,3}));
    const auto metrics=vmm::compute_metrics(flat->mesh());
    EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),6.,6e-12);
    const auto custom=[](const vmm::Mesh2D&,const vmm::HorizonGrid&,vmm::CellId column,std::size_t h)->vmm::Result<double> {
        return h==0 ? static_cast<double>(column.index()) : h==1 ? 2. : 3.;
    };
    const auto stepped=vmm::generate_layered_mesh(*base,*grid,{},vmm::FlatColumns<decltype(custom)>{custom});
    ASSERT_TRUE(stepped) << stepped.error().message();
}
