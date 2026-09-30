// ============================================================================
// File: backend3d.cpp
// Description: CGAL implementation of Backend3D (P16): partition checks,
//              Delaunay 3D (Epick), exact circumcentres and the exact
//              labelled clipping of a convex cell by a region (Epeck,
//              Polygon Mesh Processing corefinement, P15a): against the
//              region triangles near the cell (P17), or against the whole
//              region when the local result is not certain.
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <CGAL/AABB_traits_3.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/boost/graph/helpers.h>
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
#include "backend_cgal.hpp"


namespace vmm::cgal_detail {
namespace {

namespace PMP = CGAL::Polygon_mesh_processing;
using Epick = CGAL::Exact_predicates_inexact_constructions_kernel;
using Epeck = CGAL::Exact_predicates_exact_constructions_kernel;
using IMesh = CGAL::Surface_mesh<Epick::Point_3>;
using EMesh = CGAL::Surface_mesh<Epeck::Point_3>;
using Primitive = CGAL::AABB_face_graph_triangle_primitive<IMesh>;
using Tree = CGAL::AABB_tree<CGAL::AABB_traits_3<Epick, Primitive>>;
using LabelMap = EMesh::Property_map<EMesh::Face_index, FaceLabel>;

constexpr FaceLabel kUnlabelled = kBoxFace | 0xffffu;
constexpr const char* kLabelProperty = "f:vmm_label";

/// Exact structures of one region (PreparedDomain3::state).
struct State3 {
    EMesh domain;     ///< faces labelled kDomainFace | partition triangle
    IMesh domain_i;   ///< the same surface for the AABB tree (same face order)
    Tree tree;
    std::unique_ptr<CGAL::Side_of_triangle_mesh<EMesh, Epeck>> inside;  ///< exact point-in-region test
};

Vec3 to_vec(const Epeck::Point_3& p) {
    return {CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z())};
}

/// Carries the label of every face through the corefinement: subfaces
/// inherit the label of the face they split, copied faces the label of their
/// source. Satisfies CGAL's PMPCorefinementVisitor by composition (AGENTS.md:
/// no inheritance); the other notifications are ignored.
class LabelVisitor {
public:
    using face_descriptor = EMesh::Face_index;
    using halfedge_descriptor = EMesh::Halfedge_index;
    using vertex_descriptor = EMesh::Vertex_index;

    struct Maps {
        std::array<const EMesh*, 3> mesh{};
        std::array<LabelMap, 3> label{};
        FaceLabel pending = kUnlabelled;
    };

    explicit LabelVisitor(Maps* maps) : maps_(maps) {}

