#pragma once
//==============================================================================
// Name        : VoronoiCellBuilder2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Cells
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 1.2
// Description : Builds one clipped 2D Voronoi cell from Delaunay neighbours.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VoronoiCellBuilder2D.hpp
 * @brief Constructs one boundary-clipped Voronoi cell for a given SiteId.
 *
 * **Changes in v1.2:**
 *
 *  - Added `build_unchecked()` — an internal overload that skips per-call
 *    validation.  It is intended to be called from `ClippedVoronoiBuilder2D`
 *    **after** diagram-level validation has already passed, enabling safe
 *    parallel cell construction without redundant O(n) checks per cell.
 *
 *  - `build()` (public) still performs full validation and is safe to call
 *    independently.
 *
 * @ingroup voronoi2d_cells
 */

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DContains.hpp>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCell2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/BisectorHalfplane.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/ClippingWorkspace2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/HalfplaneClipper2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayNeighborProvider2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunaySiteIndex.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN



//==============================================================================
//  VoronoiCellBuildOptions2D
//==============================================================================

/**
 * @brief Options controlling how one Voronoi cell is built.
 * @ingroup voronoi2d_cells
 */
struct VoronoiCellBuildOptions2D {
    /**
     * @brief Reject boundaries that contain holes.
     *
     * The current implementation clips only against the outer ring.
     * Retained for source compatibility. Holes cannot be enabled with this
     * flag: the single convex polygon representation cannot represent them.
     */
    bool reject_boundaries_with_holes{true};
};

//==============================================================================
//  VoronoiCellBuilder2D
//==============================================================================

/**
 * @brief Builds one clipped Voronoi cell from its Delaunay neighbourhood.
 *
 * ### Algorithm
 *
 *  1. Query Delaunay neighbours of `site_id` (O(k) where k = degree).
 *  2. Build one perpendicular-bisector halfplane per neighbour.
 *  3. Clip the outer ring of `boundary` against all halfplanes sequentially
 *     (Sutherland-Hodgman).
 *  4. Classify the resulting polygon as boundary or internal.
 *
 * ### Thread safety
 *
 * `build_unchecked()` is safe to call from multiple threads concurrently
 * provided that:
 *  - `sites`, `boundary`, `triangulation`, and `index` are **not** mutated
 *    during the parallel phase.
 *  - Each thread passes its **own** `ClippingWorkspace2D` instance (the
 *    caller in `ClippedVoronoiBuilder2D` uses `thread_local` for this).
 *
 * @ingroup voronoi2d_cells
 */
struct VoronoiCellBuilder2D {
    using Point2 = ::vmm::s2d::Point2;
    using SiteId = ::vmm::s2d::SiteId;
    using Index  = ::vmm::s2d::Index;

