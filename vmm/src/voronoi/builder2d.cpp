// ============================================================================
// File: builder2d.cpp
// Description: 2D multi-region conforming Voronoi mesh (no CGAL here; the
//              backend is reached through Backend2D).
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
#include <limits>
#include <map>
#include <numeric>
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
#include <vmm/geometry/polygon.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace vmm {
namespace {

constexpr EdgeLabel kSegmentBase = EdgeLabel{1} << 40;
constexpr EdgeLabel kBoxBase = EdgeLabel{1} << 41;

bool is_site(EdgeLabel l) { return l < kSegmentBase; }
bool is_segment(EdgeLabel l) { return l >= kSegmentBase && l < kBoxBase; }
std::size_t segment_of(EdgeLabel l) { return static_cast<std::size_t>(l - kSegmentBase); }

struct Line2 {
    Vec2 point;
    Vec2 normal;
};

using Clock = std::chrono::steady_clock;
double seconds_since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

/// Keeps dot(x - line.point, line.normal) <= 0; the new edge gets `label`.
LabelledLoop2 clip_halfplane(const LabelledLoop2& poly, const Line2& line, EdgeLabel label) {
    std::vector<Vec2> pts;
    std::vector<EdgeLabel> incoming;
    const std::size_t n = poly.points.size();
    for (std::size_t k = 0; k < n; ++k) {
        const Vec2& p = poly.points[k];
        const Vec2& q = poly.points[(k + 1) % n];
        const Real fp = dot(p - line.point, line.normal);
        const Real fq = dot(q - line.point, line.normal);
        const bool in_p = fp <= 0;
        const bool in_q = fq <= 0;
        if (in_p && in_q) {
            pts.push_back(q);
            incoming.push_back(poly.labels[k]);
        } else if (in_p != in_q) {
            const Vec2 x = p + (fp / (fp - fq)) * (q - p);
            pts.push_back(x);
            incoming.push_back(in_p ? poly.labels[k] : label);
            if (!in_p) {
                pts.push_back(q);
                incoming.push_back(poly.labels[k]);
            }
        }
    }
    LabelledLoop2 out;
    const std::size_t m = pts.size();
    for (std::size_t k = 0; k < m; ++k) {
        if (m > 1 && pts[k] == pts[(k + m - 1) % m]) continue;
        out.points.push_back(pts[k]);
        out.labels.push_back(incoming[k]);
    }
    if (!out.labels.empty()) std::rotate(out.labels.begin(), out.labels.begin() + 1, out.labels.end());
    return out;
}

struct Piece {
    std::uint32_t cell;
    RegionId region;
    Vec2 p;
    Vec2 q;
};

struct RawFace {
    std::uint32_t owner;
    std::uint32_t neighbour;  // max for boundary
    std::uint32_t patch;      // max for internal
    Vec2 p;
    Vec2 q;
};

class Builder {
public:
    Builder(const Partition2D& p, const SiteSet& s, const Backend2D& b, const BuildOptions2D& o, Real tol)
        : part_(p), input_(s), backend_(b), options_(o), tol_(tol) {}

