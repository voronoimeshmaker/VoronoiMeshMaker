// ============================================================================
// File: voronoi3d.cpp
// Description: P05a prototype - convex 3D Voronoi cells by half-space
//              clipping of the box (no CGAL here).
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <span>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/voronoi3d.hpp>

namespace vmm::p05a {
namespace {

using Label = std::uint64_t;
constexpr Label box_label = Label{1} << 40;

struct Face3 {
    std::vector<Vec3> pts;  // counter-clockwise seen from outside the cell
    Label label;
};

using Polyhedron = std::vector<Face3>;

Polyhedron box(const Vec3& l, const Vec3& h) {
    return {
        {{{l[0], l[1], l[2]}, {l[0], l[1], h[2]}, {l[0], h[1], h[2]}, {l[0], h[1], l[2]}}, box_label + 0},
        {{{h[0], l[1], l[2]}, {h[0], h[1], l[2]}, {h[0], h[1], h[2]}, {h[0], l[1], h[2]}}, box_label + 1},
        {{{l[0], l[1], l[2]}, {h[0], l[1], l[2]}, {h[0], l[1], h[2]}, {l[0], l[1], h[2]}}, box_label + 2},
        {{{l[0], h[1], l[2]}, {l[0], h[1], h[2]}, {h[0], h[1], h[2]}, {h[0], h[1], l[2]}}, box_label + 3},
        {{{l[0], l[1], l[2]}, {l[0], h[1], l[2]}, {h[0], h[1], l[2]}, {h[0], l[1], l[2]}}, box_label + 4},
        {{{l[0], l[1], h[2]}, {h[0], l[1], h[2]}, {h[0], h[1], h[2]}, {l[0], h[1], h[2]}}, box_label + 5},
    };
}

/// Cut point of edge (p, q) with the plane, computed from the ordered pair so
/// that the two faces sharing the edge obtain identical doubles.
Vec3 cut(Vec3 p, Vec3 q, const Vec3& m, const Vec3& n) {
    if (q < p) std::swap(p, q);
    const Real fp = dot(p - m, n);
    const Real fq = dot(q - m, n);
    return p + (fp / (fp - fq)) * (q - p);
}

/// Keeps dot(x - m, n) <= 0. The cap face on the plane gets `label`.
Polyhedron clip(const Polyhedron& poly, const Vec3& m, const Vec3& n, Label label) {
    bool any_out = false;
    for (const auto& f : poly) {
        for (const Vec3& p : f.pts) any_out = any_out || dot(p - m, n) > 0;
    }
    if (!any_out) return poly;

    Polyhedron out;
    std::vector<Vec3> cap;
    for (const auto& f : poly) {
        Face3 g{{}, f.label};
        const std::size_t k_n = f.pts.size();
        for (std::size_t k = 0; k < k_n; ++k) {
            const Vec3& p = f.pts[k];
            const Vec3& q = f.pts[(k + 1) % k_n];
            const Real fp = dot(p - m, n);
            const Real fq = dot(q - m, n);
            if (fp <= 0) {
                g.pts.push_back(p);
                if (fp == 0) cap.push_back(p);
            }
            if ((fp < 0 && fq > 0) || (fp > 0 && fq < 0)) {
                const Vec3 x = cut(p, q, m, n);
                g.pts.push_back(x);
                cap.push_back(x);
            }
        }
        std::vector<Vec3> clean;
        for (std::size_t k = 0; k < g.pts.size(); ++k) {
            if (g.pts[k] != g.pts[(k + g.pts.size() - 1) % g.pts.size()]) clean.push_back(g.pts[k]);
        }
        g.pts = std::move(clean);
        if (g.pts.size() >= 3) out.push_back(std::move(g));
    }
    std::ranges::sort(cap);
    cap.erase(std::unique(cap.begin(), cap.end()), cap.end());
    if (cap.size() >= 3) {
        Vec3 c{};
        for (const Vec3& p : cap) c = c + p;
        c = (1.0 / static_cast<Real>(cap.size())) * c;
        const Vec3 nn = (1.0 / norm(n)) * n;
        const Vec3 helper = std::abs(nn[0]) < 0.9 ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
        Vec3 u = cross(nn, helper);
        u = (1.0 / norm(u)) * u;
        const Vec3 v = cross(nn, u);
        std::ranges::sort(cap, [&](const Vec3& a, const Vec3& b) {
            return std::atan2(dot(a - c, v), dot(a - c, u)) < std::atan2(dot(b - c, v), dot(b - c, u));
        });
        out.push_back({std::move(cap), label});
    }
    return out;
}

Real polyhedron_volume(const Polyhedron& poly, const Vec3& origin) {
    Real v = 0;
    for (const auto& f : poly) {
        const FaceGeometry<3> g = face_geometry(std::span<const Vec3>(f.pts));
        v += dot(g.centroid - origin, g.area_vector) / 3;
    }
    return v;
}

/// Euler characteristic of the polyhedron surface, from exact vertex identity.
long euler(const Polyhedron& poly) {
    std::set<Vec3> vertices;
    std::size_t half_edges = 0;
    for (const auto& f : poly) {
        vertices.insert(f.pts.begin(), f.pts.end());
        half_edges += f.pts.size();
    }
    return static_cast<long>(vertices.size()) - static_cast<long>(half_edges / 2) + static_cast<long>(poly.size());
}

}  // namespace

Build3D build_box_mesh_3d(std::span<const Vec3> sites, const Vec3& lo, const Vec3& hi, const Backend3D& backend,
                          Real tiny_area) {
    Build3D out;
    out.mesh.sites.assign(sites.begin(), sites.end());
    out.mesh.cell_region.assign(sites.size(), RegionId{0});
    out.cell_polyhedron_volume.assign(sites.size(), 0);
    out.stats.min_face_area = std::numeric_limits<Real>::max();

    std::vector<std::vector<std::uint32_t>> nb(sites.size());
    for (const auto& [a, b] : backend.delaunay_pairs(sites)) {
        nb[a.index()].push_back(b.value);
        nb[b.index()].push_back(a.value);
    }

    // Face copies by unordered pair: first from the lower id, second from the higher.
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::pair<const Face3*, const Face3*>> shared;
    std::vector<Polyhedron> cells(sites.size());
    for (std::uint32_t i = 0; i < sites.size(); ++i) {
        std::ranges::sort(nb[i]);
        Polyhedron cell = box(lo, hi);
        for (const std::uint32_t j : nb[i]) {
            const std::uint32_t a = std::min(i, j);
            const std::uint32_t b = std::max(i, j);
            const Vec3 mid = 0.5 * (sites[a] + sites[b]);  // canonical midpoint
            cell = clip(cell, mid, sites[j] - sites[i], j);
        }
        out.cell_polyhedron_volume[i] = polyhedron_volume(cell, sites[i]);
        if (euler(cell) != 2) ++out.stats.euler_failures;
        cells[i] = std::move(cell);
    }
    for (std::uint32_t i = 0; i < sites.size(); ++i) {
        for (const Face3& f : cells[i]) {
            if (f.label >= box_label) {
                out.mesh.add_face(CellId{i}, CellId::invalid(), PatchId{static_cast<std::uint32_t>(f.label - box_label)},
                                  f.pts);
                continue;
            }
            const auto j = static_cast<std::uint32_t>(f.label);
            auto& entry = shared[{std::min(i, j), std::max(i, j)}];
            (i < j ? entry.first : entry.second) = &f;
        }
    }
    for (const auto& [key, copies] : shared) {
        const auto [own, other] = copies;
        const Face3* present = own != nullptr ? own : other;
        const Real area = norm(face_geometry(std::span<const Vec3>(present->pts)).area_vector);
        if (own == nullptr || other == nullptr) {
            ++(area <= tiny_area ? out.stats.dropped_tiny_faces : out.stats.unmatched_faces);
            if (own == nullptr) continue;  // the owner copy is the one stored
            if (area <= tiny_area) continue;
        } else {
            const Vec3 s_own = face_geometry(std::span<const Vec3>(own->pts)).area_vector;
            const Vec3 s_other = face_geometry(std::span<const Vec3>(other->pts)).area_vector;
            out.stats.max_partner_mismatch = std::max(out.stats.max_partner_mismatch, norm(s_own + s_other) / area);
        }
        out.stats.min_face_area = std::min(out.stats.min_face_area, area);
        out.mesh.add_face(CellId{key.first}, CellId{key.second}, PatchId::invalid(), own->pts);
    }
    return out;
}

}  // namespace vmm::p05a
