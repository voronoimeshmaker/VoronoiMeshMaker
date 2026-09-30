// ============================================================================
// File: ex_voronoi3d.cpp
// Title: 3D - Voronoi volumes in an L-shaped block
// Description: Extrudes an L-shaped outline into a non-convex block with
//              tagged walls, fills it with random sites, builds the 3D
//              Voronoi finite-volume mesh (version 0.3, one region) and
//              writes it as .vtu and native files.
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
    // The L-shaped outline in the xy-plane; each edge tag becomes a wall patch.
    const auto outline = vmm::ShapeOutline::make({{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}},
                                                 {"south", "east", "notch", "notch", "north", "west"});
    if (!outline) {
        std::println("error: {}", outline.error().message());
        return 1;
    }
    vmm::MeshRequest3D request;
    auto& d = request.declaration;
    const auto rock = *d.media().add("rock");
    const auto block = d.add_region("block", rock, vmm::Extrusion(*outline, 0, 0.5, "floor", "top"));
    if (!block) {
        std::println("error: {}", block.error().message());
        return 1;
    }
    request.sources = {vmm::sites_for_3d(*block, vmm::UniformRandomSource3D(0.12))};
    request.sites.seed = 42;

    const auto result = vmm::generate_mesh_3d(request);
    if (!result) {
        std::println("error: {}", result.error().message());
        return 1;
    }
    const auto& mesh = result->mesh;
    std::println("cells {} | internal faces {} | boundary faces {} | cells clipped exactly {}", mesh.cell_count(),
                 mesh.internal_face_count(), mesh.boundary_faces().size(), result->stats.clipped_cells);
    for (const auto& patch : mesh.patches()) std::println("  patch {:6} {} faces", patch.name, patch.count);
    std::println("invariants: volume error {:.1e}, closure {:.1e}", result->invariants.total_relative_error,
                 result->invariants.max_closure);
    if (!vmm::write_vtu(mesh, "voronoi3d.vtu") || !vmm::write_native(mesh, "voronoi3d.vmesh")) return 1;
    std::println("wrote voronoi3d.vtu and voronoi3d.vmesh");
    return 0;
}