    Result<Build2D> run() {
        canonical_order();
        prepare_regions();
        auto t0 = Clock::now();
        const auto neighbours = region_neighbours();
        out_.stats.seconds_delaunay = seconds_since(t0);
        t0 = Clock::now();
        const Box2 box = part_.bounding_box().inflated(0.1 * part_.length_scale());
        const LabelledLoop2 start{{box.lo(), Vec2{box.hi()[0], box.lo()[1]}, box.hi(), Vec2{box.lo()[0], box.hi()[1]}},
                                  {kBoxBase, kBoxBase + 1, kBoxBase + 2, kBoxBase + 3}};
        out_.cell_polygon_area.assign(sites_.size(), 0);
        interface_pieces_.resize(part_.segments().size());
        for (std::uint32_t i = 0; i < sites_.size(); ++i) {
            LabelledLoop2 cell = start;
            for (const std::uint32_t j : neighbours[i]) {
                const std::uint32_t a = std::min(i, j);
                const std::uint32_t b = std::max(i, j);
                cell = clip_halfplane(cell, Line2{0.5 * (sites_[a] + sites_[b]), sites_[j] - sites_[i]}, j);
            }
            merge_close_vertices(cell);
            if (auto ok = add_cell(i, cell); !ok) return std::unexpected(ok.error());
        }
        out_.stats.seconds_cells = seconds_since(t0);
        t0 = Clock::now();
        emit_internal_faces();
        for (std::size_t s = 0; s < part_.segments().size(); ++s) emit_interface_faces(s);
        auto mesh = assemble();
        out_.stats.seconds_assembly = seconds_since(t0);
        if (!mesh) return std::unexpected(mesh.error());
        out_.mesh = std::move(*mesh);
        out_.stats.cells = sites_.size();
        if (out_.stats.unresolved_vertices + out_.stats.unlabelled_edges > 0) {
            return fail(ErrorCode::UnresolvedLabel, std::format("{} vertices, {} edges", out_.stats.unresolved_vertices,
                                                                out_.stats.unlabelled_edges));
        }
        if (out_.stats.interface_bad_coverage > 0 || out_.stats.unmatched_bisector_pieces > 0) {
            return fail(ErrorCode::InterfaceNotConforming,
                        std::format("{} interface sub-intervals, {} bisector pieces", out_.stats.interface_bad_coverage,
                                    out_.stats.unmatched_bisector_pieces));
        }
        return std::move(out_);
    }

private:
    /// Canonical order: region, then lexicographic position (R18).
    void canonical_order() {
        std::vector<std::size_t> order(input_.size());
        std::iota(order.begin(), order.end(), std::size_t{0});
        const auto pos = input_.positions();
        const auto reg = input_.regions();
        std::ranges::sort(order, [&](std::size_t a, std::size_t b) {
            return std::tie(reg[a], pos[a]) < std::tie(reg[b], pos[b]);
        });
        for (const std::size_t k : order) {
            sites_.push_back(pos[k]);
            region_.push_back(reg[k]);
            input_id_.push_back(SiteId::from_index(k));
        }
    }

    void prepare_regions() {
        const auto& v = part_.vertices();
        for (const auto& s : part_.segments()) seg_.push_back({v[s.v0], v[s.v1]});
        const std::size_t nr = part_.region_count();
        components_.resize(nr);
        component_boxes_.resize(nr);
        std::vector<std::vector<Vec2>> a(nr);
        std::vector<std::vector<Vec2>> b(nr);
        for (std::size_t s = 0; s < part_.segments().size(); ++s) {
            for (const RegionId r : {part_.segments()[s].left, part_.segments()[s].right}) {
                if (r.valid()) {
                    a[r.index()].push_back(seg_[s].first);
                    b[r.index()].push_back(seg_[s].second);
                }
            }
        }
        for (std::size_t r = 0; r < nr; ++r) {
            index_.emplace_back(a[r], b[r]);
            const RegionId id = RegionId::from_index(r);
            for (const auto& comp : part_.components(id)) {
                LabelledPolygon2 lp;
                Box2 box;
                for (std::size_t l = 0; l < comp.loops.size(); ++l) {
                    LabelledLoop2 loop;
                    for (const auto& u : comp.loops[l]) {
                        loop.points.push_back(part_.start(u));
                        loop.labels.push_back(kSegmentBase + u.segment);
                        box.expand(part_.start(u));
                    }
                    (l == 0 ? lp.outer : lp.holes.emplace_back()) = std::move(loop);
                }
                components_[r].push_back(std::move(lp));
                component_boxes_[r].push_back(box);
            }
        }
    }

    std::vector<std::vector<std::uint32_t>> region_neighbours() const {
        std::vector<std::vector<std::uint32_t>> nb(sites_.size());
        std::size_t first = 0;
        while (first < sites_.size()) {
            std::size_t last = first;
            while (last < sites_.size() && region_[last] == region_[first]) ++last;
            const std::span<const Vec2> local(sites_.data() + first, last - first);
            for (const auto& [a, b] : backend_.delaunay_pairs(local)) {
                const auto ga = static_cast<std::uint32_t>(first + a.index());
                const auto gb = static_cast<std::uint32_t>(first + b.index());
                nb[ga].push_back(gb);
                nb[gb].push_back(ga);
            }
            first = last;
        }
        for (auto& row : nb) std::ranges::sort(row);
        return nb;
    }

