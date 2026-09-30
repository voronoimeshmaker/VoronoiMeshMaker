// ============================================================================
// File: p05a_2_multiregion.cpp
// Description: P05a.2 - 2D multi-region proof: unit square with a central
//              square hole and two regions separated by an L-shaped
//              interface. Strategies E1 (per-region Voronoi + common
//              refinement) and E2 (mirrored sites), each checked at three
//              scales and five insertion orders.
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <limits>
#include <map>
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
#include <p05a/voronoi2d.hpp>

namespace {

using namespace vmm::p05a;

constexpr std::uint32_t kRegionA = 0;  // [0, 0.7]^2 minus the hole
constexpr std::uint32_t kRegionB = 1;  // L-shaped remainder of the unit square
constexpr std::uint32_t kPatchOuter = 0;
constexpr std::uint32_t kPatchHole = 1;
constexpr std::uint32_t kPatchInterface = 2;
constexpr std::size_t kInterfaceSegments[] = {6, 7};

// Unit-layout points; the scaled layout multiplies each point once.
constexpr Vec2 O0{0, 0}, O1{1, 0}, O2{1, 1}, O3{0, 1};
constexpr Vec2 P0{0.7, 0}, P1{0.7, 0.7}, P2{0, 0.7};
constexpr Vec2 H0{0.4, 0.4}, H1{0.6, 0.4}, H2{0.6, 0.6}, H3{0.4, 0.6};

Layout2D make_layout(Real s) {
    auto S = [s](const Vec2& p) { return Vec2{s * p[0], s * p[1]}; };
    Layout2D layout;
    const std::pair<Vec2, Vec2> segs[] = {
        {O0, P0}, {P0, O1}, {O1, O2}, {O2, O3}, {O3, P2}, {P2, O0},  // 0-5 outer boundary
        {P0, P1}, {P1, P2},                                          // 6-7 interface
        {H0, H3}, {H3, H2}, {H2, H1}, {H1, H0}};                     // 8-11 hole (clockwise)
    const std::uint32_t patch[] = {kPatchOuter, kPatchOuter, kPatchOuter, kPatchOuter, kPatchOuter, kPatchOuter,
                                   kPatchInterface, kPatchInterface, kPatchHole, kPatchHole, kPatchHole, kPatchHole};
    for (std::size_t k = 0; k < std::size(segs); ++k) {
        layout.segments.push_back({S(segs[k].first), S(segs[k].second)});
        layout.segment_patch.push_back(PatchId{patch[k]});
    }
    layout.interface_patch = PatchId{kPatchInterface};

    // Each loop edge k starts at the start point of segment ids[k] (or its end if reversed).
    auto loop = [&](std::initializer_list<std::pair<std::size_t, bool>> ids) {
        LabelledLoop2 l;
        for (const auto& [id, reversed] : ids) {
            l.points.push_back(reversed ? layout.segments[id].b : layout.segments[id].a);
            l.labels.push_back(segment_label(id));
        }
        return l;
    };
    LabelledPolygon2 a;
    a.outer = loop({{0, false}, {6, false}, {7, false}, {5, false}});
    a.holes.push_back(loop({{8, false}, {9, false}, {10, false}, {11, false}}));
    LabelledPolygon2 b;
    b.outer = loop({{1, false}, {2, false}, {3, false}, {4, false}, {7, true}, {6, true}});
    layout.regions = {a, b};
    return layout;
}

Real polygon_area(const LabelledPolygon2& poly) {
    auto area = [](const std::vector<Vec2>& pts) {
        Real a = 0;
        for (std::size_t k = 0; k < pts.size(); ++k) a += cross(pts[k], pts[(k + 1) % pts.size()]);
        return 0.5 * a;
    };
    Real total = area(poly.outer.points);
    for (const auto& h : poly.holes) total += area(h.points);
    return total;
}

InvariantReference make_reference(const Layout2D& layout, Real s) {
    InvariantReference ref;
    ref.length_scale = s * std::numbers::sqrt2;
    for (const auto& region : layout.regions) ref.region_measure.push_back(polygon_area(region));
    ref.total_measure = ref.region_measure[0] + ref.region_measure[1];
    ref.boundary_measure = 0;
    ref.interface_measure = 0;
    for (std::size_t k = 0; k < layout.segments.size(); ++k) {
        const Real len = norm(layout.segments[k].b - layout.segments[k].a);
        (layout.segment_patch[k].value == kPatchInterface ? ref.interface_measure : ref.boundary_measure) += len;
    }
    return ref;
}

// ----------------------------------------------------------------------------
// Site generation in the unit layout (dart throwing, fixed seed).
// ----------------------------------------------------------------------------
Real distance_to_segment(const Vec2& x, const Segment2& s) {
    const Vec2 e = s.b - s.a;
    const Real t = std::clamp(dot(x - s.a, e) / dot(e, e), Real{0}, Real{1});
    return norm(x - (s.a + t * e));
}

struct SiteSet {
    std::vector<Vec2> sites;
    std::vector<RegionId> region;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::size_t> mirror_pairs;  // (a, b), a < b -> segment
    std::size_t dropped_mirrors = 0;
};

class Generator {
public:
    static constexpr Real h = 0.07;              // nominal spacing
    static constexpr Real min_distance = 0.6 * h;
    static constexpr Real margin = 0.25 * h;     // distance to every layout segment
    static constexpr Real layer = h;             // E2 mirror layer thickness

