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

/// Declaration -> partition -> validation -> sites -> mesh -> invariants,
/// with the CGAL backend. Fails on the first validation error or on any
/// violated DEC-011 invariant.
[[nodiscard]] Result<MeshResult2D> generate_mesh_2d(const MeshRequest2D& request);

}  // namespace vmm