    Vec2 circumcenter(std::uint32_t i, std::uint32_t j, std::uint32_t k) const {
        std::array<std::uint32_t, 3> id{i, j, k};
        std::ranges::sort(id);
        const Vec2 a = sites_[id[0]];
        const Vec2 b = sites_[id[1]] - a;
        const Vec2 c = sites_[id[2]] - a;
        const Real d = 2 * cross(b, c);
        const Real bb = dot(b, b);
        const Real cc = dot(c, c);
        return a + Vec2{(c[1] * bb - b[1] * cc) / d, (b[0] * cc - c[0] * bb) / d};
    }

    Vec2 bisector_cut(std::uint32_t i, std::uint32_t j, std::size_t s) const {
        const std::uint32_t a = std::min(i, j);
        const std::uint32_t b = std::max(i, j);
        const Vec2 m = 0.5 * (sites_[a] + sites_[b]);
        const Vec2 n = sites_[b] - sites_[a];
        const Vec2 e = seg_[s].second - seg_[s].first;
        const Real t = dot(m - seg_[s].first, n) / dot(e, n);
        return seg_[s].first + t * e;
    }

    Vec2 corner(std::size_t s, std::size_t t) const {
        const auto& [a, b] = seg_[s];
        const auto& [c, d] = seg_[t];
        if (a == c || a == d) return a;
        if (b == c || b == d) return b;
        const Vec2 e = b - a;
        const Vec2 f = d - c;
        return a + (cross(c - a, f) / cross(e, f)) * e;
    }

    /// Consecutive vertices closer than the point tolerance are one vertex
    /// (DEC-020). Near-cocircular sites (e.g. mirrored pairs) otherwise leave
    /// a tiny reversed edge that makes the polygon self-overlapping. The edge
    /// that disappears is the short one; vertices are recomputed canonically
    /// from the labels afterwards.
    void merge_close_vertices(LabelledLoop2& loop) {
        bool changed = true;
        while (changed && loop.points.size() > 3) {
            changed = false;
            const std::size_t n = loop.points.size();
            for (std::size_t k = 0; k < n; ++k) {
                const std::size_t prev = (k + n - 1) % n;
                if (norm(loop.points[k] - loop.points[prev]) <= tol_) {
                    // Drop vertex k and the short edge prev -> k (label of prev).
                    loop.labels[prev] = loop.labels[k];
                    ++out_.stats.collapsed_faces;
                    loop.points.erase(loop.points.begin() + static_cast<std::ptrdiff_t>(k));
                    loop.labels.erase(loop.labels.begin() + static_cast<std::ptrdiff_t>(k));
                    changed = true;
                    break;
                }
            }
        }
    }

    void canonicalise(std::uint32_t i, LabelledLoop2& loop) {
        LabelledLoop2 merged;
        const std::size_t n = loop.points.size();
        for (std::size_t k = 0; k < n; ++k) {
            if (loop.labels[k] == loop.labels[(k + n - 1) % n]) continue;
            merged.points.push_back(loop.points[k]);
            merged.labels.push_back(loop.labels[k]);
        }
        const std::size_t m = merged.points.size();
        for (std::size_t k = 0; k < m; ++k) {
            const EdgeLabel in = merged.labels[(k + m - 1) % m];
            const EdgeLabel out = merged.labels[k];
            Vec2& v = merged.points[k];
            if (is_site(in) && is_site(out)) {
                v = circumcenter(i, static_cast<std::uint32_t>(in), static_cast<std::uint32_t>(out));
            } else if (is_site(in) && is_segment(out)) {
                v = bisector_cut(i, static_cast<std::uint32_t>(in), segment_of(out));
            } else if (is_segment(in) && is_site(out)) {
                v = bisector_cut(i, static_cast<std::uint32_t>(out), segment_of(in));
            } else if (is_segment(in) && is_segment(out)) {
                v = corner(segment_of(in), segment_of(out));
            } else {
                ++out_.stats.unresolved_vertices;
            }
        }
        loop = std::move(merged);
    }