    /** @brief Validate the single convex outer ring supported by this builder. */
    static void validate_domain(const ::vmm::b2d::Boundary2DData& boundary) {
        if (!boundary.invariant_ok() || boundary.ring_count() == 0) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"name", "invalid boundary storage"}});
        }
        if (boundary.ring_count() != 1 ||
            boundary.kinds.front() != ::vmm::b2d::LoopKind::Outer) {
            VMM_THROW(::vmm::error::CoreErr::NotImplemented,
                      {{"reason", "multiple rings and holes require multi-component cells"}});
        }
        const auto ring = boundary.ring(0);
        for (std::size_t i = 0; i < ring.size(); ++i) {
            const auto a = ring[i];
            const auto b = ring[(i + 1U) % ring.size()];
            if (!std::isfinite(a.x) || !std::isfinite(a.y) ||
                (a.x == b.x && a.y == b.y)) {
                VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                          {{"name", "non-finite or repeated boundary vertex"}});
            }
        }
        if (!CgalKernelTraits2D::is_simple_ccw_convex(ring)) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"name", "boundary must be simple, counter-clockwise and convex"}});
        }
    }

    //--------------------------------------------------------------------------
    // Public API — performs full validation
    //--------------------------------------------------------------------------

    /**
     * @brief Builds a clipped Voronoi cell with workspace reuse.
     *
     * Validates all inputs before computing.  Pass a persistent
     * `ClippingWorkspace2D` to avoid repeated heap allocations when building
     * many cells in a loop.
     *
     * @param[in]     sites          Generator site set.
     * @param[in]     boundary       Domain boundary.
     * @param[in]     triangulation  CGAL Delaunay triangulation.
     * @param[in]     index          Site-to-vertex handle index.
     * @param[in]     site_id        Identifier of the cell to build.
     * @param[in,out] workspace      Scratch buffers (cleared internally).
     * @param[in]     options        Build options (default: reject holes).
     *
     * @return The clipped `VoronoiCell2D`.
     *
     * @throws vmm::error::VMMException CoreErr::InvalidArgument on invalid inputs.
     * @throws vmm::error::VMMException CoreErr::OutOfRange if `site_id` is out of bounds.
     * @throws vmm::error::VMMException CoreErr::NotImplemented if the boundary has holes
     *         and `options.reject_boundaries_with_holes` is true.
     */
    [[nodiscard]] static VoronoiCell2D build(
        const ::vmm::s2d::SiteSet&          sites,
        const ::vmm::b2d::Boundary2DData&   boundary,
        const DelaunayTriangulation2D&      triangulation,
        const DelaunaySiteIndex&            index,
        SiteId                              site_id,
        ClippingWorkspace2D&                workspace,
        const VoronoiCellBuildOptions2D&    options = {})
    {
        validate_inputs(sites, boundary, site_id, options);
        return build_impl(sites, boundary, triangulation, index, site_id, workspace);
    }

    /**
     * @brief Builds a clipped Voronoi cell, creating a local workspace.
     *
     * Convenience overload for one-off builds.  Use the workspace-reuse
     * overload when building many cells in sequence.
     *
     * @param[in] sites          Generator site set.
     * @param[in] boundary       Domain boundary.
     * @param[in] triangulation  CGAL Delaunay triangulation.
     * @param[in] index          Site-to-vertex handle index.
     * @param[in] site_id        Identifier of the cell to build.
     * @param[in] options        Build options (default: reject holes).
     *
     * @return The clipped `VoronoiCell2D`.
     *
     * @throws Same exceptions as the workspace-reuse overload.
     */
    [[nodiscard]] static VoronoiCell2D build(
        const ::vmm::s2d::SiteSet&          sites,
        const ::vmm::b2d::Boundary2DData&   boundary,
        const DelaunayTriangulation2D&      triangulation,
        const DelaunaySiteIndex&            index,
        SiteId                              site_id,
        const VoronoiCellBuildOptions2D&    options = {})
    {
        ClippingWorkspace2D workspace;
        return build(sites, boundary, triangulation, index,
                     site_id, workspace, options);
    }

    //--------------------------------------------------------------------------
    // Internal API — skips validation (for use by ClippedVoronoiBuilder2D)
    //--------------------------------------------------------------------------

    /**
     * @brief Builds a clipped Voronoi cell **without** input validation.
     *
     * @warning This overload must only be called after the diagram-level
     *          builder (`ClippedVoronoiBuilder2D`) has already verified:
     *           - `sites.ids_are_sequential()`,
     *           - `site_id` is in range,
     *           - `boundary.invariant_ok()`,
     *           - no boundary holes (when required).
     *
     *          It is `private` in intent but declared in an internal-only
     *          section so that `ClippedVoronoiBuilder2D` (a separate struct
     *          in the same namespace) can call it.  Do **not** call it from
     *          application code.
     *
     * @param[in]     sites          Generator site set (pre-validated).
     * @param[in]     boundary       Domain boundary (pre-validated).
     * @param[in]     triangulation  CGAL Delaunay triangulation.
     * @param[in]     index          Site-to-vertex handle index.
     * @param[in]     site_id        Identifier of the cell to build.
     * @param[in,out] workspace      Thread-local scratch buffers.
     * @param[in]     options        Build options.
     *
     * @return The clipped `VoronoiCell2D`.
     */
    [[nodiscard]] static VoronoiCell2D build_unchecked(
        const ::vmm::s2d::SiteSet&          sites,
        const ::vmm::b2d::Boundary2DData&   boundary,
        const DelaunayTriangulation2D&      triangulation,
        const DelaunaySiteIndex&            index,
        SiteId                              site_id,
        ClippingWorkspace2D&                workspace,
        const VoronoiCellBuildOptions2D&    /*options*/ = {})
    {
        return build_impl(sites, boundary, triangulation, index, site_id, workspace);
    }

