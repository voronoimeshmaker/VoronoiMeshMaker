// ============================================================================
// File: voronoi2d.cpp
// Description: P05a prototype - 2D multi-region Voronoi mesh (no CGAL here).
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
#include <span>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/voronoi2d.hpp>

namespace vmm::p05a {
namespace {

bool is_site(EdgeLabel l) { return l < segment_label_base; }
bool is_segment(EdgeLabel l) { return l >= segment_label_base && l < box_label_base; }
std::size_t segment_index(EdgeLabel l) { return static_cast<std::size_t>(l - segment_label_base); }

/// Line through the bisector of two sites, built from the ordered pair so
/// both cells obtain the same doubles.
struct Line2 {
    Vec2 point;
    Vec2 normal;
};

Line2 bisector(std::span<const Vec2> sites, std::uint32_t i, std::uint32_t j) {
    if (j < i) std::swap(i, j);
    return {0.5 * (sites[i] + sites[j]), sites[j] - sites[i]};
}

Vec2 circumcenter(std::span<const Vec2> sites, std::uint32_t i, std::uint32_t j, std::uint32_t k) {
    std::uint32_t id[3] = {i, j, k};
    std::sort(id, id + 3);
    const Vec2 a = sites[id[0]];
    const Vec2 b = sites[id[1]] - a;
    const Vec2 c = sites[id[2]] - a;
    const Real d = 2 * cross(b, c);
    const Real bb = dot(b, b);
    const Real cc = dot(c, c);
    return a + Vec2{(c[1] * bb - b[1] * cc) / d, (b[0] * cc - c[0] * bb) / d};
}

Vec2 intersect(const Line2& line, const Segment2& s) {
    const Vec2 e = s.b - s.a;
    const Real t = dot(line.point - s.a, line.normal) / dot(e, line.normal);
    return s.a + t * e;
}

Vec2 segment_corner(const Segment2& s, const Segment2& t) {
    if (s.a == t.a || s.a == t.b) return s.a;
    if (s.b == t.a || s.b == t.b) return s.b;
    const Line2 line{t.a, Vec2{t.b[1] - t.a[1], t.a[0] - t.b[0]}};
    return intersect(line, s);
}

LabelledLoop2 box_loop(const Vec2& lo, const Vec2& hi) {
    return {{lo, Vec2{hi[0], lo[1]}, hi, Vec2{lo[0], hi[1]}},
            {box_label_base + 0, box_label_base + 1, box_label_base + 2, box_label_base + 3}};
}

/// Sutherland-Hodgman step for a convex loop: keeps dot(x - line.point, line.normal) <= 0.
/// The new edge on the clip line gets `label`.
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
            if (in_p) {
                pts.push_back(x);
                incoming.push_back(poly.labels[k]);
            } else {
                pts.push_back(x);
                incoming.push_back(label);
                pts.push_back(q);
                incoming.push_back(poly.labels[k]);
            }
        }
    }
    LabelledLoop2 out;
    const std::size_t m = pts.size();
    for (std::size_t k = 0; k < m; ++k) {
        if (m > 1 && pts[k] == pts[(k + m - 1) % m]) continue;  // zero-length edge
        out.points.push_back(pts[k]);
        out.labels.push_back(incoming[k]);
    }
    // incoming label of vertex k+1 is the outgoing label of vertex k
    std::rotate(out.labels.begin(), out.labels.begin() + (out.labels.empty() ? 0 : 1), out.labels.end());
    return out;
}

Real loop_area(const std::vector<Vec2>& pts, const Vec2& origin) {
    Real a = 0;
    for (std::size_t k = 0; k < pts.size(); ++k) {
        a += cross(pts[k] - origin, pts[(k + 1) % pts.size()] - origin);
    }
    return 0.5 * a;
}

struct Piece {
    CellId cell;
    RegionId region;
    Vec2 p;
    Vec2 q;
};

class Builder {
public:
    Builder(const Layout2D& layout, std::span<const Vec2> sites, std::span<const RegionId> site_region,
            const Backend2D& backend, Real tolerance)
        : layout_(layout), sites_(sites), region_(site_region), backend_(backend), tol_(tolerance) {}

