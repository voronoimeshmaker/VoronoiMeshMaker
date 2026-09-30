// ============================================================================
// File: p18a.cpp
// Description: P18a - throwaway proof of concept of conforming 3D interfaces
//              (P15 §12). Hypothesis: as in 2D (DEC-028), every region gets
//              its own Voronoi cells clipped by its closed surface; on every
//              interface triangle the pieces of the cells of both sides are
//              convex and cover the triangle, and the interface face between
//              two cells is the intersection of their pieces (common
//              refinement). The regions are built with vmm::build_mesh_3d,
//              each interface triangle being its own patch; the merged mesh
//              is checked with vmm::check_invariants (interface areas too).
//   p18a [all | quick | <case>]
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <numbers>
#include <print>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/cgal.hpp>
#include <vmm/domain/declaration3d.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/sites/sources3d.hpp>
#include <vmm/voronoi/builder3d.hpp>

namespace {

using vmm::Real;
using vmm::Vec3;
using Clock = std::chrono::steady_clock;

int failures = 0;

void expect(bool ok, const std::string& what) {
    if (!ok) ++failures;
    std::println("    [{}] {}", ok ? "ok" : "FAIL", what);
}

// ----------------------------------------------------------------------------
// Layered box: regions between heightfield interfaces on one n x n grid
// ----------------------------------------------------------------------------

using Height = std::function<Real(Real, Real)>;

struct Layered {
    std::vector<Vec3> points;
    /// Per region: triangles (outward) with a patch name; interface triangles
    /// are named "if:<global id>".
    std::vector<std::vector<std::pair<vmm::Triangle, std::string>>> regions;
    std::vector<Real> interface_area;           ///< per global interface triangle
    std::vector<std::size_t> interface_between;  ///< lower region of each interface triangle
};

/// Regions r = 0..k between z = 0, the heightfields levels[0..k-1] and z = scale.
Layered layered_box(int n, const std::vector<Height>& levels, Real scale) {
    Layered L;
    std::map<std::array<long long, 3>, std::uint32_t> id;
    const auto point = [&](Real x, Real y, Real z) {
        const std::array<long long, 3> key{std::llround(x * 1e9), std::llround(y * 1e9), std::llround(z * 1e9)};
        const auto [it, added] = id.emplace(key, static_cast<std::uint32_t>(L.points.size()));
        if (added) L.points.push_back({scale * x, scale * y, scale * z});
        return it->second;
    };
    std::vector<Height> h{[](Real, Real) { return 0.0; }};
    h.insert(h.end(), levels.begin(), levels.end());
    h.push_back([](Real, Real) { return 1.0; });
    const std::size_t k = h.size() - 1;
    L.regions.resize(k);
    std::map<std::array<std::uint32_t, 3>, std::size_t> iface;  // sorted triangle -> global id
    const auto add = [&](std::size_t r, std::uint32_t a, std::uint32_t b, std::uint32_t c, const std::string& patch,
                         bool interface) {
        std::string name = patch;
        if (interface) {
            std::array<std::uint32_t, 3> key{a, b, c};
            std::ranges::sort(key);
            auto it = iface.find(key);
            if (it == iface.end()) {
                it = iface.emplace(key, L.interface_area.size()).first;
                const Vec3 cr = vmm::cross(L.points[b] - L.points[a], L.points[c] - L.points[a]);
                L.interface_area.push_back(0.5 * vmm::norm(cr));
                L.interface_between.push_back(r);  // the lower region adds the triangle first
            }
            name = std::format("if:{}", it->second);
        }
        L.regions[r].push_back({{a, b, c}, name});
    };
    const Real m = n;
    for (std::size_t r = 0; r < k; ++r) {
        const auto& lo = h[r];
        const auto& hi = h[r + 1];
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                const Real x0 = i / m, x1 = (i + 1) / m, y0 = j / m, y1 = (j + 1) / m;
                // Upper face (+z) and lower face (-z) of the region.
                const auto a = point(x0, y0, hi(x0, y0)), b = point(x1, y0, hi(x1, y0)), c = point(x1, y1, hi(x1, y1)),
                           d = point(x0, y1, hi(x0, y1));
                add(r, a, b, c, "top", r + 1 < k);
                add(r, a, c, d, "top", r + 1 < k);
                const auto e = point(x0, y0, lo(x0, y0)), f = point(x1, y0, lo(x1, y0)), g = point(x1, y1, lo(x1, y1)),
                           q = point(x0, y1, lo(x0, y1));
                add(r, e, g, f, "bottom", r > 0);  // (e, g, f) and (e, q, g): -z
                add(r, e, q, g, "bottom", r > 0);
            }
            const Real a = i / m, b = (i + 1) / m;
            const auto side = [&](Vec3 p0, Vec3 p1) {  // vertical strip over the segment p0 -> p1 (outward on the right)
                const auto s0 = point(p0[0], p0[1], lo(p0[0], p0[1])), s1 = point(p1[0], p1[1], lo(p1[0], p1[1]));
                const auto t1 = point(p1[0], p1[1], hi(p1[0], p1[1])), t0 = point(p0[0], p0[1], hi(p0[0], p0[1]));
                add(r, s0, s1, t1, "side", false);
                add(r, s0, t1, t0, "side", false);
            };
            side({a, 0, 0}, {b, 0, 0});
            side({b, 1, 0}, {a, 1, 0});
            side({0, b, 0}, {0, a, 0});
            side({1, a, 0}, {1, b, 0});
        }
    }
    return L;
}

