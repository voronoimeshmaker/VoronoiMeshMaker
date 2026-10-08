// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/run.hpp>
#include <vmm/backend/cgal.hpp>
#include <vmm/cvt.hpp>
#include <vmm/domain/shapes3d.hpp>
#include <vmm/io/native.hpp>
#include <vmm/layered.hpp>
#include <vmm/vmm.hpp>
namespace {
auto square() {
    vmm::Declaration2D d; auto medium=*d.media().add("geometry");
    (void)d.add_region("square",medium,vmm::Rectangle({0,0},{1,1}));
    return vmm::cgal_backend_2d().build_partition(d);
}
TEST(Cvt, SquareAnalyticEnergyAndConvergence) {
    auto p=square(); ASSERT_TRUE(p);
    vmm::SiteSet s; s.add({0.2,0.3},vmm::RegionId{0});
    vmm::CvtOptions options; options.max_iterations=0;
    auto initial=vmm::optimize_cvt(*p,s,options); ASSERT_TRUE(initial) << initial.error().message();
    EXPECT_NEAR(initial->report.energy[0],1./6+0.09+0.04,1e-12);
    options.max_iterations=5;
    auto result=vmm::optimize_cvt(*p,s,options); ASSERT_TRUE(result) << result.error().message();
    EXPECT_TRUE(result->report.converged);
    EXPECT_FALSE(result->report.stalled);
    EXPECT_EQ(result->report.energy.size(),2u);
    EXPECT_NEAR(result->report.energy.back(),1./6,1e-12);
    EXPECT_NEAR(result->mesh.sites()[0][0],0.5,1e-12);
}
TEST(Cvt, CubeAnalyticEnergyAndConvergence) {
    vmm::Declaration3D d; auto medium=*d.media().add("geometry");
    (void)d.add_region("cube",medium,vmm::Cuboid({0,0,0},{1,1,1}));
    auto p=vmm::cgal_backend_3d().build_partition(d); ASSERT_TRUE(p);
    vmm::SiteSet3D sites; sites.add({0.2,0.3,0.4},vmm::RegionId{0});
    vmm::CvtOptions options; options.max_iterations=0;
    auto initial=vmm::optimize_cvt(*p,sites,options); ASSERT_TRUE(initial) << initial.error().message();
    EXPECT_NEAR(initial->report.energy.front(),0.25+0.09+0.04+0.01,1e-12);
    options.max_iterations=4;
    auto result=vmm::optimize_cvt(*p,sites,options); ASSERT_TRUE(result) << result.error().message();
    EXPECT_TRUE(result->report.converged);
    EXPECT_NEAR(result->report.energy.back(),0.25,1e-12);
}
TEST(Cvt, InvalidControlsAndSites) {
    auto p=square(); ASSERT_TRUE(p);
    vmm::SiteSet s; s.add({0.2,0.3},vmm::RegionId{0});
    vmm::CvtOptions o; o.relaxation=0; EXPECT_FALSE(vmm::optimize_cvt(*p,s,o));
    o={}; o.max_backtracks=0; EXPECT_FALSE(vmm::optimize_cvt(*p,s,o));
    o={}; o.relative_tolerance=std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(vmm::optimize_cvt(*p,s,o));
    s.set_weights({1}); EXPECT_FALSE(vmm::optimize_cvt(*p,s));
    EXPECT_FALSE(vmm::optimize_cvt(*p,vmm::SiteSet{}));
}
TEST(Cvt, LayeredDomainAndVolumetricReconstruction) {
    vmm::MeshRequest2D request;
    auto medium=*request.declaration.media().add("geometry");
    auto region=*request.declaration.add_region("base",medium,vmm::Rectangle({0,0},{1,1}));
    request.sources.push_back(vmm::sites_for(region,vmm::ExplicitSites({{0.5,0.5}})));
    auto base=vmm::generate_mesh_2d(request); ASSERT_TRUE(base);
    auto h=vmm::HorizonGrid::make({0,1},{0,1},{"bottom","interface","top"},
        {{0,0,0,0},{0.8,1.2,0.8,1.2},{2,2,2,2}}); ASSERT_TRUE(h);
    auto m=vmm::generate_layered_mesh(base->mesh,*h); ASSERT_TRUE(m) << m.error().message();
    auto domain=vmm::cvt_domain(*m); ASSERT_TRUE(domain) << domain.error().message();
    EXPECT_EQ(domain->source_region.size(),2u);
    EXPECT_NEAR(domain->partition.total_volume(),2,1e-12);
    vmm::SiteSet3D sites;
    sites.add({0.3,0.4,0.3},vmm::RegionId{0});
    sites.add({0.7,0.6,1.7},vmm::RegionId{1});
    vmm::CvtOptions o; o.max_iterations=3;
    auto result=vmm::optimize_cvt(domain->partition,sites,o);
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_LT(result->report.energy.back(),result->report.energy.front());
    for(auto f:result->mesh.internal_faces()) {
        if(result->mesh.region(result->mesh.owner(f))==result->mesh.region(result->mesh.neighbour(f))) continue;
        for(auto v:result->mesh.face_vertices(f)) {
            const auto q=result->mesh.point(v);
            EXPECT_NEAR(q[2],0.8+0.4*q[0],1e-12);
        }
    }
}
} // namespace