    Build2D run() {
        out_.mesh.sites.assign(sites_.begin(), sites_.end());
        out_.mesh.cell_region.assign(region_.begin(), region_.end());
        out_.cell_polygon_area.assign(sites_.size(), 0);
        interface_pieces_.resize(layout_.segments.size());
        const auto neighbours = region_neighbours();
        const auto [lo, hi] = working_box();
        for (std::uint32_t i = 0; i < sites_.size(); ++i) {
            LabelledLoop2 cell = box_loop(lo, hi);
            for (const std::uint32_t j : neighbours[i]) {
                // Canonical midpoint; the normal points from i to j so that i's side is kept.
                const Line2 line{bisector(sites_, i, j).point, sites_[j] - sites_[i]};
                cell = clip_halfplane(cell, line, j);
            }
            add_cell(i, cell);
        }
        emit_internal_faces();
        for (std::size_t s = 0; s < layout_.segments.size(); ++s) emit_interface_faces(s);
        return std::move(out_);
    }

private:
    std::vector<std::vector<std::uint32_t>> region_neighbours() const {
        std::vector<std::vector<std::uint32_t>> nb(sites_.size());
        for (std::size_t r = 0; r < layout_.regions.size(); ++r) {
            std::vector<std::uint32_t> members;
            std::vector<Vec2> local;
            for (std::uint32_t i = 0; i < sites_.size(); ++i) {
                if (region_[i].index() == r) {
                    members.push_back(i);
                    local.push_back(sites_[i]);
                }
            }
            for (const auto& [a, b] : backend_.delaunay_pairs(local)) {
                nb[members[a.index()]].push_back(members[b.index()]);
                nb[members[b.index()]].push_back(members[a.index()]);
            }
        }
        for (auto& row : nb) std::sort(row.begin(), row.end());
        return nb;
    }

    std::pair<Vec2, Vec2> working_box() const {
        Vec2 lo{std::numeric_limits<Real>::max(), std::numeric_limits<Real>::max()};
        Vec2 hi{-lo[0], -lo[1]};
        for (const auto& s : layout_.segments) {
            for (const Vec2& p : {s.a, s.b}) {
                for (std::size_t k = 0; k < 2; ++k) {
                    lo[k] = std::min(lo[k], p[k]);
                    hi[k] = std::max(hi[k], p[k]);
                }
            }
        }
        const Vec2 pad = 0.1 * (hi - lo);
        return {lo - pad, hi + pad};
    }

