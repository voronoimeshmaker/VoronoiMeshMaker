// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "simple_meshes.hpp"
#include <vmm/app/run.hpp>
#include <vmm/backend/column_overlay.hpp>
#include <vmm/io/layered.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/layered.hpp>
#include <vmm/reorder/layered.hpp>
#include <vmm/vmm.hpp>
namespace {
vmm::Result<vmm::MeshResult2D> base(bool hole, bool separate, double spacing=0.4) {
    vmm::MeshRequest2D r;
    auto& d=r.declaration;
    const auto medium=*d.media().add("geometry");
    const auto region=*d.add_region("base",medium,vmm::PolygonShape({{0,0},{3,0},{3,1},{1,1},{1,3},{0,3}}));
    if(hole) (void)d.add_hole(vmm::Rectangle({0.2,0.2},{0.6,0.6}));
    if(separate) (void)d.add_region("island",medium,vmm::Rectangle({2,2},{3,3}));
    r.sources.push_back(vmm::sites_for(region,vmm::CartesianGridSource(spacing)));
    if(separate) r.sources.push_back(vmm::sites_for(vmm::RegionId{1},vmm::CartesianGridSource(spacing)));
    return vmm::generate_mesh_2d(r);
}
auto horizons() {
    std::vector<vmm::HorizonGrid::Surface> f{
        [](auto p){return 0.2*p[0]+0.1*p[1];},
        [](auto p){return 0.2*p[0]+0.1*p[1]+1;},
        [](auto p){return 0.2*p[0]+0.1*p[1]+2;}};
    return vmm::HorizonGrid::sample({0,1,2,3},{0,1,2,3},{"bottom","interface","top"},f);
}
TEST(LayeredIntegration, NonconvexHoleAndDisconnectedRegions) {
    const auto b=base(true,true);
    ASSERT_TRUE(b) << b.error().message();
    const auto h=horizons(); ASSERT_TRUE(h);
    const auto m=vmm::generate_layered_mesh(b->mesh,*h,{{0,0.1,1},{0,0.5,1}});
    ASSERT_TRUE(m) << m.error().message();
    auto metrics=vmm::compute_metrics(m->mesh());
    EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),2*(6-0.16),1e-10);
    EXPECT_EQ(m->mesh().regions().size(),4u);
}
TEST(LayeredIntegration, FixedObliqueSupportWithChangedBaseSites) {
    const auto h=horizons(); ASSERT_TRUE(h);
    std::vector<std::size_t> counts;
    for(double spacing:{0.4,0.6}) {
        auto b=base(false,false,spacing); ASSERT_TRUE(b) << b.error().message();
        auto m=vmm::generate_layered_mesh(b->mesh,*h); ASSERT_TRUE(m) << m.error().message();
        double area=0; std::size_t faces=0;
        for(vmm::FaceId f:m->mesh().internal_faces()) if(m->data().face_level[f.index()]==1) {
            ++faces;
            area+=vmm::norm(vmm::face_geometry(m->mesh(),f).area_vector);
            for(auto v:m->mesh().face_vertices(f)) {
                const auto p=m->mesh().point(v);
                EXPECT_NEAR(p[2]-0.2*p[0]-0.1*p[1],1,2e-12);
            }
        }
        EXPECT_NEAR(area,5*std::sqrt(1.05),1e-10);
        counts.push_back(faces);
    }
    EXPECT_NE(counts[0],counts[1]);
}
TEST(LayeredIntegration, RoundTripAndRenumbering) {
    const auto b=base(false,false); ASSERT_TRUE(b);
    const auto h=horizons(); ASSERT_TRUE(h);
    const auto m=vmm::generate_layered_mesh(b->mesh,*h); ASSERT_TRUE(m) << m.error().message();
    std::stringstream stream;
    ASSERT_TRUE(vmm::write_layered(*m,stream));
    auto restored=vmm::read_layered(stream); ASSERT_TRUE(restored) << restored.error().message();
    EXPECT_EQ(restored->mesh().points(),m->mesh().points());
    EXPECT_EQ(restored->data().column,m->data().column);
    EXPECT_EQ(restored->data().layer,m->data().layer);
    EXPECT_EQ(restored->data().face_level,m->data().face_level);
    EXPECT_EQ(restored->data().horizons.elevations(),h->elevations());
    const auto permutation=vmm::RcmOrdering{}.permutation(m->mesh());
    auto numbered=vmm::renumber(*m,permutation); ASSERT_TRUE(numbered);
    auto back=vmm::renumber(*numbered,permutation.inverse()); ASSERT_TRUE(back);
    EXPECT_EQ(back->data().column,m->data().column);
    EXPECT_EQ(back->data().layer,m->data().layer);
    for(std::size_t c=0;c<m->data().column_count;++c)
        EXPECT_EQ(numbered->cells_in_column(c).size(),m->cells_in_column(c).size());
    std::stringstream vtk;
    EXPECT_TRUE(vmm::write_vtu(m->mesh(),vtk));
    EXPECT_NE(vtk.str().find("faceoffsets"),std::string::npos);
}
TEST(LayeredIntegration, MalformedFiles) {
    for(const std::string text:{"","WRONG 1","VMM_LAYERS 9","VMM_LAYERS 1\n2 0"}) {
        std::istringstream in(text); EXPECT_FALSE(vmm::read_layered(in));
    }
    std::istringstream bad("VMM_HORIZONS 2");
    EXPECT_FALSE(vmm::read_horizons(bad));
    EXPECT_FALSE(vmm::read_layered(std::filesystem::path("/no/such/vmm/layers")));
}
TEST(LayeredIntegration, ConfigComposesBaseAndHorizons) {
    const auto dir=std::filesystem::temp_directory_path()/"vmm_layered_config_test";
    std::filesystem::create_directories(dir);
    const auto source=dir/"levels.hgrid";
    {
        std::ofstream out(source);
        out << "VMM_HORIZONS 1\n2 0 2\n2 0 1\n2\n"
               "\"bottom\"\n4 0 0 0 0\n\"top\"\n4 1 1 1 1\n1\n3 0 0.25 1\n";
    }
    const auto config=vmm::MeshConfig::parse(
        "dimension = 2\nhorizons = levels.hgrid\noutput = result\n"
        "[region base]\nshape = rectangle\nlo = 0 0\nhi = 2 1\n"
        "sites = grid\nsites.spacing = 0.4\n",dir);
    ASSERT_TRUE(config) << config.error().message();
    auto report=vmm::run_config(*config); ASSERT_TRUE(report) << report.error().message();
    EXPECT_EQ(report->dimension,3);
    EXPECT_EQ(report->written.size(),3u);
    auto m=vmm::read_layered(dir/"result.vlayers"); ASSERT_TRUE(m);
    EXPECT_EQ(m->data().fractions[0],(std::vector<double>{0,0.25,1}));
}
}