// ----------------------------------------------------------------------------
// Convex polygon clipping inside one interface triangle
// ----------------------------------------------------------------------------

using Polygon = std::vector<Vec3>;

Vec3 area_vector(const Polygon& p) { return vmm::face_geometry(std::span<const Vec3>(p)).area_vector; }

/// Part of the convex polygon `a` inside the convex polygon `b` (both in one
/// plane; b counter-clockwise around its own area vector).
Polygon clip_convex(Polygon a, const Polygon& b) {
    const Vec3 nb = area_vector(b);
    for (std::size_t k = 0; k < b.size() && a.size() >= 3; ++k) {
        const Vec3& p = b[k];
        const Vec3& q = b[(k + 1) % b.size()];
        const auto f = [&](const Vec3& x) { return vmm::dot(vmm::cross(q - p, x - p), nb); };
        Polygon out;
        for (std::size_t i = 0; i < a.size(); ++i) {
            const Vec3& u = a[i];
            const Vec3& v = a[(i + 1) % a.size()];
            const Real fu = f(u);
            const Real fv = f(v);
            if (fu >= 0) out.push_back(u);
            if ((fu > 0 && fv < 0) || (fu < 0 && fv > 0)) out.push_back(u + (fu / (fu - fv)) * (v - u));
        }
        a = std::move(out);
    }
    return a.size() >= 3 ? a : Polygon{};
}

// ----------------------------------------------------------------------------
// Build and merge
// ----------------------------------------------------------------------------

struct Stats {
    std::size_t cells = 0;
    std::size_t interface_faces = 0;
    std::size_t slivers = 0;               ///< overlaps below the tiny area, dropped
    std::size_t uncovered_pieces = 0;      ///< a piece that overlaps no piece of the other side
    Real max_coverage_error = 0;           ///< |sum of overlaps - triangle area| / triangle area
    Real max_coverage_error_l2 = 0;        ///< |sum of overlaps - triangle area| / L^2 (DEC-020 measure tolerance)
    Real max_piece_coverage_error = 0;     ///< |sum of one side's pieces - triangle area| / triangle area
    std::size_t fragmented = 0;
    double seconds_regions = 0;
    double seconds_overlay = 0;
};

struct Outcome {
    vmm::Mesh3D mesh;
    vmm::InvariantReport report;
    vmm::InvariantReference reference;
    Stats stats;
    std::string error;
};