TEST(Cvt, MultipleSitesAndFixedObliqueRegions) {
    vmm::Declaration2D d; const auto medium=*d.media().add("geometry");
    (void)d.add_region("a",medium,vmm::PolygonShape({{0,0},{2,0},{0,2}}));
    (void)d.add_region("b",medium,vmm::PolygonShape({{2,0},{2,2},{0,2}}));
    auto p=vmm::cgal_backend_2d().build_partition(d); ASSERT_TRUE(p);
    vmm::SiteSet sites;
    sites.add({0.2,0.2},vmm::RegionId{0}); sites.add({1.2,0.2},vmm::RegionId{0});
    sites.add({1.8,1.8},vmm::RegionId{1}); sites.add({0.6,1.8},vmm::RegionId{1});
    vmm::CvtOptions o; o.max_iterations=12;
    const auto vertices=p->vertices();
    auto m=vmm::optimize_cvt(*p,sites,o); ASSERT_TRUE(m) << m.error().message();
    EXPECT_EQ(p->vertices(),vertices);
    ASSERT_GT(m->report.energy.size(),2u);
    for(std::size_t i=1;i<m->report.energy.size();++i) EXPECT_LT(m->report.energy[i],m->report.energy[i-1]);
    double length=0;
    for(auto f:m->mesh.internal_faces()) if(m->mesh.region(m->mesh.owner(f))!=m->mesh.region(m->mesh.neighbour(f))) {
        length+=vmm::norm(vmm::face_geometry(m->mesh,f).area_vector);
        for(auto v:m->mesh.face_vertices(f)) {
            auto q=m->mesh.point(v); EXPECT_NEAR(q[0]+q[1],2,1e-12);
            EXPECT_GE(q[0],0); EXPECT_LE(q[0],2);
        }
    }
    EXPECT_NEAR(length,2*std::sqrt(2.),1e-12);
    auto again=vmm::optimize_cvt(*p,sites,o); ASSERT_TRUE(again);
    EXPECT_EQ(again->mesh.points(),m->mesh.points());
    EXPECT_EQ(again->report.energy,m->report.energy);
}
TEST(Cvt, NonconvexCentroidBacktrackingAndStagnation) {
    vmm::Declaration2D d; const auto medium=*d.media().add("geometry");
    (void)d.add_region("u",medium,vmm::PolygonShape({{0,0},{3,0},{3,3},{2,3},{2,1},{1,1},{1,3},{0,3}}));
    auto p=vmm::cgal_backend_2d().build_partition(d); ASSERT_TRUE(p);
    vmm::SiteSet sites; sites.add({0.5,2},vmm::RegionId{0});
    vmm::CvtOptions o; o.max_iterations=1;
    auto m=vmm::optimize_cvt(*p,sites,o); ASSERT_TRUE(m) << m.error().message();
    EXPECT_GT(m->report.rejected_steps,0u);
    EXPECT_FALSE(m->report.converged);
    EXPECT_LT(m->report.energy.back(),m->report.energy.front());
    o.max_backtracks=1;
    auto stalled=vmm::optimize_cvt(*p,sites,o); ASSERT_TRUE(stalled);
    EXPECT_TRUE(stalled->report.stalled);
    EXPECT_FALSE(stalled->report.converged);
    EXPECT_EQ(stalled->report.energy.size(),1u);
}
TEST(Cvt, ConfigRunsAndRejectsInvalidOptions) {
    const auto dir=std::filesystem::temp_directory_path()/"vmm_cvt_test";
    std::filesystem::create_directories(dir);
    const std::string shape="[region base]\nshape = rectangle\nlo = 0 0\nhi = 1 1\nsites = grid\nsites.spacing = 0.3\n";
    auto config=vmm::MeshConfig::parse("dimension = 2\ncvt_iterations = 3\noutput = optimized\n"+shape,dir);
    ASSERT_TRUE(config);
    auto result=vmm::run_config(*config); ASSERT_TRUE(result) << result.error().message();
    ASSERT_TRUE(result->cvt);
    EXPECT_FALSE(result->cvt->energy.empty());
    for(const std::string control:{"cvt_iterations = -1\n","cvt_tolerance = 0.1\n","cvt_iterations = 2\ncvt_relaxation = 2\n",
        "cvt_iterations = 2\ncvt_tolerance = nan\n","cvt_iterations = abc\n"}) {
        auto bad=vmm::MeshConfig::parse("dimension = 2\n"+control+shape,dir);
        ASSERT_TRUE(bad); EXPECT_FALSE(vmm::run_config(*bad));
    }
}

