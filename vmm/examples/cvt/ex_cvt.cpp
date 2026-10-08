// SPDX-License-Identifier: BSD-3-Clause
// Title: Constrained centroidal Voronoi tessellation
// Description: Relaxes four sites while preserving the square boundary.
//              Prints energy and convergence; colors represent geometric regions.
//==============================================================================
//  C++ standard library
//==============================================================================
#include <print>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/cvt.hpp>
#include <vmm/io/native.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/vmm.hpp>
int main() {
    vmm::MeshRequest2D request;
    auto medium=request.declaration.media().add("geometry");
    if(!medium) return 1;
    auto region=request.declaration.add_region("square",*medium,vmm::Rectangle({0,0},{1,1}));
    if(!region) return 1;
    const std::vector<vmm::Vec2> points{{0.1,0.1},{0.7,0.2},{0.2,0.8},{0.9,0.9}};
    request.sources.push_back(vmm::sites_for(*region,vmm::ExplicitSites(points)));
    auto initial=vmm::generate_mesh_2d(request);
    if(!initial) { std::println("{}",initial.error().message()); return 1; }
    vmm::SiteSet sites;
    sites.append(points,*region);
    vmm::CvtOptions options; options.max_iterations=100;
    auto result=vmm::optimize_cvt(initial->partition,sites,options);
    if(!result) { std::println("{}",result.error().message()); return 1; }
    std::println("accepted iterations: {}, converged: {}, stalled: {}",
        result->report.relative_displacement.size(),result->report.converged,result->report.stalled);
    std::println("energy: {:.10g} -> {:.10g}; relative residual: {:.3e}",
        result->report.energy.front(),result->report.energy.back(),result->report.relative_residual);
    if(!vmm::write_vtu(initial->mesh,"initial.vtu") ||
       !vmm::write_vtu(result->mesh,"optimized.vtu") ||
       !vmm::write_native(result->mesh,"optimized.vmesh")) return 1;
}
