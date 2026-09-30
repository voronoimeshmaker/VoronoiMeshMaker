// ============================================================================
// File: ex_stl_domain.cpp
// Title: 3D - domain from an STL file
// Description: Writes a terrain block as an ASCII STL file (one solid per
//              patch), reads it back as a real file would be read, repairs
//              it into a closed surface and builds the 3D Voronoi mesh of
//              the block, clipping the cells against the terrain.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <print>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "anchors/anchors.hpp"
#include <vmm/io/native.hpp>
#include <vmm/io/stl.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/vmm.hpp>

int main() {
    // An STL file to start from (in practice it comes from a CAD or GIS tool).
    if (!vmm::write_stl(vmm::anchors::terrain_block(24), "terrain.stl")) return 1;

    vmm::SurfaceRepairReport repair;
    const auto surface = vmm::read_stl_surface("terrain.stl", {}, {}, &repair);
    if (!surface) {
        std::println("error: {}", surface.error().message());
        return 1;
    }
    std::println("terrain.stl: {} triangles, {} points welded, {} components, volume {:.6f}", surface->triangle_count(),
                 repair.welded_points, repair.components, surface->volume());

    vmm::MeshRequest3D request;
    const auto soil = *request.declaration.media().add("soil");
    const auto ground = *request.declaration.add_region("ground", soil, vmm::SurfaceShape(*surface));
    request.sources = {vmm::sites_for_3d(ground, vmm::UniformRandomSource3D(0.06))};
    request.sites.seed = 7;

    const auto result = vmm::generate_mesh_3d(request);
    if (!result) {
        std::println("error: {}", result.error().message());
        return 1;
    }
    const auto& mesh = result->mesh;
    std::println("cells {} | internal faces {} | boundary faces {} | cells clipped {} (local {})", mesh.cell_count(),
                 mesh.internal_face_count(), mesh.boundary_faces().size(), result->stats.clipped_cells,
                 result->stats.local_clips);
    for (const auto& patch : mesh.patches()) std::println("  patch {:8} {} faces", patch.name, patch.count);
    if (!vmm::write_vtu(mesh, "stl_domain.vtu") || !vmm::write_native(mesh, "stl_domain.vmesh")) return 1;
    std::println("wrote stl_domain.vtu and stl_domain.vmesh");
    return 0;
}