TEST(LayeredIntegration, DeterministicAcrossScalesAndTranslations) {
    for(double scale:{1e-3,1.,1e3}) for(double offset:{0.,100.}) {
        auto raw=vmm::test::two_squares_data();
        for(auto& p:raw.points) for(auto& x:p) x=offset+scale*x;
        for(auto& p:raw.sites) for(auto& x:p) x=offset+scale*x;
        auto b=vmm::Mesh2D::from_data(raw); ASSERT_TRUE(b);
        auto h=vmm::HorizonGrid::make({offset,offset+2*scale},{offset,offset+scale},
            {"bottom","top"},{{offset,offset,offset,offset},
            {offset+scale,offset+scale,offset+scale,offset+scale}});
        ASSERT_TRUE(h);
        auto m=vmm::generate_layered_mesh(*b,*h);
        ASSERT_TRUE(m) << "scale=" << scale << " offset=" << offset << ": " << m.error().message();
        auto again=vmm::generate_layered_mesh(*b,*h); ASSERT_TRUE(again);
        std::stringstream a,c;
        ASSERT_TRUE(vmm::write_layered(*m,a));
        ASSERT_TRUE(vmm::write_layered(*again,c));
        EXPECT_EQ(a.str(),c.str());
        const auto metrics=vmm::compute_metrics(m->mesh());
        EXPECT_NEAR(std::accumulate(metrics.cell_measure.begin(),metrics.cell_measure.end(),0.),
            2*scale*scale*scale,2e-8*scale*scale*scale);
    }
}