    void before_subface_creations(face_descriptor f, const EMesh& tm) { maps_->pending = get(tm, f); }
    void after_subface_creations(const EMesh&) {}
    void before_subface_created(const EMesh&) {}
    void after_subface_created(face_descriptor f, const EMesh& tm) { put(tm, f, maps_->pending); }
    void before_face_copy(face_descriptor, const EMesh&, const EMesh&) {}
    void after_face_copy(face_descriptor f_old, const EMesh& src, face_descriptor f_new, const EMesh& tgt) {
        put(tgt, f_new, get(src, f_old));
    }
    void subface_of_coplanar_faces_intersection(face_descriptor, const EMesh&) {}
    void before_edge_split(halfedge_descriptor, const EMesh&) {}
    void edge_split(halfedge_descriptor, const EMesh&) {}
    void after_edge_split() {}
    void add_retriangulation_edge(halfedge_descriptor, const EMesh&) {}
    void before_edge_copy(halfedge_descriptor, const EMesh&, const EMesh&) {}
    void after_edge_copy(halfedge_descriptor, const EMesh&, halfedge_descriptor, const EMesh&) {}
    void before_edge_duplicated(halfedge_descriptor, const EMesh&) {}
    void after_edge_duplicated(halfedge_descriptor, halfedge_descriptor, const EMesh&) {}
    void intersection_edge_copy(halfedge_descriptor, const EMesh&, halfedge_descriptor, const EMesh&,
                                halfedge_descriptor, const EMesh&) {}
    void new_vertex_added(std::size_t, vertex_descriptor, const EMesh&) {}
    void intersection_point_detected(std::size_t, int, halfedge_descriptor, halfedge_descriptor, const EMesh&,
                                     const EMesh&, bool, bool) {}
    void before_vertex_copy(vertex_descriptor, const EMesh&, const EMesh&) {}
    void after_vertex_copy(vertex_descriptor, const EMesh&, vertex_descriptor, const EMesh&) {}
    void start_filtering_intersections() const {}
    void progress_filtering_intersections(double) const {}
    void end_filtering_intersections() const {}
    void start_triangulating_faces(std::size_t) const {}
    void triangulating_faces_step() const {}
    void end_triangulating_faces() const {}
    void start_handling_intersection_of_coplanar_faces(std::size_t) const {}
    void intersection_of_coplanar_faces_step() const {}
    void end_handling_intersection_of_coplanar_faces() const {}
    void start_handling_edge_face_intersections(std::size_t) const {}
    void edge_face_intersections_step() const {}
    void end_handling_edge_face_intersections() const {}
    void start_building_output() const {}
    void end_building_output() const {}
    void filter_coplanar_edges() const {}
    void detect_patches() const {}
    void classify_patches() const {}
    void classify_intersection_free_patches(const EMesh&) const {}
    void out_of_place_operation(PMP::Corefinement::Boolean_operation_type) const {}
    void in_place_operation(PMP::Corefinement::Boolean_operation_type) const {}
    void in_place_operations(PMP::Corefinement::Boolean_operation_type,
                             PMP::Corefinement::Boolean_operation_type) const {}

private:
    [[nodiscard]] std::size_t slot(const EMesh& tm) const {
        for (std::size_t k = 0; k < 3; ++k) {
            if (maps_->mesh[k] == &tm) return k;
        }
        return 3;
    }
    [[nodiscard]] FaceLabel get(const EMesh& tm, face_descriptor f) const {
        const std::size_t k = slot(tm);
        return k < 3 ? maps_->label[k][f] : kUnlabelled;
    }
    void put(const EMesh& tm, face_descriptor f, FaceLabel l) const {
        const std::size_t k = slot(tm);
        if (k < 3) maps_->label[k][f] = l;
    }