private:
    //--------------------------------------------------------------------------
    // Core implementation  (shared by build() and build_unchecked())
    //--------------------------------------------------------------------------

    [[nodiscard]] static VoronoiCell2D build_impl(
        const ::vmm::s2d::SiteSet&        sites,
        const ::vmm::b2d::Boundary2DData& boundary,
        const DelaunayTriangulation2D&    triangulation,
        const DelaunaySiteIndex&          index,
        SiteId                            site_id,
        ClippingWorkspace2D&              workspace)
    {
        const auto owner = sites.at(static_cast<std::size_t>(site_id.value));

        auto neighbor_ids = DelaunayNeighborProvider2D::neighbor_site_ids(
            triangulation, index, site_id);

        // Build halfplanes from Delaunay neighbours.
        std::vector<Halfplane2D> halfplanes;
        halfplanes.reserve(neighbor_ids.size());
        for (const auto nbr_id : neighbor_ids) {
            const auto& nbr = sites.at(static_cast<std::size_t>(nbr_id.value));
            halfplanes.push_back(BisectorHalfplane::between(owner.point, nbr.point));
        }

        // Clip outer boundary ring against all halfplanes.
        const auto initial_polygon = outer_ring(boundary);
        auto polygon = HalfplaneClipper2D::clip_all(
            initial_polygon,
            std::span<const Halfplane2D>(halfplanes),
            workspace);
        remove_consecutive_duplicate_vertices(polygon);

        const bool on_boundary = touches_boundary(polygon, boundary);

        return VoronoiCell2D{
            site_id,
            std::move(neighbor_ids),
            std::move(polygon),
            on_boundary
        };
    }

    //--------------------------------------------------------------------------
    // Validation (called only by the public build() overloads)
    //--------------------------------------------------------------------------

    static void validate_inputs(
        const ::vmm::s2d::SiteSet&        sites,
        const ::vmm::b2d::Boundary2DData& boundary,
        SiteId                            site_id,
        const VoronoiCellBuildOptions2D&  /*options*/)
    {
        if (!sites.ids_are_sequential()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where",  "VoronoiCellBuilder2D"},
                       {"reason", "site_ids_must_be_sequential"}});
        }
        if (site_id.value < 0 ||
            static_cast<std::size_t>(site_id.value) >= sites.size()) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"index", std::to_string(site_id.value)}});
        }
        if (!boundary.invariant_ok()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where",  "VoronoiCellBuilder2D"},
                       {"reason", "invalid_boundary"}});
        }
        validate_domain(boundary);
    }

    //--------------------------------------------------------------------------
    // Geometry helpers
    //--------------------------------------------------------------------------

    static void remove_consecutive_duplicate_vertices(
        std::vector<Point2>& polygon) noexcept
    {
        if (polygon.size() < 2U) {
            return;
        }

        using Real = ::vmm::s2d::Real;
        // Remove numerical duplicates, not genuine short Voronoi faces. An
        // absolute mesh-scale threshold changes the neighbouring bisectors.
        const auto duplicate = [](Point2 p, Point2 q) noexcept {
            const Real scale = std::max({Real{1}, std::abs(p.x), std::abs(p.y),
                                        std::abs(q.x), std::abs(q.y)});
            const Real tolerance = Real{8} * std::numeric_limits<Real>::epsilon() * scale;
            return std::hypot(p.x - q.x, p.y - q.y) <= tolerance;
        };
        std::size_t count = 1U;
        for (std::size_t i = 1U; i < polygon.size(); ++i) {
            if (!duplicate(polygon[count - 1U], polygon[i])) {
                polygon[count++] = polygon[i];
            }
        }
        if (count > 1U && duplicate(polygon.front(), polygon[count - 1U])) --count;
        polygon.resize(count);
    }

    /**
     * @brief Returns a span over the first outer ring of `boundary`.
     * @throws vmm::error::VMMException CoreErr::InvalidArgument if no outer ring exists.
     */
    [[nodiscard]] static std::span<const Point2> outer_ring(
        const ::vmm::b2d::Boundary2DData& boundary)
    {
        for (Index i = 0; i < boundary.ring_count(); ++i) {
            if (boundary.kinds[static_cast<std::size_t>(i)] ==
                ::vmm::b2d::LoopKind::Outer) {
                return boundary.ring(i);
            }
        }
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where",  "VoronoiCellBuilder2D"},
                   {"reason", "missing_outer_ring"}});
    }

    /**
     * @brief Returns true if any polygon vertex lies on a boundary ring.
     */
    [[nodiscard]] static bool touches_boundary(
        std::span<const Point2>           polygon,
        const ::vmm::b2d::Boundary2DData& boundary) noexcept
    {
        for (const auto& pt : polygon) {
            for (Index i = 0; i < boundary.ring_count(); ++i) {
                if (::vmm::b2d::point_on_ring(pt, boundary.ring(i))) {
                    return true;
                }
            }
        }
        return false;
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
