// ============================================================================
// File: builder3d.hpp
// Description: 3D Voronoi mesh of one region (P16, P15 §5): convex cells by
//              half-spaces, canonical vertices, exact clipping of the cells
//              that touch the boundary, streaming assembly into Mesh<3>.
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
#include <vmm/backend/backend3d.hpp>
#include <vmm/core/types.hpp>
#include <vmm/domain/partition3d.hpp>
#include <vmm/error/error.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/mesh/mesh.hpp>
#include <vmm/sites/sources3d.hpp>

namespace vmm {

struct BuildOptions3D {
    /// Point tolerance relative to L (DEC-020): vertices closer than this are one vertex.
    Real relative_tolerance = 1e-12;
    /// Skip the exact clipping for cells whose box does not touch the boundary.
    bool fast_path = true;
};

struct BuildStats3D {
    std::size_t cells = 0;
    std::size_t fast_cells = 0;           ///< cells built without the exact clipping
    std::size_t clipped_cells = 0;
    std::size_t local_clips = 0;          ///< clipped cells that used only the region triangles near them
    std::size_t fragmented_cells = 0;     ///< cells made of several pieces (kept whole, DEC-035)
    std::size_t unsnapped_vertices = 0;   ///< vertex on three bisectors whose four sites are coplanar
    std::size_t merged_vertices = 0;      ///< vertices moved by the merge within the tolerance
    std::size_t t_vertices = 0;           ///< vertices inserted into a neighbouring face edge
    std::size_t collapsed_faces = 0;      ///< faces thinner than the tolerance, removed
    double seconds_delaunay = 0;
    double seconds_cells = 0;             ///< convex cells and canonical vertices
    double seconds_clip = 0;
    double seconds_assembly = 0;
};

struct Build3D {
    Mesh3D mesh;
    BuildStats3D stats;
    std::vector<Real> cell_volume;  ///< independent volume of every cell (exact for clipped cells)
};

/// @brief Builds the 3D Voronoi mesh of a one-region partition.
/// @param partition Partition (from Backend3D::build_partition), one region in version 0.3.
/// @param sites Sites of the region, strictly inside it (generate_sites_3d).
/// @param backend Geometric backend (cgal_backend_3d()).
/// @param options Point tolerance relative to L and the fast path switch.
/// @return Mesh, statistics and the independent volume of every cell; InterfaceNotConforming when
///         a face is seen by one cell only, BackendFailure when a clipping fails.
/// @note Sites are put in canonical order, so the mesh does not depend on the input order
///       (bit-identical); Mesh::cell_input_sites() maps cells back to the input. A cell split by a
///       non-convex domain stays whole (DEC-035).
/// @par Level
/// Intermediate
/// @sa generate_mesh_3d, invariant_reference, check_invariants
/// @par Location
/// vmm/voronoi/builder3d.hpp
/// @par Examples
/// ex_voronoi3d.cpp (through generate_mesh_3d)
[[nodiscard]] Result<Build3D> build_mesh_3d(const Partition3D& partition, const SiteSet3D& sites,
                                            const Backend3D& backend, const BuildOptions3D& options = {});

/// Invariant reference taken from a 3D partition (volumes, boundary area, interfaces).
[[nodiscard]] InvariantReference invariant_reference(const Partition3D& partition);

}  // namespace vmm