TEST(Cvt, LayeredPinchOutAndAbsentRegion) {
    vmm::MeshRequest2D request;
    auto medium=*request.declaration.media().add("geometry");
    auto region=*request.declaration.add_region("base",medium,vmm::Rectangle({0,0},{2,1}));
    request.sources.push_back(vmm::sites_for(region,vmm::ExplicitSites({{0.4,0.5},{1.5,0.5}})));
    auto base=vmm::generate_mesh_2d(request); ASSERT_TRUE(base);
    auto h=vmm::HorizonGrid::make({0,1,2},{0,1},{"bottom","absent","pinch","top"},
        {{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,1,0,0,1},{2,2,2,2,2,2}});
    ASSERT_TRUE(h);
    auto mesh=vmm::generate_layered_mesh(base->mesh,*h); ASSERT_TRUE(mesh) << mesh.error().message();
    auto domain=vmm::cvt_domain(*mesh); ASSERT_TRUE(domain) << domain.error().message();
    EXPECT_EQ(domain->source_region,(std::vector<vmm::RegionId>{vmm::RegionId{1},vmm::RegionId{2}}));
    EXPECT_NEAR(domain->partition.total_volume(),4,1e-12);
    vmm::SiteSet3D sites; sites.add({1.6,0.4,0.2},vmm::RegionId{0}); sites.add({0.5,0.5,1.4},vmm::RegionId{1});
    vmm::CvtOptions options; options.max_iterations=2;
    auto result=vmm::optimize_cvt(domain->partition,sites,options);
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_LE(result->report.energy.back(),result->report.energy.front());
    EXPECT_FALSE(vmm::cvt_domain(vmm::LayeredMesh{}));
}
TEST(Cvt, ConfigVolumetricAndLayeredOutputs) {
    const auto dir=std::filesystem::temp_directory_path()/"vmm_cvt_3d_test";
    std::filesystem::create_directories(dir);
    auto cfg=vmm::MeshConfig::parse("dimension = 3\ncvt_iterations = 2\noutput = spatial\n"
        "[region cube]\nshape = cuboid\nlo = 0 0 0\nhi = 1 1 1\nsites = grid\nsites.spacing = 0.6\n",dir);
    ASSERT_TRUE(cfg);
    auto result=vmm::run_config(*cfg); ASSERT_TRUE(result) << result.error().message();
    ASSERT_TRUE(result->cvt); EXPECT_EQ(result->dimension,3);
    {
        std::ofstream file(dir/"levels.hgrid");
        file << "VMM_HORIZONS 1\n2 0 1\n2 0 1\n3\n\"bottom\"\n4 0 0 0 0\n"
             << "\"middle\"\n4 1 1 1 1\n\"top\"\n4 2 2 2 2\n2\n2 0 1\n2 0 1\n";
    }
    cfg=vmm::MeshConfig::parse("dimension = 2\nhorizons = levels.hgrid\ncvt_iterations = 2\noutput = layered_cvt\n"
        "[region base]\nshape = rectangle\nlo = 0 0\nhi = 1 1\nsites = grid\nsites.spacing = 0.6\n",dir);
    ASSERT_TRUE(cfg);
    result=vmm::run_config(*cfg); ASSERT_TRUE(result) << result.error().message();
    ASSERT_TRUE(result->cvt); EXPECT_EQ(result->dimension,3);
    for(const auto& path:result->written) EXPECT_NE(path.extension(),".vlayers");
    auto first=vmm::read_native<3>(dir/"layered_cvt.vmesh"); ASSERT_TRUE(first);
    auto repeated=vmm::run_config(*cfg); ASSERT_TRUE(repeated);
    auto second=vmm::read_native<3>(dir/"layered_cvt.vmesh"); ASSERT_TRUE(second);
    EXPECT_EQ(first->points(),second->points());
    EXPECT_TRUE(std::ranges::equal(first->sites(),second->sites()));
    EXPECT_EQ(result->cvt->energy,repeated->cvt->energy);
    EXPECT_EQ(first->cell_count(),result->cells);
    EXPECT_FALSE(std::filesystem::exists(dir/"layered_cvt.vlayers"));
}

