// ============================================================================
// File: p15a_main.cpp
// Description: P15a - the cases of sequencia_prompts.md (convex, non-convex,
//              sharp edges, complex boundary cells, several components,
//              near-degenerate, order independence, stress) with the DEC-011
//              invariants as success criterion. Returns non-zero on failure.
//   p15a [all | quick | <case name>]
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <functional>
#include <numbers>
#include <print>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <sys/resource.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "p15a.hpp"

namespace {

using p15a::Domain;
using p15a::Real;
using p15a::Vec3;

int failures = 0;

void expect(bool ok, const std::string& what) {
    if (!ok) ++failures;
    std::println("    [{}] {}", ok ? "ok" : "FAIL", what);
}

Real peak_rss_mb() {
    rusage u{};
    getrusage(RUSAGE_SELF, &u);
    return static_cast<Real>(u.ru_maxrss) / 1024.0;
}

std::vector<Vec3> sites_for(const Domain& d, std::size_t count, std::uint64_t seed) {
    const auto m = p15a::measures(d);
    const Real h = std::cbrt(m.volume / static_cast<Real>(count));
    return p15a::random_sites(d, count, 0.6 * h, 0.2 * h, seed);
}

struct Outcome {
    p15a::Build build;
    vmm::InvariantReport report;
    bool passed = false;
};

/// Builds, checks the invariants and prints one block; `strict` adds the
/// construction criteria (every face matched, no clip failure, ...).
Outcome run(const std::string& name, const Domain& d, const std::vector<Vec3>& sites, bool print = true) {
    Outcome o;
    o.build = p15a::build_mesh(d, sites);
    const auto m = p15a::measures(d);
    vmm::InvariantReference ref;
    ref.length_scale = m.diagonal;
    ref.total_measure = m.volume;
    ref.region_measure = {m.volume};
    ref.boundary_measure = m.area;
    ref.cell_measure = o.build.cell_volume;
    const auto& s = o.build.stats;
    if (!o.build.error.empty()) {
        std::println("  {}: Mesh::from_data failed: {}", name, o.build.error);
        ++failures;
        return o;
    }
    o.report = vmm::check_invariants(o.build.mesh, ref);
    const auto& r = o.report;
    const Real total = s.seconds_delaunay + s.seconds_convex + s.seconds_classify + s.seconds_clip + s.seconds_assembly;
    if (print) {
        std::println("  {}: {} cells ({} clipped exactly), {} faces ({} internal)", name, s.cells, s.boundary_cells,
                     o.build.mesh.face_count(), o.build.mesh.internal_face_count());
        std::println("    invariants: volume {:.1e} | cell volume {:.1e} | boundary area {:.1e} | closure {:.1e} "
                     "(tol {:.1e}) | bad faces {} | non-positive cells {} | adjacency symmetric {} | max non-ortho "
                     "{:.1e} rad",
                     r.total_relative_error, r.max_cell_measure_error, r.boundary_relative_error, r.max_closure,
                     r.closure_tolerance, r.bad_faces, r.nonpositive_cells, r.adjacency_symmetric,
                     r.max_nonortho_internal);
        std::println("    construction: clip failures {} (invalid cell meshes {}) | unmatched faces {} (tiny dropped {}) | piece-count mismatch {} "
                     "| max |S_ij+S_ji|/|S_ij| {:.1e} | faces with holes {} | unlabelled {} | ambiguous {} | leftover "
                     "box faces {} | Euler failures {} | unsnapped vertices {} | merged vertices {} | T-vertices {} | disconnected cells {}",
                     s.clip_failures, s.invalid_cell_meshes, s.unmatched_faces, s.dropped_tiny_faces, s.piece_count_mismatch,
                     s.max_partner_mismatch, s.faces_with_holes, s.unlabelled_triangles, s.ambiguous_triangles,
                     s.leftover_box_faces, s.euler_failures, s.unsnapped_vertices, s.merged_vertices, s.t_vertices, s.disconnected_cells);
        std::println("    time: delaunay {:.3f} s | convex cells {:.3f} s | classify {:.3f} s | exact clip {:.3f} s "
                     "({:.2f} ms per clipped cell) | assembly {:.3f} s | total {:.3f} s ({:.1f} us per cell) | peak RSS "
                     "{:.0f} MB",
                     s.seconds_delaunay, s.seconds_convex, s.seconds_classify, s.seconds_clip,
                     s.boundary_cells ? 1e3 * s.seconds_clip / static_cast<Real>(s.boundary_cells) : 0.0,
                     s.seconds_assembly, total, 1e6 * total / static_cast<Real>(std::max<std::size_t>(s.cells, 1)),
                     peak_rss_mb());
    }
    o.passed = r.passed(ref) && s.clip_failures == 0 && s.unmatched_faces == 0 && s.faces_with_holes == 0 &&
               s.unlabelled_triangles == 0 && s.leftover_box_faces == 0 && s.euler_failures == 0;
    if (print) {
        expect(r.passed(ref), name + ": DEC-011 invariants within 1e-12");
        expect(s.clip_failures == 0 && s.unmatched_faces == 0 && s.faces_with_holes == 0 &&
                   s.unlabelled_triangles == 0 && s.leftover_box_faces == 0 && s.euler_failures == 0,
               name + ": construction consistent (every clip done, every face seen from both cells)");
    }
    return o;
}

std::vector<std::array<Real, 2>> l_shape() { return {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}}; }

