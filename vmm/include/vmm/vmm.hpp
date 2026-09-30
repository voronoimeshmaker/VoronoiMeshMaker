// ============================================================================
// File: vmm.hpp
// Description: Facade of the library: one call from a domain declaration and
//              site sources to a checked finite-volume mesh. Includes the
//              public headers a typical application needs.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/csr.hpp>
#include <vmm/core/types.hpp>
#include <vmm/domain/declaration.hpp>
#include <vmm/domain/partition.hpp>
#include <vmm/domain/shapes.hpp>
#include <vmm/domain/validator.hpp>
#include <vmm/error/error.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/mesh/mesh.hpp>
#include <vmm/mesh/metrics.hpp>
#include <vmm/sites/sources.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace vmm {

struct MeshRequest2D {
    Declaration2D declaration;
    std::vector<RegionSites> sources;
    SiteGenerationOptions sites;
    BuildOptions2D build;
    ValidationOptions validation;
};

struct MeshResult2D {
    Mesh2D mesh;
    Partition2D partition;
    BuildStats2D stats;
    ValidationReport validation;  ///< warnings only (errors stop the pipeline)
    InvariantReport invariants;
};

/// @brief Generates a checked 2D multi-region finite-volume mesh in one call.
/// @param request Domain declaration, site sources per region, site, build and validation options.
/// @return The mesh, its partition, build statistics, validation warnings and invariant report,
///         or the first error (validation, sites, backend or a violated DEC-011 invariant).
/// @note Uses the CGAL backend; link VoronoiMeshMaker::vmm.
/// @par Level
/// Beginner
/// @sa build_mesh_2d, generate_sites, validate_partition, check_invariants
/// @par Location
/// vmm/vmm.hpp
/// @par Examples
/// ex_quickstart.cpp, ex_anchor_a1.cpp, ex_anchor_a2.cpp
[[nodiscard]] Result<MeshResult2D> generate_mesh_2d(const MeshRequest2D& request);

}  // namespace vmm