Outcome build(const Layered& L, const std::vector<Real>& spacing, std::uint64_t seed,
              const std::vector<std::vector<Vec3>>& explicit_sites = {}) {
    Outcome o;
    Stats& st = o.stats;
    const auto backend = vmm::cgal_backend_3d();
    const std::size_t k = L.regions.size();
    std::vector<vmm::Build3D> builds;
    std::vector<std::vector<std::string>> patch_names;
    const auto t0 = Clock::now();
    for (std::size_t r = 0; r < k; ++r) {
        std::vector<Vec3> pts;
        std::vector<vmm::Triangle> tris;
        std::vector<std::uint32_t> patch;
        std::vector<std::string> names;
        std::unordered_map<std::uint32_t, std::uint32_t> local;
        std::map<std::string, std::uint32_t> name_id;
        for (const auto& [t, name] : L.regions[r]) {
            vmm::Triangle x;
            for (std::size_t c = 0; c < 3; ++c) {
                const auto [it, added] = local.emplace(t[c], static_cast<std::uint32_t>(pts.size()));
                if (added) pts.push_back(L.points[t[c]]);
                x[c] = it->second;
            }
            const auto [nit, nadded] = name_id.emplace(name, static_cast<std::uint32_t>(names.size()));
            if (nadded) names.push_back(name);
            tris.push_back(x);
            patch.push_back(nit->second);
        }
        auto surface = vmm::TriangleSurface::make(std::move(pts), std::move(tris), std::move(patch), names);
        if (!surface) {
            o.error = std::format("region {}: {}", r, surface.error().message());
            return o;
        }
        vmm::Declaration3D d;
        const auto m = *d.media().add("m");
        (void)d.add_region_surface(std::format("r{}", r), m, *surface);
        auto partition = backend.build_partition(d);
        if (!partition) {
            o.error = std::format("region {}: {}", r, partition.error().message());
            return o;
        }
        vmm::SiteSet3D sites;
        if (explicit_sites.empty()) {
            const std::vector<vmm::RegionSites3D> src{
                vmm::sites_for_3d(vmm::RegionId{0}, vmm::UniformRandomSource3D(spacing[r]))};
            auto s = vmm::generate_sites_3d(*partition, src, {seed + r});
            if (!s) {
                o.error = std::format("region {}: {}", r, s.error().message());
                return o;
            }
            sites = std::move(*s);
        } else {
            sites.append(explicit_sites[r], vmm::RegionId{0});
        }
        auto b = vmm::build_mesh_3d(*partition, sites, backend);
        if (!b) {
            o.error = std::format("region {}: {}", r, b.error().message());
            return o;
        }
        st.fragmented += b->stats.fragmented_cells;
        builds.push_back(std::move(*b));
        patch_names.push_back(partition->patches());
    }
    st.seconds_regions = std::chrono::duration<double>(Clock::now() - t0).count();

    // Merge: cells of region r follow those of the previous regions.
    const auto t1 = Clock::now();
    std::vector<std::uint32_t> offset{0};
    for (const auto& b : builds) offset.push_back(offset.back() + static_cast<std::uint32_t>(b.mesh.cell_count()));
    st.cells = offset.back();
    struct Face {
        std::uint32_t owner, neighbour;
        Polygon pts;
    };
    std::vector<Face> internal;
    std::map<std::string, std::vector<Face>> boundary;
    struct Piece {
        std::uint32_t cell;
        Polygon pts;
    };
    std::vector<std::array<std::vector<Piece>, 2>> pieces(L.interface_area.size());
    const auto polygon = [](const vmm::Mesh3D& m, vmm::FaceId f) {
        Polygon p;
        for (const auto v : m.face_vertices(f)) p.push_back(m.point(v));
        return p;
    };
    for (std::size_t r = 0; r < k; ++r) {
        const auto& m = builds[r].mesh;
        for (const auto f : m.internal_faces()) {
            internal.push_back({offset[r] + m.owner(f).value, offset[r] + m.neighbour(f).value, polygon(m, f)});
        }
        for (std::size_t p = 0; p < m.patches().size(); ++p) {
            const auto& range = m.patches()[p];
            for (std::size_t f = range.start; f < range.start + range.count; ++f) {
                const auto fid = vmm::FaceId::from_index(f);
                const std::uint32_t cell = offset[r] + m.owner(fid).value;
                if (range.name.starts_with("if:")) {
                    const std::size_t t = std::stoul(range.name.substr(3));
                    pieces[t][L.interface_between[t] == r ? 0 : 1].push_back({cell, polygon(m, fid)});
                } else {
                    boundary[range.name].push_back({cell, 0, polygon(m, fid)});
                }
            }
        }
    }
    // Common refinement of every interface triangle; overlaps below (1e-12 L)^2 are slivers.
    Vec3 blo = L.points[0], bhi = L.points[0];
    for (const Vec3& p : L.points) {
        for (std::size_t c = 0; c < 3; ++c) blo[c] = std::min(blo[c], p[c]), bhi[c] = std::max(bhi[c], p[c]);
    }
    const Real tiny = std::pow(1e-12 * vmm::norm(bhi - blo), 2);
    for (std::size_t t = 0; t < pieces.size(); ++t) {
        const Real area = L.interface_area[t];
        for (int side = 0; side < 2; ++side) {
            Real sum = 0;
            for (const auto& p : pieces[t][static_cast<std::size_t>(side)]) sum += vmm::norm(area_vector(p.pts));
            st.max_piece_coverage_error = std::max(st.max_piece_coverage_error, std::abs(sum - area) / area);
        }
        Real covered = 0;
        std::vector<char> used_b(pieces[t][1].size(), 0);
        for (const auto& pa : pieces[t][0]) {
            bool used = false;
            for (std::size_t j = 0; j < pieces[t][1].size(); ++j) {
                const auto& pb = pieces[t][1][j];
                const Polygon x = clip_convex(pa.pts, pb.pts);
                if (x.empty()) continue;
                const Real a = vmm::norm(area_vector(x));
                if (a <= tiny) {
                    if (a > 0) ++st.slivers;
                    continue;
                }
                covered += a;
                used = true;
                used_b[j] = 1;
                const std::uint32_t owner = std::min(pa.cell, pb.cell);
                const std::uint32_t neighbour = std::max(pa.cell, pb.cell);
                Polygon oriented = x;  // area vector out of pa's cell
                if (owner != pa.cell) std::ranges::reverse(oriented);
                internal.push_back({owner, neighbour, std::move(oriented)});
                ++st.interface_faces;
            }
            if (!used) ++st.uncovered_pieces;
        }
        st.uncovered_pieces += static_cast<std::size_t>(std::ranges::count(used_b, 0));
        st.max_coverage_error = std::max(st.max_coverage_error, std::abs(covered - area) / area);
        st.max_coverage_error_l2 = std::max(st.max_coverage_error_l2, std::abs(covered - area) / std::pow(vmm::norm(bhi - blo), 2));
    }
    std::ranges::stable_sort(internal, [](const Face& a, const Face& b) {
        return std::tie(a.owner, a.neighbour) < std::tie(b.owner, b.neighbour);
    });
    vmm::MeshData<3> md;
    struct Hash {
        std::size_t operator()(const Vec3& v) const noexcept {
            std::uint64_t h = 1469598103934665603ULL;
            for (const Real x : v.data) h = (h ^ std::bit_cast<std::uint64_t>(x)) * 1099511628211ULL;
            return static_cast<std::size_t>(h);
        }
    };
    std::unordered_map<Vec3, vmm::VertexId, Hash> vid;
    const auto push = [&](const Polygon& p, std::uint32_t owner) {
        std::vector<vmm::VertexId> row;
        for (const Vec3& x : p) {
            auto it = vid.find(x);
            if (it == vid.end()) {
                it = vid.emplace(x, vmm::VertexId::from_index(md.points.size())).first;
                md.points.push_back(x);
            }
            row.push_back(it->second);
        }
        md.face_vertices.push_row(row);
        md.owner.push_back(vmm::CellId{owner});
    };
    for (const auto& f : internal) {
        push(f.pts, f.owner);
        md.neighbour.push_back(vmm::CellId{f.neighbour});
    }
    for (const auto& [name, faces] : boundary) {
        const std::size_t start = md.owner.size();
        auto sorted = faces;
        std::ranges::stable_sort(sorted, [](const Face& a, const Face& b) { return a.owner < b.owner; });
        for (const auto& f : sorted) push(f.pts, f.owner);
        md.patches.push_back({name, start, md.owner.size() - start});
    }
    std::vector<Real> cell_volume;
    for (std::size_t r = 0; r < k; ++r) {
        const auto& m = builds[r].mesh;
        md.sites.insert(md.sites.end(), m.sites().begin(), m.sites().end());
        for (std::size_t c = 0; c < m.cell_count(); ++c) {
            md.cell_region.push_back(vmm::RegionId::from_index(r));
            md.cell_input_site.push_back(vmm::SiteId::from_index(md.cell_input_site.size()));
        }
        cell_volume.insert(cell_volume.end(), builds[r].cell_volume.begin(), builds[r].cell_volume.end());
        md.regions.push_back({std::format("r{}", r), vmm::MediumId::from_index(0)});
    }
    md.media = {"m"};
    auto mesh = vmm::Mesh3D::from_data(std::move(md));
    if (!mesh) {
        o.error = mesh.error().message();
        return o;
    }
    o.mesh = std::move(*mesh);
    st.seconds_overlay = std::chrono::duration<double>(Clock::now() - t1).count();

    // Reference: region volumes, outer boundary area, interface area per pair of regions.
    vmm::InvariantReference& ref = o.reference;
    Vec3 lo = L.points[0], hi = L.points[0];
    for (const Vec3& p : L.points) {
        for (std::size_t c = 0; c < 3; ++c) lo[c] = std::min(lo[c], p[c]), hi[c] = std::max(hi[c], p[c]);
    }
    ref.length_scale = vmm::norm(hi - lo);
    ref.boundary_measure = 0;
    for (std::size_t r = 0; r < k; ++r) {
        Real v = 0;
        for (const auto& [t, name] : L.regions[r]) {
            v += vmm::dot(L.points[t[0]], vmm::cross(L.points[t[1]], L.points[t[2]])) / 6;
            if (!name.starts_with("if:")) {
                ref.boundary_measure += 0.5 * vmm::norm(vmm::cross(L.points[t[1]] - L.points[t[0]], L.points[t[2]] - L.points[t[0]]));
            }
        }
        ref.region_measure.push_back(v);
        ref.total_measure += v;
    }
    for (std::size_t t = 0; t < L.interface_area.size(); ++t) {
        const std::size_t a = L.interface_between[t];
        ref.interface_measure[{a, a + 1}] += L.interface_area[t];
    }
    o.reference.cell_measure = cell_volume;
    o.report = vmm::check_invariants(o.mesh, ref);
    o.reference.cell_measure = {};  // the span points into a local vector
    return o;
}

