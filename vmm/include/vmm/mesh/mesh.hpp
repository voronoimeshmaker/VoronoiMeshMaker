// ============================================================================
// File: mesh.hpp
// Description: Dimension-generic finite-volume mesh (P06 §3), face based and
//              immutable once built (R7). Faces [0, n_internal) are internal
//              (owner < neighbour, upper-triangular order); the rest are
//              boundary faces grouped in contiguous patch ranges (DEC-029).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <iterator>
#include <span>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/csr.hpp>
#include <vmm/core/types.hpp>
#include <vmm/error/error.hpp>

namespace vmm {

/// Contiguous range of ids [first, last): iterable, sized, no storage.
template <class Tag>
class IdRange {
public:
    class iterator {
    public:
        using value_type = Id<Tag>;
        using difference_type = std::ptrdiff_t;
        iterator() = default;
        explicit iterator(std::size_t i) noexcept : i_(i) {}
        value_type operator*() const noexcept { return value_type::from_index(i_); }
        iterator& operator++() noexcept { ++i_; return *this; }
        iterator operator++(int) noexcept { auto t = *this; ++i_; return t; }
        friend bool operator==(iterator, iterator) = default;

    private:
        std::size_t i_ = 0;
    };

    IdRange() = default;
    IdRange(std::size_t first, std::size_t last) noexcept : first_(first), last_(last) {}
    [[nodiscard]] iterator begin() const noexcept { return iterator(first_); }
    [[nodiscard]] iterator end() const noexcept { return iterator(last_); }
    [[nodiscard]] std::size_t size() const noexcept { return last_ - first_; }
    [[nodiscard]] bool empty() const noexcept { return first_ == last_; }
    [[nodiscard]] bool contains(Id<Tag> id) const noexcept { return id.index() >= first_ && id.index() < last_; }

private:
    std::size_t first_ = 0;
    std::size_t last_ = 0;
};

struct PatchRange {
    std::string name;
    std::size_t start = 0;  ///< first face of the patch
    std::size_t count = 0;
};

struct RegionInfoMesh {
    std::string name;
    MediumId medium;
};

/// Plain data from which a Mesh is assembled (and validated).
template <std::size_t D>
struct MeshData {
    std::vector<Vec<D>> points;
    Csr<VertexId> face_vertices;
    std::vector<CellId> owner;
    std::vector<CellId> neighbour;  ///< size n_internal
    std::vector<PatchRange> patches;
    std::vector<Vec<D>> sites;
    std::vector<RegionId> cell_region;
    std::vector<SiteId> cell_input_site;  ///< index of the site in the user's input
    std::vector<RegionInfoMesh> regions;
    std::vector<std::string> media;
    std::vector<Real> site_weight;             ///< optional (empty = none)
    std::vector<Vec<D>> face_periodic_offset;  ///< optional (empty = none)
};

template <std::size_t D>
class Mesh {
public:
    Mesh() = default;

    /// Checks sizes, id ranges, internal-face ordering and patch ranges.
    [[nodiscard]] static Result<Mesh> from_data(MeshData<D> data);

    [[nodiscard]] std::size_t cell_count() const noexcept { return d_.sites.size(); }
    [[nodiscard]] std::size_t face_count() const noexcept { return d_.owner.size(); }
    [[nodiscard]] std::size_t internal_face_count() const noexcept { return d_.neighbour.size(); }
    [[nodiscard]] std::size_t point_count() const noexcept { return d_.points.size(); }

    [[nodiscard]] IdRange<CellTag> cells() const noexcept { return {0, cell_count()}; }
    [[nodiscard]] IdRange<FaceTag> faces() const noexcept { return {0, face_count()}; }
    [[nodiscard]] IdRange<FaceTag> internal_faces() const noexcept { return {0, internal_face_count()}; }
    [[nodiscard]] IdRange<FaceTag> boundary_faces() const noexcept { return {internal_face_count(), face_count()}; }
    [[nodiscard]] IdRange<FaceTag> patch_faces(PatchId p) const {
        const auto& r = d_.patches.at(p.index());
        return {r.start, r.start + r.count};
    }

    [[nodiscard]] bool is_internal(FaceId f) const noexcept { return f.index() < internal_face_count(); }
    [[nodiscard]] CellId owner(FaceId f) const { return d_.owner.at(f.index()); }
    /// Invalid for boundary faces.
    [[nodiscard]] CellId neighbour(FaceId f) const {
        return is_internal(f) ? d_.neighbour[f.index()] : CellId::invalid();
    }
    [[nodiscard]] PatchId patch(FaceId f) const;
    [[nodiscard]] std::span<const VertexId> face_vertices(FaceId f) const { return d_.face_vertices.row(f.index()); }
    [[nodiscard]] const Vec<D>& point(VertexId v) const { return d_.points.at(v.index()); }