    Maps* maps_;
};

IMesh to_imesh(const std::vector<Vec3>& points, const std::vector<Triangle>& triangles) {
    IMesh m;
    std::vector<IMesh::Vertex_index> v;
    v.reserve(points.size());
    for (const Vec3& p : points) v.push_back(m.add_vertex(Epick::Point_3(p[0], p[1], p[2])));
    for (const Triangle& t : triangles) m.add_face(v[t[0]], v[t[1]], v[t[2]]);
    return m;
}

struct Piece {
    FaceLabel label;
    std::vector<Vec3> loop;
};

std::array<Epeck::Point_3, 3> face_points(const EMesh& m, EMesh::Face_index f) {
    std::array<Epeck::Point_3, 3> p;
    std::size_t k = 0;
    for (const auto v : CGAL::vertices_around_face(m.halfedge(f), m)) p[k++] = m.point(v);
    return p;
}

/// Local clipping (P17): corefines the cell with the region triangles that meet its box and
/// keeps the cell pieces inside the region and the region pieces inside the cell, every test
/// exact. Returns false when the result is not certain (a piece lying on the other surface,
/// a result that is not closed): the caller then clips against the whole region.
bool clip_local(const EMesh& cell, const State3& state, const Box3& box, EMesh& out, LabelMap& out_label) {
    std::vector<IMesh::Face_index> near;
    state.tree.all_intersected_primitives(
        Epick::Iso_cuboid_3(box.lo()[0], box.lo()[1], box.lo()[2], box.hi()[0], box.hi()[1], box.hi()[2]),
        std::back_inserter(near));
    if (near.empty()) return false;
    EMesh cm = cell;  // the copy carries the label property
    LabelMap cm_label = cm.property_map<EMesh::Face_index, FaceLabel>(kLabelProperty).value();
    const LabelMap domain_label = state.domain.property_map<EMesh::Face_index, FaceLabel>(kLabelProperty).value();
    EMesh lm;
    LabelMap lm_label = lm.add_property_map<EMesh::Face_index, FaceLabel>(kLabelProperty, kUnlabelled).first;
    std::unordered_map<std::size_t, EMesh::Vertex_index> local;
    for (const auto fi : near) {
        const EMesh::Face_index f(static_cast<EMesh::size_type>(static_cast<std::size_t>(fi)));
        std::array<EMesh::Vertex_index, 3> v;
        std::size_t k = 0;
        for (const auto x : CGAL::vertices_around_face(state.domain.halfedge(f), state.domain)) {
            auto [it, added] = local.emplace(static_cast<std::size_t>(x), EMesh::Vertex_index{});
            if (added) it->second = lm.add_vertex(state.domain.point(x));
            v[k++] = it->second;
        }
        const auto nf = lm.add_face(v[0], v[1], v[2]);
        if (nf == EMesh::null_face()) return false;
        lm_label[nf] = domain_label[f];
    }
    LabelVisitor::Maps maps;
    maps.mesh = {&cm, &lm, nullptr};
    maps.label = {cm_label, lm_label, LabelMap{}};
    auto cm_cut = cm.add_property_map<EMesh::Edge_index, bool>("e:vmm_cut", false).first;
    auto lm_cut = lm.add_property_map<EMesh::Edge_index, bool>("e:vmm_cut", false).first;
    try {
        PMP::corefine(cm, lm, CGAL::parameters::visitor(LabelVisitor(&maps)).edge_is_constrained_map(cm_cut),
                      CGAL::parameters::edge_is_constrained_map(lm_cut));
    } catch (const std::exception&) {
        return false;
    }
    const CGAL::Side_of_triangle_mesh<EMesh, Epeck> in_cell(cell);
    std::map<Epeck::Point_3, EMesh::Vertex_index> shared;
    const auto vertex = [&](const Epeck::Point_3& p) {
        auto it = shared.find(p);
        if (it == shared.end()) it = shared.emplace(p, out.add_vertex(p)).first;
        return it->second;
    };
    // The intersection edges cut each mesh into patches that lie wholly inside or
    // wholly outside the other surface: one exact query per patch.
    const auto copy = [&](const EMesh& m, const LabelMap& label, const auto& cut, const auto& side) {
        std::vector<std::size_t> parent(m.number_of_faces());
        std::iota(parent.begin(), parent.end(), std::size_t{0});
        const auto find = [&](std::size_t x) {
            while (parent[x] != x) x = parent[x] = parent[parent[x]];
            return x;
        };
        for (const auto e : m.edges()) {
            if (cut[e] || m.is_border(e)) continue;
            const auto h = m.halfedge(e);
            parent[find(static_cast<std::size_t>(m.face(h)))] = find(static_cast<std::size_t>(m.face(m.opposite(h))));
        }
        std::unordered_map<std::size_t, CGAL::Bounded_side> where;
        for (const auto f : m.faces()) {
            const std::size_t g = find(static_cast<std::size_t>(f));
            auto it = where.find(g);
            const auto p = face_points(m, f);
            if (it == where.end()) it = where.emplace(g, side(CGAL::centroid(p[0], p[1], p[2]))).first;
            if (it->second == CGAL::ON_BOUNDARY) return false;
            if (it->second != CGAL::ON_BOUNDED_SIDE) continue;
            const auto nf = out.add_face(vertex(p[0]), vertex(p[1]), vertex(p[2]));
            if (nf == EMesh::null_face()) return false;
            out_label[nf] = label[f];
        }
        return true;
    };
    if (!copy(cm, cm_label, cm_cut, *state.inside)) return false;
    if (!copy(lm, lm_label, lm_cut, in_cell)) return false;
    return out.number_of_faces() > 0 && CGAL::is_closed(out);
}

}  // namespace

