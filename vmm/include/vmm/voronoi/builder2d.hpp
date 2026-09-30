// ============================================================================
// File: builder2d.hpp
// Description: 2D multi-region conforming Voronoi mesh (P10, DEC-028):
//              per-region Voronoi cells clipped by their region, topology
//              from edge labels, canonical vertices, common refinement of the
//              interfaces.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/backend2d.hpp>
#include <vmm/domain/partition.hpp>
#include <vmm/error/error.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/mesh/mesh.hpp>
#include <vmm/sites/site_set.hpp>

namespace vmm {

struct BuildOptions2D {
    /// Point tolerance relative to L (DEC-020): vertices closer than this are one vertex.
    Real relative_tolerance = 1e-12;
    /// Skip the exact clipping for cells whose convex cell touches no region segment.
    bool fast_path = true;
};

/// Diagnostic counters of a build; not stable API (DEC-041): fields may change in minor versions.
struct BuildStats2D {
    std::size_t cells = 0;
    std::size_t fast_cells = 0;              ///< cells built without the backend clipping
    std::size_t clipped_cells = 0;
    std::size_t fragmented_cells = 0;        ///< cells made of several pieces (kept whole)
    std::size_t unlabelled_edges = 0;
    std::size_t unresolved_vertices = 0;
    std::size_t unmatched_bisector_pieces = 0;
    std::size_t interface_pieces = 0;
    std::size_t interface_bad_coverage = 0;
    std::size_t merged_breakpoints = 0;
    std::size_t merged_vertices = 0;         ///< distinct points merged within the tolerance
    std::size_t collapsed_faces = 0;         ///< faces that became degenerate and were removed
    double seconds_delaunay = 0;
    double seconds_cells = 0;
    double seconds_assembly = 0;
};

struct Build2D {
    Mesh2D mesh;
    BuildStats2D stats;
    std::vector<Real> cell_polygon_area;  ///< shoelace area of each cell (independent check)
};

/// @brief Builds the conforming multi-region Voronoi mesh of a partition.
/// @param partition Explicit partition (from Backend2D::build_partition).
/// @param sites Sites with their regions; strictly inside their regions.
/// @param backend Geometric backend (e.g. cgal_backend_2d()).
/// @param options Point tolerance relative to L and the fast path switch.
/// @return Mesh, statistics and the independent polygon area of every cell.
/// @note Sites are put in canonical order, so the result does not depend on the input order;
///       Mesh::cell_input_sites() maps cells back to the input.
/// @par Level
/// Intermediate
/// @sa generate_mesh_2d, invariant_reference, check_invariants
/// @par Location
/// vmm/voronoi/builder2d.hpp
/// @par Examples
/// ex_quickstart.cpp (through generate_mesh_2d)
[[nodiscard]] Result<Build2D> build_mesh_2d(const Partition2D& partition, const SiteSet& sites,
                                            const Backend2D& backend, const BuildOptions2D& options = {});

/// Invariant reference taken from the partition (areas, boundary, interfaces).
[[nodiscard]] InvariantReference invariant_reference(const Partition2D& partition);

}  // namespace vmm
