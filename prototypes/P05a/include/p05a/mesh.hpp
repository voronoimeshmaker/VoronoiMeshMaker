// ============================================================================
// File: mesh.hpp
// Description: P05a prototype - dimension-generic polyhedral mesh (face based,
//              owner/neighbour) shared by the 2D and 3D proofs.
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/csr.hpp>
#include <p05a/types.hpp>

namespace vmm::p05a {

/// Face-based mesh. A face with an invalid neighbour is a boundary face and
/// carries a patch; otherwise owner < neighbour and the area vector points
/// from owner to neighbour. Interface faces are internal faces whose two
/// cells belong to different regions: no extra tag is stored.
template <std::size_t D>
struct PolyMesh {
    std::vector<Vec<D>> sites;          ///< generator of each cell
    std::vector<RegionId> cell_region;  ///< region of each cell
    std::vector<CellId> owner;          ///< per face
    std::vector<CellId> neighbour;      ///< per face; invalid on the boundary
    std::vector<PatchId> patch;         ///< per face; invalid when internal
    Csr<Vec<D>> face_points;            ///< 2 points in 2D, a polygon loop in 3D

    [[nodiscard]] std::size_t cell_count() const noexcept { return sites.size(); }
    [[nodiscard]] std::size_t face_count() const noexcept { return owner.size(); }

    void add_face(CellId o, CellId n, PatchId p, std::span<const Vec<D>> points) {
        owner.push_back(o);
        neighbour.push_back(n);
        patch.push_back(p);
        face_points.values.insert(face_points.values.end(), points.begin(), points.end());
        face_points.offsets.push_back(face_points.values.size());
    }
};

/// Area vector and centroid of one face. This is the only place where the
/// dimension changes the formula.
template <std::size_t D>
struct FaceGeometry {
    Vec<D> area_vector{};
    Vec<D> centroid{};
};

/// 2D face: segment p -> q; the area vector is the outward normal of a
/// counter-clockwise owner loop, scaled by the length.
[[nodiscard]] inline FaceGeometry<2> face_geometry(std::span<const Vec2> pts) noexcept {
    const Vec2 d = pts[1] - pts[0];
    return {Vec2{d[1], -d[0]}, 0.5 * (pts[0] + pts[1])};
}

/// 3D face: planar polygon, counter-clockwise when seen from outside the owner.
[[nodiscard]] inline FaceGeometry<3> face_geometry(std::span<const Vec3> pts) noexcept {
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
[[nodiscard]] FaceGeometry<D> face_geometry(const PolyMesh<D>& m, std::size_t f) noexcept {
    return face_geometry(m.face_points.row(f));
}

/// Faces of every cell, derived from owner/neighbour only.
template <std::size_t D>
[[nodiscard]] Csr<FaceId> cell_faces(const PolyMesh<D>& m) {
    std::vector<std::size_t> count(m.cell_count(), 0);
    for (std::size_t f = 0; f < m.face_count(); ++f) {
        ++count[m.owner[f].index()];
        if (m.neighbour[f].valid()) ++count[m.neighbour[f].index()];
    }
    Csr<FaceId> csr;
    csr.offsets.assign(m.cell_count() + 1, 0);
    for (std::size_t c = 0; c < m.cell_count(); ++c) csr.offsets[c + 1] = csr.offsets[c] + count[c];
    csr.values.resize(csr.offsets.back());
    std::vector<std::size_t> cursor(csr.offsets.begin(), csr.offsets.end() - 1);
    for (std::size_t f = 0; f < m.face_count(); ++f) {
        const FaceId id{static_cast<std::uint32_t>(f)};
        csr.values[cursor[m.owner[f].index()]++] = id;
        if (m.neighbour[f].valid()) csr.values[cursor[m.neighbour[f].index()]++] = id;
    }
    return csr;
}

/// Cell adjacency (DEC-015) read from owner/neighbour, without geometry.
template <std::size_t D>
[[nodiscard]] Csr<CellId> cell_adjacency(const PolyMesh<D>& m) {
    std::vector<CellPair> pairs;
    for (std::size_t f = 0; f < m.face_count(); ++f) {
        if (m.neighbour[f].valid()) pairs.emplace_back(m.owner[f], m.neighbour[f]);
    }
    return build_adjacency(m.cell_count(), pairs);
}

/// Faces with a neighbour (internal and interface faces), as FaceId.
/// Lazy view over the mesh: the mesh must outlive it. Like every
/// std::views::filter, iterate it as a non-const object.
template <std::size_t D>
[[nodiscard]] auto internal_faces(const PolyMesh<D>& m) {
    return std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(m.face_count())) |
           std::views::filter([&m](std::uint32_t f) { return m.neighbour[f].valid(); }) |
           std::views::transform([](std::uint32_t f) { return FaceId{f}; });
}

/// Faces on the domain boundary (invalid neighbour, valid patch), as FaceId.
template <std::size_t D>
[[nodiscard]] auto boundary_faces(const PolyMesh<D>& m) {
    return std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(m.face_count())) |
           std::views::filter([&m](std::uint32_t f) { return !m.neighbour[f].valid(); }) |
           std::views::transform([](std::uint32_t f) { return FaceId{f}; });
}

/// Faces of every cell plus the boundary flag, built once from owner and
/// neighbour. A boundary cell has at least one boundary face; an internal
/// cell has none. The cell views are lazy and must not outlive the index.
struct CellFaceIndex {
    Csr<FaceId> faces;
    std::vector<std::uint8_t> on_boundary;  ///< 1 if the cell has a boundary face

    [[nodiscard]] std::span<const FaceId> faces_of(CellId c) const noexcept { return faces.row(c.index()); }

    [[nodiscard]] auto internal_cells() const {
        return std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(on_boundary.size())) |
               std::views::filter([this](std::uint32_t c) { return on_boundary[c] == 0; }) |
               std::views::transform([](std::uint32_t c) { return CellId{c}; });
    }

    [[nodiscard]] auto boundary_cells() const {
        return std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(on_boundary.size())) |
               std::views::filter([this](std::uint32_t c) { return on_boundary[c] != 0; }) |
               std::views::transform([](std::uint32_t c) { return CellId{c}; });
    }
};

template <std::size_t D>
[[nodiscard]] CellFaceIndex index_cell_faces(const PolyMesh<D>& m) {
    CellFaceIndex index{cell_faces(m), std::vector<std::uint8_t>(m.cell_count(), 0)};
    for (std::size_t f = 0; f < m.face_count(); ++f) {
        if (!m.neighbour[f].valid()) index.on_boundary[m.owner[f].index()] = 1;
    }
    return index;
}

}  // namespace vmm::p05a