Result<Partition3D> build_partition_3d(const Declaration3D& declaration) {
    if (declaration.layers().empty()) return fail(ErrorCode::EmptyDeclaration);
    if (declaration.layers().size() > 1) {
        return fail(ErrorCode::InvalidArgument,
                    "version 0.3 builds one region; several regions are planned for version 0.5 (P18)");
    }
    const auto& layer = declaration.layers().front();
    const TriangleSurface& s = layer.surface;
    const IMesh m = to_imesh(s.points(), s.triangles());
    if (!CGAL::is_closed(m) || PMP::does_self_intersect(m)) {
        return fail(ErrorCode::InvalidSurface, std::format("surface of region '{}' is open or self-intersecting",
                                                           declaration.regions()[layer.region.index()].name));
    }
    std::vector<PartitionTriangle> triangles;
    triangles.reserve(s.triangle_count());
    for (std::size_t t = 0; t < s.triangle_count(); ++t) {
        triangles.push_back({s.triangles()[t], layer.region, RegionId::invalid(), PatchId::from_index(s.triangle_patch()[t])});
    }
    return Partition3D(s.points(), std::move(triangles), declaration.regions(), declaration.media().names(), s.patches());
}

Result<PreparedDomain3> prepare_3d(const Partition3D& partition, RegionId region) {
    if (!region.valid() || region.index() >= partition.region_count()) return fail(ErrorCode::InvalidArgument, "region id");
    auto state = std::make_shared<State3>();
    std::unordered_map<std::uint32_t, EMesh::Vertex_index> ev;
    std::unordered_map<std::uint32_t, IMesh::Vertex_index> iv;
    const auto& pts = partition.vertices();
    const auto evertex = [&](std::uint32_t v) {
        auto it = ev.find(v);
        if (it == ev.end()) it = ev.emplace(v, state->domain.add_vertex(Epeck::Point_3(pts[v][0], pts[v][1], pts[v][2]))).first;
        return it->second;
    };
    const auto ivertex = [&](std::uint32_t v) {
        auto it = iv.find(v);
        if (it == iv.end()) it = iv.emplace(v, state->domain_i.add_vertex(Epick::Point_3(pts[v][0], pts[v][1], pts[v][2]))).first;
        return it->second;
    };
    LabelMap label = state->domain.add_property_map<EMesh::Face_index, FaceLabel>(kLabelProperty, kUnlabelled).first;
    const auto& tris = partition.triangles();
    for (std::size_t t = 0; t < tris.size(); ++t) {
        Triangle v = tris[t].v;
        if (tris[t].outside == region) {
            std::swap(v[1], v[2]);
        } else if (tris[t].inside != region) {
            continue;
        }
        const auto f = state->domain.add_face(evertex(v[0]), evertex(v[1]), evertex(v[2]));
        state->domain_i.add_face(ivertex(v[0]), ivertex(v[1]), ivertex(v[2]));
        if (f == EMesh::null_face()) return fail(ErrorCode::InvalidSurface, std::format("triangle {} is non-manifold", t));
        label[f] = kDomainFace | t;
    }
    if (state->domain.number_of_faces() == 0 || !CGAL::is_closed(state->domain)) {
        return fail(ErrorCode::InvalidSurface, "region surface is not closed");
    }
    state->tree.rebuild(faces(state->domain_i).first, faces(state->domain_i).second, state->domain_i);
    state->inside = std::make_unique<CGAL::Side_of_triangle_mesh<EMesh, Epeck>>(state->domain);
    return PreparedDomain3{std::shared_ptr<const void>(std::move(state))};
}

std::vector<CellPair> delaunay_pairs_3d(std::span<const Vec3> sites) {
    using Vb = CGAL::Triangulation_vertex_base_with_info_3<std::uint32_t, Epick>;
    using Tds = CGAL::Triangulation_data_structure_3<Vb>;
    using Delaunay = CGAL::Delaunay_triangulation_3<Epick, Tds>;
    std::vector<std::pair<Epick::Point_3, std::uint32_t>> points;
    points.reserve(sites.size());
    for (std::size_t i = 0; i < sites.size(); ++i) {
        points.emplace_back(Epick::Point_3(sites[i][0], sites[i][1], sites[i][2]), static_cast<std::uint32_t>(i));
    }
    const Delaunay dt(points.begin(), points.end());
    std::vector<CellPair> pairs;
    for (auto e = dt.finite_edges_begin(); e != dt.finite_edges_end(); ++e) {
        const std::uint32_t a = e->first->vertex(e->second)->info();
        const std::uint32_t b = e->first->vertex(e->third)->info();
        pairs.emplace_back(CellId::from_index(std::min(a, b)), CellId::from_index(std::max(a, b)));
    }
    std::ranges::sort(pairs);
    return pairs;
}

