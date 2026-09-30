// ============================================================================
// File: p05a_3_box3d.cpp
// Description: P05a.3 - minimal 3D proof: unit box, one region, Voronoi cells
//              by half-space clipping, checked with the same PolyMesh and the
//              same invariant checker as the 2D proof.
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <numbers>
#include <print>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "app_support.hpp"
#include <p05a/backend.hpp>
#include <p05a/invariants.hpp>
#include <p05a/voronoi3d.hpp>

namespace {

using namespace vmm::p05a;

constexpr std::size_t kSites = 500;
constexpr std::uint64_t kSeed = 20260928;

/// Dart throwing in the unit cube: minimum spacing and a margin to the faces.
std::vector<Vec3> make_sites() {
    const Real h = std::cbrt(1.0 / static_cast<Real>(kSites));
    const Real min_distance = 0.5 * h;
    const Real margin = 0.25 * h;
    app::Random rng(kSeed);
    std::vector<Vec3> sites;
    for (std::size_t attempt = 0; sites.size() < kSites && attempt < 1000000; ++attempt) {
        const Vec3 x{rng.uniform(), rng.uniform(), rng.uniform()};
        const bool inside = std::ranges::all_of(x, [&](Real c) { return c > margin && c < 1 - margin; });
        if (inside && std::ranges::all_of(sites, [&](const Vec3& y) { return norm(x - y) >= min_distance; })) {
            sites.push_back(x);
        }
    }
    return sites;
}

struct Run {
    Build3D build;
    InvariantReference ref;
    InvariantReport report;
    app::Signature sig;
};

Run run(const std::vector<Vec3>& unit_sites, Real scale, int order) {
    const auto perm = app::permutation(unit_sites.size(), order);
    std::vector<Vec3> sites;
    for (const std::uint32_t k : perm) sites.push_back(scale * unit_sites[k]);
    const Vec3 lo{0, 0, 0};
    const Vec3 hi{scale, scale, scale};

    Run r;
    r.ref.length_scale = scale * std::numbers::sqrt3;
    r.ref.total_measure = hi[0] * hi[1] * hi[2];
    r.ref.region_measure = {r.ref.total_measure};
    r.ref.boundary_measure = 6 * scale * scale;
    const Backend3D backend{&delaunay_pairs_3d};
    const Real tiny_area = 1e-24 * r.ref.length_scale * r.ref.length_scale;
    r.build = build_box_mesh_3d(sites, lo, hi, backend, tiny_area);
    r.ref.cell_measure = r.build.cell_polyhedron_volume;
    r.report = check_invariants(r.build.mesh, r.ref);
    r.sig = app::signature(r.build.mesh);
    return r;
}

void expect_build(app::Checks& checks, const Build3DStats& s, const Run& r, std::string_view label) {
    std::println("    build: Euler failures {} | unmatched faces {} | dropped tiny faces {} | max |S_ij + S_ji|/|S_ij| "
                 "{:.3e} | smallest internal face {:.3e} L^2",
                 s.euler_failures, s.unmatched_faces, s.dropped_tiny_faces, s.max_partner_mismatch,
                 s.min_face_area / (r.ref.length_scale * r.ref.length_scale));
    checks.expect(s.euler_failures == 0, std::format("{}: every cell is a closed polyhedron (V - E + F = 2)", label));
    checks.expect(s.unmatched_faces == 0, std::format("{}: every internal face seen from both cells", label));
}

}  // namespace

int main() {
    std::println("P05a.3 - minimal 3D proof: unit box, one region, {} sites", kSites);
    app::print_backend();
    app::Checks checks;
    const auto sites = make_sites();
    checks.expect(sites.size() == kSites, std::format("{} sites generated", sites.size()));

    const Run base = run(sites, 1, 0);
    app::expect_invariants(checks, base.report, base.ref, "3D scale 1, order 0");
    app::expect_iteration(checks, base.build.mesh, base.report, "3D scale 1, order 0");
    expect_build(checks, base.build.stats, base, "3D scale 1, order 0");
    std::println("    faces per cell {:.2f} | memory of the mesh arrays {:.0f} bytes/cell",
                 (2.0 * static_cast<double>(base.report.internal_faces) + static_cast<double>(base.report.boundary_faces)) /
                     static_cast<double>(base.report.cells),
                 static_cast<double>(base.build.mesh.face_points.values.size() * sizeof(Vec3) +
                                     base.build.mesh.face_points.offsets.size() * sizeof(std::size_t) +
                                     base.build.mesh.face_count() * (2 * sizeof(CellId) + sizeof(PatchId))) /
                     static_cast<double>(base.report.cells));

    const std::pair<Real, int> variants[] = {{1e-3, 0}, {1e6, 0}, {1, 1}, {1, 2}};
    for (const auto& [scale, order] : variants) {
        const Run r = run(sites, scale, order);
        const std::string label = std::format("3D scale {:g}, order {}", scale, order);
        app::expect_invariants(checks, r.report, r.ref, label);
        expect_build(checks, r.build.stats, r, label);
        checks.expect(r.sig == base.sig,
                      std::format("{}: canonical topology equals scale 1, order 0 ({} faces)", label, r.sig.size()));
    }
    std::println("P05a.3 result: {} ({} failed checks)", checks.failures() == 0 ? "PASS" : "FAIL", checks.failures());
    return checks.exit_code();
}