TEST(Cvt, SpatialSitesKeepInternalPlaneAndCurvedBoundary) {
    vmm::Declaration3D d; const auto medium=*d.media().add("geometry");
    (void)d.add_region("low",medium,vmm::Cuboid({0,0,0},{1,1,1}));
    (void)d.add_region("high",medium,vmm::Cuboid({0,0,1},{1,1,2}));
    auto p=vmm::cgal_backend_3d().build_partition(d); ASSERT_TRUE(p);
    vmm::SiteSet3D sites;
    sites.add({0.2,0.2,0.2},vmm::RegionId{0}); sites.add({0.75,0.7,0.6},vmm::RegionId{0});
    sites.add({0.3,0.7,1.2},vmm::RegionId{1}); sites.add({0.8,0.25,1.8},vmm::RegionId{1});
    vmm::CvtOptions o; o.max_iterations=4;
    auto result=vmm::optimize_cvt(*p,sites,o); ASSERT_TRUE(result) << result.error().message();
    EXPECT_LT(result->report.energy.back(),result->report.energy.front());
    double area=0;
    for(auto f:result->mesh.internal_faces()) if(result->mesh.region(result->mesh.owner(f))!=result->mesh.region(result->mesh.neighbour(f))) {
        area+=vmm::norm(vmm::face_geometry(result->mesh,f).area_vector);
        for(auto v:result->mesh.face_vertices(f)) EXPECT_NEAR(result->mesh.point(v)[2],1,1e-12);
    }
    EXPECT_NEAR(area,1,1e-12);
    vmm::Declaration3D curved;
    const auto med=*curved.media().add("geometry");
    (void)curved.add_region("sphere",med,vmm::Sphere({0,0,0},1));
    auto sphere=vmm::cgal_backend_3d().build_partition(curved); ASSERT_TRUE(sphere);
    vmm::SiteSet3D seed; seed.add({0.1,0.1,0.1},vmm::RegionId{0});
    auto centered=vmm::optimize_cvt(*sphere,seed,o); ASSERT_TRUE(centered) << centered.error().message();
    EXPECT_TRUE(centered->report.converged);
    EXPECT_NEAR(vmm::norm(centered->mesh.sites()[0]),0,1e-10);
}
TEST(Cvt, ScaleTranslationAndIterationLimit) {
    for(double scale:{0.01,1.,100.}) {
        vmm::Declaration2D d; auto medium=*d.media().add("geometry");
        (void)d.add_region("square",medium,vmm::Rectangle({10,10},{10+scale,10+scale}));
        auto p=vmm::cgal_backend_2d().build_partition(d); ASSERT_TRUE(p);
        vmm::SiteSet sites; sites.add({10+0.2*scale,10+0.3*scale},vmm::RegionId{0});
        vmm::CvtOptions o; o.max_iterations=1; o.relaxation=0.5;
        auto m=vmm::optimize_cvt(*p,sites,o); ASSERT_TRUE(m) << m.error().message();
        EXPECT_FALSE(m->report.converged);
        EXPECT_FALSE(m->report.stalled);
        EXPECT_EQ(m->report.relative_displacement.size(),1u);
        EXPECT_NEAR(m->report.energy.back()/std::pow(scale,4),1./6+(0.09+0.04)/4,1e-9);
    }
}
