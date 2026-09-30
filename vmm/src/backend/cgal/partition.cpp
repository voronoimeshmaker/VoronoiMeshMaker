// ============================================================================
// File: partition.cpp
// Description: Declaration2D -> Partition2D with an exact CGAL arrangement.
//              Every layer edge is inserted with its record id; the layers
//              covering a face follow from crossing parity starting at the
//              unbounded face (exact, no point location with tolerances).
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <numeric>
#include <queue>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <CGAL/Arr_consolidated_curve_data_traits_2.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "backend_cgal.hpp"

namespace vmm::cgal_detail {
namespace {

using K = CGAL::Exact_predicates_exact_constructions_kernel;
using SegmentTraits = CGAL::Arr_segment_traits_2<K>;
using Traits = CGAL::Arr_consolidated_curve_data_traits_2<SegmentTraits, std::uint32_t>;
using Dcel = CGAL::Arr_face_extended_dcel<Traits, int>;
using Arrangement = CGAL::Arrangement_2<Traits, Dcel>;
using Halfedge = Arrangement::Halfedge_const_handle;
using Face = Arrangement::Face_const_handle;

constexpr int kOutside = -1;
constexpr int kVoid = -2;

struct Record {
    std::size_t layer;
    std::string tag;
    bool hole_ring;  ///< edge of a hole ring of its shape
};

struct UnionFind {
    std::vector<std::size_t> parent;
    explicit UnionFind(std::size_t n) : parent(n) { std::iota(parent.begin(), parent.end(), std::size_t{0}); }
    std::size_t find(std::size_t x) {
        while (parent[x] != x) x = parent[x] = parent[parent[x]];
        return x;
    }
    void unite(std::size_t a, std::size_t b) { parent[find(a)] = find(b); }
};

class Builder {
public:
    explicit Builder(const Declaration2D& d) : decl_(d) {}

    Result<Partition2D> run() {
        if (decl_.layers().empty()) return fail(ErrorCode::EmptyDeclaration);
        insert_layers();
        label_faces();
        build_segments();
        return assemble();
    }

private:
    void add_ring(std::size_t layer, const std::vector<Vec2>& ring, const std::vector<std::string>& tags,
                  bool hole_ring, std::vector<Traits::Curve_2>& curves) {
        for (std::size_t k = 0; k < ring.size(); ++k) {
            const K::Point_2 p(ring[k][0], ring[k][1]);
            const K::Point_2 q(ring[(k + 1) % ring.size()][0], ring[(k + 1) % ring.size()][1]);
            if (p == q) continue;
            records_.push_back({layer, tags[k], hole_ring});
            curves.emplace_back(SegmentTraits::Curve_2(p, q), static_cast<std::uint32_t>(records_.size() - 1));
        }
    }

    void insert_layers() {
        std::vector<Traits::Curve_2> curves;
        for (std::size_t l = 0; l < decl_.layers().size(); ++l) {
            const auto& o = decl_.layers()[l].outline;
            add_ring(l, o.outer(), o.outer_tags(), false, curves);
            for (std::size_t h = 0; h < o.holes().size(); ++h) add_ring(l, o.holes()[h], o.hole_tags()[h], true, curves);
        }
        CGAL::insert(arr_, curves.begin(), curves.end());
    }