    explicit Generator(std::uint64_t seed) : rng_(seed), layout_(make_layout(1)) {}

    bool inside(std::uint32_t r, const Vec2& x) const {
        const bool in_square = x[0] > 0 && x[0] < 1 && x[1] > 0 && x[1] < 1;
        const bool in_a = x[0] < 0.7 && x[1] < 0.7;
        const bool in_hole = x[0] > 0.4 && x[0] < 0.6 && x[1] > 0.4 && x[1] < 0.6;
        return in_square && !in_hole && (r == kRegionA ? in_a : !in_a);
    }
    Real boundary_distance(const Vec2& x) const {
        Real d = std::numeric_limits<Real>::max();
        for (const auto& s : layout_.segments) d = std::min(d, distance_to_segment(x, s));
        return d;
    }
    Real interface_distance(const Vec2& x) const {
        Real d = std::numeric_limits<Real>::max();
        for (const std::size_t k : kInterfaceSegments) d = std::min(d, distance_to_segment(x, layout_.segments[k]));
        return d;
    }
    bool far_from(const std::vector<Vec2>& existing, const Vec2& x) const {
        return std::ranges::all_of(existing, [&](const Vec2& y) { return norm(x - y) >= min_distance; });
    }

    /// Adds `count` sites to region r; `extra` rejects candidates for E2.
    template <class Extra>
    void throw_darts(SiteSet& set, std::uint32_t r, std::size_t count, Extra extra) {
        std::size_t added = 0;
        for (std::size_t attempt = 0; added < count && attempt < 200000; ++attempt) {
            const Vec2 x{rng_.uniform(), rng_.uniform()};
            if (!inside(r, x) || boundary_distance(x) < margin || !far_from(set.sites, x) || !extra(x)) continue;
            set.sites.push_back(x);
            set.region.push_back(RegionId{r});
            ++added;
        }
    }

