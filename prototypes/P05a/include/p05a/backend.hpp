// ============================================================================
// File: backend.hpp
// Description: P05a prototype - geometry backend seen from the core. Only
//              VMM and standard types appear here; the CGAL implementation
//              lives in src/backend_cgal/*.cpp (DEC-007 firewall).
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
#include <p05a/csr.hpp>
#include <p05a/types.hpp>

namespace vmm::p05a {

/// Versions actually compiled into the backend (DEC-012).
struct BackendInfo {
    std::string cgal_version;
    std::string boost_version;
    std::string exact_number_type;
};

[[nodiscard]] BackendInfo backend_info();

/// Delaunay edges of the sites, as pairs of positions in the input span.
[[nodiscard]] std::vector<CellPair> delaunay_pairs_2d(std::span<const Vec2> sites);
[[nodiscard]] std::vector<CellPair> delaunay_pairs_3d(std::span<const Vec3> sites);

/// Opaque label carried by a polygon edge through clipping. The caller
/// chooses the encoding; the backend only propagates it.
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
    std::size_t ambiguous_edges = 0;       ///< output edges on a cell edge and a region edge
};

/// Intersection of a convex cell with a region polygon (possibly with holes),
/// computed with exact predicates and constructions. Each output edge gets
/// the label of the input edge that supports it; region edges win ties.
[[nodiscard]] RegionClip2 clip_by_region_2d(const LabelledLoop2& convex_cell, const LabelledPolygon2& region);

}  // namespace vmm::p05a
