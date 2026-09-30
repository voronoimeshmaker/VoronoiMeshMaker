// ============================================================================
// File: builder3d.cpp
// Description: build_mesh_3d (P16): the P15a construction in the library.
//              1 canonical site order; 2 Delaunay neighbours; 3 convex cell by
//              half-spaces with three plane labels per vertex; 4 canonical
//              vertices (exact circumcentres); 5 merge within the tolerance,
//              thin faces removed, T-vertices inserted; 6 exact clipping of
//              the cells touching the boundary; 7 streaming assembly.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <numeric>
#include <optional>
#include <span>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/tolerance.hpp>
#include <vmm/error/log.hpp>
#include <vmm/voronoi/builder3d.hpp>

namespace vmm {
namespace {

using Clock = std::chrono::steady_clock;
constexpr FaceLabel kUnknownPlane = kBoxFace | 0xffffu;

double seconds_since(Clock::time_point t0) { return std::chrono::duration<double>(Clock::now() - t0).count(); }

// ----------------------------------------------------------------------------
// Convex cell with three plane labels on every vertex
// ----------------------------------------------------------------------------

struct PlaneVertex {
    Vec3 p;
    std::array<FaceLabel, 3> planes;
};

struct Face {
    FaceLabel label;
    std::vector<std::uint32_t> v;  // counter-clockwise seen from outside
};

struct Poly {
    std::vector<PlaneVertex> verts;
    std::vector<Face> faces;
};

Poly box_poly(const Box3& b) {
    const Vec3& l = b.lo();
    const Vec3& h = b.hi();
    Poly p;
    for (std::uint32_t k = 0; k < 8; ++k) {
        const Vec3 x{(k & 1) ? h[0] : l[0], (k & 2) ? h[1] : l[1], (k & 4) ? h[2] : l[2]};
        p.verts.push_back({x, {kBoxFace | ((k & 1) ? 1u : 0u), kBoxFace | ((k & 2) ? 3u : 2u), kBoxFace | ((k & 4) ? 5u : 4u)}});
    }
    p.faces = {{kBoxFace | 0, {0, 4, 6, 2}}, {kBoxFace | 1, {1, 3, 7, 5}}, {kBoxFace | 2, {0, 1, 5, 4}},
               {kBoxFace | 3, {2, 6, 7, 3}}, {kBoxFace | 4, {0, 2, 3, 1}}, {kBoxFace | 5, {4, 5, 7, 6}}};
    return p;
}

FaceLabel other_common_plane(const PlaneVertex& a, const PlaneVertex& b, FaceLabel face) {
    for (const FaceLabel x : a.planes) {
        if (x != face && std::ranges::find(b.planes, x) != b.planes.end()) return x;
    }
    return kUnknownPlane;
}

/// Keeps dot(x - m, n) <= 0; the cap face on the plane gets `label`.
void clip(Poly& poly, const Vec3& m, const Vec3& n, FaceLabel label) {
    std::vector<Real> s(poly.verts.size(), 0);
    bool any_out = false;
    for (const Face& f : poly.faces) {
        for (const std::uint32_t v : f.v) {
            s[v] = dot(poly.verts[v].p - m, n);
            any_out = any_out || s[v] > 0;
        }
    }
    if (!any_out) return;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> made;
    std::vector<std::uint32_t> cap;
    std::vector<Face> out;
    for (const Face& f : poly.faces) {
        Face g{f.label, {}};
        const std::size_t count = f.v.size();
        for (std::size_t k = 0; k < count; ++k) {
            const std::uint32_t a = f.v[k];
            const std::uint32_t b = f.v[(k + 1) % count];
            if (s[a] <= 0) {
                g.v.push_back(a);
                if (s[a] == 0) cap.push_back(a);
            }
            if ((s[a] < 0 && s[b] > 0) || (s[a] > 0 && s[b] < 0)) {
                const auto key = std::minmax(a, b);
                auto it = made.find(key);
                if (it == made.end()) {
                    // Cut point from the ordered pair: the two faces of the edge get identical doubles.
                    const Vec3& p = poly.verts[key.first].p;
                    const Vec3& q = poly.verts[key.second].p;
                    const Real fp = dot(p - m, n);
                    const Real fq = dot(q - m, n);
                    const FaceLabel other = other_common_plane(poly.verts[a], poly.verts[b], f.label);
                    poly.verts.push_back({p + (fp / (fp - fq)) * (q - p), {f.label, other, label}});
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
        c = (1 / static_cast<Real>(cap.size())) * c;
        const Vec3 nn = (1 / norm(n)) * n;
        const Vec3 helper = std::abs(nn[0]) < 0.9 ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
        Vec3 u = cross(nn, helper);
        u = (1 / norm(u)) * u;
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
// Canonical vertices and the shared point set
// ----------------------------------------------------------------------------

using VertexKey = std::array<std::uint32_t, 4>;

struct KeyHash {
    std::size_t operator()(const VertexKey& k) const noexcept {
        std::uint64_t h = 1469598103934665603ULL;
        for (const std::uint32_t x : k) h = (h ^ x) * 1099511628211ULL;
        return static_cast<std::size_t>(h);
    }
};

/// Points of the mesh with the merge within the tolerance (DEC-020): the
/// first point seen (canonical cell order) represents the others.
class PointSet {
public:
    explicit PointSet(Real tol) : tol_(tol), cell_(4 * tol) {}

    std::uint32_t id(const Vec3& p) {
        const std::int64_t a = q(p[0]);
        const std::int64_t b = q(p[1]);
        const std::int64_t c = q(p[2]);
        for (std::int64_t da = -1; da <= 1; ++da) {
            for (std::int64_t db = -1; db <= 1; ++db) {
                for (std::int64_t dc = -1; dc <= 1; ++dc) {
                    const auto it = grid_.find(key(a + da, b + db, c + dc));
                    if (it == grid_.end()) continue;
                    for (const std::uint32_t r : it->second) {
                        if (points_[r] == p) return r;
                        if (norm(points_[r] - p) <= tol_) {
                            ++merged_;
                            return r;
                        }
                    }
                }
            }
        }
        const auto r = static_cast<std::uint32_t>(points_.size());
        points_.push_back(p);
        grid_[key(a, b, c)].push_back(r);
        return r;
    }
    [[nodiscard]] const Vec3& point(std::uint32_t r) const { return points_[r]; }
    [[nodiscard]] std::vector<Vec3> take_points() { return std::move(points_); }
    [[nodiscard]] std::size_t merged() const noexcept { return merged_; }

private:
    [[nodiscard]] std::int64_t q(Real x) const { return static_cast<std::int64_t>(std::floor(x / cell_)); }
    [[nodiscard]] static std::uint64_t key(std::int64_t a, std::int64_t b, std::int64_t c) {
        const auto u = [](std::int64_t v) { return static_cast<std::uint64_t>(v) & 0x1fffffULL; };
        return (u(a) << 42) | (u(b) << 21) | u(c);
    }
    Real tol_;
    Real cell_;
    std::vector<Vec3> points_;
    std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> grid_;
    std::size_t merged_ = 0;
};

/// A face of a cell as point ids of the PointSet.
struct IdFace {
    FaceLabel label;
    std::vector<std::uint32_t> v;
};

/// True when the loop is at least `tol` wide (2 A / longest edge).
bool wide_enough(const std::vector<std::uint32_t>& v, const PointSet& ps, Real tol) {
    if (v.size() < 3) return false;
    std::vector<Vec3> pts;
    Real longest = 0;
    for (std::size_t k = 0; k < v.size(); ++k) {
        pts.push_back(ps.point(v[k]));
        longest = std::max(longest, norm(ps.point(v[(k + 1) % v.size()]) - ps.point(v[k])));
    }
    return 2 * norm(face_geometry(std::span<const Vec3>(pts)).area_vector) > tol * longest;
}

/// Removes repeated consecutive ids, splits a loop that touches itself (a point
/// repeated further on, after the merge of close points) into two loops, and
/// removes the loops thinner than `tol`. Returns the number of loops removed.
std::size_t clean_faces(std::vector<IdFace>& faces, const PointSet& ps, Real tol) {
    std::size_t removed = 0;
    std::vector<IdFace> kept;
    std::vector<IdFace> todo = std::move(faces);
    while (!todo.empty()) {
        IdFace f = std::move(todo.back());
        todo.pop_back();
        std::vector<std::uint32_t> v;
        for (const std::uint32_t x : f.v) {
            if (v.empty() || v.back() != x) v.push_back(x);
        }
        while (v.size() > 1 && v.front() == v.back()) v.pop_back();
        bool split = false;
        for (std::size_t i = 0; i < v.size() && !split; ++i) {
            for (std::size_t j = i + 1; j < v.size() && !split; ++j) {
                if (v[i] != v[j]) continue;
                std::vector<std::uint32_t> a(v.begin() + static_cast<std::ptrdiff_t>(i), v.begin() + static_cast<std::ptrdiff_t>(j));
                std::vector<std::uint32_t> b(v.begin() + static_cast<std::ptrdiff_t>(j), v.end());
                b.insert(b.end(), v.begin(), v.begin() + static_cast<std::ptrdiff_t>(i));
                todo.push_back({f.label, std::move(a)});
                todo.push_back({f.label, std::move(b)});
                split = true;
            }
        }
        if (split) continue;
        if (wide_enough(v, ps, tol)) {
            kept.push_back({f.label, std::move(v)});
        } else {
            ++removed;
        }
    }
    std::ranges::reverse(kept);  // the order of the input faces
    faces = std::move(kept);
    return removed;
}

/// Inserts into every face edge the cell's points lying on it (within tol),
/// so that faces sharing a split edge conform. Returns the count.
std::size_t insert_t_vertices(std::vector<IdFace>& faces, const PointSet& ps, Real tol) {
    std::vector<std::uint32_t> all;
    for (const IdFace& f : faces) all.insert(all.end(), f.v.begin(), f.v.end());
    std::ranges::sort(all);
    all.erase(std::unique(all.begin(), all.end()), all.end());
    std::size_t inserted = 0;
    for (IdFace& f : faces) {
        std::vector<std::uint32_t> loop;
        for (std::size_t k = 0; k < f.v.size(); ++k) {
            const std::uint32_t ia = f.v[k];
            const std::uint32_t ib = f.v[(k + 1) % f.v.size()];
            const Vec3& a = ps.point(ia);
            const Vec3 d = ps.point(ib) - a;
            const Real len = norm(d);
            loop.push_back(ia);
            std::vector<std::pair<Real, std::uint32_t>> on;
            for (const std::uint32_t iv : all) {
                if (iv == ia || iv == ib) continue;
                const Vec3& v = ps.point(iv);
                const Real t = dot(v - a, d) / (len * len);
                if (t * len <= tol || (1 - t) * len <= tol) continue;
                if (norm(v - (a + t * d)) <= tol) on.emplace_back(t, iv);
            }
            std::ranges::sort(on);
            for (const auto& [t, iv] : on) loop.push_back(iv);
            inserted += on.size();
        }
        f.v = std::move(loop);
    }
    return inserted;
}

Vec3 area_vector(const IdFace& f, const PointSet& ps) {
    std::vector<Vec3> pts;
    for (const std::uint32_t x : f.v) pts.push_back(ps.point(x));
    return face_geometry(std::span<const Vec3>(pts)).area_vector;
}

/// Volume by tetrahedra from the site (independent of the invariant checker).
Real tetra_volume(const std::vector<IdFace>& faces, const PointSet& ps, const Vec3& o) {
    Real v = 0;
    for (const IdFace& f : faces) {
        for (std::size_t k = 1; k + 1 < f.v.size(); ++k) {
            v += dot(ps.point(f.v[0]) - o, cross(ps.point(f.v[k]) - o, ps.point(f.v[k + 1]) - o)) / 6;
        }
    }
    return v;
}

/// Both copies of an internal face (i, j), i < j, until cell j is done.
struct Pending {
    Vec3 area_vector{};         ///< sum of the two copies (zero when they agree)
    std::size_t owner_pieces = 0;
    std::size_t neighbour_pieces = 0;
};

std::vector<VertexId> vertex_ids(const std::vector<std::uint32_t>& v) {
    std::vector<VertexId> ids;
    ids.reserve(v.size());
    for (const std::uint32_t x : v) ids.push_back(VertexId{x});
    return ids;
}

/// Part of the convex polygon `a` inside the convex polygon `b`, both in one
/// plane (b counter-clockwise around its own area vector); empty if none.
std::vector<Vec3> clip_convex(std::vector<Vec3> a, const std::vector<Vec3>& b) {
    const Vec3 nb = face_geometry(std::span<const Vec3>(b)).area_vector;
    for (std::size_t k = 0; k < b.size() && a.size() >= 3; ++k) {
        const Vec3& p = b[k];
        const Vec3& q = b[(k + 1) % b.size()];
        const auto f = [&](const Vec3& x) { return dot(cross(q - p, x - p), nb); };
        std::vector<Vec3> out;
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
    return a.size() >= 3 ? a : std::vector<Vec3>{};
}

/// Internal faces stored contiguously (no allocation per face), in (owner, neighbour) order.
struct FlatFaces {
    std::vector<CellId> owner;
    std::vector<CellId> neighbour;
    Csr<VertexId> v;

    void push(std::uint32_t o, std::uint32_t n, const std::vector<std::uint32_t>& ids) {
        owner.push_back(CellId{o});
        neighbour.push_back(CellId{n});
        for (const std::uint32_t x : ids) v.values.push_back(VertexId{x});
        v.offsets.push_back(v.values.size());
    }
};

/// A face of a cell lying on an interface triangle (P18).
struct InterfacePiece {
    std::uint32_t cell;
    std::vector<std::uint32_t> v;
};

}  // namespace

Result<Build3D> build_mesh_3d(const Partition3D& partition, const SiteSet3D& sites, const Backend3D& backend,
                              const BuildOptions3D& options) {
    if (!backend.complete()) return fail(ErrorCode::InvalidArgument, "incomplete 3D backend");
    const std::size_t nr = partition.region_count();
    if (nr == 0) return fail(ErrorCode::InvalidArgument, "empty partition");
    const auto tolerance = Tolerance::from_length(partition.length_scale(), options.relative_tolerance);
    if (!tolerance) return fail(ErrorCode::InvalidLengthScale, "empty partition or invalid relative tolerance");
    const Real L = tolerance->length_scale();
    const Real tol = tolerance->point();
    const Real tiny_area = tol * tol;

    Build3D out;
    BuildStats3D& st = out.stats;

    // 1. Canonical order (R18): by region, then by position.
    const auto in = sites.positions();
    const auto rin = sites.regions();
    for (std::size_t k = 0; k < sites.size(); ++k) {
        if (!rin[k].valid() || rin[k].index() >= nr) return fail(ErrorCode::SiteOutsideRegion, "bad region", SiteId::from_index(k));
    }
    std::vector<std::uint32_t> order(sites.size());
    std::iota(order.begin(), order.end(), 0u);
    std::ranges::sort(order, [&](std::uint32_t a, std::uint32_t b) { return in[a] < in[b]; });
    for (std::size_t k = 1; k < order.size(); ++k) {
        if (in[order[k]] == in[order[k - 1]]) return fail(ErrorCode::DuplicateSite, {}, SiteId{order[k]});
    }
    std::ranges::stable_sort(order, [&](std::uint32_t a, std::uint32_t b) { return rin[a].value < rin[b].value; });
    std::vector<Vec3> s;
    s.reserve(order.size());
    std::vector<std::uint32_t> start(nr + 1, 0);
    for (const std::uint32_t k : order) {
        s.push_back(in[k]);
        ++start[rin[k].index() + 1];
    }
    for (std::size_t r = 0; r < nr; ++r) {
        if (start[r + 1] == 0) return fail(ErrorCode::RegionWithoutSites, partition.regions()[r].name, RegionId::from_index(r));
        start[r + 1] += start[r];
    }
    const std::size_t n = s.size();
    st.cells = n;

    const Box3 enclosing = partition.bounding_box().inflated(0.05 * L);
    PointSet points(tol);
    const auto& ptri = partition.triangles();
    struct BoundaryFace {
        std::uint32_t patch;
        std::uint32_t cell;
        std::vector<std::uint32_t> v;
    };
    std::vector<BoundaryFace> boundary;
    FlatFaces internal;   // faces inside the regions: already in (owner, neighbour) order
    FlatFaces interfaces; // faces between regions, sorted at the end
    std::unordered_map<std::size_t, std::array<std::vector<InterfacePiece>, 2>> pieces;
    const auto pair_key = [](std::uint32_t a, std::uint32_t b) { return (std::uint64_t{a} << 32) | b; };
    out.cell_volume.assign(n, 0);

    for (std::uint32_t r = 0; r < nr; ++r) {
        const RegionId region = RegionId{r};
        auto prepared = backend.prepare(partition, region);
        if (!prepared) return std::unexpected(prepared.error());
        const std::uint32_t first = start[r];
        const std::span<const Vec3> rs(s.data() + first, start[r + 1] - first);  // sites of the region
        const auto m = static_cast<std::uint32_t>(rs.size());

        // 2. Neighbours inside the region (one Voronoi diagram per region, DEC-028).
        auto t0 = Clock::now();
        std::vector<std::vector<std::uint32_t>> nb(m);
        for (const auto& [a, b] : backend.delaunay_pairs(rs)) {
            nb[a.index()].push_back(b.value);
            nb[b.index()].push_back(a.value);
        }
        st.seconds_delaunay += seconds_since(t0);
        std::unordered_map<VertexKey, std::optional<Vec3>, KeyHash> centres;
        std::unordered_map<std::uint64_t, Pending> pending;

        for (std::uint32_t i = 0; i < m; ++i) {
            const std::uint32_t gi = first + i;
            // 3. Convex cell.
            auto tc = Clock::now();
            std::ranges::sort(nb[i]);
            Poly poly = box_poly(enclosing);
            for (const std::uint32_t j : nb[i]) {
                clip(poly, 0.5 * (rs[std::min(i, j)] + rs[std::max(i, j)]), rs[j] - rs[i], j);
            }
            // 4. Canonical vertices.
            std::vector<char> used(poly.verts.size(), 0);
            for (const Face& f : poly.faces) {
                for (const std::uint32_t v : f.v) used[v] = 1;
            }
            for (std::size_t v = 0; v < poly.verts.size(); ++v) {
                const auto& pl = poly.verts[v].planes;
                if (!used[v] || !std::ranges::all_of(pl, is_neighbour_face)) continue;
                VertexKey key{i, static_cast<std::uint32_t>(pl[0]), static_cast<std::uint32_t>(pl[1]),
                              static_cast<std::uint32_t>(pl[2])};
                std::ranges::sort(key);
                auto it = centres.find(key);
                if (it == centres.end()) {
                    it = centres.emplace(key, backend.circumcentre({rs[key[0]], rs[key[1]], rs[key[2]], rs[key[3]]})).first;
                }
                if (it->second) {
                    poly.verts[v].p = *it->second;
                } else {
                    ++st.unsnapped_vertices;
                }
                if (key[3] == i) centres.erase(it);  // cell i is the last one to use this vertex
            }
            // 5. Merge, thin faces, T-vertices.
            std::vector<IdFace> faces;
            for (const Face& f : poly.faces) {
                IdFace g{f.label, {}};
                for (const std::uint32_t v : f.v) g.v.push_back(points.id(poly.verts[v].p));
                faces.push_back(std::move(g));
            }
            st.collapsed_faces += clean_faces(faces, points, tol);
            st.t_vertices += insert_t_vertices(faces, points, tol);
            st.collapsed_faces += clean_faces(faces, points, tol);  // spikes left by the insertion
            st.seconds_cells += seconds_since(tc);

            // 6. Exact clipping of the cells touching the region surface.
            tc = Clock::now();
            Box3 cell_box;
            for (const IdFace& f : faces) {
                for (const std::uint32_t v : f.v) cell_box.expand(points.point(v));
            }
            const bool reaches_box = std::ranges::any_of(faces, [](const IdFace& f) { return is_box_face(f.label); });
            const bool needs_clip = !options.fast_path || reaches_box || backend.touches_boundary(*prepared, cell_box);
            if (needs_clip) {
                LabelledPolyhedron3 lp;
                std::unordered_map<std::uint32_t, std::uint32_t> local;
                for (const IdFace& f : faces) {
                    std::vector<std::uint32_t> row;
                    for (const std::uint32_t v : f.v) {
                        const auto [it, added] = local.emplace(v, static_cast<std::uint32_t>(lp.points.size()));
                        if (added) lp.points.push_back(points.point(v));
                        row.push_back(it->second);
                    }
                    lp.faces.push_row(row);
                    lp.labels.push_back(f.label);
                }
                CellClip3 c = backend.clip_cell(lp, *prepared);
                if (!c.error.empty()) return fail(ErrorCode::BackendFailure, c.error, CellId{gi});
                ++st.clipped_cells;
                if (c.local) ++st.local_clips;
                if (c.components > 1) {
                    ++st.fragmented_cells;
                    log(Error(ErrorCode::InvariantViolated, "cell made of several pieces (kept whole)", CellId{gi},
                              Severity::Warning));
                }
                faces.clear();
                for (std::size_t f = 0; f < c.cell.faces.rows(); ++f) {
                    IdFace g{c.cell.labels[f], {}};
                    for (const std::uint32_t v : c.cell.faces.row(f)) g.v.push_back(points.id(c.cell.points[v]));
                    faces.push_back(std::move(g));
                }
                st.collapsed_faces += clean_faces(faces, points, tol);
                out.cell_volume[gi] = c.volume;
            } else {
                ++st.fast_cells;
                out.cell_volume[gi] = tetra_volume(faces, points, rs[i]);
            }
            st.seconds_clip += seconds_since(tc);

            // 7. Faces: internal ones matched with the neighbour's copy, boundary ones by
            //    patch, the ones on an interface kept for the common refinement.
            tc = Clock::now();
            std::ranges::stable_sort(faces, [](const IdFace& a, const IdFace& b) { return a.label < b.label; });
            for (IdFace& f : faces) {
                if (is_box_face(f.label)) return fail(ErrorCode::BackendFailure, "enclosing-box face left in a cell", CellId{gi});
                if (is_domain_face(f.label)) {
                    const std::size_t t = f.label & ~kDomainFace;
                    const PartitionTriangle& pt = ptri.at(t);
                    if (pt.inside.valid() && pt.outside.valid()) {
                        pieces[t][pt.inside == region ? 0 : 1].push_back({gi, std::move(f.v)});
                    } else {
                        boundary.push_back({pt.patch.value, gi, std::move(f.v)});
                    }
                    continue;
                }
                const auto j = static_cast<std::uint32_t>(f.label);
                const Vec3 sv = area_vector(f, points);
                Pending& p = pending[j > i ? pair_key(i, j) : pair_key(j, i)];
                p.area_vector = p.area_vector + sv;
                if (j > i) {
                    ++p.owner_pieces;
                    internal.push(gi, first + j, f.v);
                } else {
                    ++p.neighbour_pieces;
                }
            }
            // Pairs closed by this cell (owner j < i, neighbour i).
            for (const std::uint32_t j : nb[i]) {
                if (j >= i) continue;
                const auto it = pending.find(pair_key(j, i));
                if (it == pending.end()) continue;
                if ((it->second.owner_pieces == 0) != (it->second.neighbour_pieces == 0) &&
                    norm(it->second.area_vector) > tiny_area) {
                    return fail(ErrorCode::InterfaceNotConforming,
                                std::format("face ({}, {}) seen by one cell only", first + j, gi), CellId{gi});
                }
                pending.erase(it);
            }
            st.seconds_assembly += seconds_since(tc);
        }
        for (const auto& [key, p] : pending) {
            if (norm(p.area_vector) > tiny_area) {
                return fail(ErrorCode::InterfaceNotConforming,
                            std::format("face ({}, {}) seen by one cell only", first + (key >> 32), first + (key & 0xffffffffu)));
            }
        }
    }

    // 8. Common refinement of every interface triangle (P18a): the face between two
    //    cells of different regions is the intersection of their pieces.
    auto t1 = Clock::now();
    std::vector<std::size_t> interface_triangles;
    for (const auto& [t, sides] : pieces) interface_triangles.push_back(t);
    std::ranges::sort(interface_triangles);  // deterministic order
    for (const std::size_t t : interface_triangles) {
        const auto& sides = pieces.at(t);
        const auto polygon = [&](const InterfacePiece& p) {
            std::vector<Vec3> pts;
            for (const std::uint32_t v : p.v) pts.push_back(points.point(v));
            return pts;
        };
        std::vector<std::vector<Vec3>> pb;
        for (const auto& b : sides[1]) pb.push_back(polygon(b));
        std::vector<char> b_used(sides[1].size(), 0);
        for (const auto& a : sides[0]) {
            const std::vector<Vec3> pa = polygon(a);
            bool a_used = false;
            for (std::size_t k = 0; k < sides[1].size(); ++k) {
                std::vector<Vec3> x = clip_convex(pa, pb[k]);
                if (x.empty()) continue;
                if (norm(face_geometry(std::span<const Vec3>(x)).area_vector) <= tiny_area) {
                    ++st.interface_slivers;
                    continue;
                }
                IdFace g{0, {}};
                for (const Vec3& p : x) {
                    const std::uint32_t id = points.id(p);
                    if (g.v.empty() || g.v.back() != id) g.v.push_back(id);
                }
                while (g.v.size() > 1 && g.v.front() == g.v.back()) g.v.pop_back();
                // Thin after the merge of close points: dropped like any thin face.
                if (!wide_enough(g.v, points, tol)) {
                    ++st.interface_slivers;
                    continue;
                }
                a_used = true;
                b_used[k] = 1;
                const std::uint32_t cb = sides[1][k].cell;
                if (a.cell > cb) std::ranges::reverse(g.v);  // the owner is the cell of lower id
                interfaces.push(std::min(a.cell, cb), std::max(a.cell, cb), g.v);
                ++st.interface_faces;
            }
            if (!a_used && norm(face_geometry(std::span<const Vec3>(pa)).area_vector) > tiny_area) {
                return fail(ErrorCode::InterfaceNotConforming, std::format("interface piece of cell {} has no partner", a.cell),
                            CellId{a.cell});
            }
        }
        for (std::size_t k = 0; k < sides[1].size(); ++k) {
            if (!b_used[k] && norm(face_geometry(std::span<const Vec3>(pb[k])).area_vector) > tiny_area) {
                return fail(ErrorCode::InterfaceNotConforming,
                            std::format("interface piece of cell {} has no partner", sides[1][k].cell), CellId{sides[1][k].cell});
            }
        }
    }

    // 9. Assembly: internal faces in (owner, neighbour) order (the region faces merged
    //    with the sorted interface faces), then boundary faces by patch.
    MeshData<3> md;
    if (interfaces.owner.empty()) {
        md.owner = std::move(internal.owner);
        md.neighbour = std::move(internal.neighbour);
        md.face_vertices = std::move(internal.v);
    } else {
        std::vector<std::size_t> order_if(interfaces.owner.size());
        std::iota(order_if.begin(), order_if.end(), std::size_t{0});
        std::ranges::stable_sort(order_if, [&](std::size_t a, std::size_t b) {
            return std::pair(interfaces.owner[a], interfaces.neighbour[a]) < std::pair(interfaces.owner[b], interfaces.neighbour[b]);
        });
        const std::size_t total = internal.owner.size() + interfaces.owner.size();
        md.owner.reserve(total);
        md.neighbour.reserve(total);
        md.face_vertices.values.reserve(internal.v.values.size() + interfaces.v.values.size());
        const auto take = [&](const FlatFaces& from, std::size_t f) {
            md.owner.push_back(from.owner[f]);
            md.neighbour.push_back(from.neighbour[f]);
            md.face_vertices.push_row(from.v.row(f));
        };
        std::size_t a = 0;
        std::size_t b = 0;
        while (a < internal.owner.size() || b < order_if.size()) {
            const bool region_first =
                b == order_if.size() ||
                (a < internal.owner.size() && std::pair(internal.owner[a], internal.neighbour[a]) <
                                                  std::pair(interfaces.owner[order_if[b]], interfaces.neighbour[order_if[b]]));
            if (region_first) {
                take(internal, a++);
            } else {
                take(interfaces, order_if[b++]);
            }
        }
        internal = FlatFaces{};
        interfaces = FlatFaces{};
    }
    std::ranges::stable_sort(boundary, [](const BoundaryFace& a, const BoundaryFace& b) {
        return std::tie(a.patch, a.cell) < std::tie(b.patch, b.cell);
    });
    std::size_t next = 0;
    for (std::uint32_t patch = 0; patch < partition.patches().size(); ++patch) {
        const std::size_t begin = md.owner.size();
        for (; next < boundary.size() && boundary[next].patch == patch; ++next) {
            md.face_vertices.push_row(vertex_ids(boundary[next].v));
            md.owner.push_back(CellId{boundary[next].cell});
        }
        md.patches.push_back({partition.patches()[patch], begin, md.owner.size() - begin});
    }
    st.merged_vertices = points.merged();
    md.points = points.take_points();
    md.sites = std::move(s);
    for (const std::uint32_t k : order) {
        md.cell_region.push_back(rin[k]);
        md.cell_input_site.push_back(SiteId{k});
    }
    for (const auto& reg : partition.regions()) md.regions.push_back({reg.name, reg.medium});
    md.media = partition.media();
    auto mesh = Mesh3D::from_data(std::move(md));
    if (!mesh) return std::unexpected(mesh.error());
    out.mesh = std::move(*mesh);
    st.seconds_assembly += seconds_since(t1);
    return out;
}

InvariantReference invariant_reference(const Partition3D& p) {
    InvariantReference ref;
    ref.length_scale = p.length_scale();
    ref.total_measure = p.total_volume();
    for (std::size_t r = 0; r < p.region_count(); ++r) ref.region_measure.push_back(p.region_volume(RegionId::from_index(r)));
    ref.boundary_measure = p.boundary_area();
    for (const auto& t : p.triangles()) {
        if (!t.inside.valid() || !t.outside.valid()) continue;
        const std::size_t a = t.inside.index();
        const std::size_t b = t.outside.index();
        const auto& v = p.vertices();
        ref.interface_measure[{std::min(a, b), std::max(a, b)}] +=
            0.5 * norm(cross(v[t.v[1]] - v[t.v[0]], v[t.v[2]] - v[t.v[0]]));
    }
    return ref;
}

}  // namespace vmm