    [[nodiscard]] const std::vector<Vec<D>>& points() const noexcept { return d_.points; }
    [[nodiscard]] const Csr<VertexId>& face_vertex_csr() const noexcept { return d_.face_vertices; }
    [[nodiscard]] std::span<const CellId> owners() const noexcept { return d_.owner; }
    [[nodiscard]] std::span<const CellId> neighbours() const noexcept { return d_.neighbour; }
    [[nodiscard]] const std::vector<PatchRange>& patches() const noexcept { return d_.patches; }
    [[nodiscard]] std::span<const Vec<D>> sites() const noexcept { return d_.sites; }
    [[nodiscard]] const Vec<D>& site(CellId c) const { return d_.sites.at(c.index()); }
    [[nodiscard]] RegionId region(CellId c) const { return d_.cell_region.at(c.index()); }
    [[nodiscard]] std::span<const RegionId> cell_regions() const noexcept { return d_.cell_region; }
    [[nodiscard]] std::span<const SiteId> cell_input_sites() const noexcept { return d_.cell_input_site; }
    [[nodiscard]] const std::vector<RegionInfoMesh>& regions() const noexcept { return d_.regions; }
    [[nodiscard]] const std::vector<std::string>& media() const noexcept { return d_.media; }
    [[nodiscard]] std::span<const Real> site_weights() const noexcept { return d_.site_weight; }
    [[nodiscard]] std::span<const Vec<D>> face_periodic_offsets() const noexcept { return d_.face_periodic_offset; }
    [[nodiscard]] const MeshData<D>& data() const noexcept { return d_; }

private:
    explicit Mesh(MeshData<D> d) : d_(std::move(d)) {}
    MeshData<D> d_;
};

using Mesh2D = Mesh<2>;
using Mesh3D = Mesh<3>;

// ----------------------------------------------------------------------------
// Face geometry: the only place where the dimension changes the formula.
// ----------------------------------------------------------------------------
template <std::size_t D>
struct FaceGeometry {
    Vec<D> area_vector{};  ///< |S| = length (2D) or area (3D); points out of the owner
    Vec<D> centroid{};
};

/// 2D face p -> q: outward normal of a counter-clockwise owner, times the length.
[[nodiscard]] FaceGeometry<2> face_geometry(std::span<const Vec2> points) noexcept;
/// 3D face: planar polygon, counter-clockwise seen from outside the owner.
[[nodiscard]] FaceGeometry<3> face_geometry(std::span<const Vec3> points) noexcept;

template <std::size_t D>
[[nodiscard]] FaceGeometry<D> face_geometry(const Mesh<D>& m, FaceId f) {
    std::vector<Vec<D>> pts;
    for (const VertexId v : m.face_vertices(f)) pts.push_back(m.point(v));
    return face_geometry(std::span<const Vec<D>>(pts));
}

// ----------------------------------------------------------------------------
// Cell-to-face index and adjacency (DEC-015, DEC-029).
// ----------------------------------------------------------------------------
class CellFaceIndex {
public:
    template <std::size_t D>
    explicit CellFaceIndex(const Mesh<D>& m);

    [[nodiscard]] std::span<const FaceId> faces_of(CellId c) const { return faces_.row(c.index()); }
    /// Cells with no boundary face.
    [[nodiscard]] std::span<const CellId> internal_cells() const noexcept { return internal_; }
    /// Cells with at least one boundary face.
    [[nodiscard]] std::span<const CellId> boundary_cells() const noexcept { return boundary_; }
    [[nodiscard]] const Csr<FaceId>& csr() const noexcept { return faces_; }

private:
    Csr<FaceId> faces_;
    std::vector<CellId> internal_;
    std::vector<CellId> boundary_;
};

/// Symmetric cell adjacency from owner/neighbour, without geometry.
template <std::size_t D>
[[nodiscard]] Csr<CellId> cell_adjacency(const Mesh<D>& m) {
    std::vector<CellPair> pairs;
    pairs.reserve(m.internal_face_count());
    for (const FaceId f : m.internal_faces()) pairs.emplace_back(m.owner(f), m.neighbour(f));
    return build_adjacency(m.cell_count(), pairs);
}

extern template class Mesh<2>;
extern template class Mesh<3>;
extern template CellFaceIndex::CellFaceIndex(const Mesh<2>&);
extern template CellFaceIndex::CellFaceIndex(const Mesh<3>&);

}  // namespace vmm
