// ============================================================================
// File: p15a_build.cpp
// Description: P15a - domains, sites and the 3D clipped Voronoi construction.
//              Throwaway code: not part of the library (sequencia_prompts P15a).
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
#include <cstdlib>
#include <cstring>
#include <format>
#include <map>
#include <numeric>
#include <print>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <boost/version.hpp>
#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <CGAL/AABB_traits_3.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/connected_components.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Side_of_triangle_mesh.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Triangulation_data_structure_3.h>
#include <CGAL/Triangulation_vertex_base_with_info_3.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "p15a.hpp"

namespace p15a {
namespace {

namespace PMP = CGAL::Polygon_mesh_processing;
using Epick = CGAL::Exact_predicates_inexact_constructions_kernel;
using Epeck = CGAL::Exact_predicates_exact_constructions_kernel;
using IMesh = CGAL::Surface_mesh<Epick::Point_3>;
using EMesh = CGAL::Surface_mesh<Epeck::Point_3>;
using Primitive = CGAL::AABB_face_graph_triangle_primitive<IMesh>;
using Tree = CGAL::AABB_tree<CGAL::AABB_traits_3<Epick, Primitive>>;
using Clock = std::chrono::steady_clock;

using vmm::cross;
using vmm::dot;
using vmm::norm;

Real seconds_since(Clock::time_point t0) { return std::chrono::duration<Real>(Clock::now() - t0).count(); }

// ----------------------------------------------------------------------------
// Domains
// ----------------------------------------------------------------------------

template <class Mesh, class Point>
Mesh to_mesh(const Domain& d) {
    Mesh m;
    std::vector<typename Mesh::Vertex_index> v;
    for (const Vec3& p : d.points) v.push_back(m.add_vertex(Point(p[0], p[1], p[2])));
    for (const auto& t : d.triangles) m.add_face(v[t[0]], v[t[1]], v[t[2]]);
    return m;
}

Real signed_volume(const Domain& d) {
    Real v = 0;
    for (const auto& t : d.triangles) {
        v += dot(d.points[t[0]], cross(d.points[t[1]], d.points[t[2]])) / 6;
    }
    return v;
}

void orient_outward(Domain& d) {
    if (signed_volume(d) < 0) {
        for (auto& t : d.triangles) std::swap(t[1], t[2]);
    }
}

/// Ear clipping of a simple CCW polygon; returns index triples (CCW).
std::vector<std::array<std::uint32_t, 3>> ear_clip(const std::vector<std::array<Real, 2>>& p) {
    const auto area2 = [&](std::uint32_t a, std::uint32_t b, std::uint32_t c) {
        return (p[b][0] - p[a][0]) * (p[c][1] - p[a][1]) - (p[b][1] - p[a][1]) * (p[c][0] - p[a][0]);
    };
    std::vector<std::uint32_t> ring(p.size());
    std::iota(ring.begin(), ring.end(), 0u);
    std::vector<std::array<std::uint32_t, 3>> out;
    while (ring.size() > 3) {
        bool cut = false;
        for (std::size_t k = 0; k < ring.size() && !cut; ++k) {
            const std::uint32_t a = ring[(k + ring.size() - 1) % ring.size()];
            const std::uint32_t b = ring[k];
            const std::uint32_t c = ring[(k + 1) % ring.size()];
            if (area2(a, b, c) <= 0) continue;
            bool empty = true;
            for (const std::uint32_t q : ring) {
                if (q == a || q == b || q == c) continue;
                if (area2(a, b, q) >= 0 && area2(b, c, q) >= 0 && area2(c, a, q) >= 0) empty = false;
            }
            if (!empty) continue;
            out.push_back({a, b, c});
            ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(k));
            cut = true;
        }
        if (!cut) break;  // not simple: leave the rest (never for the test polygons)
    }
    if (ring.size() == 3) out.push_back({ring[0], ring[1], ring[2]});
    return out;
}

std::uint64_t splitmix(std::uint64_t& s) {
    std::uint64_t z = (s += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

Real uniform(std::uint64_t& s) { return static_cast<Real>(splitmix(s) >> 11) * 0x1.0p-53; }

// ----------------------------------------------------------------------------
// Convex cells with plane labels on every vertex
// ----------------------------------------------------------------------------

using Label = std::int64_t;  // >= 0: neighbour site (canonical); -1..-6: enclosing box; <= -10: domain triangle
constexpr Label kUnknown = -1000;

Label domain_label(std::size_t t) { return -10 - static_cast<Label>(t); }
bool is_domain(Label l) { return l <= -10 && l != kUnknown; }
std::size_t domain_triangle(Label l) { return static_cast<std::size_t>(-10 - l); }

struct PV {
    Vec3 p;
    std::array<Label, 3> planes;
};

struct PF {
    Label label;
    std::vector<std::uint32_t> v;  // counter-clockwise seen from outside
};

struct Poly {
    std::vector<PV> verts;
    std::vector<PF> faces;
};

Poly box_poly(const Vec3& l, const Vec3& h) {
    Poly p;
    for (int k = 0; k < 8; ++k) {
        const Vec3 x{(k & 1) ? h[0] : l[0], (k & 2) ? h[1] : l[1], (k & 4) ? h[2] : l[2]};
        p.verts.push_back({x, {(k & 1) ? -2 : -1, (k & 2) ? -4 : -3, (k & 4) ? -6 : -5}});
    }
    // Corner k = x + 2y + 4z; quads counter-clockwise seen from outside.
    p.faces = {{-1, {0, 4, 6, 2}}, {-2, {1, 3, 7, 5}}, {-3, {0, 1, 5, 4}},
               {-4, {2, 6, 7, 3}}, {-5, {0, 2, 3, 1}}, {-6, {4, 5, 7, 6}}};
    return p;
}

Label other_common_plane(const PV& a, const PV& b, Label face) {
    for (const Label x : a.planes) {
        if (x != face && std::ranges::find(b.planes, x) != b.planes.end()) return x;
    }
    return kUnknown;
}

/// Keeps dot(x - m, n) <= 0; the cap face gets `label`.
void clip(Poly& poly, const Vec3& m, const Vec3& n, Label label) {
    std::vector<Real> s(poly.verts.size());
    bool any_out = false;
    for (const PF& f : poly.faces) {
        for (const std::uint32_t v : f.v) {
            s[v] = dot(poly.verts[v].p - m, n);
            any_out = any_out || s[v] > 0;
        }
    }
    if (!any_out) return;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> made;
    std::vector<std::uint32_t> cap;
    std::vector<PF> out;
    for (const PF& f : poly.faces) {
        PF g{f.label, {}};
        const std::size_t k_n = f.v.size();
        for (std::size_t k = 0; k < k_n; ++k) {
            const std::uint32_t a = f.v[k];
            const std::uint32_t b = f.v[(k + 1) % k_n];
            if (s[a] <= 0) {
                g.v.push_back(a);
                if (s[a] == 0) cap.push_back(a);
            }
            if ((s[a] < 0 && s[b] > 0) || (s[a] > 0 && s[b] < 0)) {
                const auto key = std::minmax(a, b);
                auto it = made.find(key);
                if (it == made.end()) {
                    const Vec3& p = poly.verts[key.first].p;
                    const Vec3& q = poly.verts[key.second].p;
                    const Real fp = dot(p - m, n);
                    const Real fq = dot(q - m, n);
                    const Vec3 x = p + (fp / (fp - fq)) * (q - p);
                    const Label o = other_common_plane(poly.verts[a], poly.verts[b], f.label);
                    poly.verts.push_back({x, {f.label, o, label}});
                    s.push_back(0);
                    it = made.emplace(key, static_cast<std::uint32_t>(poly.verts.size() - 1)).first;
                }
                g.v.push_back(it->second);
                cap.push_back(it->second);
            }
        }
        if (g.v.size() >= 3) out.push_back(std::move(g));
    }
    std::ranges::sort(cap);
    cap.erase(std::unique(cap.begin(), cap.end()), cap.end());
    if (cap.size() >= 3) {
        Vec3 c{};
        for (const std::uint32_t v : cap) c = c + poly.verts[v].p;
        c = (1.0 / static_cast<Real>(cap.size())) * c;
        const Vec3 nn = (1.0 / norm(n)) * n;
        const Vec3 helper = std::abs(nn[0]) < 0.9 ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
        Vec3 u = cross(nn, helper);
        u = (1.0 / norm(u)) * u;
        const Vec3 w = cross(nn, u);
        const auto angle = [&](std::uint32_t v) {
            const Vec3 d = poly.verts[v].p - c;
            return std::atan2(dot(d, w), dot(d, u));
        };
        std::ranges::sort(cap, [&](std::uint32_t a, std::uint32_t b) { return angle(a) < angle(b); });
        out.push_back({label, std::move(cap)});
    }
    poly.faces = std::move(out);
}

// ----------------------------------------------------------------------------
// Canonical vertices: a vertex on three bisectors of cell i is the
// circumcentre of the four sites, computed exactly once and rounded.
// ----------------------------------------------------------------------------

struct KeyHash {
    std::size_t operator()(const std::array<std::uint32_t, 4>& k) const noexcept {
        std::uint64_t h = 1469598103934665603ULL;
        for (const std::uint32_t x : k) h = (h ^ x) * 1099511628211ULL;
        return static_cast<std::size_t>(h);
    }
};

struct Canonical {
    const std::vector<Vec3>& sites;
    std::unordered_map<std::array<std::uint32_t, 4>, std::pair<bool, Vec3>, KeyHash> cache;

    /// false when the four sites are coplanar (no circumcentre).
    std::pair<bool, Vec3> vertex(std::array<std::uint32_t, 4> k) {
        std::ranges::sort(k);
        auto it = cache.find(k);
        if (it != cache.end()) return it->second;
        std::array<Epeck::Point_3, 4> p;
        for (int a = 0; a < 4; ++a) {
            const Vec3& x = sites[k[static_cast<std::size_t>(a)]];
            p[static_cast<std::size_t>(a)] = Epeck::Point_3(x[0], x[1], x[2]);
        }
        std::pair<bool, Vec3> r{false, {}};
        if (!CGAL::coplanar(p[0], p[1], p[2], p[3])) {
            const Epeck::Point_3 c = CGAL::circumcenter(p[0], p[1], p[2], p[3]);
            r = {true, Vec3{CGAL::to_double(CGAL::exact(c.x())), CGAL::to_double(CGAL::exact(c.y())),
                            CGAL::to_double(CGAL::exact(c.z()))}};
        }
        return cache.emplace(k, r).first->second;
    }
};

struct Piece {
    Label label;
    std::vector<Vec3> pts;  // counter-clockwise seen from outside the cell
};

/// Faces of a convex cell after snapping, with consecutive duplicates removed.
std::vector<Piece> pieces_of(const Poly& poly) {
    std::vector<Piece> out;
    for (const PF& f : poly.faces) {
        Piece q{f.label, {}};
        for (const std::uint32_t v : f.v) {
            const Vec3& p = poly.verts[v].p;
            if (q.pts.empty() || q.pts.back() != p) q.pts.push_back(p);
        }
        while (q.pts.size() > 1 && q.pts.front() == q.pts.back()) q.pts.pop_back();
        if (q.pts.size() >= 3) out.push_back(std::move(q));
    }
    return out;
}

/// Vertex merge within `tol` over every cell (DEC-020, as in 2D): the first
/// vertex seen (canonical cell order) represents the others; faces left with
/// fewer than 3 distinct vertices or thinner than `tol` are dropped.
struct VertexMerger {
    Real tol;
    std::unordered_map<std::uint64_t, std::vector<Vec3>> grid;
    std::size_t merged = 0;

    static std::int64_t q(Real x, Real h) { return static_cast<std::int64_t>(std::floor(x / h)); }
    static std::uint64_t key(std::int64_t a, std::int64_t b, std::int64_t c) {
        const auto u = [](std::int64_t v) { return static_cast<std::uint64_t>(v) & 0x1fffffULL; };
        return (u(a) << 42) | (u(b) << 21) | u(c);
    }
    Vec3 operator()(const Vec3& p) {
        const Real h = 4 * tol;
        const std::int64_t a = q(p[0], h), b = q(p[1], h), c = q(p[2], h);
        for (std::int64_t da = -1; da <= 1; ++da) {
            for (std::int64_t db = -1; db <= 1; ++db) {
                for (std::int64_t dc = -1; dc <= 1; ++dc) {
                    const auto it = grid.find(key(a + da, b + db, c + dc));
                    if (it == grid.end()) continue;
                    for (const Vec3& r : it->second) {
                        if (norm(r - p) <= tol) {
                            if (r != p) ++merged;
                            return r;
                        }
                    }
                }
            }
        }
        grid[key(a, b, c)].push_back(p);
        return p;
    }
};

void merge_cell(std::vector<Piece>& cell, VertexMerger& merge) {
    std::vector<Piece> kept;
    for (Piece& f : cell) {
        Piece g{f.label, {}};
        for (const Vec3& p : f.pts) {
            const Vec3 r = merge(p);
            if (g.pts.empty() || g.pts.back() != r) g.pts.push_back(r);
        }
        while (g.pts.size() > 1 && g.pts.front() == g.pts.back()) g.pts.pop_back();
        if (g.pts.size() < 3) continue;
        // Sliver: width 2 A / (longest edge) below the merge tolerance.
        Real longest = 0;
        for (std::size_t k = 0; k < g.pts.size(); ++k) longest = std::max(longest, norm(g.pts[(k + 1) % g.pts.size()] - g.pts[k]));
        const Real area = norm(vmm::face_geometry(std::span<const Vec3>(g.pts)).area_vector);
        if (2 * area <= merge.tol * longest) continue;
        kept.push_back(std::move(g));
    }
    cell = std::move(kept);
}

/// Inserts into every face edge the cell vertices lying on it (within tol), so
/// that faces sharing a split edge conform (no T-junction). Returns the count.
std::size_t insert_t_vertices(std::vector<Piece>& cell, Real tol) {
    std::vector<Vec3> all;
    for (const Piece& f : cell) all.insert(all.end(), f.pts.begin(), f.pts.end());
    std::ranges::sort(all);
    all.erase(std::unique(all.begin(), all.end()), all.end());
    std::size_t inserted = 0;
    for (Piece& f : cell) {
        std::vector<Vec3> loop;
        for (std::size_t k = 0; k < f.pts.size(); ++k) {
            const Vec3& a = f.pts[k];
            const Vec3& b = f.pts[(k + 1) % f.pts.size()];
            loop.push_back(a);
            const Vec3 d = b - a;
            const Real len = norm(d);
            std::vector<std::pair<Real, Vec3>> on;
            for (const Vec3& v : all) {
                if (v == a || v == b) continue;
                const Real t = dot(v - a, d) / (len * len);
                if (t * len <= tol || (1 - t) * len <= tol) continue;
                if (norm(v - (a + t * d)) <= tol) on.emplace_back(t, v);
            }
            std::ranges::sort(on);
            for (const auto& [t, v] : on) loop.push_back(v);
            inserted += on.size();
        }
        f.pts = std::move(loop);
    }
    return inserted;
}

long euler(const std::vector<Piece>& cell) {
    std::map<Vec3, int> vertices;
    std::size_t half_edges = 0;
    for (const Piece& f : cell) {
        for (const Vec3& p : f.pts) vertices[p] = 0;
        half_edges += f.pts.size();
    }
    return static_cast<long>(vertices.size()) - static_cast<long>(half_edges / 2) + static_cast<long>(cell.size());
}

/// Volume by tetrahedra from the site (independent of the invariant checker's
/// divergence formula on face centroids).
Real tetra_volume(const std::vector<Piece>& cell, const Vec3& o) {
    Real v = 0;
    for (const Piece& f : cell) {
        for (std::size_t k = 1; k + 1 < f.pts.size(); ++k) {
            v += dot(f.pts[0] - o, cross(f.pts[k] - o, f.pts[k + 1] - o)) / 6;
        }
    }
    return v;
}

// ----------------------------------------------------------------------------
// Exact clipping of a boundary cell and recovery of its labelled faces
// ----------------------------------------------------------------------------

Vec3 to_vec(const Epeck::Point_3& p) {
    return {CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z())};
}

struct LabelledTriangle {
    std::array<Vec3, 3> p;
    Label label;
};

/// Distance of x to the plane of t below tol and x inside t (with tol).
bool on_triangle(const std::array<Vec3, 3>& t, const Vec3& x, Real tol) {
    const Vec3 n = cross(t[1] - t[0], t[2] - t[0]);
    const Real nn = norm(n);
    if (nn == 0) return false;
    if (std::abs(dot(x - t[0], n)) / nn > tol) return false;
    for (int k = 0; k < 3; ++k) {
        const Vec3& a = t[static_cast<std::size_t>(k)];
        const Vec3& b = t[static_cast<std::size_t>((k + 1) % 3)];
        if (dot(cross(b - a, x - a), n) / nn < -tol * norm(b - a)) return false;
    }
    return true;
}

struct ClipResult {
    bool ok = false;
    bool invalid_input = false;  ///< the cell mesh is open or self-intersecting
    std::vector<Piece> pieces;
    Real volume = 0;
    std::size_t components = 0;
    std::size_t unlabelled = 0;
    std::size_t ambiguous = 0;
    std::size_t holes = 0;
};

ClipResult clip_exact(const std::vector<Piece>& cell, const EMesh& domain_e, const Domain& domain, const Tree& tree,
                      Real tol) {
    ClipResult r;
    EMesh cm;
    std::map<Vec3, EMesh::Vertex_index> vid;
    std::vector<LabelledTriangle> cell_triangles;
    const auto vertex = [&](const Vec3& p) {
        auto it = vid.find(p);
        if (it == vid.end()) it = vid.emplace(p, cm.add_vertex(Epeck::Point_3(p[0], p[1], p[2]))).first;
        return it->second;
    };
    // Fan from the face centroid: collinear vertices on a face edge (several
    // planes through one vertex) never give a zero-area triangle.
    for (const Piece& f : cell) {
        if (norm(vmm::face_geometry(std::span<const Vec3>(f.pts)).area_vector) == 0) continue;
        Vec3 c{};
        for (const Vec3& p : f.pts) c = c + p;
        c = (1.0 / static_cast<Real>(f.pts.size())) * c;
        for (std::size_t k = 0; k < f.pts.size(); ++k) {
            const Vec3& p = f.pts[k];
            const Vec3& q = f.pts[(k + 1) % f.pts.size()];
            const auto face = cm.add_face(vertex(c), vertex(p), vertex(q));
            if (face == EMesh::null_face()) {
                r.invalid_input = true;
                if (std::getenv("P15A_DEBUG") != nullptr) {
                    std::println(stderr, "non-manifold cell mesh, faces {}", cell.size());
                    for (const Piece& g : cell) {
                        std::print(stderr, "  label {}:", g.label);
                        for (const Vec3& x : g.pts) std::print(stderr, " ({:.17g},{:.17g},{:.17g})", x[0], x[1], x[2]);
                        std::println(stderr, "");
                    }
                }
                return r;
            }
            cell_triangles.push_back({{c, p, q}, f.label});
        }
    }
    // Release builds have no CGAL preconditions: an invalid input would crash the corefinement.
    if (!CGAL::is_closed(cm) || PMP::does_self_intersect(cm)) {
        r.invalid_input = true;
        if (std::getenv("P15A_DEBUG") != nullptr) {
            std::println(stderr, "invalid cell mesh: closed {} self-intersecting {} faces {}", CGAL::is_closed(cm),
                         PMP::does_self_intersect(cm), cell.size());
            for (const Piece& f : cell) {
                std::print(stderr, "  label {} area {:.3e}:", f.label, norm(vmm::face_geometry(std::span<const Vec3>(f.pts)).area_vector));
                for (const Vec3& p : f.pts) std::print(stderr, " ({:.17g},{:.17g},{:.17g})", p[0], p[1], p[2]);
                std::println(stderr, "");
            }
        }
        return r;
    }
    EMesh out;
    try {
        if (std::getenv("P15A_SHARED_DOMAIN") == nullptr) {
            EMesh dm = domain_e;  // a fresh copy per cell (fastest, see P15 report)
            if (!PMP::corefine_and_compute_intersection(cm, dm, out)) return r;
        } else {
            // Shared domain left untouched (do_not_modify): measured 22-142x slower in P15.
            auto& shared = const_cast<EMesh&>(domain_e);
            if (!PMP::corefine_and_compute_intersection(cm, shared, out, CGAL::parameters::default_values(),
                                                        CGAL::parameters::do_not_modify(true))) {
                return r;
            }
        }
    } catch (...) {
        return r;
    }
    r.ok = true;
    r.volume = CGAL::to_double(PMP::volume(out));
    auto fcc = out.add_property_map<EMesh::Face_index, std::size_t>("f:cc", 0).first;
    r.components = PMP::connected_components(out, fcc);

    // Label every output triangle by the input triangle that supports it.
    auto flabel = out.add_property_map<EMesh::Face_index, Label>("f:label", kUnknown).first;
    for (const auto f : out.faces()) {
        std::array<Vec3, 3> t;
        int k = 0;
        for (const auto v : CGAL::vertices_around_face(out.halfedge(f), out)) t[static_cast<std::size_t>(k++)] = to_vec(out.point(v));
        const Vec3 c = (1.0 / 3.0) * (t[0] + t[1] + t[2]);
        Label from_domain = kUnknown;
        Label from_cell = kUnknown;
        const auto on_all = [&](const std::array<Vec3, 3>& s) {
            return on_triangle(s, t[0], tol) && on_triangle(s, t[1], tol) && on_triangle(s, t[2], tol) &&
                   on_triangle(s, c, tol);
        };
        CGAL::Bbox_3 box(std::min({t[0][0], t[1][0], t[2][0]}) - tol, std::min({t[0][1], t[1][1], t[2][1]}) - tol,
                         std::min({t[0][2], t[1][2], t[2][2]}) - tol, std::max({t[0][0], t[1][0], t[2][0]}) + tol,
                         std::max({t[0][1], t[1][1], t[2][1]}) + tol, std::max({t[0][2], t[1][2], t[2][2]}) + tol);
        std::vector<Primitive::Id> hits;
        tree.all_intersected_primitives(box, std::back_inserter(hits));
        for (const auto h : hits) {
            const auto& tri = domain.triangles[static_cast<std::size_t>(h)];
            if (on_all({domain.points[tri[0]], domain.points[tri[1]], domain.points[tri[2]]})) {
                from_domain = domain_label(static_cast<std::size_t>(h));
                break;
            }
        }
        for (const auto& ct : cell_triangles) {
            if (on_all(ct.p)) {
                from_cell = ct.label;
                break;
            }
        }
        if (from_domain != kUnknown && from_cell != kUnknown) ++r.ambiguous;
        if (from_domain == kUnknown && from_cell == kUnknown) ++r.unlabelled;
        flabel[f] = from_domain != kUnknown ? from_domain : from_cell;
    }

    // Faces: connected sets of triangles with one label; boundary loops.
    std::vector<std::size_t> parent(out.number_of_faces());
    std::iota(parent.begin(), parent.end(), 0u);
    const auto find = [&](std::size_t x) {
        while (parent[x] != x) x = parent[x] = parent[parent[x]];
        return x;
    };
    for (const auto h : out.halfedges()) {
        const auto o = out.opposite(h);
        if (out.is_border(h) || out.is_border(o)) continue;
        if (flabel[out.face(h)] == flabel[out.face(o)]) {
            parent[find(static_cast<std::size_t>(out.face(h)))] = find(static_cast<std::size_t>(out.face(o)));
        }
    }
    std::map<std::size_t, std::vector<EMesh::Halfedge_index>> boundary;  // group -> boundary halfedges
    for (const auto h : out.halfedges()) {
        if (out.is_border(h)) continue;
        const auto o = out.opposite(h);
        const std::size_t g = find(static_cast<std::size_t>(out.face(h)));
        if (out.is_border(o) || find(static_cast<std::size_t>(out.face(o))) != g) boundary[g].push_back(h);
    }
    for (auto& [g, hs] : boundary) {
        std::multimap<EMesh::Vertex_index, EMesh::Halfedge_index> from;
        for (const auto h : hs) from.emplace(out.source(h), h);
        std::vector<std::vector<Vec3>> loops;
        while (!from.empty()) {
            std::vector<Vec3> loop;
            auto it = from.begin();
            EMesh::Halfedge_index h = it->second;
            from.erase(it);
            const auto start = out.source(h);
            while (true) {
                loop.push_back(to_vec(out.point(out.source(h))));
                if (out.target(h) == start) break;
                auto nx = from.find(out.target(h));
                if (nx == from.end()) break;
                h = nx->second;
                from.erase(nx);
            }
            if (loop.size() >= 3) loops.push_back(std::move(loop));
        }
        if (loops.empty()) continue;
        if (loops.size() > 1) ++r.holes;
        std::size_t best = 0;
        Real best_area = -1;
        for (std::size_t k = 0; k < loops.size(); ++k) {
            const Real a = norm(vmm::face_geometry(std::span<const Vec3>(loops[k])).area_vector);
            if (a > best_area) best_area = a, best = k;
        }
        r.pieces.push_back({flabel[EMesh::Face_index(static_cast<EMesh::size_type>(g))], std::move(loops[best])});
    }
    // The corefinement output has no canonical order (R18): every loop starts
    // at its smallest point and the pieces are sorted.
    for (Piece& f : r.pieces) std::ranges::rotate(f.pts, std::ranges::min_element(f.pts));
    std::ranges::sort(r.pieces, [](const Piece& a, const Piece& c) {
        return std::tie(a.label, a.pts) < std::tie(c.label, c.pts);
    });
    return r;
}

struct VecHash {
    std::size_t operator()(const Vec3& v) const noexcept {
        std::uint64_t h = 1469598103934665603ULL;
        for (const Real x : v.data) h = (h ^ std::bit_cast<std::uint64_t>(x)) * 1099511628211ULL;
        return static_cast<std::size_t>(h);
    }
};

}  // namespace

// ----------------------------------------------------------------------------
// Public functions
// ----------------------------------------------------------------------------

Domain box(const Vec3& lo, const Vec3& hi) {
    Domain d;
    for (int k = 0; k < 8; ++k) {
        d.points.push_back({(k & 1) ? hi[0] : lo[0], (k & 2) ? hi[1] : lo[1], (k & 4) ? hi[2] : lo[2]});
    }
    const std::array<std::array<std::uint32_t, 4>, 6> quads{{{0, 4, 6, 2}, {1, 3, 7, 5}, {0, 1, 5, 4},
                                                              {2, 6, 7, 3}, {0, 2, 3, 1}, {4, 5, 7, 6}}};
    for (std::uint32_t q = 0; q < 6; ++q) {
        d.triangles.push_back({quads[q][0], quads[q][1], quads[q][2]});
        d.triangles.push_back({quads[q][0], quads[q][2], quads[q][3]});
        d.tri_patch.push_back(q);
        d.tri_patch.push_back(q);
    }
    d.patch_names = {"x-", "x+", "y-", "y+", "z-", "z+"};
    return d;
}

Domain prism(const std::vector<std::array<Real, 2>>& polygon, Real z0, Real z1) {
    Domain d;
    const auto n = static_cast<std::uint32_t>(polygon.size());
    for (const auto& p : polygon) d.points.push_back({p[0], p[1], z0});
    for (const auto& p : polygon) d.points.push_back({p[0], p[1], z1});
    for (const auto& t : ear_clip(polygon)) {
        d.triangles.push_back({t[0], t[2], t[1]});  // bottom faces -z
        d.tri_patch.push_back(0);
        d.triangles.push_back({t[0] + n, t[1] + n, t[2] + n});
        d.tri_patch.push_back(1);
    }
    for (std::uint32_t k = 0; k < n; ++k) {
        const std::uint32_t a = k;
        const std::uint32_t b = (k + 1) % n;
        d.triangles.push_back({a, b, b + n});
        d.triangles.push_back({a, b + n, a + n});
        d.tri_patch.push_back(2);
        d.tri_patch.push_back(2);
    }
    d.patch_names = {"bottom", "top", "side"};
    return d;
}

Domain icosphere(const Vec3& centre, Real radius, int subdivisions) {
    const Real t = (1 + std::sqrt(5.0)) / 2;
    std::vector<Vec3> p{{-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t},
                        {0, -1, -t}, {0, 1, -t}, {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
    std::vector<std::array<std::uint32_t, 3>> f{{0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
                                                {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
                                                {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
                                                {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}};
    for (auto& x : p) x = (1.0 / norm(x)) * x;
    for (int s = 0; s < subdivisions; ++s) {
        std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> mid;
        const auto midpoint = [&](std::uint32_t a, std::uint32_t b) {
            const auto key = std::minmax(a, b);
            auto it = mid.find(key);
            if (it != mid.end()) return it->second;
            Vec3 m = 0.5 * (p[a] + p[b]);
            p.push_back((1.0 / norm(m)) * m);
            return mid.emplace(key, static_cast<std::uint32_t>(p.size() - 1)).first->second;
        };
        std::vector<std::array<std::uint32_t, 3>> g;
        for (const auto& tr : f) {
            const std::uint32_t a = midpoint(tr[0], tr[1]);
            const std::uint32_t b = midpoint(tr[1], tr[2]);
            const std::uint32_t c = midpoint(tr[2], tr[0]);
            g.insert(g.end(), {{tr[0], a, c}, {tr[1], b, a}, {tr[2], c, b}, {a, b, c}});
        }
        f = std::move(g);
    }
    Domain d;
    for (const auto& x : p) d.points.push_back(centre + radius * x);
    d.triangles = f;
    d.tri_patch.assign(f.size(), 0);
    d.patch_names = {"sphere"};
    orient_outward(d);
    return d;
}

Domain merge(const std::vector<Domain>& parts) {
    Domain d;
    for (std::size_t c = 0; c < parts.size(); ++c) {
        const auto offset = static_cast<std::uint32_t>(d.points.size());
        const auto patch_offset = static_cast<std::uint32_t>(d.patch_names.size());
        d.points.insert(d.points.end(), parts[c].points.begin(), parts[c].points.end());
        for (const auto& t : parts[c].triangles) d.triangles.push_back({t[0] + offset, t[1] + offset, t[2] + offset});
        for (const auto p : parts[c].tri_patch) d.tri_patch.push_back(p + patch_offset);
        for (const auto& n : parts[c].patch_names) d.patch_names.push_back(std::format("c{}:{}", c, n));
    }
    return d;
}

std::string backend_versions() { return std::format("CGAL {}; Boost {}", CGAL_VERSION_STR, BOOST_LIB_VERSION); }

Domain scaled(const Domain& d, Real s) {
    Domain r = d;
    for (auto& p : r.points) p = s * p;
    return r;
}

DomainMeasures measures(const Domain& d) {
    const EMesh m = to_mesh<EMesh, Epeck::Point_3>(d);
    DomainMeasures r;
    r.volume = CGAL::to_double(PMP::volume(m));
    r.area = CGAL::to_double(PMP::area(m));
    Vec3 lo = d.points[0];
    Vec3 hi = d.points[0];
    for (const Vec3& p : d.points) {
        for (std::size_t k = 0; k < 3; ++k) lo[k] = std::min(lo[k], p[k]), hi[k] = std::max(hi[k], p[k]);
    }
    r.diagonal = norm(hi - lo);
    return r;
}

std::vector<Vec3> random_sites(const Domain& d, std::size_t count, Real spacing, Real margin, std::uint64_t seed) {
    const IMesh m = to_mesh<IMesh, Epick::Point_3>(d);
    Tree tree(faces(m).first, faces(m).second, m);
    tree.accelerate_distance_queries();
    const CGAL::Side_of_triangle_mesh<IMesh, Epick> side(tree);
    Vec3 lo = d.points[0];
    Vec3 hi = d.points[0];
    for (const Vec3& p : d.points) {
        for (std::size_t k = 0; k < 3; ++k) lo[k] = std::min(lo[k], p[k]), hi[k] = std::max(hi[k], p[k]);
    }
    std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> grid;
    const auto cell_key = [&](const Vec3& x, int dx, int dy, int dz) {
        const auto c = [&](std::size_t k, int o) {
            return static_cast<std::uint64_t>(static_cast<std::int64_t>(std::floor((x[k] - lo[k]) / spacing)) + o + (1 << 20));
        };
        return (c(0, dx) << 42) | (c(1, dy) << 21) | c(2, dz);
    };
    std::vector<Vec3> sites;
    std::uint64_t s = seed;
    for (std::size_t attempt = 0; sites.size() < count && attempt < 200 * count; ++attempt) {
        const Vec3 x{lo[0] + uniform(s) * (hi[0] - lo[0]), lo[1] + uniform(s) * (hi[1] - lo[1]),
                     lo[2] + uniform(s) * (hi[2] - lo[2])};
        bool free = true;
        for (int dx = -1; dx <= 1 && free; ++dx) {
            for (int dy = -1; dy <= 1 && free; ++dy) {
                for (int dz = -1; dz <= 1 && free; ++dz) {
                    const auto it = grid.find(cell_key(x, dx, dy, dz));
                    if (it == grid.end()) continue;
                    for (const std::uint32_t j : it->second) free = free && norm(sites[j] - x) >= spacing;
                }
            }
        }
        if (!free) continue;
        const Epick::Point_3 q(x[0], x[1], x[2]);
        if (side(q) != CGAL::ON_BOUNDED_SIDE) continue;
        if (tree.squared_distance(q) < margin * margin) continue;
        grid[cell_key(x, 0, 0, 0)].push_back(static_cast<std::uint32_t>(sites.size()));
        sites.push_back(x);
    }
    return sites;
}

std::vector<Vec3> inside(const Domain& d, const std::vector<Vec3>& candidates) {
    const IMesh m = to_mesh<IMesh, Epick::Point_3>(d);
    Tree tree(faces(m).first, faces(m).second, m);
    const CGAL::Side_of_triangle_mesh<IMesh, Epick> side(tree);
    std::vector<Vec3> out;
    for (const Vec3& x : candidates) {
        if (side(Epick::Point_3(x[0], x[1], x[2])) == CGAL::ON_BOUNDED_SIDE) out.push_back(x);
    }
    return out;
}

Build build_mesh(const Domain& domain, const std::vector<Vec3>& input_sites) {
    Build b;
    BuildStats& st = b.stats;
    const DomainMeasures dm = measures(domain);
    const Real L = dm.diagonal;
    const Real tol = 1e-10 * L;
    const Real tiny_area = 1e-24 * L * L;

    // Canonical order (input-order independence, R18).
    std::vector<std::uint32_t> order(input_sites.size());
    std::iota(order.begin(), order.end(), 0u);
    std::ranges::sort(order, [&](std::uint32_t a, std::uint32_t c) { return input_sites[a] < input_sites[c]; });
    std::vector<Vec3> sites;
    for (const std::uint32_t k : order) sites.push_back(input_sites[k]);
    const std::size_t n = sites.size();
    st.cells = n;

    auto t0 = Clock::now();
    using Vb = CGAL::Triangulation_vertex_base_with_info_3<std::uint32_t, Epick>;
    using Tds = CGAL::Triangulation_data_structure_3<Vb>;
    using Delaunay = CGAL::Delaunay_triangulation_3<Epick, Tds>;
    std::vector<std::pair<Epick::Point_3, std::uint32_t>> points;
    for (std::uint32_t i = 0; i < n; ++i) points.emplace_back(Epick::Point_3(sites[i][0], sites[i][1], sites[i][2]), i);
    const Delaunay dt(points.begin(), points.end());
    std::vector<std::vector<std::uint32_t>> nb(n);
    for (auto e = dt.finite_edges_begin(); e != dt.finite_edges_end(); ++e) {
        const std::uint32_t a = e->first->vertex(e->second)->info();
        const std::uint32_t c = e->first->vertex(e->third)->info();
        nb[a].push_back(c);
        nb[c].push_back(a);
    }
    st.seconds_delaunay = seconds_since(t0);

    // Convex cells, snapped to canonical vertices.
    t0 = Clock::now();
    Vec3 lo = domain.points[0];
    Vec3 hi = domain.points[0];
    for (const Vec3& p : domain.points) {
        for (std::size_t k = 0; k < 3; ++k) lo[k] = std::min(lo[k], p[k]), hi[k] = std::max(hi[k], p[k]);
    }
    const Vec3 pad{0.05 * L, 0.05 * L, 0.05 * L};
    Canonical canonical{sites, {}};
    std::vector<std::vector<Piece>> cells(n);
    for (std::uint32_t i = 0; i < n; ++i) {
        std::ranges::sort(nb[i]);
        Poly poly = box_poly(lo - pad, hi + pad);
        for (const std::uint32_t j : nb[i]) {
            const std::uint32_t a = std::min(i, j);
            const std::uint32_t c = std::max(i, j);
            clip(poly, 0.5 * (sites[a] + sites[c]), sites[j] - sites[i], static_cast<Label>(j));
        }
        std::vector<char> used(poly.verts.size(), 0);
        for (const PF& f : poly.faces) {
            for (const std::uint32_t v : f.v) used[v] = 1;
        }
        for (std::size_t v = 0; v < poly.verts.size(); ++v) {
            if (!used[v]) continue;
            const auto& pl = poly.verts[v].planes;
            if (std::ranges::any_of(pl, [](Label x) { return x < 0; })) continue;
            const auto [ok, x] = canonical.vertex({i, static_cast<std::uint32_t>(pl[0]), static_cast<std::uint32_t>(pl[1]),
                                                   static_cast<std::uint32_t>(pl[2])});
            if (ok) {
                poly.verts[v].p = x;
            } else {
                ++st.unsnapped_vertices;
            }
        }
        cells[i] = pieces_of(poly);
    }
    VertexMerger merger{1e-12 * L, {}, 0};
    for (std::uint32_t i = 0; i < n; ++i) {
        merge_cell(cells[i], merger);
        st.t_vertices += insert_t_vertices(cells[i], 1e-12 * L);
    }
    st.merged_vertices = merger.merged;
    st.seconds_convex = seconds_since(t0);

    // Which cells touch the boundary: box of the cell against the domain triangles.
    t0 = Clock::now();
    const IMesh im = to_mesh<IMesh, Epick::Point_3>(domain);
    const Tree tree(faces(im).first, faces(im).second, im);
    std::vector<char> clipped(n, 0);
    for (std::uint32_t i = 0; i < n; ++i) {
        Vec3 a = cells[i][0].pts[0];
        Vec3 c = a;
        for (const Piece& f : cells[i]) {
            for (const Vec3& p : f.pts) {
                for (std::size_t k = 0; k < 3; ++k) a[k] = std::min(a[k], p[k]), c[k] = std::max(c[k], p[k]);
            }
        }
        const bool reaches_box = std::ranges::any_of(cells[i], [](const Piece& f) { return f.label < 0; });
        clipped[i] = reaches_box || tree.do_intersect(Epick::Iso_cuboid_3(a[0], a[1], a[2], c[0], c[1], c[2]));
    }
    st.seconds_classify = seconds_since(t0);

    // Exact clipping of the boundary cells.
    t0 = Clock::now();
    const EMesh de = to_mesh<EMesh, Epeck::Point_3>(domain);
    b.cell_volume.assign(n, 0);
    for (std::uint32_t i = 0; i < n; ++i) {
        if (!clipped[i]) {
            if (euler(cells[i]) != 2) ++st.euler_failures;
            b.cell_volume[i] = tetra_volume(cells[i], sites[i]);
            continue;
        }
        ++st.boundary_cells;
        ClipResult r = clip_exact(cells[i], de, domain, tree, tol);
        if (!r.ok) {
            ++st.clip_failures;
            if (r.invalid_input) ++st.invalid_cell_meshes;
            cells[i].clear();
            continue;
        }
        if (r.components > 1) ++st.disconnected_cells;
        st.unlabelled_triangles += r.unlabelled;
        st.ambiguous_triangles += r.ambiguous;
        st.faces_with_holes += r.holes;
        b.cell_volume[i] = r.volume;
        cells[i] = std::move(r.pieces);
    }
    st.seconds_clip = seconds_since(t0);

    // Assembly into the generic mesh.
    t0 = Clock::now();
    struct Side {
        std::uint32_t a, c;  // pair, a < c
        bool from_owner;
        std::uint32_t cell;
        std::size_t piece;
    };
    std::vector<Side> sides;
    struct BoundaryPiece {
        std::uint32_t patch, cell;
        std::size_t piece;
    };
    std::vector<BoundaryPiece> bpieces;
    for (std::uint32_t i = 0; i < n; ++i) {
        for (std::size_t k = 0; k < cells[i].size(); ++k) {
            const Label l = cells[i][k].label;
            if (l >= 0) {
                const auto j = static_cast<std::uint32_t>(l);
                sides.push_back({std::min(i, j), std::max(i, j), i < j, i, k});
            } else if (is_domain(l)) {
                bpieces.push_back({domain.tri_patch[domain_triangle(l)], i, k});
            } else {
                ++st.leftover_box_faces;
            }
        }
    }
    std::ranges::sort(sides, [](const Side& x, const Side& y) {
        return std::tie(x.a, x.c, x.from_owner, x.piece) < std::tie(y.a, y.c, y.from_owner, y.piece);
    });
    vmm::MeshData<3> md;
    std::unordered_map<Vec3, vmm::VertexId, VecHash> vids;
    const auto add_face = [&](const std::vector<Vec3>& pts, std::uint32_t owner, bool reversed) {
        std::vector<vmm::VertexId> row;
        for (std::size_t k = 0; k < pts.size(); ++k) {
            const Vec3& p = reversed ? pts[pts.size() - 1 - k] : pts[k];
            auto it = vids.find(p);
            if (it == vids.end()) {
                it = vids.emplace(p, vmm::VertexId::from_index(md.points.size())).first;
                md.points.push_back(p);
            }
            row.push_back(it->second);
        }
        md.face_vertices.push_row(row);
        md.owner.push_back(vmm::CellId::from_index(owner));
    };
    const auto area_sum = [&](std::span<const Side> ss) {
        Vec3 s{};
        for (const Side& x : ss) s = s + vmm::face_geometry(std::span<const Vec3>(cells[x.cell][x.piece].pts)).area_vector;
        return s;
    };
    for (std::size_t k = 0; k < sides.size();) {
        std::size_t e = k;
        while (e < sides.size() && sides[e].a == sides[k].a && sides[e].c == sides[k].c) ++e;
        std::vector<Side> own;
        std::vector<Side> other;
        for (std::size_t q = k; q < e; ++q) (sides[q].from_owner ? own : other).push_back(sides[q]);
        const Vec3 s_own = area_sum(own);
        const Vec3 s_other = area_sum(other);
        if (own.empty() || other.empty()) {
            const Real area = norm(own.empty() ? s_other : s_own);
            ++(area <= tiny_area ? st.dropped_tiny_faces : st.unmatched_faces);
        } else {
            if (own.size() != other.size()) ++st.piece_count_mismatch;
            st.max_partner_mismatch = std::max(st.max_partner_mismatch, norm(s_own + s_other) / norm(s_own));
        }
        const bool use_own = !own.empty();
        for (const Side& x : use_own ? own : other) {
            add_face(cells[x.cell][x.piece].pts, sides[k].a, !use_own);
            md.neighbour.push_back(vmm::CellId::from_index(sides[k].c));
        }
        k = e;
    }
    std::ranges::sort(bpieces, [](const BoundaryPiece& x, const BoundaryPiece& y) {
        return std::tie(x.patch, x.cell, x.piece) < std::tie(y.patch, y.cell, y.piece);
    });
    for (std::uint32_t p = 0; p < domain.patch_names.size(); ++p) {
        const std::size_t start = md.owner.size();
        for (const auto& x : bpieces) {
            if (x.patch == p) add_face(cells[x.cell][x.piece].pts, x.cell, false);
        }
        md.patches.push_back({domain.patch_names[p], start, md.owner.size() - start});
    }
    md.sites = sites;
    md.cell_region.assign(n, vmm::RegionId::from_index(0));
    for (const std::uint32_t k : order) md.cell_input_site.push_back(vmm::SiteId::from_index(k));
    md.regions = {{"domain", vmm::MediumId::from_index(0)}};
    md.media = {"medium"};
    auto mesh = vmm::Mesh3D::from_data(std::move(md));
    if (mesh) {
        b.mesh = std::move(*mesh);
    } else {
        b.error = std::string(mesh.error().message());
    }
    st.seconds_assembly = seconds_since(t0);
    return b;
}

std::vector<std::array<std::uint64_t, 2>> topology(const vmm::Mesh3D& mesh) {
    std::vector<std::array<std::uint64_t, 2>> t;
    for (const auto f : mesh.internal_faces()) t.push_back({mesh.owner(f).value, mesh.neighbour(f).value});
    for (const auto f : mesh.boundary_faces()) {
        t.push_back({mesh.owner(f).value, (std::uint64_t{1} << 62) + mesh.patch(f).value});
    }
    std::ranges::sort(t);
    return t;
}

std::uint64_t checksum(const vmm::Mesh3D& mesh) {
    std::uint64_t h = 1469598103934665603ULL;
    const auto mix = [&](const void* p, std::size_t bytes) {
        const auto* c = static_cast<const unsigned char*>(p);
        for (std::size_t k = 0; k < bytes; ++k) h = (h ^ c[k]) * 1099511628211ULL;
    };
    const auto& d = mesh.data();
    mix(d.points.data(), d.points.size() * sizeof(Vec3));
    mix(d.face_vertices.values.data(), d.face_vertices.values.size() * sizeof(vmm::VertexId));
    mix(d.face_vertices.offsets.data(), d.face_vertices.offsets.size() * sizeof(std::size_t));
    mix(d.owner.data(), d.owner.size() * sizeof(vmm::CellId));
    mix(d.neighbour.data(), d.neighbour.size() * sizeof(vmm::CellId));
    return h;
}

}  // namespace p15a