bool run(const std::string& name, const Outcome& o) {
    if (!o.error.empty()) {
        std::println("  {}: FAILED TO BUILD: {}", name, o.error);
        ++failures;
        return false;
    }
    const auto& r = o.report;
    const auto& s = o.stats;
    std::println("  {}: {} cells, {} faces ({} internal, {} on interfaces)", name, s.cells, o.mesh.face_count(),
                 o.mesh.internal_face_count(), s.interface_faces);
    std::println("    invariants: volume {:.1e} | regions {:.1e} | cells {:.1e} | boundary {:.1e} | interfaces {:.1e} | "
                 "closure {:.1e} (tol {:.1e}) | bad faces {} | nonconforming {} | symmetric {} | non-ortho internal {:.1e} "
                 "interface {:.2f} rad",
                 r.total_relative_error, r.max_region_relative_error, r.max_cell_measure_error, r.boundary_relative_error,
                 r.max_interface_relative_error, r.max_closure, r.closure_tolerance, r.bad_faces, r.nonconforming_faces,
                 r.adjacency_symmetric, r.max_nonortho_internal, r.max_nonortho_interface);
    std::println("    refinement: max coverage error {:.1e} of the triangle, {:.1e} L^2 | max one-side coverage error {:.1e} | slivers dropped {} | "
                 "uncovered pieces {} | fragmented cells {} | time regions {:.2f} s, overlay {:.2f} s",
                 s.max_coverage_error, s.max_coverage_error_l2, s.max_piece_coverage_error, s.slivers, s.uncovered_pieces, s.fragmented,
                 s.seconds_regions, s.seconds_overlay);
    // The report already holds the cell measures; the thresholds of passed() are applied here.
    const bool invariants = r.total_relative_error <= 1e-12 && r.max_region_relative_error <= 1e-12 &&
                            r.max_cell_measure_error <= 1e-12 && r.boundary_relative_error <= 1e-12 &&
                            r.max_interface_relative_error <= 1e-12 && r.max_closure <= r.closure_tolerance &&
                            r.bad_faces == 0 && r.nonpositive_cells == 0 && r.nonconforming_faces == 0 &&
                            r.adjacency_symmetric;
    expect(invariants, name + ": DEC-011 invariants within 1e-12 (interface areas included)");
    expect(s.uncovered_pieces == 0 && s.max_coverage_error_l2 <= 1e-12,
           name + ": every interface triangle covered by the common refinement (1e-12 L^2)");
    return invariants;
}