    const Layout2D& layout() const { return layout_; }

private:
    app::Random rng_;
    Layout2D layout_;
};

constexpr std::size_t kSitesA = 70;
constexpr std::size_t kSitesB = 75;
constexpr std::uint64_t kSeed = 20260928;

SiteSet make_e1() {
    Generator g(kSeed);
    SiteSet set;
    g.throw_darts(set, kRegionA, kSitesA, [](const Vec2&) { return true; });
    g.throw_darts(set, kRegionB, kSitesB, [](const Vec2&) { return true; });
    return set;
}

/// Same region-A sites as E1; the region-A sites closer than `layer` to an
/// interface segment are reflected across it into region B, and the
/// remaining region-B sites keep out of the layer.
SiteSet make_e2() {
    Generator g(kSeed);
    SiteSet set;
    g.throw_darts(set, kRegionA, kSitesA, [](const Vec2&) { return true; });
    const std::size_t a_count = set.sites.size();
    std::vector<Vec2> mirrors;
    std::vector<std::uint32_t> mirror_of;
    std::vector<std::size_t> mirror_segment;
    for (std::uint32_t i = 0; i < a_count; ++i) {
        for (const std::size_t k : kInterfaceSegments) {
            const Segment2& s = g.layout().segments[k];
            const Vec2 e = s.b - s.a;
            const Real t = dot(set.sites[i] - s.a, e) / dot(e, e);
            if (t <= 0 || t >= 1 || distance_to_segment(set.sites[i], s) >= Generator::layer) continue;
            const Vec2 foot = s.a + t * e;
            const Vec2 m = foot + (foot - set.sites[i]);
            if (!g.inside(kRegionB, m) || !g.far_from(mirrors, m)) {
                ++set.dropped_mirrors;
                continue;
            }
            mirrors.push_back(m);
            mirror_of.push_back(i);
            mirror_segment.push_back(k);
        }
    }
    for (std::size_t k = 0; k < mirrors.size(); ++k) {
        set.mirror_pairs.emplace(std::pair{mirror_of[k], static_cast<std::uint32_t>(set.sites.size())}, mirror_segment[k]);
        set.sites.push_back(mirrors[k]);
        set.region.push_back(RegionId{kRegionB});
    }
    const std::size_t remaining = kSitesB > mirrors.size() ? kSitesB - mirrors.size() : 0;
    g.throw_darts(set, kRegionB, remaining, [&](const Vec2& x) { return g.interface_distance(x) >= Generator::layer; });
    return set;
}

// ----------------------------------------------------------------------------
// Runs and canonical topology.
// ----------------------------------------------------------------------------
struct InterfaceStats {
    std::size_t faces = 0;
    std::size_t mirror_faces = 0;
    Real length = 0;
    Real mirror_length = 0;
    Real mean_nonortho = 0;
    Real max_nonortho_mirror = 0;
};

struct Run {
    Build2D build;
    InvariantReport report;
    InvariantReference ref;
    app::Signature sig;
    InterfaceStats iface;
};

Run run(const SiteSet& set, Real scale, int order) {
    const Layout2D layout = make_layout(scale);
    const auto perm = app::permutation(set.sites.size(), order);
    std::vector<Vec2> sites;
    std::vector<RegionId> region;
    for (const std::uint32_t k : perm) {
        sites.push_back(Vec2{scale * set.sites[k][0], scale * set.sites[k][1]});
        region.push_back(set.region[k]);
    }
    Run r;
    r.ref = make_reference(layout, scale);
    const Backend2D backend{&delaunay_pairs_2d, &clip_by_region_2d};
    r.build = build_multiregion_mesh_2d(layout, sites, region, backend, 1e-12 * r.ref.length_scale);
    r.ref.cell_measure = r.build.cell_polygon_area;
    r.report = check_invariants(r.build.mesh, r.ref);
    r.sig = app::signature(r.build.mesh);

    const auto& m = r.build.mesh;
    for (std::size_t f = 0; f < m.face_count(); ++f) {
        if (!m.neighbour[f].valid()) continue;
        const std::uint32_t o = perm[m.owner[f].index()];
        const std::uint32_t n = perm[m.neighbour[f].index()];
        if (set.region[o] == set.region[n]) continue;
        const Real theta = angle_between(face_geometry(m, f).area_vector,
                                         m.sites[m.neighbour[f].index()] - m.sites[m.owner[f].index()]);
        const Real length = norm(face_geometry(m, f).area_vector);
        ++r.iface.faces;
        r.iface.length += length;
        r.iface.mean_nonortho += theta;
        // A mirror face joins a mirror pair on the segment used for the reflection.
        const auto pair = set.mirror_pairs.find({std::min(o, n), std::max(o, n)});
        if (pair != set.mirror_pairs.end() &&
            distance_to_segment(face_geometry(m, f).centroid, layout.segments[pair->second]) <= 1e-12 * r.ref.length_scale) {
            ++r.iface.mirror_faces;
            r.iface.mirror_length += length;
            r.iface.max_nonortho_mirror = std::max(r.iface.max_nonortho_mirror, theta);
        }
    }
    if (r.iface.faces > 0) r.iface.mean_nonortho /= static_cast<Real>(r.iface.faces);
    if (std::getenv("P05A_VERBOSE") != nullptr && scale == 1 && order == 0) {
        for (std::size_t f = 0; f < m.face_count(); ++f) {
            if (!m.neighbour[f].valid() || m.cell_region[m.owner[f].index()] == m.cell_region[m.neighbour[f].index()]) continue;
            const auto pts = m.face_points.row(f);
            const Vec2 so = m.sites[m.owner[f].index()];
            const Vec2 sn = m.sites[m.neighbour[f].index()];
            const std::uint32_t o = perm[m.owner[f].index()];
            const std::uint32_t n = perm[m.neighbour[f].index()];
            std::println("    iface f{:<3} ({:.17g},{:.17g})-({:.17g},{:.17g}) len {:.3e} | owner {} ({:.4f},{:.4f}) nb {} ({:.4f},{:.4f}) "
                         "| mirror {} | angle {:.3e}",
                         f, pts[0][0], pts[0][1], pts[1][0], pts[1][1], norm(pts[1] - pts[0]), o, so[0], so[1], n, sn[0],
                         sn[1], set.mirror_pairs.contains({std::min(o, n), std::max(o, n)}),
                         angle_between(face_geometry(m, f).area_vector, sn - so));
        }
    }
    return r;
}

void expect_build(app::Checks& checks, const Build2DStats& s, std::string_view label) {
    std::println("    build: fragmented cells {} | unlabelled edges {} | ambiguous edges {} | unresolved vertices {}",
                 s.fragmented_cells, s.unlabelled_edges, s.ambiguous_edges, s.unresolved_vertices);
    std::println("    build: unmatched bisector pieces {} | interface pieces {} | bad interface coverage {} | "
                 "merged breakpoints {} (max distance {:.3e})",
                 s.unmatched_internal_pieces, s.interface_pieces, s.interface_bad_coverage, s.merged_breakpoints,
                 s.max_merge_distance);
    checks.expect(s.unlabelled_edges == 0 && s.unresolved_vertices == 0,
                  std::format("{}: every edge and vertex resolved from labels", label));
    checks.expect(s.unmatched_internal_pieces == 0, std::format("{}: every internal face seen from both cells", label));
    checks.expect(s.interface_bad_coverage == 0,
                  std::format("{}: interface conforming (each sub-interval: one cell of each region)", label));
    checks.expect(s.fragmented_cells == 0, std::format("{}: every clipped cell is connected", label));
}

struct Summary {
    std::string name;
    Run base;
    bool invariant_topology = true;
};

Summary run_strategy(const std::string& name, const SiteSet& set, app::Checks& checks) {
    constexpr Real deg = 180.0 / std::numbers::pi;
    std::println("\n=== {} === sites {} (A {}, B {}), mirror pairs {}, dropped mirrors {}", name, set.sites.size(),
                 std::ranges::count(set.region, RegionId{kRegionA}), std::ranges::count(set.region, RegionId{kRegionB}),
                 set.mirror_pairs.size(), set.dropped_mirrors);
    Summary summary{name, run(set, 1, 0)};
    const Run& base = summary.base;
    app::expect_invariants(checks, base.report, base.ref, name + " scale 1, order 0");
    app::expect_iteration(checks, base.build.mesh, base.report, name + " scale 1, order 0");
    expect_build(checks, base.build.stats, name + " scale 1, order 0");
    std::println("    interface faces {} | mirror faces (pair on its reflection segment) {} covering {:.1f}% of the "
                 "interface | mean non-orthogonality {:.2f} deg | max on mirror faces {:.3e} rad",
                 base.iface.faces, base.iface.mirror_faces, 100 * base.iface.mirror_length / base.iface.length,
                 base.iface.mean_nonortho * deg, base.iface.max_nonortho_mirror);

    const std::pair<Real, int> variants[] = {{1e-3, 0}, {1e6, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}};
    for (const auto& [scale, order] : variants) {
        const Run r = run(set, scale, order);
        const std::string label = std::format("{} scale {:g}, order {}", name, scale, order);
        app::expect_invariants(checks, r.report, r.ref, label);
        expect_build(checks, r.build.stats, label);
        const bool same = r.sig == base.sig;
        summary.invariant_topology = summary.invariant_topology && same;
        checks.expect(same, std::format("{}: canonical topology equals scale 1, order 0 ({} faces)", label, r.sig.size()));
    }
    return summary;
}

}  // namespace