std::optional<Vec3> circumcentre_3d(const std::array<Vec3, 4>& s) {
    std::array<Epeck::Point_3, 4> p;
    for (std::size_t k = 0; k < 4; ++k) p[k] = Epeck::Point_3(s[k][0], s[k][1], s[k][2]);
    if (CGAL::coplanar(p[0], p[1], p[2], p[3])) return std::nullopt;
    const Epeck::Point_3 c = CGAL::circumcenter(p[0], p[1], p[2], p[3]);
    // The interval of the lazy value is enough when it is a few ulps wide; the
    // exact value is computed only for ill-conditioned (near-flat) tetrahedra.
    // Deterministic for given sites (R18); the builder shares one value per
    // vertex among its four cells, so the cells agree whatever the rounding.
    const auto round = [](const Epeck::FT& v) {
        const auto [lo, hi] = CGAL::to_interval(v);
        if (hi - lo <= 4 * std::numeric_limits<double>::epsilon() * std::max(std::abs(lo), std::abs(hi))) {
            return 0.5 * (lo + hi);
        }
        return CGAL::to_double(CGAL::exact(v));
    };
    return Vec3{round(c.x()), round(c.y()), round(c.z())};
}

bool touches_boundary_3d(const PreparedDomain3& domain, const Box3& box) {
    const auto* state = static_cast<const State3*>(domain.state.get());
    if (state == nullptr || box.empty()) return false;
    return state->tree.do_intersect(
        Epick::Iso_cuboid_3(box.lo()[0], box.lo()[1], box.lo()[2], box.hi()[0], box.hi()[1], box.hi()[2]));
}