    Status add_cell(std::uint32_t i, const LabelledLoop2& convex) {
        const std::size_t r = region_[i].index();
        std::vector<LabelledLoop2> loops;
        const Box2 cell_box = Box2::of(convex.points);
        if (options_.fast_path && !index_[r].may_touch(cell_box)) {
            ++out_.stats.fast_cells;
            loops.push_back(convex);
        } else {
            ++out_.stats.clipped_cells;
            std::vector<LabelledPolygon2> near;
            for (std::size_t c = 0; c < components_[r].size(); ++c) {
                if (component_boxes_[r][c].overlaps(cell_box)) near.push_back(components_[r][c]);
            }
            RegionClip2 clip = backend_.clip_by_region(convex, near);
            if (!clip.error.empty()) return fail(ErrorCode::BackendFailure, clip.error, CellId{i});
            out_.stats.unlabelled_edges += clip.unlabelled_edges;
            if (clip.pieces.empty()) return fail(ErrorCode::InvariantViolated, "empty clipped cell", CellId{i});
            if (clip.pieces.size() > 1) {
                ++out_.stats.fragmented_cells;
                log(Error(ErrorCode::InvariantViolated, "cell made of several pieces (kept whole)", CellId{i},
                          Severity::Warning));
            }
            for (auto& piece : clip.pieces) {
                loops.push_back(std::move(piece.outer));
                for (auto& h : piece.holes) loops.push_back(std::move(h));
            }
        }
        for (auto& loop : loops) {
            canonicalise(i, loop);
            const std::size_t n = loop.points.size();
            Real area = 0;
            for (std::size_t k = 0; k < n; ++k) {
                const Vec2& p = loop.points[k];
                const Vec2& q = loop.points[(k + 1) % n];
                area += cross(p - sites_[i], q - sites_[i]);
                // Edges shorter than the point tolerance are degenerate (e.g. between
                // cocircular sites); their endpoints are merged in assemble().
                if (norm(q - p) <= tol_) {
                    ++out_.stats.collapsed_faces;
                    continue;
                }
                const EdgeLabel l = loop.labels[k];
                if (is_site(l)) {
                    const auto j = static_cast<std::uint32_t>(l);
                    const std::uint64_t key = (std::uint64_t{std::min(i, j)} << 32) | std::max(i, j);
                    auto& entry = bisector_pieces_[key];
                    (i < j ? entry.first : entry.second).emplace_back(p, q);
                } else if (is_segment(l) && part_.is_interface(segment_of(l))) {
                    interface_pieces_[segment_of(l)].push_back({i, region_[i], p, q});
                    ++out_.stats.interface_pieces;
                } else if (is_segment(l)) {
                    raw_.push_back({i, kNone, part_.segments()[segment_of(l)].patch.value, p, q});
                } else {
                    ++out_.stats.unresolved_vertices;
                }
            }
            out_.cell_polygon_area[i] += 0.5 * area;
        }
        return {};
    }

    void emit_internal_faces() {
        std::vector<std::uint64_t> keys;
        keys.reserve(bisector_pieces_.size());
        for (const auto& [k, v] : bisector_pieces_) keys.push_back(k);
        std::ranges::sort(keys);
        for (const std::uint64_t key : keys) {
            auto& [own, other] = bisector_pieces_[key];
            for (const auto& [p, q] : own) {
                // Same geometry from both cells, within the point tolerance (vertices of
                // degree > 3 have no unique canonical construction).
                const auto match = std::ranges::find_if(
                    other, [&](const auto& e) { return norm(e.first - q) <= tol_ && norm(e.second - p) <= tol_; });
                if (match == other.end()) {
                    ++out_.stats.unmatched_bisector_pieces;
                } else {
                    other.erase(match);
                }
                raw_.push_back({static_cast<std::uint32_t>(key >> 32), static_cast<std::uint32_t>(key & 0xffffffffu), kNone, p, q});
            }
            out_.stats.unmatched_bisector_pieces += other.size();
        }
    }