int main() {
    std::println("P05a.2 - 2D multi-region proof: square 1x1, central hole 0.2x0.2, L-shaped interface");
    app::print_backend();
    app::Checks checks;
    const Summary e1 = run_strategy("E1", make_e1(), checks);
    const Summary e2 = run_strategy("E2", make_e2(), checks);

    constexpr Real deg = 180.0 / std::numbers::pi;
    std::println("\n=== E1 x E2 (scale 1, order 0) ===");
    std::println("{:<4} {:>6} {:>10} {:>10} {:>10} {:>12} {:>14} {:>14} {:>10}", "", "cells", "if.faces", "mirror",
                 "mirror len", "merged bp", "max nonortho", "mean nonortho", "topology");
    for (const Summary* s : {&e1, &e2}) {
        const Run& b = s->base;
        std::println("{:<4} {:>6} {:>10} {:>10} {:>9.1f}% {:>12} {:>10.2f} deg {:>10.2f} deg {:>10}", s->name,
                     b.report.cells, b.iface.faces, b.iface.mirror_faces, 100 * b.iface.mirror_length / b.iface.length,
                     b.build.stats.merged_breakpoints,
                     b.report.max_nonortho_interface * deg, b.iface.mean_nonortho * deg,
                     s->invariant_topology ? "stable" : "CHANGED");
    }
    std::println("P05a.2 result: {} ({} failed checks)", checks.failures() == 0 ? "PASS" : "FAIL", checks.failures());
    return checks.exit_code();
}