Real wave(Real x, Real y) { return 0.5 + 0.08 * std::sin(2 * std::numbers::pi * x) * std::cos(2 * std::numbers::pi * y) + 0.05 * x; }

void flat() {
    std::println("C1 flat interface");
    run("two regions, z = 0.5", build(layered_box(8, {[](Real, Real) { return 0.5; }}, 1), {0.08, 0.08}, 1));
}

void wavy() {
    std::println("C2 wavy interface, different spacings");
    run("two regions, fine below, coarse above", build(layered_box(16, {wave}, 1), {0.05, 0.1}, 2));
}

void three() {
    std::println("C3 three regions (two interfaces meeting the outer boundary)");
    const Height low = [](Real x, Real y) { return 0.25 + 0.05 * std::cos(2 * std::numbers::pi * (x + y)); };
    run("three layers", build(layered_box(12, {low, wave}, 1), {0.07, 0.06, 0.09}, 3));
}

void degenerate() {
    std::println("C4 near-degenerate");
    // Cartesian grids on both sides of a flat interface: cell faces coincide with the interface.
    std::vector<std::vector<Vec3>> grid(2);
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            for (int l = 0; l < 3; ++l) {
                grid[0].push_back({(i + 0.5) / 6, (j + 0.5) / 6, (l + 0.5) / 6});
                grid[1].push_back({(i + 0.5) / 6, (j + 0.5) / 6, 0.5 + (l + 0.5) / 6});
            }
        }
    }
    run("mirrored Cartesian grids", build(layered_box(6, {[](Real, Real) { return 0.5; }}, 1), {}, 0, grid));
    // Sites 1e-7 from a wavy interface on both sides.
    std::vector<std::vector<Vec3>> near(2);
    for (int i = 0; i < 12; ++i) {
        for (int j = 0; j < 12; ++j) {
            const Real x = (i + 0.5) / 12, y = (j + 0.5) / 12, z = wave(x, y);
            near[0].push_back({x, y, z - 1e-7});
            near[0].push_back({x, y, 0.5 * z});
            near[1].push_back({x, y, z + 1e-7});
            near[1].push_back({x, y, 0.5 * (z + 1)});
        }
    }
    run("sites 1e-7 from the interface", build(layered_box(16, {wave}, 1), {}, 0, near));
}

