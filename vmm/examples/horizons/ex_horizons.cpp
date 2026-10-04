// SPDX-License-Identifier: BSD-3-Clause
// Title: Fixed horizons and graded columns
// Description: Generates flat depth intervals and a faceted terrain with pinch-out.
//              Region identifiers are geometry labels; no physical properties are assigned.
//==============================================================================
//  C++ standard library
//==============================================================================
#include <print>
#include <string>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/io/layered.hpp>
#include <vmm/io/native.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/layered.hpp>
#include <vmm/vmm.hpp>

int main() {
    vmm::MeshRequest2D request;
    auto& d=request.declaration;
    const auto medium=d.media().add("geometry");
    if(!medium) return 1;
    const auto region=d.add_region("base",*medium,vmm::Rectangle({0,0},{2,1}));
    if(!region) return 1;
    request.sources.push_back(vmm::sites_for(*region,vmm::CartesianGridSource(0.25)));
    const auto base=vmm::generate_mesh_2d(request);
    if(!base) { std::println("{}",base.error().message()); return 1; }
    auto flat=vmm::HorizonGrid::make({0,1,2},{0,1},{"bottom","interface","top"},
        {{0,0,0,0,0,0},{1,1,1,1,1,1},{2,2,2,2,2,2}});
    auto terrain=vmm::HorizonGrid::make({0,1,2},{0,1},{"bottom","interface","top"},
        {{0,0,0,0,0,0},{0,0.5,1,0,0.8,1},{2,2,2,2,2,2}});
    if(!flat || !terrain) return 1;
    for(const auto& name:std::vector<std::string>{"flat","terrain"}) {
        const auto mesh=vmm::generate_layered_mesh(base->mesh,name=="flat"?*flat:*terrain,
            {{0,0.25,1},{0,0.5,1}});
        if(!mesh) { std::println("{}",mesh.error().message()); return 1; }
        std::println("{}: {} columns, {} cells, {} regions",name,mesh->data().column_count,
            mesh->mesh().cell_count(),mesh->mesh().regions().size());
        if(!vmm::write_layered(*mesh,name+".vlayers") ||
           !vmm::write_native(mesh->mesh(),name+".vmesh") ||
           !vmm::write_vtu(mesh->mesh(),name+".vtu")) return 1;
    }
}