    /// Replaces every vertex by the canonical construction of its two edge labels.
    void canonicalise(std::uint32_t i, LabelledLoop2& loop) {
        // Merge consecutive edges with the same label (collinear split points).
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
                v = circumcenter(sites_, i, static_cast<std::uint32_t>(in), static_cast<std::uint32_t>(out));
            } else if (is_site(in) && is_segment(out)) {
                v = intersect(bisector(sites_, i, static_cast<std::uint32_t>(in)), layout_.segments[segment_index(out)]);
            } else if (is_segment(in) && is_site(out)) {
                v = intersect(bisector(sites_, i, static_cast<std::uint32_t>(out)), layout_.segments[segment_index(in)]);
            } else if (is_segment(in) && is_segment(out)) {
                v = segment_corner(layout_.segments[segment_index(in)], layout_.segments[segment_index(out)]);
            } else {
                ++out_.stats.unresolved_vertices;
            }
        }
        loop = std::move(merged);
    }

    void add_cell(std::uint32_t i, const LabelledLoop2& convex) {
        const RegionId r = region_[i];
        RegionClip2 clip = backend_.clip_by_region(convex, layout_.regions[r.index()]);
        out_.stats.unlabelled_edges += clip.unlabelled_edges;
        out_.stats.ambiguous_edges += clip.ambiguous_edges;
        if (clip.pieces.size() != 1) ++out_.stats.fragmented_cells;
        const CellId c{i};
        for (auto& piece : clip.pieces) {
            std::vector<LabelledLoop2*> loops{&piece.outer};
            for (auto& h : piece.holes) loops.push_back(&h);
            for (LabelledLoop2* loop : loops) {
                canonicalise(i, *loop);
                out_.cell_polygon_area[i] += loop_area(loop->points, sites_[i]);
                const std::size_t n = loop->points.size();
                for (std::size_t k = 0; k < n; ++k) {
                    const Vec2& p = loop->points[k];
                    const Vec2& q = loop->points[(k + 1) % n];
                    const EdgeLabel l = loop->labels[k];
                    if (is_site(l)) {
                        const auto j = static_cast<std::uint32_t>(l);
                        auto& entry = bisector_pieces_[{std::min(i, j), std::max(i, j)}];
                        (i < j ? entry.first : entry.second).push_back({p, q});
                    } else if (is_segment(l) && layout_.segment_patch[segment_index(l)] == layout_.interface_patch) {
                        interface_pieces_[segment_index(l)].push_back({c, r, p, q});
                        ++out_.stats.interface_pieces;
                    } else if (is_segment(l)) {
                        const Vec2 pts[2] = {p, q};
                        out_.mesh.add_face(c, CellId::invalid(), layout_.segment_patch[segment_index(l)], pts);
                    } else {
                        ++out_.stats.unresolved_vertices;
                    }
                }
            }
        }
    }

    void emit_internal_faces() {
        for (auto& [key, sides] : bisector_pieces_) {
            auto& other = sides.second;
            for (const auto& [p, q] : sides.first) {
                const auto match = std::find_if(other.begin(), other.end(),
                                                [&](const auto& e) { return e.first == q && e.second == p; });
                if (match == other.end()) {
                    ++out_.stats.unmatched_internal_pieces;
                } else {
                    other.erase(match);
                }
                const Vec2 pts[2] = {p, q};
                out_.mesh.add_face(CellId{key.first}, CellId{key.second}, PatchId::invalid(), pts);
            }
            out_.stats.unmatched_internal_pieces += other.size();
        }
    }

    /// Common refinement of one interface segment: breakpoints from both sides
    /// are merged within the point tolerance; every sub-interval must be
    /// covered by exactly one piece of each region.
    void emit_interface_faces(std::size_t s) {
        const auto& pieces = interface_pieces_[s];
        if (pieces.empty()) return;
        const Segment2& seg = layout_.segments[s];
        const Vec2 e = seg.b - seg.a;
        const Real ee = dot(e, e);
        const Real tol_t = tol_ / std::sqrt(ee);
        auto param = [&](const Vec2& x) { return dot(x - seg.a, e) / ee; };

        std::vector<std::pair<Real, Vec2>> ends;
        for (const auto& pc : pieces) {
            ends.emplace_back(param(pc.p), pc.p);
            ends.emplace_back(param(pc.q), pc.q);
        }
        std::sort(ends.begin(), ends.end());
        std::vector<Real> cluster_t;   // smallest parameter of each cluster
        std::vector<Vec2> cluster_x;   // representative point: lexicographic minimum
        for (std::size_t k = 0; k < ends.size(); ++k) {
            if (!cluster_t.empty() && ends[k].first - cluster_t.back() <= tol_t) {
                if (ends[k].second != cluster_x.back()) {
                    ++out_.stats.merged_breakpoints;
                    out_.stats.max_merge_distance =
                        std::max(out_.stats.max_merge_distance, norm(ends[k].second - cluster_x.back()));
                    cluster_x.back() = std::min(cluster_x.back(), ends[k].second);
                }
                continue;
            }
            cluster_t.push_back(ends[k].first);
            cluster_x.push_back(ends[k].second);
        }
        auto cluster_of = [&](Real t) {
            const auto it = std::upper_bound(cluster_t.begin(), cluster_t.end(), t);
            return static_cast<std::size_t>(it - cluster_t.begin()) - 1;
        };
        for (std::size_t c = 0; c + 1 < cluster_t.size(); ++c) {
            std::vector<const Piece*> cover;
            for (const auto& pc : pieces) {
                const std::size_t a = cluster_of(param(pc.p));
                const std::size_t b = cluster_of(param(pc.q));
                if (std::min(a, b) <= c && c < std::max(a, b)) cover.push_back(&pc);
            }
            if (cover.size() != 2 || cover[0]->region == cover[1]->region) {
                ++out_.stats.interface_bad_coverage;
                continue;
            }
            const Piece* own = cover[0]->cell < cover[1]->cell ? cover[0] : cover[1];
            const Piece* nb = own == cover[0] ? cover[1] : cover[0];
            const bool forward = param(own->p) < param(own->q);
            const Vec2 pts[2] = {forward ? cluster_x[c] : cluster_x[c + 1], forward ? cluster_x[c + 1] : cluster_x[c]};
            out_.mesh.add_face(own->cell, nb->cell, PatchId::invalid(), pts);
        }
    }

    const Layout2D& layout_;
    std::span<const Vec2> sites_;
    std::span<const RegionId> region_;
    const Backend2D& backend_;
    Real tol_;
    Build2D out_;
    std::map<std::pair<std::uint32_t, std::uint32_t>,
             std::pair<std::vector<std::pair<Vec2, Vec2>>, std::vector<std::pair<Vec2, Vec2>>>>
        bisector_pieces_;
    std::vector<std::vector<Piece>> interface_pieces_;
};

}  // namespace

Build2D build_multiregion_mesh_2d(const Layout2D& layout, std::span<const Vec2> sites,
                                  std::span<const RegionId> site_region, const Backend2D& backend,
                                  Real point_tolerance) {
    return Builder(layout, sites, site_region, backend, point_tolerance).run();
}

}  // namespace vmm::p05a