std::vector<std::array<Real, 2>> star(int spikes, Real r_out, Real r_in) {
    std::vector<std::array<Real, 2>> p;
    for (int k = 0; k < 2 * spikes; ++k) {
        const Real a = std::numbers::pi * static_cast<Real>(k) / static_cast<Real>(spikes);
        const Real r = k % 2 == 0 ? r_out : r_in;
        p.push_back({r * std::cos(a), r * std::sin(a)});
    }
    return p;
}

std::vector<std::array<Real, 2>> wedge(Real degrees) {
    const Real a = degrees * std::numbers::pi / 180;
    return {{0, 0}, {2, 0}, {2 * std::cos(a), 2 * std::sin(a)}};
}

void convex() {
    std::println("C1 convex domains");
    run("cube, 2000 random sites", p15a::box({0, 0, 0}, {1, 1, 1}), sites_for(p15a::box({0, 0, 0}, {1, 1, 1}), 2000, 1));
    const Domain s = p15a::icosphere({0, 0, 0}, 1, 2);
    run("icosphere (320 triangles), 2000 random sites", s, sites_for(s, 2000, 2));
}

void nonconvex() {
    std::println("C2 non-convex domain");
    const Domain d = p15a::prism(l_shape(), 0, 1);
    run("L-shaped prism, 3000 random sites", d, sites_for(d, 3000, 3));
}

void sharp() {
    std::println("C3 sharp edges");
    const Domain d = p15a::prism(wedge(15), 0, 1);
    run("15-degree wedge, 1500 random sites", d, sites_for(d, 1500, 4));
    const Domain e = p15a::prism(wedge(3), 0, 1);
    run("3-degree wedge, 800 random sites", e, sites_for(e, 800, 5));
}

void complex_boundary() {
    std::println("C4 complex boundary cells");
    const Domain d = p15a::prism(star(12, 1, 0.35), 0, 0.3);
    run("12-spike star, 150 sites (cells spanning several spikes)", d, sites_for(d, 150, 6));
    run("12-spike star, 3000 sites", d, sites_for(d, 3000, 7));
}

void components() {
    std::println("C5 several components");
    const Domain d = p15a::merge({p15a::box({0, 0, 0}, {1, 1, 1}), p15a::box({1.5, 0, 0}, {2.5, 1, 1}),
                                  p15a::icosphere({4, 0.5, 0.5}, 0.5, 2)});
    run("two cubes and a sphere, 1500 random sites", d, sites_for(d, 1500, 8));
}