    /// Breadth-first search over faces; crossing an edge flips the inside
    /// state of every layer owning an odd number of the edge's records.
    void label_faces() {
        std::vector<Face> faces;
        for (auto f = arr_.faces_begin(); f != arr_.faces_end(); ++f) {
            f->set_data(static_cast<int>(faces.size()));
            faces.push_back(f);
        }
        const std::size_t layers = decl_.layers().size();
        // inside[f][l]: face f covered by layer l; in_hole[f][l]: face f inside a hole ring of layer l.
        std::vector<std::vector<char>> inside(faces.size(), std::vector<char>(layers, 0));
        std::vector<std::vector<char>> in_hole(faces.size(), std::vector<char>(layers, 0));
        std::vector<char> seen(faces.size(), 0);
        std::queue<Face> queue;
        const Face start = arr_.unbounded_face();
        seen[static_cast<std::size_t>(start->data())] = 1;
        queue.push(start);
        auto visit_ccb = [&](Face f, Arrangement::Ccb_halfedge_const_circulator first) {
            auto h = first;
            do {
                const Face g = h->twin()->face();
                const auto gi = static_cast<std::size_t>(g->data());
                if (!seen[gi]) {
                    seen[gi] = 1;
                    inside[gi] = inside[static_cast<std::size_t>(f->data())];
                    in_hole[gi] = in_hole[static_cast<std::size_t>(f->data())];
                    for (const std::uint32_t rec : h->curve().data()) {
                        const std::size_t l = records_[rec].layer;
                        inside[gi][l] = static_cast<char>(inside[gi][l] ^ 1);
                        if (records_[rec].hole_ring) in_hole[gi][l] = static_cast<char>(in_hole[gi][l] ^ 1);
                    }
                    queue.push(g);
                }
            } while (++h != first);
        };
        while (!queue.empty()) {
            const Face f = queue.front();
            queue.pop();
            if (!f->is_unbounded()) visit_ccb(f, f->outer_ccb());
            for (auto ic = f->inner_ccbs_begin(); ic != f->inner_ccbs_end(); ++ic) visit_ccb(f, *ic);
        }
        label_.assign(faces.size(), kOutside);
        const RegionId background = decl_.background();
        for (std::size_t i = 0; i < faces.size(); ++i) {
            if (faces[i]->is_unbounded()) continue;
            int label = kVoid;
            for (std::size_t l = layers; l-- > 0;) {
                if (inside[i][l]) {
                    const RegionId r = decl_.layers()[l].region;
                    label = r.valid() ? static_cast<int>(r.index()) : kOutside;
                    break;
                }
            }
            // Inside a shape's own hole ring and covered by nothing else: an intended hole.
            if (label == kVoid && std::ranges::any_of(in_hole[i], [](char c) { return c != 0; })) label = kOutside;
            if (label == kVoid && background.valid()) label = static_cast<int>(background.index());
            label_[i] = label;
        }
    }

    int label(Face f) const { return label_[static_cast<std::size_t>(f->data())]; }

    /// Keeps the edges separating different labels; vertices sorted
    /// lexicographically and segments by vertex pair (determinism).
    void build_segments() {
        std::vector<Halfedge> kept;
        std::map<const void*, Vec2> coords;
        for (auto e = arr_.edges_begin(); e != arr_.edges_end(); ++e) {
            Halfedge h = e;
            if (label(h->face()) == label(h->twin()->face())) continue;
            kept.push_back(h);
            for (const auto& v : {h->source(), h->target()}) {
                coords.emplace(&*v, Vec2{CGAL::to_double(v->point().x()), CGAL::to_double(v->point().y())});
            }
        }
        std::vector<std::pair<Vec2, const void*>> order;
        for (const auto& [ptr, p] : coords) order.emplace_back(p, ptr);
        std::ranges::sort(order);
        for (std::size_t i = 0; i < order.size(); ++i) {
            vertex_id_[order[i].second] = static_cast<std::uint32_t>(i);
            vertices_.push_back(order[i].first);
        }
        struct Raw {
            std::uint32_t v0, v1;
            Halfedge forward;
        };
        std::vector<Raw> raw;
        for (Halfedge h : kept) {
            std::uint32_t a = vertex_id_.at(&*h->source());
            std::uint32_t b = vertex_id_.at(&*h->target());
            if (b < a) {
                h = h->twin();
                std::swap(a, b);
            }
            raw.push_back({a, b, h});
        }
        std::ranges::sort(raw, [](const Raw& x, const Raw& y) { return std::pair(x.v0, x.v1) < std::pair(y.v0, y.v1); });
        for (std::size_t s = 0; s < raw.size(); ++s) {
            const Halfedge h = raw[s].forward;
            PartitionSegment seg;
            seg.v0 = raw[s].v0;
            seg.v1 = raw[s].v1;
            const int left = label(h->face());
            const int right = label(h->twin()->face());
            seg.left = left >= 0 ? RegionId::from_index(static_cast<std::size_t>(left)) : RegionId::invalid();
            seg.right = right >= 0 ? RegionId::from_index(static_cast<std::size_t>(right)) : RegionId::invalid();
            if (seg.left.valid() != seg.right.valid()) seg.patch = patch_of(h);
            segments_.push_back(seg);
            use_[&*h] = {static_cast<std::uint32_t>(s), false};
            use_[&*h->twin()] = {static_cast<std::uint32_t>(s), true};
        }
    }

