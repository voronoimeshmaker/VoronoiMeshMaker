// ============================================================================
// File: repair.hpp
// Description: Repair of a triangle soup into a closed TriangleSurface (P17):
//              vertex welding within a relative tolerance, removal of
//              collapsed and duplicated triangles, consistent orientation per
//              component. Holes are reported, never filled.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/error/error.hpp>
#include <vmm/geometry/surface.hpp>

namespace vmm {

/// Triangles as read from a file: no guarantee of welding, orientation or closure.
struct TriangleSoup {
    std::vector<Vec3> points;
    std::vector<Triangle> triangles;
    std::vector<std::uint32_t> triangle_patch;  ///< index into patches
    std::vector<std::string> patches;
};

struct SurfaceRepairOptions {
    /// Points closer than this times the bounding-box diagonal are welded into one.
    Real weld_relative_tolerance = 1e-9;
};

struct SurfaceRepairReport {
    std::size_t welded_points = 0;          ///< points merged into another
    std::size_t collapsed_triangles = 0;    ///< triangles with a repeated vertex after welding, removed
    std::size_t duplicate_triangles = 0;    ///< repeated triangles removed (a pair of opposite copies is removed whole)
    std::size_t flipped_triangles = 0;      ///< triangles reversed to agree with their component
    std::size_t components = 0;
};

/// @brief Repairs a triangle soup into a closed, consistently oriented surface.
/// @param soup Triangles (e.g. from read_stl).
/// @param options Welding tolerance relative to the bounding-box diagonal.
/// @param report Optional counters of every change.
/// @return The surface, or InvalidSurface when an edge is shared by more than two triangles
///         (non-manifold), when the surface has holes (edges with one triangle; counted in the
///         message) or when a triangle has zero area without a repeated vertex.
/// @note Each component keeps the orientation of the majority of its triangles; the whole
///       surface is then turned outward (positive volume), so cavities of a well-formed input
///       are preserved.
/// @par Level
/// Intermediate
/// @sa read_stl, TriangleSurface::make
/// @par Location
/// vmm/geometry/repair.hpp
[[nodiscard]] Result<TriangleSurface> repair_surface(const TriangleSoup& soup, const SurfaceRepairOptions& options = {},
                                                     SurfaceRepairReport* report = nullptr);

}  // namespace vmm