void order() {
    std::println("C5 input order and scale");
    const auto L = layered_box(10, {wave}, 1);
    const auto base = build(L, {0.08, 0.08}, 5);
    run("base", base);
    std::vector<std::vector<Vec3>> sites(2);
    for (const auto c : base.mesh.cells()) sites[base.mesh.region(c).index()].push_back(base.mesh.site(c));
    for (auto& s : sites) std::ranges::reverse(s);
    const auto rev = build(L, {}, 0, sites);
    const auto& a = base.mesh.data();
    const auto& b = rev.mesh.data();
    expect(a.points == b.points && a.face_vertices.values == b.face_vertices.values && a.owner == b.owner &&
               a.neighbour == b.neighbour,
           "reversed site order: bit-identical merged mesh");
    for (const Real s : {1e-3, 1e5}) {
        std::vector<std::vector<Vec3>> scaled(2);
        for (std::size_t r = 0; r < 2; ++r) {
            for (const Vec3& p : sites[r]) scaled[r].push_back(s * p);
        }
        const auto o = build(layered_box(10, {wave}, s), {}, 0, scaled);
        run(std::format("scale {:g}", s), o);
        expect(o.mesh.owners().size() == base.mesh.owners().size() &&
                   std::ranges::equal(o.mesh.neighbours(), base.mesh.neighbours()),
               std::format("scale {:g}: same topology", s));
    }
}

void stress() {
    std::println("C6 stress");
    const Height low = [](Real x, Real y) { return 0.25 + 0.05 * std::cos(2 * std::numbers::pi * (x + y)); };
    run("three layers, ~60000 cells", build(layered_box(40, {low, wave}, 1), {0.028, 0.025, 0.03}, 6));
}

}  // namespace

int main(int argc, char** argv) {
    const std::string which = argc > 1 ? argv[1] : "all";
    std::println("P18a - conforming 3D interfaces ({})", vmm::cgal_backend_3d().info().versions);
    const std::vector<std::pair<std::string, void (*)()>> cases{{"flat", flat},       {"wavy", wavy},   {"three", three},
                                                               {"degenerate", degenerate}, {"order", order}, {"stress", stress}};
    for (const auto& [name, fn] : cases) {
        if (which == "all" || which == name || (which == "quick" && name != "stress")) fn();
    }
    std::println("P18a result: {} ({} failed checks)", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}