TEST(LayeredIntegration, FixedFiniteObliqueInterfaceIn2DAndExtrusion) {
    std::vector<std::size_t> counts;
    for(double spacing:{0.25,0.4}) {
        vmm::MeshRequest2D request;
        auto& d=request.declaration;
        const auto medium=*d.media().add("geometry");
        const auto a=*d.add_region("below",medium,vmm::PolygonShape({{0,0},{2,0},{0,2}}));
        const auto b=*d.add_region("above",medium,vmm::PolygonShape({{2,0},{2,2},{0,2}}));
        request.sources.push_back(vmm::sites_for(a,vmm::CartesianGridSource(spacing)));
        request.sources.push_back(vmm::sites_for(b,vmm::CartesianGridSource(spacing)));
        auto base=vmm::generate_mesh_2d(request); ASSERT_TRUE(base) << base.error().message();
        std::vector<std::pair<double,double>> segments;
        for(auto f:base->mesh.internal_faces()) {
            if(base->mesh.region(base->mesh.owner(f))==base->mesh.region(base->mesh.neighbour(f))) continue;
            const auto vertices=base->mesh.face_vertices(f);
            const auto p=base->mesh.point(vertices[0]),q=base->mesh.point(vertices[1]);
            EXPECT_NEAR(p[0]+p[1],2,1e-12);
            EXPECT_NEAR(q[0]+q[1],2,1e-12);
            segments.emplace_back(std::min(p[0],q[0]),std::max(p[0],q[0]));
        }
        std::ranges::sort(segments);
        ASSERT_FALSE(segments.empty());
        EXPECT_NEAR(segments.front().first,0,1e-12);
        EXPECT_NEAR(segments.back().second,2,1e-12);
        for(std::size_t i=1;i<segments.size();++i)
            EXPECT_NEAR(segments[i-1].second,segments[i].first,1e-12);
        counts.push_back(segments.size());
        auto h=vmm::HorizonGrid::make({0,1,2},{0,1,2},{"bottom","middle","top"},
            {{0,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1,1},{2,2,2,2,2,2,2,2,2}});
        ASSERT_TRUE(h);
        auto m=vmm::generate_layered_mesh(base->mesh,*h); ASSERT_TRUE(m) << m.error().message();
        double area=0;
        for(auto f:m->mesh().internal_faces()) {
            const auto ra=m->mesh().region(m->mesh().owner(f)).index()/2;
            const auto rb=m->mesh().region(m->mesh().neighbour(f)).index()/2;
            if(ra==rb) continue;
            area+=vmm::norm(vmm::face_geometry(m->mesh(),f).area_vector);
            for(auto v:m->mesh().face_vertices(f)) {
                const auto p=m->mesh().point(v);
                EXPECT_NEAR(p[0]+p[1],2,1e-12);
                EXPECT_GE(p[2],0); EXPECT_LE(p[2],2);
            }
        }
        EXPECT_NEAR(area,4*std::sqrt(2.),1e-10);
    }
    EXPECT_NE(counts[0],counts[1]);
}

