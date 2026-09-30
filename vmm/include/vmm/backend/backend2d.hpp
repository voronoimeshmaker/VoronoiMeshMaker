// ============================================================================
// File: backend2d.hpp
// Description: Geometric backend seen from the core (DEC-007): a struct of
//              callables using only VMM and standard types. The CGAL
//              implementation lives in vmm_backend_cgal.
//              Internal (DEC-041): only cgal_backend_2d(), build_partition
//              and passing the backend to build_mesh_2d are stable API.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/csr.hpp>
#include <vmm/core/types.hpp>
#include <vmm/domain/declaration.hpp>
#include <vmm/domain/partition.hpp>
#include <vmm/error/error.hpp>

namespace vmm {

/// Opaque label carried by a polygon edge through clipping.
using EdgeLabel = std::uint64_t;

/// Closed loop: edge k goes from points[k] to points[(k + 1) % n] and carries labels[k].
struct LabelledLoop2 {
    std::vector<Vec2> points;
    std::vector<EdgeLabel> labels;
};

/// Outer loop counter-clockwise, holes clockwise.
struct LabelledPolygon2 {
    LabelledLoop2 outer;
    std::vector<LabelledLoop2> holes;
};

struct RegionClip2 {
    std::vector<LabelledPolygon2> pieces;  ///< connected components of the intersection
    std::size_t unlabelled_edges = 0;      ///< output edges on no input edge (must be 0)
    std::string error;                     ///< backend failure (e.g. a violated CGAL precondition)
};

struct BackendInfo {
    std::string name;
    std::string versions;  ///< e.g. "CGAL 6.2.1; Boost 1_92; GMP"
};

struct Backend2D {
    /// Declaration -> explicit partition (exact arrangement of all layer edges).
    Result<Partition2D> (*build_partition)(const Declaration2D&) = nullptr;
    /// Delaunay edges of the sites, as pairs of positions in the span.
    std::vector<CellPair> (*delaunay_pairs)(std::span<const Vec2>) = nullptr;
    /// Exact intersection of a convex labelled cell with region components;
    /// every output edge gets the label of the input edge that supports it
    /// (region edges win ties).
    RegionClip2 (*clip_by_region)(const LabelledLoop2&, std::span<const LabelledPolygon2>) = nullptr;
    BackendInfo (*info)() = nullptr;

    [[nodiscard]] bool complete() const noexcept {
        return build_partition != nullptr && delaunay_pairs != nullptr && clip_by_region != nullptr && info != nullptr;
    }
};

}  // namespace vmm