void degenerate() {
    std::println("C6 near-degenerate cases");
    const Domain cube = p15a::box({0, 0, 0}, {1, 1, 1});
    std::vector<Vec3> grid;
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            for (int k = 0; k < 10; ++k) grid.push_back({(i + 0.5) / 10, (j + 0.5) / 10, (k + 0.5) / 10});
        }
    }
    run("cube, 10x10x10 Cartesian grid (8 cospherical sites per vertex)", cube, grid);
    std::vector<Vec3> near = sites_for(cube, 1000, 9);
    for (int k = 0; k < 40; ++k) {
        const Real t = (k + 0.5) / 40;
        near.push_back({1e-9, t, 0.5});
        near.push_back({t, 1 - 1e-9, 0.37});
    }
    run("cube, 1000 random + 80 sites at 1e-9 L from the faces", cube, p15a::inside(cube, near));
    const Domain l = p15a::prism(l_shape(), 0, 1);
    std::vector<Vec3> lg;
    for (int i = 0; i < 20; ++i) {
        for (int j = 0; j < 20; ++j) {
            for (int k = 0; k < 10; ++k) lg.push_back({(i + 0.5) / 10, (j + 0.5) / 10, (k + 0.5) / 10});
        }
    }
    run("L-shaped prism, Cartesian grid", l, p15a::inside(l, lg));
}

void order() {
    std::println("C7 input-order independence and scale");
    const Domain d = p15a::prism(l_shape(), 0, 1);
    const auto sites = sites_for(d, 2000, 10);
    const Outcome base = run("L-shaped prism, 2000 sites, input order", d, sites);
    const auto topo = p15a::topology(base.build.mesh);
    const auto sum = p15a::checksum(base.build.mesh);
    std::uint64_t seed = 11;
    for (int k = 0; k < 3; ++k) {
        auto p = sites;
        for (std::size_t i = p.size(); i > 1; --i) {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            std::swap(p[i - 1], p[(seed >> 33) % i]);
        }
        const Outcome o = run(std::format("permutation {}", k + 1), d, p, false);
        bool map_ok = true;
        for (const auto c : o.build.mesh.cells()) {
            map_ok = map_ok && o.build.mesh.site(c) == p[o.build.mesh.cell_input_sites()[c.index()].index()];
        }
        const bool same_topology = p15a::topology(o.build.mesh) == topo;
        const bool same_bits = p15a::checksum(o.build.mesh) == sum;
        if (!same_bits && std::getenv("P15A_DEBUG") != nullptr) {
            const auto& a = base.build.mesh.points();
            const auto& c = o.build.mesh.points();
            std::size_t diff = 0;
            for (std::size_t q = 0; q < std::min(a.size(), c.size()); ++q) {
                if (a[q] != c[q]) {
                    if (diff++ < 3) {
                        std::println("      point {}: ({:.17g},{:.17g},{:.17g}) vs ({:.17g},{:.17g},{:.17g})", q, a[q][0],
                                     a[q][1], a[q][2], c[q][0], c[q][1], c[q][2]);
                    }
                }
            }
            std::println("      points {} vs {}, {} differ", a.size(), c.size(), diff);
        }
        expect(same_topology && same_bits && map_ok,
               std::format("permutation {}: bit-identical mesh, input map consistent (topology {}, bits {}, map {})",
                           k + 1, same_topology, same_bits, map_ok));
    }
    for (const Real s : {1e-3, 1e6}) {
        std::vector<Vec3> ps;
        for (const Vec3& x : sites) ps.push_back(s * x);
        const Outcome o = run(std::format("scale {:g}", s), p15a::scaled(d, s), ps);
        expect(p15a::topology(o.build.mesh) == topo, std::format("scale {:g}: same topology", s));
    }
}

void stress() {
    std::println("C8 stress of the exact clipping and time per cell");
    const Domain l = p15a::prism(l_shape(), 0, 1);
    run("L-shaped prism, 100000 sites", l, sites_for(l, 100000, 12));
    const Domain s = p15a::prism(star(12, 1, 0.35), 0, 0.3);
    run("12-spike star, 30000 sites", s, sites_for(s, 30000, 13));
}

}  // namespace

int main(int argc, char** argv) {
    const std::string which = argc > 1 ? argv[1] : "all";
    std::println("P15a - 3D clipping proof of concept ({})", p15a::backend_versions());
    const std::vector<std::pair<std::string, std::function<void()>>> cases{
        {"convex", convex},     {"nonconvex", nonconvex}, {"sharp", sharp},   {"complex", complex_boundary},
        {"components", components}, {"degenerate", degenerate}, {"order", order}, {"stress", stress}};
    for (const auto& [name, fn] : cases) {
        if (which == "all" || which == name || (which == "quick" && name != "stress")) fn();
    }
    std::println("P15a result: {} ({} failed checks)", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}