    PatchId patch_of(Halfedge h) {
        std::string name = "boundary";
        std::size_t best = 0;
        bool found = false;
        for (const std::uint32_t rec : h->curve().data()) {
            const Record& r = records_[rec];
            if (!r.tag.empty() && (!found || r.layer > best)) {
                best = r.layer;
                name = r.tag;
                found = true;
            }
        }
        const auto it = std::ranges::find(patches_, name);
        if (it != patches_.end()) return PatchId::from_index(static_cast<std::size_t>(it - patches_.begin()));
        patches_.push_back(name);
        return PatchId::from_index(patches_.size() - 1);
    }

    /// Loops of the faces labelled `target`, grouped by connected component.
    std::vector<RegionComponent> components_of(int target) {
        std::vector<Face> faces;
        std::map<const void*, std::size_t> local;
        for (auto f = arr_.faces_begin(); f != arr_.faces_end(); ++f) {
            if (label(f) == target) {
                local[&*f] = faces.size();
                faces.push_back(f);
            }
        }
        if (faces.empty()) return {};
        UnionFind uf(faces.size());
        for (auto e = arr_.edges_begin(); e != arr_.edges_end(); ++e) {
            if (label(e->face()) == target && label(e->twin()->face()) == target) {
                uf.unite(local.at(&*e->face()), local.at(&*e->twin()->face()));
            }
        }
        std::map<std::size_t, std::vector<std::vector<SegmentUse>>> loops_by_root;
        std::map<const void*, char> visited;
        for (auto h = arr_.halfedges_begin(); h != arr_.halfedges_end(); ++h) {
            if (label(h->face()) != target || label(h->twin()->face()) == target || visited.contains(&*h)) continue;
            std::vector<SegmentUse> loop;
            Halfedge cur = h;
            do {
                visited[&*cur] = 1;
                loop.push_back(use_.at(&*cur));
                Halfedge next = cur->next();
                while (label(next->twin()->face()) == target) next = next->twin()->next();
                cur = next;
            } while (cur != Halfedge(h));
            std::ranges::rotate(loop, std::ranges::min_element(loop, {}, &SegmentUse::segment));
            loops_by_root[uf.find(local.at(&*h->face()))].push_back(std::move(loop));
        }
        std::vector<RegionComponent> comps;
        for (auto& [root, loops] : loops_by_root) {
            RegionComponent comp;
            std::vector<std::vector<SegmentUse>> holes;
            for (auto& loop : loops) {
                if (signed_area(points_of(loop)) > 0) {
                    comp.loops.insert(comp.loops.begin(), std::move(loop));
                } else {
                    holes.push_back(std::move(loop));
                }
            }
            std::ranges::sort(holes, {}, [](const auto& l) { return l.front().segment; });
            for (auto& h : holes) comp.loops.push_back(std::move(h));
            comps.push_back(std::move(comp));
        }
        std::ranges::sort(comps, {}, [](const RegionComponent& c) { return c.loops.front().front().segment; });
        return comps;
    }

    std::vector<Vec2> points_of(const std::vector<SegmentUse>& loop) const {
        std::vector<Vec2> pts;
        for (const auto& u : loop) {
            const auto& s = segments_[u.segment];
            pts.push_back(vertices_[u.reversed ? s.v1 : s.v0]);
        }
        return pts;
    }

    Result<Partition2D> assemble() {
        std::vector<std::vector<RegionComponent>> components;
        for (std::size_t r = 0; r < decl_.regions().size(); ++r) components.push_back(components_of(static_cast<int>(r)));
        std::vector<PolygonWithHoles2> voids;
        for (const auto& comp : components_of(kVoid)) {
            std::vector<std::vector<Vec2>> holes;
            for (std::size_t k = 1; k < comp.loops.size(); ++k) holes.push_back(points_of(comp.loops[k]));
            voids.emplace_back(points_of(comp.loops[0]), std::move(holes));
        }
        return Partition2D(std::move(vertices_), std::move(segments_), decl_.regions(), decl_.media().names(),
                           std::move(patches_), std::move(components), std::move(voids));
    }

    const Declaration2D& decl_;
    Arrangement arr_;
    std::vector<Record> records_;
    std::vector<int> label_;
    std::map<const void*, std::uint32_t> vertex_id_;
    std::map<const void*, SegmentUse> use_;
    std::vector<Vec2> vertices_;
    std::vector<PartitionSegment> segments_;
    std::vector<std::string> patches_;
};

}  // namespace

Result<Partition2D> build_partition(const Declaration2D& declaration) {
    try {
        return Builder(declaration).run();
    } catch (const std::exception& e) {
        return fail(ErrorCode::BackendFailure, e.what());
    }
}

}  // namespace vmm::cgal_detail
