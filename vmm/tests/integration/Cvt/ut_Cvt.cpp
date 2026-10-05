// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <limits>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/cgal.hpp>
#include <vmm/cvt.hpp>
#include <vmm/domain/shapes3d.hpp>
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
