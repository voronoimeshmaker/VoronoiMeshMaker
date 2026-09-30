// ============================================================================
// File: ex_anchor_a3.cpp
// Title: 3D - anchor A3, soil block with a river
// Description: Four regions by precedence (lower and upper soil with an
//              inclined contact, air above the terrain, a curved trapezoidal
//              channel of water), conforming interfaces that meet along triple
//              lines, and the area of every interface (P04 §2.3).
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
#include <vmm/io/vtu.hpp>
#include <vmm/vmm.hpp>

int main() {
    auto a3 = vmm::anchors::a3();
    vmm::MeshRequest3D request{std::move(a3.declaration), std::move(a3.sources), {}, {}};
    const auto result = vmm::generate_mesh_3d(request);
    if (!result) {
        std::println("error: {}", result.error().message());
        return 1;
    }
    const auto& mesh = result->mesh;
    const auto& p = result->partition;
    std::println("cells {} | internal faces {} ({} on interfaces) | boundary faces {}", mesh.cell_count(),
                 mesh.internal_face_count(), result->stats.interface_faces, mesh.boundary_faces().size());
    for (std::size_t r = 0; r < p.region_count(); ++r) {
        std::println("  region {:10} volume {:>10.1f} m3", p.regions()[r].name, p.region_volume(vmm::RegionId::from_index(r)));
    }
    for (std::size_t a = 0; a < p.region_count(); ++a) {
        for (std::size_t b = a + 1; b < p.region_count(); ++b) {
            const double area = p.interface_area(vmm::RegionId::from_index(a), vmm::RegionId::from_index(b));
            if (area > 0) std::println("  interface {}/{}: {:.1f} m2", p.regions()[a].name, p.regions()[b].name, area);
        }
    }
    std::println("invariants: interface area error {:.1e}, closure {:.1e}", result->invariants.max_interface_relative_error,
                 result->invariants.max_closure);
    if (!vmm::write_vtu(mesh, "anchor_a3.vtu") || !vmm::write_native(mesh, "anchor_a3.vmesh")) return 1;
    std::println("wrote anchor_a3.vtu and anchor_a3.vmesh");
    return 0;
}
