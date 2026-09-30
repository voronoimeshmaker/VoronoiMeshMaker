// ============================================================================
// File: backend3d.hpp
// Description: 3D geometric backend seen from the core (P15 §4, DEC-007): a
//              struct of callables using only VMM and standard types. The
//              CGAL implementation lives in vmm_backend_cgal.
//              Internal (DEC-041): only cgal_backend_3d(), build_partition
//              and passing the backend to build_mesh_3d are stable API.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/backend2d.hpp>
#include <vmm/core/csr.hpp>
#include <vmm/core/types.hpp>
#include <vmm/domain/declaration3d.hpp>
#include <vmm/domain/partition3d.hpp>
#include <vmm/error/error.hpp>
#include <vmm/geometry/surface.hpp>

namespace vmm {

/// Label of a polyhedron face: a neighbour site (its canonical index), a
/// partition triangle (kDomainFace | index) or a face of the enclosing box
/// (kBoxFace | 0..5).
using FaceLabel = std::uint64_t;
inline constexpr FaceLabel kDomainFace = FaceLabel{1} << 62;
inline constexpr FaceLabel kBoxFace = FaceLabel{1} << 63;

[[nodiscard]] constexpr bool is_domain_face(FaceLabel l) noexcept { return (l & kBoxFace) == 0 && (l & kDomainFace) != 0; }
[[nodiscard]] constexpr bool is_box_face(FaceLabel l) noexcept { return (l & kBoxFace) != 0; }
[[nodiscard]] constexpr bool is_neighbour_face(FaceLabel l) noexcept { return (l & (kBoxFace | kDomainFace)) == 0; }

/// Polyhedron with labelled polygonal faces; each face loop is
/// counter-clockwise seen from outside.
struct LabelledPolyhedron3 {
    std::vector<Vec3> points;
    Csr<std::uint32_t> faces;
    std::vector<FaceLabel> labels;
};

/// A cell clipped by the domain. Faces with one label are grouped into
/// connected pieces; every loop starts at its smallest point and the pieces
/// are sorted (R18).
struct CellClip3 {
    LabelledPolyhedron3 cell;
    Real volume = 0;              ///< exact volume, rounded
    std::size_t components = 0;   ///< > 1: cell split by the domain (kept whole, DEC-035)
    bool local = false;           ///< clipped against the region triangles near the cell (P17)
    std::string error;            ///< invalid input or backend failure (empty on success)
};

/// Backend state for the clipping against one region (opaque: it holds CGAL
/// structures that must not cross the compilation firewall).
struct PreparedDomain3 {
    std::shared_ptr<const void> state;
};

struct Backend3D {
    /// Declaration -> explicit partition; checks that the surfaces do not self-intersect (exactly).
    Result<Partition3D> (*build_partition)(const Declaration3D&) = nullptr;
    /// Exact structures of one region for touches_boundary and clip_cell.
    Result<PreparedDomain3> (*prepare)(const Partition3D&, RegionId) = nullptr;
    /// Delaunay edges of the sites, as pairs of positions in the span.
    std::vector<CellPair> (*delaunay_pairs)(std::span<const Vec3>) = nullptr;
    /// Circumcentre of four sites (exact predicates; interval arithmetic, exact evaluation when
    /// the interval is wide), deterministic; nullopt when the sites are coplanar (exact test).
    std::optional<Vec3> (*circumcentre)(const std::array<Vec3, 4>&) = nullptr;
    /// True when the box meets the region surface.
    bool (*touches_boundary)(const PreparedDomain3&, const Box3&) = nullptr;
    /// Exact intersection of a closed convex cell with the region.
    CellClip3 (*clip_cell)(const LabelledPolyhedron3&, const PreparedDomain3&) = nullptr;
    BackendInfo (*info)() = nullptr;

    [[nodiscard]] bool complete() const noexcept {
        return build_partition != nullptr && prepare != nullptr && delaunay_pairs != nullptr &&
               circumcentre != nullptr && touches_boundary != nullptr && clip_cell != nullptr && info != nullptr;
    }
};

}  // namespace vmm
