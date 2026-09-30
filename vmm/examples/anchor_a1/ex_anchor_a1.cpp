// ============================================================================
// File: ex_anchor_a1.cpp
// Title: Anchor problem A1
// Description: Builds anchor A1 of P04 (DEC-017), checks the DEC-011
//              invariants, prints the quality report per region and writes
//              anchor_a1.vtu and anchor_a1.vmesh.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <numbers>
#include <print>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <anchors/anchors.hpp>
#include <vmm/io/native.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/vmm.hpp>

int main() {
    auto anchor = vmm::anchors::a1();
    vmm::MeshRequest2D request{std::move(anchor.declaration), std::move(anchor.sources), {}, {}, {}};
    const auto result = vmm::generate_mesh_2d(request);
    if (!result) {
        std::println("error: {}", result.error().message());
        return 1;
    }
    const auto& mesh = result->mesh;
    const auto metrics = vmm::compute_metrics(mesh);
    const auto quality = vmm::quality_report(mesh, metrics);
    std::println("A1: cells {} | faces {} | interface faces {} | fast cells {} | clipped cells {}", mesh.cell_count(),
                 mesh.face_count(), result->invariants.interface_faces, result->stats.fast_cells, result->stats.clipped_cells);
    for (std::size_t r = 0; r < quality.regions.size(); ++r) {
        const auto& q = quality.regions[r];
        std::println("  {:10} cells {:7} | non-orthogonality internal {:.1e} rad, interface {:.1f} deg | p99 aspect {:.2f}",
                     mesh.regions()[r].name, q.cells, q.max_nonortho_internal,
                     q.max_nonortho_interface * 180 / std::numbers::pi, q.p99_aspect_ratio);
    }
    if (!vmm::write_vtu(mesh, "anchor_a1.vtu") || !vmm::write_native(mesh, "anchor_a1.vmesh")) return 1;
    std::println("wrote anchor_a1.vtu and anchor_a1.vmesh");
    return 0;
}
