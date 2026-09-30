// ============================================================================
// File: ex_quickstart.cpp
// Title: Quick start - two regions with a hole
// Description: Declares a unit square with a square hole and an inner region
//              painted over it, generates sites per region, builds the
//              conforming Voronoi mesh and writes it as .vtu and native files.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <print>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/io/native.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/vmm.hpp>

int main() {
    vmm::MeshRequest2D request;
    auto& d = request.declaration;
    const auto rock = *d.media().add("rock");
    const auto outer = *d.add_region("outer", rock, vmm::Rectangle({0, 0}, {1, 1}, {"south", "east", "north", "west"}));
    const auto inner = *d.add_region("inner", rock, vmm::Rectangle({0, 0}, {0.7, 0.7}));
    (void)d.add_hole(vmm::Rectangle({0.4, 0.4}, {0.6, 0.6}, {"hole", "hole", "hole", "hole"}));
    request.sources = {vmm::sites_for(outer, vmm::UniformRandomSource(0.05)),
                       vmm::sites_for(inner, vmm::UniformRandomSource(0.03))};
    request.sites.seed = 42;

    const auto result = vmm::generate_mesh_2d(request);
    if (!result) {
        std::println("error: {}", result.error().message());
        return 1;
    }
    const auto& mesh = result->mesh;
    std::println("cells {} | internal faces {} | boundary faces {} | interface faces {}", mesh.cell_count(),
                 mesh.internal_face_count(), mesh.boundary_faces().size(), result->invariants.interface_faces);
    for (const auto& patch : mesh.patches()) std::println("  patch {:6} {} faces", patch.name, patch.count);
    if (!vmm::write_vtu(mesh, "quickstart.vtu") || !vmm::write_native(mesh, "quickstart.vmesh")) return 1;
    std::println("wrote quickstart.vtu and quickstart.vmesh");
    return 0;
}