    void emit_interface_faces(std::size_t s) {
        const auto& pieces = interface_pieces_[s];
        if (pieces.empty()) return;
        const Vec2 a = seg_[s].first;
        const Vec2 e = seg_[s].second - a;
        const Real ee = dot(e, e);
        const Real tol_t = tol_ / std::sqrt(ee);
        auto param = [&](const Vec2& x) { return dot(x - a, e) / ee; };
        std::vector<std::pair<Real, Vec2>> ends;
        for (const auto& pc : pieces) {
            ends.emplace_back(param(pc.p), pc.p);
            ends.emplace_back(param(pc.q), pc.q);
        }
        std::ranges::sort(ends);
        std::vector<Real> cluster_t;
        std::vector<Vec2> cluster_x;
        for (const auto& [t, x] : ends) {
            if (!cluster_t.empty() && t - cluster_t.back() <= tol_t) {
                if (x != cluster_x.back()) {
                    ++out_.stats.merged_breakpoints;
                    cluster_x.back() = std::min(cluster_x.back(), x);
                }
                continue;
            }
            cluster_t.push_back(t);
            cluster_x.push_back(x);
        }
        auto cluster_of = [&](Real t) {
            return static_cast<std::size_t>(std::ranges::upper_bound(cluster_t, t) - cluster_t.begin()) - 1;
        };
        for (std::size_t c = 0; c + 1 < cluster_t.size(); ++c) {
            std::vector<const Piece*> cover;
            for (const auto& pc : pieces) {
                const std::size_t i0 = cluster_of(param(pc.p));
                const std::size_t i1 = cluster_of(param(pc.q));
                if (std::min(i0, i1) <= c && c < std::max(i0, i1)) cover.push_back(&pc);
            }
            if (cover.size() != 2 || cover[0]->region == cover[1]->region) {
                ++out_.stats.interface_bad_coverage;
                continue;
            }
            const Piece* own = cover[0]->cell < cover[1]->cell ? cover[0] : cover[1];
            const Piece* nb = own == cover[0] ? cover[1] : cover[0];
            const bool forward = param(own->p) < param(own->q);
            raw_.push_back({own->cell, nb->cell, kNone, forward ? cluster_x[c] : cluster_x[c + 1],
                            forward ? cluster_x[c + 1] : cluster_x[c]});
        }
    }

    /// Vertex table: points closer than the tolerance become one vertex (the
    /// lexicographically smallest); faces that collapse are removed.
    Result<Mesh2D> assemble() {
        std::vector<Vec2> pts;
        pts.reserve(2 * raw_.size());
        for (const auto& f : raw_) {
            pts.push_back(f.p);
            pts.push_back(f.q);
        }
        std::ranges::sort(pts);
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        // Cluster within the tolerance with a hash grid of cell tol.
        const Real cell = tol_ > 0 ? tol_ : 1;
        auto key = [&](Real x, Real y) {
            return std::pair(static_cast<std::int64_t>(std::floor(x / cell)), static_cast<std::int64_t>(std::floor(y / cell)));
        };
        std::map<std::pair<std::int64_t, std::int64_t>, std::vector<std::uint32_t>> grid;
        std::vector<std::uint32_t> rep(pts.size());
        std::vector<Vec2> reps;
        for (std::size_t k = 0; k < pts.size(); ++k) {
            const auto [cx, cy] = key(pts[k][0], pts[k][1]);
            std::uint32_t found = std::numeric_limits<std::uint32_t>::max();
            for (std::int64_t dy = -1; dy <= 1 && found == std::numeric_limits<std::uint32_t>::max(); ++dy) {
                for (std::int64_t dx = -1; dx <= 1; ++dx) {
                    const auto it = grid.find({cx + dx, cy + dy});
                    if (it == grid.end()) continue;
                    for (const std::uint32_t r : it->second) {
                        if (norm(reps[r] - pts[k]) <= tol_) {
                            found = r;
                            break;
                        }
                    }
                    if (found != std::numeric_limits<std::uint32_t>::max()) break;
                }
            }
            if (found == std::numeric_limits<std::uint32_t>::max()) {
                found = static_cast<std::uint32_t>(reps.size());
                reps.push_back(pts[k]);
                grid[{cx, cy}].push_back(found);
            } else {
                ++out_.stats.merged_vertices;
            }
            rep[k] = found;
        }
        auto vertex_of = [&](const Vec2& x) {
            const auto it = std::ranges::lower_bound(pts, x);
            return rep[static_cast<std::size_t>(it - pts.begin())];
        };
        struct Face {
            std::uint32_t patch;
            std::uint32_t owner;
            std::uint32_t neighbour;
            std::uint32_t v0;
            std::uint32_t v1;
        };
        std::vector<Face> faces;
        faces.reserve(raw_.size());
        for (const auto& f : raw_) {
            const std::uint32_t v0 = vertex_of(f.p);
            const std::uint32_t v1 = vertex_of(f.q);
            if (v0 == v1) {
                ++out_.stats.collapsed_faces;
                continue;
            }
            faces.push_back({f.patch, f.owner, f.neighbour, v0, v1});
        }
        auto order_key = [&](const Face& f) {
            const bool internal = f.neighbour != kNone;
            return std::tuple(internal ? 0u : 1u, internal ? 0u : f.patch, f.owner, f.neighbour, reps[f.v0], reps[f.v1]);
        };
        std::ranges::sort(faces, [&](const Face& x, const Face& y) { return order_key(x) < order_key(y); });

        MeshData<2> d;
        d.points = std::move(reps);
        d.sites = sites_;
        d.cell_region = region_;
        d.cell_input_site = input_id_;
        for (const auto& r : part_.regions()) d.regions.push_back({r.name, r.medium});
        d.media = part_.media();
        std::vector<std::size_t> patch_count(part_.patches().size(), 0);
        for (const auto& f : faces) {
            const VertexId verts[2] = {VertexId{f.v0}, VertexId{f.v1}};
            d.face_vertices.push_row(verts);
            d.owner.push_back(CellId{f.owner});
            if (f.neighbour != kNone) {
                d.neighbour.push_back(CellId{f.neighbour});
            } else {
                ++patch_count[f.patch];
            }
        }
        std::size_t start = d.neighbour.size();
        for (std::size_t p = 0; p < part_.patches().size(); ++p) {
            d.patches.push_back({part_.patches()[p], start, patch_count[p]});
            start += patch_count[p];
        }
        return Mesh2D::from_data(std::move(d));
    }

