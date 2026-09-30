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

/// Removes repeated consecutive ids and the faces thinner than `tol`
/// (width 2 A / longest edge). Returns the number of faces removed.
std::size_t clean_faces(std::vector<IdFace>& faces, const PointSet& ps, Real tol) {
    std::size_t removed = 0;
    std::vector<IdFace> kept;
    for (IdFace& f : faces) {
        std::vector<std::uint32_t> v;
        for (const std::uint32_t x : f.v) {
            if (v.empty() || v.back() != x) v.push_back(x);
        }
        while (v.size() > 1 && v.front() == v.back()) v.pop_back();
        bool keep = v.size() >= 3;
        if (keep) {
            std::vector<Vec3> pts;
            Real longest = 0;
            for (std::size_t k = 0; k < v.size(); ++k) {
                pts.push_back(ps.point(v[k]));
                longest = std::max(longest, norm(ps.point(v[(k + 1) % v.size()]) - ps.point(v[k])));
            }
            keep = 2 * norm(face_geometry(std::span<const Vec3>(pts)).area_vector) > tol * longest;
        }
        if (keep) {
            kept.push_back({f.label, std::move(v)});
        } else {
            ++removed;
        }
    }
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

}  // namespace

Result<Build3D> build_mesh_3d(const Partition3D& partition, const SiteSet3D& sites, const Backend3D& backend,
                              const BuildOptions3D& options) {
    if (!backend.complete()) return fail(ErrorCode::InvalidArgument, "incomplete 3D backend");
    if (partition.region_count() != 1) {
        return fail(ErrorCode::InvalidArgument, "version 0.3 builds one region; several regions are planned for 0.5 (P18)");
    }
    if (sites.empty()) return fail(ErrorCode::RegionWithoutSites, partition.regions().front().name, RegionId::from_index(0));
    const auto tolerance = Tolerance::from_length(partition.length_scale(), options.relative_tolerance);
    if (!tolerance) return fail(ErrorCode::InvalidLengthScale, "empty partition or invalid relative tolerance");
    const Real L = tolerance->length_scale();
    const Real tol = tolerance->point();
    const RegionId region = RegionId::from_index(0);
    auto prepared = backend.prepare(partition, region);
    if (!prepared) return std::unexpected(prepared.error());

    Build3D out;
    BuildStats3D& st = out.stats;

    // 1. Canonical order (R18).
    const auto in = sites.positions();
    std::vector<std::uint32_t> order(sites.size());
    std::iota(order.begin(), order.end(), 0u);
    std::ranges::sort(order, [&](std::uint32_t a, std::uint32_t b) { return in[a] < in[b]; });
    std::vector<Vec3> s;
    s.reserve(order.size());
    for (std::size_t k = 0; k < order.size(); ++k) {
        if (sites.regions()[order[k]] != region) return fail(ErrorCode::SiteOutsideRegion, "bad region", SiteId{order[k]});
        if (k > 0 && in[order[k]] == in[order[k - 1]]) return fail(ErrorCode::DuplicateSite, {}, SiteId{order[k]});
        s.push_back(in[order[k]]);
    }
    const std::size_t n = s.size();
    st.cells = n;

    // 2. Neighbours.
    auto t0 = Clock::now();
    std::vector<std::vector<std::uint32_t>> nb(n);
    for (const auto& [a, b] : backend.delaunay_pairs(s)) {
        nb[a.index()].push_back(b.value);
        nb[b.index()].push_back(a.value);
    }
    st.seconds_delaunay = seconds_since(t0);

    const Box3 enclosing = partition.bounding_box().inflated(0.05 * L);
    std::unordered_map<VertexKey, std::optional<Vec3>, KeyHash> centres;
    PointSet points(tol);
    const auto& ptri = partition.triangles();

    MeshData<3> md;
    struct BoundaryFace {
        std::uint32_t patch;
        std::uint32_t cell;
        std::vector<std::uint32_t> v;
    };
    std::vector<BoundaryFace> boundary;
    std::unordered_map<std::uint64_t, Pending> pending;
    const auto pair_key = [](std::uint32_t a, std::uint32_t b) { return (std::uint64_t{a} << 32) | b; };
    const Real tiny_area = tol * tol;
    out.cell_volume.assign(n, 0);

    for (std::uint32_t i = 0; i < n; ++i) {
        // 3. Convex cell.
        auto tc = Clock::now();
        std::ranges::sort(nb[i]);
        Poly poly = box_poly(enclosing);
        for (const std::uint32_t j : nb[i]) {
            clip(poly, 0.5 * (s[std::min(i, j)] + s[std::max(i, j)]), s[j] - s[i], j);
        }
        // 4. Canonical vertices.
        std::vector<char> used(poly.verts.size(), 0);
        for (const Face& f : poly.faces) {
            for (const std::uint32_t v : f.v) used[v] = 1;
        }
        for (std::size_t v = 0; v < poly.verts.size(); ++v) {
            const auto& pl = poly.verts[v].planes;
            if (!used[v] || !std::ranges::all_of(pl, is_neighbour_face)) continue;
            VertexKey key{i, static_cast<std::uint32_t>(pl[0]), static_cast<std::uint32_t>(pl[1]), static_cast<std::uint32_t>(pl[2])};
            std::ranges::sort(key);
            auto it = centres.find(key);
            if (it == centres.end()) {
                it = centres.emplace(key, backend.circumcentre({s[key[0]], s[key[1]], s[key[2]], s[key[3]]})).first;
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
        st.seconds_cells += seconds_since(tc);

        // 6. Exact clipping of the cells touching the boundary.
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
            if (!c.error.empty()) return fail(ErrorCode::BackendFailure, c.error, CellId{i});
            ++st.clipped_cells;
            if (c.local) ++st.local_clips;
            if (c.components > 1) {
                ++st.fragmented_cells;
                log(Error(ErrorCode::InvariantViolated, "cell made of several pieces (kept whole)", CellId{i}, Severity::Warning));
            }
            faces.clear();
            for (std::size_t f = 0; f < c.cell.faces.rows(); ++f) {
                IdFace g{c.cell.labels[f], {}};
                for (const std::uint32_t v : c.cell.faces.row(f)) g.v.push_back(points.id(c.cell.points[v]));
                faces.push_back(std::move(g));
            }
            st.collapsed_faces += clean_faces(faces, points, tol);
            out.cell_volume[i] = c.volume;
        } else {
            ++st.fast_cells;
            out.cell_volume[i] = tetra_volume(faces, points, s[i]);
        }
        st.seconds_clip += seconds_since(tc);

        // 7. Streaming assembly: owner copies now, in (owner, neighbour) order; the
        //    neighbour's copy is only matched against them.
        tc = Clock::now();
        std::ranges::stable_sort(faces, [](const IdFace& a, const IdFace& b) { return a.label < b.label; });
        for (const IdFace& f : faces) {
            if (is_box_face(f.label)) return fail(ErrorCode::BackendFailure, "enclosing-box face left in a cell", CellId{i});
            if (is_domain_face(f.label)) {
                const std::size_t t = f.label & ~kDomainFace;
                boundary.push_back({ptri.at(t).patch.value, i, f.v});
                continue;
            }
            const auto j = static_cast<std::uint32_t>(f.label);
            const Vec3 sv = area_vector(f, points);
            if (j > i) {
                md.face_vertices.push_row(vertex_ids(f.v));
                md.owner.push_back(CellId{i});
                md.neighbour.push_back(CellId{j});
                Pending& p = pending[pair_key(i, j)];
                p.area_vector = p.area_vector + sv;
                ++p.owner_pieces;
            } else {
                Pending& p = pending[pair_key(j, i)];
                p.area_vector = p.area_vector + sv;
                ++p.neighbour_pieces;
            }
        }
        // Pairs closed by this cell (owner j < i, neighbour i).
        for (const std::uint32_t j : nb[i]) {
            if (j >= i) continue;
            const auto it = pending.find(pair_key(j, i));
            if (it == pending.end()) continue;
            if ((it->second.owner_pieces == 0) != (it->second.neighbour_pieces == 0)) {
                // Face seen by one cell only: acceptable only when it is below the tolerance.
                if (norm(it->second.area_vector) > tiny_area) {
                    return fail(ErrorCode::InterfaceNotConforming,
                                std::format("face ({}, {}) seen by one cell only", j, i), CellId{i});
                }
            }
            pending.erase(it);
        }
        st.seconds_assembly += seconds_since(tc);
    }
    // Owner copies whose neighbour produced nothing.
    for (const auto& [key, p] : pending) {
        if (norm(p.area_vector) > tiny_area) {
            return fail(ErrorCode::InterfaceNotConforming,
                        std::format("face ({}, {}) seen by one cell only", key >> 32, key & 0xffffffffu));
        }
    }

    auto t1 = Clock::now();
    std::ranges::stable_sort(boundary, [](const BoundaryFace& a, const BoundaryFace& b) {
        return std::tie(a.patch, a.cell) < std::tie(b.patch, b.cell);
    });
    std::size_t next = 0;
    for (std::uint32_t patch = 0; patch < partition.patches().size(); ++patch) {
        const std::size_t start = md.owner.size();
        for (; next < boundary.size() && boundary[next].patch == patch; ++next) {
            const auto& v = boundary[next].v;
            md.face_vertices.push_row(vertex_ids(v));
            md.owner.push_back(CellId{boundary[next].cell});
        }
        md.patches.push_back({partition.patches()[patch], start, md.owner.size() - start});
    }
    st.merged_vertices = points.merged();
    md.points = points.take_points();
    md.sites = std::move(s);
    md.cell_region.assign(n, region);
    for (const std::uint32_t k : order) md.cell_input_site.push_back(SiteId{k});
    for (const auto& r : partition.regions()) md.regions.push_back({r.name, r.medium});
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
