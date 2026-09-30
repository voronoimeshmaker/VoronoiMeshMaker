// ============================================================================
// File: mesh.cpp
// Description: Mesh assembly checks, face geometry and cell-face index.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/mesh.hpp>

namespace vmm {

template <std::size_t D>
Result<Mesh<D>> Mesh<D>::from_data(MeshData<D> d) {
    auto bad = [](std::string what) { return fail(ErrorCode::InvariantViolated, std::move(what)); };
    const std::size_t nc = d.sites.size();
    const std::size_t nf = d.owner.size();
    const std::size_t ni = d.neighbour.size();
    if (d.cell_region.size() != nc || d.cell_input_site.size() != nc) return bad("per-cell arrays differ in size");
    if (d.face_vertices.rows() != nf) return bad("face_vertices rows != faces");
    if (ni > nf) return bad("more internal faces than faces");
    if (!d.site_weight.empty() && d.site_weight.size() != nc) return bad("site_weight size");
    if (!d.face_periodic_offset.empty() && d.face_periodic_offset.size() != nf) return bad("face_periodic_offset size");
    for (std::size_t f = 0; f < nf; ++f) {
        const auto row = d.face_vertices.row(f);
        if (row.size() < D) return fail(ErrorCode::InvariantViolated, "face with too few vertices", FaceId::from_index(f));
        for (const VertexId v : row) {
            if (v.index() >= d.points.size()) return fail(ErrorCode::InvariantViolated, "vertex id", FaceId::from_index(f));
        }
        if (!d.owner[f].valid() || d.owner[f].index() >= nc) {
            return fail(ErrorCode::InvariantViolated, "owner id", FaceId::from_index(f));
        }
    }
    for (std::size_t f = 0; f < ni; ++f) {
        const CellId o = d.owner[f];
        const CellId n = d.neighbour[f];
        if (!n.valid() || n.index() >= nc || !(o < n)) {
            return fail(ErrorCode::InvariantViolated, "internal face needs owner < neighbour", FaceId::from_index(f));
        }
        if (f > 0 && std::pair(o, n) < std::pair(d.owner[f - 1], d.neighbour[f - 1])) {
            return fail(ErrorCode::InvariantViolated, "internal faces not in upper-triangular order", FaceId::from_index(f));
        }
    }
    std::size_t next = ni;
    for (const auto& p : d.patches) {
        if (p.start != next) return bad(std::format("patch '{}' does not start at face {}", p.name, next));
        next += p.count;
    }
    if (next != nf) return bad("patches do not cover the boundary faces");
    for (std::size_t c = 0; c < nc; ++c) {
        if (!d.cell_region[c].valid() || d.cell_region[c].index() >= d.regions.size()) {
            return fail(ErrorCode::InvariantViolated, "cell region", CellId::from_index(c));
        }
    }
    for (const auto& r : d.regions) {
        if (!r.medium.valid() || r.medium.index() >= d.media.size()) return bad("region medium");
    }
    return Mesh(std::move(d));
}

template <std::size_t D>
PatchId Mesh<D>::patch(FaceId f) const {
    if (is_internal(f)) return PatchId::invalid();
    for (std::size_t p = 0; p < d_.patches.size(); ++p) {
        if (f.index() < d_.patches[p].start + d_.patches[p].count) return PatchId::from_index(p);
    }
    return PatchId::invalid();
}

FaceGeometry<2> face_geometry(std::span<const Vec2> pts) noexcept {
    const Vec2 d = pts[1] - pts[0];
    return {Vec2{d[1], -d[0]}, 0.5 * (pts[0] + pts[1])};
}

FaceGeometry<3> face_geometry(std::span<const Vec3> pts) noexcept {
    FaceGeometry<3> g;
    Real total = 0;
    Vec3 weighted{};
    for (std::size_t k = 1; k + 1 < pts.size(); ++k) {
        const Vec3 tri = 0.5 * cross(pts[k] - pts[0], pts[k + 1] - pts[0]);
        g.area_vector = g.area_vector + tri;
        const Real a = norm(tri);
        total += a;
        weighted = weighted + (a / 3.0) * (pts[0] + pts[k] + pts[k + 1]);
    }
    g.centroid = total > 0 ? (1.0 / total) * weighted : pts[0];
    return g;
}

template <std::size_t D>
CellFaceIndex::CellFaceIndex(const Mesh<D>& m) {
    std::vector<std::size_t> start(m.cell_count() + 1, 0);
    for (const FaceId f : m.faces()) {
        ++start[m.owner(f).index() + 1];
        if (m.is_internal(f)) ++start[m.neighbour(f).index() + 1];
    }
    for (std::size_t c = 0; c < m.cell_count(); ++c) start[c + 1] += start[c];
    faces_.offsets = start;
    faces_.values.resize(start.back());
    std::vector<std::size_t> cursor(start.begin(), start.end() - 1);
    std::vector<char> on_boundary(m.cell_count(), 0);
    for (const FaceId f : m.faces()) {
        faces_.values[cursor[m.owner(f).index()]++] = f;
        if (m.is_internal(f)) {
            faces_.values[cursor[m.neighbour(f).index()]++] = f;
        } else {
            on_boundary[m.owner(f).index()] = 1;
        }
    }
    for (const CellId c : m.cells()) (on_boundary[c.index()] ? boundary_ : internal_).push_back(c);
}

template class Mesh<2>;
template class Mesh<3>;
template CellFaceIndex::CellFaceIndex(const Mesh<2>&);
template CellFaceIndex::CellFaceIndex(const Mesh<3>&);

}  // namespace vmm