    static constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();

    const Partition2D& part_;
    const SiteSet& input_;
    const Backend2D& backend_;
    BuildOptions2D options_;
    Real tol_;
    Build2D out_;
    std::vector<Vec2> sites_;
    std::vector<RegionId> region_;
    std::vector<SiteId> input_id_;
    std::vector<std::pair<Vec2, Vec2>> seg_;
    std::vector<SegmentIndex2> index_;
    std::vector<std::vector<LabelledPolygon2>> components_;
    std::vector<std::vector<Box2>> component_boxes_;
    std::unordered_map<std::uint64_t, std::pair<std::vector<std::pair<Vec2, Vec2>>, std::vector<std::pair<Vec2, Vec2>>>>
        bisector_pieces_;
    std::vector<std::vector<Piece>> interface_pieces_;
    std::vector<RawFace> raw_;
};

}  // namespace

Result<Build2D> build_mesh_2d(const Partition2D& partition, const SiteSet& sites, const Backend2D& backend,
                              const BuildOptions2D& options) {
    if (!backend.complete()) return fail(ErrorCode::InvalidArgument, "incomplete backend");
    const auto tol = Tolerance::from_length(partition.length_scale(), options.relative_tolerance);
    if (!tol) return fail(ErrorCode::InvalidLengthScale, "partition without extent");
    if (auto ok = validate_sites(partition, sites); !ok) return std::unexpected(ok.error());
    return Builder(partition, sites, backend, options, tol->point()).run();
}

InvariantReference invariant_reference(const Partition2D& p) {
    InvariantReference ref;
    ref.length_scale = p.length_scale();
    ref.total_measure = p.total_area();
    for (std::size_t r = 0; r < p.region_count(); ++r) ref.region_measure.push_back(p.region_area(RegionId::from_index(r)));
    ref.boundary_measure = p.boundary_length();
    for (std::size_t s = 0; s < p.segments().size(); ++s) {
        if (!p.is_interface(s)) continue;
        const std::size_t a = p.segments()[s].left.index();
        const std::size_t b = p.segments()[s].right.index();
        ref.interface_measure[{std::min(a, b), std::max(a, b)}] += p.segment_length(s);
    }
    return ref;
}

}  // namespace vmm