CellClip3 clip_cell_3d(const LabelledPolyhedron3& cell, const PreparedDomain3& domain) {
    CellClip3 r;
    const auto* state = static_cast<const State3*>(domain.state.get());
    if (state == nullptr) {
        r.error = "domain not prepared";
        return r;
    }
    // Cell mesh: every face fanned from its centroid, so collinear vertices on
    // a face edge never produce a zero-area triangle (P15a).
    EMesh cm;
    LabelMap cell_label = cm.add_property_map<EMesh::Face_index, FaceLabel>(kLabelProperty, kUnlabelled).first;
    std::vector<EMesh::Vertex_index> vid;
    vid.reserve(cell.points.size());
    for (const Vec3& p : cell.points) vid.push_back(cm.add_vertex(Epeck::Point_3(p[0], p[1], p[2])));
    for (std::size_t f = 0; f < cell.faces.rows(); ++f) {
        const auto loop = cell.faces.row(f);
        Vec3 c{};
        for (const std::uint32_t v : loop) c = c + cell.points[v];
        c = (1.0 / static_cast<Real>(loop.size())) * c;
        const auto cv = cm.add_vertex(Epeck::Point_3(c[0], c[1], c[2]));
        for (std::size_t k = 0; k < loop.size(); ++k) {
            const auto face = cm.add_face(cv, vid[loop[k]], vid[loop[(k + 1) % loop.size()]]);
            if (face == EMesh::null_face()) {
                r.error = std::format("cell face {} is non-manifold", f);
                return r;
            }
            cell_label[face] = cell.labels[f];
        }
    }
    // Release builds have no CGAL preconditions: an invalid input would crash the corefinement (P15a).
    if (!CGAL::is_closed(cm) || PMP::does_self_intersect(cm)) {
        r.error = "cell is open or self-intersecting";
        return r;
    }
    EMesh out;
    LabelMap out_label = out.add_property_map<EMesh::Face_index, FaceLabel>(kLabelProperty, kUnlabelled).first;
    r.local = clip_local(cm, *state, Box3::of(cell.points), out, out_label);
    if (!r.local) {
        // Whole region, a fresh copy per cell: sharing it (do_not_modify) was 22-142x slower (P15 §6).
        out.clear();
        out_label = out.add_property_map<EMesh::Face_index, FaceLabel>(kLabelProperty, kUnlabelled).first;
        EMesh dm = state->domain;
        LabelVisitor::Maps maps;
        maps.mesh = {&cm, &dm, &out};
        maps.label = {cell_label, dm.property_map<EMesh::Face_index, FaceLabel>(kLabelProperty).value(), out_label};
        try {
            if (!PMP::corefine_and_compute_intersection(cm, dm, out, CGAL::parameters::visitor(LabelVisitor(&maps)))) {
                r.error = "corefinement failed";
                return r;
            }
        } catch (const std::exception& e) {
            r.error = std::format("corefinement threw: {}", e.what());
            return r;
        }
    }
    r.volume = CGAL::to_double(PMP::volume(out));
    auto fcc = out.add_property_map<EMesh::Face_index, std::size_t>("f:vmm_cc", 0).first;
    r.components = PMP::connected_components(out, fcc);

    // Faces: connected sets of triangles with one label, and their boundary loop.
    std::vector<std::size_t> parent(out.number_of_faces());
    std::iota(parent.begin(), parent.end(), std::size_t{0});
    const auto find = [&](std::size_t x) {
        while (parent[x] != x) x = parent[x] = parent[parent[x]];
        return x;
    };
    for (const auto f : out.faces()) {
        if (out_label[f] == kUnlabelled) {
            r.error = "clipped face without label";
            return r;
        }
    }
    for (const auto h : out.halfedges()) {
        const auto o = out.opposite(h);
        if (out.is_border(h) || out.is_border(o)) continue;
        if (out_label[out.face(h)] == out_label[out.face(o)]) {
            parent[find(static_cast<std::size_t>(out.face(h)))] = find(static_cast<std::size_t>(out.face(o)));
        }
    }
    std::map<std::size_t, std::vector<EMesh::Halfedge_index>> boundary;
    for (const auto h : out.halfedges()) {
        if (out.is_border(h)) continue;
        const auto o = out.opposite(h);
        const std::size_t g = find(static_cast<std::size_t>(out.face(h)));
        if (out.is_border(o) || find(static_cast<std::size_t>(out.face(o))) != g) boundary[g].push_back(h);
    }
    std::vector<Piece> pieces;
    for (auto& [g, hs] : boundary) {
        std::multimap<EMesh::Vertex_index, EMesh::Halfedge_index> from;
        for (const auto h : hs) from.emplace(out.source(h), h);
        std::vector<std::vector<Vec3>> loops;
        while (!from.empty()) {
            std::vector<Vec3> loop;
            auto h = from.begin()->second;
            from.erase(from.begin());
            const auto start = out.source(h);
            while (true) {
                loop.push_back(to_vec(out.point(out.source(h))));
                if (out.target(h) == start) break;
                const auto nx = from.find(out.target(h));
                if (nx == from.end()) break;
                h = nx->second;
                from.erase(nx);
            }
            loops.push_back(std::move(loop));
        }
        if (loops.size() != 1 || loops.front().size() < 3) {
            r.error = std::format("clipped face with {} boundary loops", loops.size());
            return r;
        }
        pieces.push_back({out_label[EMesh::Face_index(static_cast<EMesh::size_type>(g))], std::move(loops.front())});
    }
    // The corefinement output has no canonical order (R18, P15a): every loop
    // starts at its smallest point and the pieces are sorted.
    for (Piece& p : pieces) std::ranges::rotate(p.loop, std::ranges::min_element(p.loop));
    std::ranges::sort(pieces, [](const Piece& a, const Piece& b) { return std::tie(a.label, a.loop) < std::tie(b.label, b.loop); });
    std::map<Vec3, std::uint32_t> index;
    for (const Piece& p : pieces) {
        std::vector<std::uint32_t> row;
        for (const Vec3& x : p.loop) {
            const auto [it, added] = index.emplace(x, static_cast<std::uint32_t>(r.cell.points.size()));
            if (added) r.cell.points.push_back(x);
            row.push_back(it->second);
        }
        r.cell.faces.push_row(row);
        r.cell.labels.push_back(p.label);
    }
    return r;
}

}  // namespace vmm::cgal_detail
