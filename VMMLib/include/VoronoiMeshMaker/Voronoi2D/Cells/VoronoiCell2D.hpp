#pragma once
//==============================================================================
// Name        : VoronoiCell2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Cells
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 1.3
// Description : Value type for one clipped 2D Voronoi cell.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VoronoiCell2D.hpp
 * @brief Stores the polygon and metadata of one clipped Voronoi cell.
 *
 * The polygon remains the authoritative clipped-cell geometry. The optional
 * per-edge metadata stores additional geometric information needed by
 * downstream finite-volume gradient reconstructions, especially the distinction
 * between the clipped boundary edge and the representative point on Boundary2D.
 *
 * Existing aggregate initialisation of VoronoiCell2D is preserved by keeping
 * the original member order and appending the new edge vector at the end.
 *
 * @ingroup voronoi2d_cells
 */

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <cstddef>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/Sites2D/Site2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

//==============================================================================
//  Helper: combined area + centroid result
//==============================================================================

/**
 * @brief Result of a single-pass signed-area and centroid computation.
 * @ingroup voronoi2d_cells
 */
struct AreaCentroid2D {
    ::vmm::s2d::Real  signed_area{0};
    ::vmm::s2d::Point2 centroid{};
    bool               valid{false};
};

//==============================================================================
//  VoronoiCellEdge2D
//==============================================================================

/**
 * @brief Metadata for one edge of a clipped Voronoi cell.
 *
 * The endpoints `a` and `b` are the actual clipped-cell edge. For non-boundary
 * edges, the representative point is the midpoint. For boundary edges, the
 * representative point is intended to be a point on Boundary2D associated with
 * the numerical boundary treatment, not necessarily the midpoint of the local
 * clipped edge.
 *
 * @ingroup voronoi2d_cells
 */
struct VoronoiCellEdge2D {
    using Point2 = ::vmm::s2d::Point2;
    using Real   = ::vmm::s2d::Real;
    using Index  = ::vmm::b2d::Index;

    Point2 a{};
    Point2 b{};

    Point2 midpoint{};
    Real   length{0};

    bool   is_boundary_edge{false};

    Point2 representative_point{};
    Real   representative_distance{0};
    bool   representative_valid{false};

    Index       boundary_ring_index{::vmm::b2d::kInvalid};
    std::size_t boundary_edge_index{0};
    Real        boundary_segment_parameter{0};

    bool representative_inside_local_edge{false};
};

//==============================================================================
//  VoronoiCell2D
//==============================================================================

/**
 * @brief One clipped Voronoi cell associated with a generator site.
 *
 * Stores the convex polygon resulting from clipping the Voronoi cell of
 * `site_id` against the domain boundary and the perpendicular-bisector
 * halfplanes of its Delaunay neighbours.
 *
 * All geometric queries (`area`, `perimeter`, `centroid`) operate on the
 * stored polygon using `long double` accumulators for numerical stability.
 *
 * @note A cell is considered empty when its polygon has fewer than 3 vertices.
 *
 * @ingroup voronoi2d_cells
 */
struct VoronoiCell2D {
    using Point2  = ::vmm::s2d::Point2;
    using Real    = ::vmm::s2d::Real;
    using SiteId  = ::vmm::s2d::SiteId;

    SiteId               site_id{};
    std::vector<SiteId>  neighbor_ids{};
    std::vector<Point2>  polygon{};

    bool                 is_boundary_cell{false};

    Point2               boundary_projection_point{};
    Real                 boundary_projection_distance{0};
    bool                 boundary_projection_valid{false};
    bool                 boundary_projection_inside_cell{false};

    std::size_t          volume_id{0};

    std::vector<VoronoiCellEdge2D> edges{};

    // -------------------------------------------------------------------------
    // Basic queries
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true when the polygon has fewer than 3 vertices.
     * @return `true` if the cell is degenerate.
     */
    [[nodiscard]] bool empty() const noexcept {
        return polygon.size() < 3U;
    }

    // -------------------------------------------------------------------------
    // Geometric computations
    // -------------------------------------------------------------------------

    /**
     * @brief Computes signed area and centroid in a single polygon traversal.
     *
     * Uses the shoelace formula with `long double` accumulators to reduce
     * floating-point cancellation error.
     *
     * @return An `AreaCentroid2D` with `valid = true` unless the polygon is
     *         degenerate, has fewer than 3 vertices or has zero area.
     */
    [[nodiscard]] AreaCentroid2D signed_area_and_centroid() const noexcept {
        const std::size_t n = polygon.size();
        if (n < 3U) return {};

        long double twice_area = 0.0L;
        long double cx = 0.0L;
        long double cy = 0.0L;

        for (std::size_t i = 0; i < n; ++i) {
            const auto& p = polygon[i];
            const auto& q = polygon[(i + 1U) % n];

            const long double px = static_cast<long double>(p.x);
            const long double py = static_cast<long double>(p.y);
            const long double qx = static_cast<long double>(q.x);
            const long double qy = static_cast<long double>(q.y);

            const long double cross = px * qy - qx * py;
            twice_area += cross;
            cx += (px + qx) * cross;
            cy += (py + qy) * cross;
        }

        const Real signed_a = static_cast<Real>(twice_area * 0.5L);

        const long double tol =
            2.0L * static_cast<long double>(::vmm::constants::kEpsilon);

        if (std::abs(twice_area) <= tol) {
            return AreaCentroid2D{signed_a, Point2{}, false};
        }

        const long double inv6 = 1.0L / (3.0L * twice_area);

        return AreaCentroid2D{
            signed_a,
            Point2{
                static_cast<Real>(cx * inv6),
                static_cast<Real>(cy * inv6)
            },
            true
        };
    }

    /**
     * @brief Signed area of the cell polygon.
     *
     * Positive when vertices are ordered counter-clockwise.
     *
     * @return Signed area in the same units as the site coordinates squared.
     */
    [[nodiscard]] Real signed_area() const noexcept {
        return signed_area_and_centroid().signed_area;
    }

    /**
     * @brief Unsigned area of the cell polygon.
     *
     * @return Non-negative area. Returns `Real{0}` for degenerate cells.
     */
    [[nodiscard]] Real area() const noexcept {
        const Real a = signed_area();
        return a < Real{0} ? -a : a;
    }

    /**
     * @brief Perimeter of the cell polygon.
     *
     * @return Total edge length. Returns `Real{0}` for cells with fewer than
     *         2 vertices.
     */
    [[nodiscard]] Real perimeter() const noexcept {
        const std::size_t n = polygon.size();
        if (n < 2U) return Real{0};

        Real total{0};

        for (std::size_t i = 0; i < n; ++i) {
            const auto& p = polygon[i];
            const auto& q = polygon[(i + 1U) % n];

            const Real dx = q.x - p.x;
            const Real dy = q.y - p.y;

            total += std::sqrt(dx * dx + dy * dy);
        }

        return total;
    }

    /**
     * @brief Centroid of the cell polygon.
     *
     * Computed in a single pass together with the signed area.
     *
     * @return The centroid point, or `Point2{}` for degenerate cells.
     */
    [[nodiscard]] Point2 centroid() const noexcept {
        const auto result = signed_area_and_centroid();
        return result.valid ? result.centroid : Point2{};
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE