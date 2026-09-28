#pragma once
//==============================================================================
// Name        : Site2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Sites2D
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 1.2
// Description : Value type for 2D Voronoi generator sites.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Site2D.hpp
 * @brief Defines the basic 2D Voronoi generator site and its strong identifier.
 *
 * A `Site2D` is a lightweight value type used as input for Delaunay/Voronoi
 * construction.  It stores only semantic data: coordinates, id, region and
 * optional weight.  Domain-related rules (minimum distance to boundary, etc.)
 * live in `SiteValidation`.
 *
 * **Changes in v1.2:**
 *  - `SiteId` now provides `operator<=>` (C++20 three-way comparison), removing
 *    the need for hand-written comparison lambdas throughout the codebase.
 *  - `RegionId` likewise gains `operator<=>`.
 *
 * @ingroup sites2d
 */

//==============================================================================
//  C++ standard library
//==============================================================================
#include <compare>       // std::strong_ordering (operator<=>)
#include <type_traits>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Core/namespace.h>

VORMAKER_NAMESPACE_OPEN
SITE2D_NAMESPACE_OPEN


//==============================================================================
//  Type aliases imported from Boundary2DTypes
//==============================================================================

using Point2   = ::vmm::b2d::Point2;    ///< 2D point POD used throughout the library.
using Real     = ::vmm::b2d::Real;      ///< Floating-point type matching the CGAL kernel.
using Index    = ::vmm::b2d::Index;     ///< Signed integer index type.
using RegionId = ::vmm::b2d::RegionId; ///< Strong identifier for a geometric region.

//==============================================================================
//  SiteId
//==============================================================================

/**
 * @brief Strong identifier for a 2D Voronoi generator site.
 *
 * Wrapping `Index` in a named type prevents accidental mixing with other
 * integer indices (ring indices, vertex indices, etc.).  Sequential ids
 * starting from zero are required by the pipeline; see `SiteSet::ids_are_sequential()`.
 *
 * Full set of comparison operators is provided via `operator<=>` so that
 * `SiteId` values can be used directly with `std::sort`, `std::min`,
 * `std::max`, and sorted containers without helper lambdas.
 *
 * @ingroup sites2d
 */
struct SiteId {
    Index value{0};  ///< Underlying integer value.

    constexpr SiteId() = default;

    /**
     * @brief Constructs a SiteId from a raw index value.
     * @param[in] v  The integer identifier.
     */
    constexpr explicit SiteId(Index v) noexcept : value(v) {}

    /**
     * @brief Explicit conversion back to the raw index type.
     * @return The underlying `Index` value.
     */
    [[nodiscard]] constexpr explicit operator Index() const noexcept {
        return value;
    }

    /**
     * @brief Three-way comparison — provides ==, !=, <, <=, >, >= for free.
     *
     * Using `= default` delegates to the member-wise comparison of `value`.
     * The ordering is a total order consistent with integer arithmetic.
     */
    friend constexpr auto operator<=>(SiteId a, SiteId b) noexcept = default;
};

inline constexpr SiteId kInvalidSiteId{::vmm::b2d::kInvalid};

static_assert(std::is_trivially_copyable_v<SiteId>,
              "SiteId must be trivially copyable for use in CGAL vertex info.");

//==============================================================================
//  Site2D
//==============================================================================

/**
 * @brief A 2D Voronoi generator site.
 *
 * Each site carries:
 *  - a 2D coordinate (`point`),
 *  - a unique sequential identifier (`id`),
 *  - an optional region tag (`region`) useful for multi-material meshes,
 *  - an optional weight (`weight`) reserved for future weighted Voronoi diagrams.
 *
 * @note The current clipped Voronoi pipeline ignores `weight`; always leave it
 *       at the default `Real{0}` until the weighted path is implemented.
 *
 * @note Domain-level constraints (minimum distance to boundary, exclusion
 *       zones, etc.) are **not** enforced here — they are handled by
 *       `SiteValidation` so that this type remains a plain data record.
 *
 * @ingroup sites2d
 */
struct Site2D {
    Point2   point{};            ///< Generator coordinates.
    SiteId   id{};               ///< Sequential identifier assigned by SiteSet.
    RegionId region{};           ///< Optional region tag (0 = default region).
    Real     weight{Real{0}};    ///< Reserved for weighted Voronoi (unused).

    constexpr Site2D() = default;

    /**
     * @brief Constructs a site from its components.
     *
     * @param[in] point_   Spatial coordinates.
     * @param[in] id_      Sequential identifier (should match the SiteSet index).
     * @param[in] region_  Region tag (default 0).
     * @param[in] weight_  Weight for future weighted diagrams (default 0).
     */
    constexpr Site2D(Point2   point_,
                     SiteId   id_      = SiteId{0},
                     RegionId region_  = RegionId{0},
                     Real     weight_  = Real{0}) noexcept
        : point{point_}
        , id{id_}
        , region{region_}
        , weight{weight_}
    {}
};

// Compile-time layout guarantees required by the Delaunay builder and IO.
static_assert(std::is_trivially_copyable_v<Site2D>,
              "Site2D must be trivially copyable.");
static_assert(std::is_standard_layout_v<Site2D>,
              "Site2D must have standard layout.");

SITE2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
