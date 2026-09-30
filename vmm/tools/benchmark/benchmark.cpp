// ============================================================================
// File: benchmark.cpp
// Description: Light benchmark (DEC-021): B1 unit square with random sites,
//              B2 anchor A1, B3 anchor A2. Reports time per phase, peak RSS,
//              invariants and a topology checksum. Usage:
//                vmm_benchmark [B1|B2|B3|all] [B1 cell count]
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <print>
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <sys/resource.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "../anchors/anchors.hpp"
#include <vmm/backend/cgal.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace {

using Clock = std::chrono::steady_clock;

double since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

long peak_rss_kb() {
    rusage u{};
    getrusage(RUSAGE_SELF, &u);
    return u.ru_maxrss;
}

std::uint64_t topology_checksum(const vmm::Mesh2D& m) {
    std::uint64_t h = 1469598103934665603ull;  // FNV-1a over owner/neighbour/patch sizes
    auto mix = [&](std::uint64_t v) {
        h ^= v;
        h *= 1099511628211ull;
    };
    for (const auto o : m.owners()) mix(o.value);
    for (const auto n : m.neighbours()) mix(n.value);
    for (const auto& p : m.patches()) mix(p.count);
    return h;
}

int run(const std::string& name, vmm::anchors::Anchor anchor) {
    const auto backend = vmm::cgal_backend_2d();
    auto t = Clock::now();
    const auto partition = backend.build_partition(anchor.declaration);
    const double t_partition = since(t);
    if (!partition) {
        std::println("{}: partition failed: {}", name, partition.error().message());
        return 1;
    }
    t = Clock::now();
    const auto sites = vmm::generate_sites(*partition, anchor.sources, {});
    const double t_sites = since(t);
    if (!sites) {
        std::println("{}: sites failed: {}", name, sites.error().message());
        return 1;
    }
    t = Clock::now();
    const auto b = vmm::build_mesh_2d(*partition, *sites, backend);
    const double t_build = since(t);
    if (!b) {
        std::println("{}: build failed: {}", name, b.error().message());
        return 1;
    }
    t = Clock::now();
    auto ref = vmm::invariant_reference(*partition);
    ref.cell_measure = b->cell_polygon_area;
    const auto inv = vmm::check_invariants(b->mesh, ref);
    const double t_check = since(t);
    const double n = static_cast<double>(b->mesh.cell_count());
    std::println("{}: cells {} faces {} | partition {:.3f} s | sites {:.3f} s | build {:.3f} s (delaunay {:.3f}, cells {:.3f}, "
                 "assembly {:.3f}; fast {} clipped {}) | invariants {:.3f} s {} | max non-orthogonality (rounding) {:.1e} rad | peak RSS {:.0f} MB ({:.0f} B/cell) | "
                 "checksum {:016x}",
                 name, b->mesh.cell_count(), b->mesh.face_count(), t_partition, t_sites, t_build,
                 b->stats.seconds_delaunay, b->stats.seconds_cells, b->stats.seconds_assembly, b->stats.fast_cells,
                 b->stats.clipped_cells, t_check, inv.passed(ref) ? "PASS" : "FAIL", inv.max_nonortho_internal,
                 static_cast<double>(peak_rss_kb()) / 1024, 1024.0 * static_cast<double>(peak_rss_kb()) / n,
                 topology_checksum(b->mesh));
    if (!inv.passed(ref)) {
        std::println("  invariants: total {:.2e} region {:.2e} cell {:.2e} boundary {:.2e} interface {:.2e} closure {:.2e}/{:.2e} "
                     "bad {} nonpositive {} nonconforming {} symmetric {} | {}",
                     inv.total_relative_error, inv.max_region_relative_error, inv.max_cell_measure_error,
                     inv.boundary_relative_error, inv.max_interface_relative_error, inv.max_closure, inv.closure_tolerance,
                     inv.bad_faces, inv.nonpositive_cells, inv.nonconforming_faces, inv.adjacency_symmetric,
                     inv.first_problem);
    }
    return inv.passed(ref) ? 0 : 1;
}

vmm::anchors::Anchor b1(std::size_t count) {
    vmm::anchors::Anchor a;
    (void)a.declaration.add_region("square", *a.declaration.media().add("m"),
                                   vmm::Rectangle(vmm::Vec2{0, 0}, vmm::Vec2{1, 1}, {"s", "e", "n", "w"}));
    a.sources.push_back(vmm::sites_for(vmm::RegionId{0}, vmm::RandomCountSource(count)));
    return a;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string which = argc > 1 ? argv[1] : "all";
    const std::size_t count = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 1000000;
    std::println("vmm benchmark (DEC-021) | {}", vmm::cgal_backend_2d().info().versions);
    int status = 0;
    if (which == "B1" || which == "all") status |= run(std::format("B1 ({} sites)", count), b1(count));
    if (which == "B2" || which == "all") status |= run("B2 (A1)", vmm::anchors::a1(false, 0.25));
    if (which == "B3" || which == "all") status |= run("B3 (A2)", vmm::anchors::a2(0.5));
    return status;
}