TEST(LayeredIntegration, TruncatedPersistenceAndStreamFailures) {
    auto b=base(false,false); ASSERT_TRUE(b);
    auto h=horizons(); ASSERT_TRUE(h);
    auto m=vmm::generate_layered_mesh(b->mesh,*h); ASSERT_TRUE(m);
    std::ostringstream output; ASSERT_TRUE(vmm::write_layered(*m,output));
    const auto text=output.str();
    const auto end=text.find("vmm-mesh");
    ASSERT_NE(end,std::string::npos);
    for(std::size_t i=0;i<end;++i) if(text[i]=='\n' || text[i]==' ') {
        std::istringstream truncated(text.substr(0,i));
        EXPECT_FALSE(vmm::read_layered(truncated)) << i;
    }
    std::ostringstream broken;
    broken.setstate(std::ios::badbit);
    EXPECT_FALSE(vmm::write_layered(*m,broken));
    EXPECT_FALSE(vmm::write_layered(*m,std::filesystem::path("/no/such/vmm/output")));
    EXPECT_FALSE(vmm::read_horizons(std::filesystem::path("/no/such/vmm/horizons")));
    for(const std::string spec:{
        "VMM_HORIZONS 1\n2 0 1\n2 0 1\n2\n",
        "VMM_HORIZONS 1\n2 0 1\n2 0 1\n2\n\"a\"\n4 0 0 0 0\n\"b\"\n4 1 1 1 1\n1\n3 0",
        "VMM_HORIZONS 1\n2 0 1\n2 0 1\n2\n\"a\"\n4 0 0 0 0\n\"a\"\n4 1 1 1 1\n0\n"}) {
        std::istringstream in(spec); EXPECT_FALSE(vmm::read_horizons(in));
    }
}
TEST(LayeredIntegration, InvalidOverlayInputs) {
    auto h=horizons(); ASSERT_TRUE(h);
    EXPECT_FALSE(vmm::cgal_column_overlay({},0,*h));
    EXPECT_FALSE(vmm::cgal_column_overlay({},1,vmm::HorizonGrid{}));
    std::vector<vmm::ColumnEdge> edges{{{0,0},{1,0},vmm::CellId{0},{},vmm::PatchId{0}}};
    auto invalid=edges; invalid[0].owner=vmm::CellId{2};
    EXPECT_FALSE(vmm::cgal_column_overlay(invalid,1,*h));
    invalid=edges; invalid[0].neighbour=vmm::CellId{2};
    EXPECT_FALSE(vmm::cgal_column_overlay(invalid,1,*h));
    invalid=edges; invalid[0].a[0]=std::numeric_limits<double>::infinity();
    EXPECT_FALSE(vmm::cgal_column_overlay(invalid,1,*h));
    invalid=edges; invalid[0].b[1]=std::numeric_limits<double>::infinity();
    EXPECT_FALSE(vmm::cgal_column_overlay(invalid,1,*h));
    invalid=edges; invalid[0].b=invalid[0].a;
    EXPECT_FALSE(vmm::cgal_column_overlay(invalid,1,*h));
    edges={{{0,0},{0,1},vmm::CellId{0},{},vmm::PatchId{0}},
           {{0,1},{1,0},vmm::CellId{0},{},vmm::PatchId{0}},
           {{1,0},{0,0},vmm::CellId{0},{},vmm::PatchId{0}}};
    EXPECT_FALSE(vmm::cgal_column_overlay(edges,1,*h));
}

TEST(LayeredIntegration, InvalidExtrusionGeometry) {
    auto raw=vmm::test::two_squares_data();
    auto b=vmm::Mesh2D::from_data(raw); ASSERT_TRUE(b);
    auto h=vmm::HorizonGrid::make({0,2},{0,1},{"a","b"},{{0,0,0,0},{1,1,1,1}});
    ASSERT_TRUE(h);
    EXPECT_FALSE(vmm::generate_layered_mesh(*b,vmm::HorizonGrid{}));
    EXPECT_FALSE(vmm::generate_layered_mesh(*b,*h,{{0}}));
    EXPECT_FALSE(vmm::generate_layered_mesh(*b,*h,{{0.1,1}}));
    EXPECT_FALSE(vmm::generate_layered_mesh(*b,*h,{{0,0.9}}));
    EXPECT_FALSE(vmm::generate_layered_mesh(*b,*h,{{0,std::numeric_limits<double>::quiet_NaN(),1}}));
    auto zero=vmm::HorizonGrid::make({0,2},{0,1},{"a","b"},{{0,0,0,0},{0,0,0,0}});
    ASSERT_TRUE(zero);
    EXPECT_FALSE(vmm::generate_layered_mesh(*b,*zero));
    for(std::size_t coordinate=0;coordinate<2;++coordinate) {
        auto bad=raw; bad.points[0][coordinate]=std::numeric_limits<double>::infinity();
        auto mesh=vmm::Mesh2D::from_data(bad); ASSERT_TRUE(mesh);
        EXPECT_FALSE(vmm::generate_layered_mesh(*mesh,*h));
    }
    auto bad=raw; bad.points[0]=bad.points[1];
    auto mesh=vmm::Mesh2D::from_data(bad); ASSERT_TRUE(mesh);
    EXPECT_FALSE(vmm::generate_layered_mesh(*mesh,*h));
}
