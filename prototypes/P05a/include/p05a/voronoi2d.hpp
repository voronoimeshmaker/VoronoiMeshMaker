// ============================================================================
// File: voronoi2d.hpp
// Description: P05a prototype - 2D multi-region Voronoi mesh: per-region
//              Voronoi cells clipped by the region polygon, then a common
//              refinement of the interface (strategy E1; E2 only changes the
//              sites). Topology comes from edge labels, never from distances.
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <span>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/backend.hpp>
#include <p05a/csr.hpp>
#include <p05a/mesh.hpp>
#include <p05a/types.hpp>

namespace vmm::p05a {

/// Backend composed from callables (no virtual dispatch, R3). The core never
/// names CGAL; the application wires the CGAL functions in.
struct Backend2D {
    std::vector<CellPair> (*delaunay_pairs)(std::span<const Vec2>) = nullptr;
    RegionClip2 (*clip_by_region)(const LabelledLoop2&, const LabelledPolygon2&) = nullptr;
};

struct Segment2 {
    Vec2 a;
    Vec2 b;
};

/// Edge-label encoding used by the 2D pipeline: sites below segment_label_base,
/// then layout segments, then the four sides of the working box.
inline constexpr EdgeLabel segment_label_base = EdgeLabel{1} << 40;
inline constexpr EdgeLabel box_label_base = EdgeLabel{1} << 41;

[[nodiscard]] constexpr EdgeLabel segment_label(std::size_t k) noexcept { return segment_label_base + k; }

/// Planar layout: each boundary or interface segment appears once; region
/// polygons refer to segments through their edge labels.
struct Layout2D {
    std::vector<Segment2> segments;
    std::vector<PatchId> segment_patch;
    PatchId interface_patch;
    std::vector<LabelledPolygon2> regions;
};

struct Build2DStats {
    std::size_t fragmented_cells = 0;          ///< cells split into several components by the region
    std::size_t unlabelled_edges = 0;          ///< backend output edges without a supporting input edge
    std::size_t ambiguous_edges = 0;           ///< edges on a bisector and a region segment at once
    std::size_t unresolved_vertices = 0;       ///< vertices whose two labels have no canonical construction
    std::size_t unmatched_internal_pieces = 0; ///< bisector pieces seen from one side only
    std::size_t interface_pieces = 0;          ///< cell edges lying on the interface (both sides)
    std::size_t interface_bad_coverage = 0;    ///< interface sub-intervals not covered once from each side
    std::size_t merged_breakpoints = 0;        ///< interface breakpoints merged within the point tolerance
    Real max_merge_distance = 0;               ///< largest distance between merged breakpoints
};

struct Build2D {
    PolyMesh<2> mesh;
    std::vector<Real> cell_polygon_area;  ///< shoelace area of each clipped cell (independent check)
    Build2DStats stats;
};

/// Builds the mesh. site_region[i] selects the region polygon of site i; sites
/// must lie strictly inside their region. point_tolerance is absolute and must
/// be derived from the layout scale by the caller (R17).
[[nodiscard]] Build2D build_multiregion_mesh_2d(const Layout2D& layout, std::span<const Vec2> sites,
                                                std::span<const RegionId> site_region, const Backend2D& backend,
                                                Real point_tolerance);

}  // namespace vmm::p05a
