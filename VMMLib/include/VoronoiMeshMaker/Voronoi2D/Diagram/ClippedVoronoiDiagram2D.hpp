#pragma once
//==============================================================================
// Name        : ClippedVoronoiDiagram2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Diagram
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 1.3
// Description : Value container for a complete clipped 2D Voronoi diagram.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file ClippedVoronoiDiagram2D.hpp
 * @brief Stores all clipped Voronoi cells and provides range views over them.
 *
 * Changes in v1.3:
 *  - The six raw-pointer arrays (`all_mutable_volume_ptrs`, etc.) and the
 *    corresponding `VoronoiCellRange2D` / `VoronoiCellPointerRange2D` custom
 *    iterator types have been removed. They stored a full second copy of
 *    every pointer and had to be kept in sync with `cells` via manual calls to
 *    `rebuild_indices()`.
 *
 *    Replacement: `all_volumes()`, `internal_volumes()`, and
 *    `boundary_volumes()` now return lazy `std::ranges` views built directly
 *    over `cells`. No extra allocations, no sync required, no pointer
 *    invalidation.
 *
 *  - Backward-compatible pointer views are provided through
 *    `all_volume_pointers()`, `internal_volume_pointers()`, and
 *    `boundary_volume_pointers()`. These methods do not store pointer arrays;
 *    they return lazy views over `cells`.
 *
 *  - `total_volume_area()` now uses `std::transform_reduce` with
 *    `std::execution::par_unseq` for parallel summation.
 *
 *  - `rebuild_indices()` still needs to be called after bulk mutation of
 *    `cells` (e.g. after renumbering) because `site_to_cell_index`,
 *    `all_indices`, `internal_indices`, and `boundary_indices` are still kept
 *    for fast O(1) lookup by `SiteId`.
 *
 * @ingroup voronoi2d_diagram
 */

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <execution>   // std::execution::par_unseq
#include <numeric>     // std::transform_reduce
#include <ranges>      // std::views::all, std::views::filter, std::views::transform
#include <span>
#include <string>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCell2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

//==============================================================================
//  Volume numbering bookkeeping
//==============================================================================

/**
 * @brief Identifies the renumbering method applied to a diagram's volumes.
 * @ingroup voronoi2d_diagram
 */
enum class VolumeNumberingMethod2D : unsigned char {
    Original,      ///< Insertion order (default).
    Hilbert,       ///< Hilbert space-filling curve over cell centroids.
    Lexicographic, ///< Sorted by centroid (x, then y).
    Custom         ///< User-supplied permutation.
};

/**
 * @brief Tracks which renumbering method was last applied to the diagram.
 * @ingroup voronoi2d_diagram
 */
struct VolumeNumberingState2D {
    VolumeNumberingMethod2D method{VolumeNumberingMethod2D::Original};
    bool explicitly_renumbered{false}; ///< True after any renumber_volumes() call.
};

//==============================================================================
//  ClippedVoronoiDiagram2D
//==============================================================================

/**
 * @brief Complete clipped 2D Voronoi diagram for one Boundary2D / SiteSet pair.
 *
 * Primary storage is the `cells` vector, one `VoronoiCell2D` per generator
 * site, in the current `volume_id` order. Auxiliary index vectors
 * (`all_indices`, `internal_indices`, `boundary_indices`, `site_to_cell_index`)
 * provide O(1) lookup and are rebuilt by `rebuild_indices()`.
 *
 * ### Iteration
 *
 * | Method                       | Returns                            | Notes               |
 * |------------------------------|------------------------------------|---------------------|
 * | `all_volumes()`              | range over every cell              | lazy, no allocation |
 * | `internal_volumes()`         | range over non-boundary cells only | lazy, no allocation |
 * | `boundary_volumes()`         | range over boundary cells only     | lazy, no allocation |
 * | `all_volume_pointers()`      | range over pointers to all cells   | lazy, no allocation |
 * | `internal_volume_pointers()` | range over pointers to internal cells | lazy, no allocation |
 * | `boundary_volume_pointers()` | range over pointers to boundary cells | lazy, no allocation |
 *
 * All range methods have `const` and non-`const` overloads.
 *
 * ### Thread safety
 *
 * The diagram is not thread-safe for concurrent writes. Concurrent reads are
 * safe once `rebuild_indices()` has been called and no mutation is in progress.
 *
 * @ingroup voronoi2d_diagram
 */
struct ClippedVoronoiDiagram2D {
    bool boundary_partitioned{false};

    void require_boundary_partition() const {
        if (!boundary_partitioned) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"reason", "boundary_partition_required"}});
        }
    }
    [[nodiscard]] std::span<VoronoiCell2D> internal_span() {
        require_boundary_partition();
        return std::span(cells).first(internal_indices.size());
    }
    [[nodiscard]] std::span<const VoronoiCell2D> internal_span() const {
        require_boundary_partition();
        return std::span(cells).first(internal_indices.size());
    }
    [[nodiscard]] std::span<VoronoiCell2D> boundary_span() {
        require_boundary_partition();
        return std::span(cells).subspan(internal_indices.size());
    }
    [[nodiscard]] std::span<const VoronoiCell2D> boundary_span() const {
        require_boundary_partition();
        return std::span(cells).subspan(internal_indices.size());
    }
    using Real   = ::vmm::s2d::Real;
    using SiteId = ::vmm::s2d::SiteId;

    //--------------------------------------------------------------------------
    // Primary data
    //--------------------------------------------------------------------------

    ::vmm::b2d::Boundary2DData boundary{};          ///< Domain boundary.
    ::vmm::s2d::SiteSet        sites{};             ///< Generator sites.
    DelaunayTriangulation2D    delaunay{};          ///< Underlying triangulation.
    std::vector<VoronoiCell2D> cells{};             ///< One cell per site, in volume_id order.

    //--------------------------------------------------------------------------
    // Index structures rebuilt by rebuild_indices()
    //--------------------------------------------------------------------------

    std::vector<std::size_t> all_indices{};         ///< `[0, 1, ..., n-1]` in current order.
    std::vector<std::size_t> internal_indices{};    ///< Indices of non-boundary cells.
    std::vector<std::size_t> boundary_indices{};    ///< Indices of boundary cells.
    std::vector<std::size_t> site_to_cell_index{};  ///< `site_id.value` to index in cells.
    VolumeNumberingState2D   numbering{};           ///< Tracks last renumbering.

    //--------------------------------------------------------------------------
    // Basic size queries
    //--------------------------------------------------------------------------

    /** @return True when no cells have been built yet. */
    [[nodiscard]] bool empty() const noexcept {
        return cells.empty();
    }

    /** @return Total number of cells. */
    [[nodiscard]] std::size_t cell_count() const noexcept {
        return cells.size();
    }

    /** @return Total number of volumes, alias for cell_count(). */
    [[nodiscard]] std::size_t volume_count() const noexcept {
        return cells.size();
    }

    /** @return Number of cells that do not intersect the boundary. */
    [[nodiscard]] std::size_t internal_volume_count() const noexcept {
        return internal_indices.size();
    }

    /** @return Number of cells that intersect the boundary. */
    [[nodiscard]] std::size_t boundary_volume_count() const noexcept {
        return boundary_indices.size();
    }

    /** @return True after any `renumber_volumes()` call. */
    [[nodiscard]] bool volumes_are_renumbered() const noexcept {
        return numbering.explicitly_renumbered;
    }

    /** @return The last renumbering method applied. */
    [[nodiscard]] VolumeNumberingMethod2D volume_numbering_method() const noexcept {
        return numbering.method;
    }

    //--------------------------------------------------------------------------
    // Direct cell access
    //--------------------------------------------------------------------------

    /**
     * @brief Returns the cell at storage position `i`.
     *
     * @param[in] i Storage index in `cells`.
     * @return Const reference to the cell.
     * @throws vmm::error::VMMException CoreErr::OutOfRange if `i >= cell_count()`.
     */
    [[nodiscard]] const VoronoiCell2D& cell(std::size_t i) const {
        if (i >= cells.size()) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"index", std::to_string(i)}});
        }
        return cells[i];
    }

    /**
     * @brief Returns the cell whose generator has identifier `id`.
     *
     * Uses `site_to_cell_index` for O(1) lookup when the index is populated.
     *
     * @param[in] id Site identifier.
     * @return Const reference to the cell.
     * @throws vmm::error::VMMException CoreErr::OutOfRange if `id` is not found.
     */
    [[nodiscard]] const VoronoiCell2D& cell(SiteId id) const {
        const auto raw = id.value;
        if (raw < 0) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"index", std::to_string(raw)}});
        }

        const auto slot = static_cast<std::size_t>(raw);

        if (!site_to_cell_index.empty()) {
            if (slot >= site_to_cell_index.size() ||
                site_to_cell_index[slot] >= cells.size()) {
                VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                          {{"index", std::to_string(raw)}});
            }
            return cells[site_to_cell_index[slot]];
        }

        if (slot >= cells.size()) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"index", std::to_string(raw)}});
        }

        return cells[slot];
    }

    //--------------------------------------------------------------------------
    // Range views, lazy and allocation-free
    //--------------------------------------------------------------------------

    /**
     * @brief Lazy range over all cells in current volume_id order.
     *
     * The returned view references `cells` directly. Mutating `cells`
     * invalidates the view.
     *
     * @return Range yielding `VoronoiCell2D&`.
     */
    [[nodiscard]] auto all_volumes() noexcept {
        return std::views::all(cells);
    }

    /**
     * @brief Lazy range over all cells in current volume_id order.
     *
     * Const overload.
     *
     * @return Range yielding `const VoronoiCell2D&`.
     */
    [[nodiscard]] auto all_volumes() const noexcept {
        return std::views::all(cells);
    }

    /**
     * @brief Lazy range over cells that do not intersect the domain boundary.
     *
     * @return Range yielding `VoronoiCell2D&`.
     */
    [[nodiscard]] auto internal_volumes() noexcept {
        return internal_indices | std::views::transform(
            [this](std::size_t i) -> VoronoiCell2D& {
                return cells[i];
            });
    }

    /**
     * @brief Lazy range over cells that do not intersect the domain boundary.
     *
     * Const overload.
     *
     * @return Range yielding `const VoronoiCell2D&`.
     */
    [[nodiscard]] auto internal_volumes() const noexcept {
        return internal_indices | std::views::transform(
            [this](std::size_t i) -> const VoronoiCell2D& {
                return cells[i];
            });
    }

    /**
     * @brief Lazy range over cells that intersect the domain boundary.
     *
     * @return Range yielding `VoronoiCell2D&`.
     */
    [[nodiscard]] auto boundary_volumes() noexcept {
        return boundary_indices | std::views::transform(
            [this](std::size_t i) -> VoronoiCell2D& {
                return cells[i];
            });
    }

    /**
     * @brief Lazy range over cells that intersect the domain boundary.
     *
     * Const overload.
     *
     * @return Range yielding `const VoronoiCell2D&`.
     */
    [[nodiscard]] auto boundary_volumes() const noexcept {
        return boundary_indices | std::views::transform(
            [this](std::size_t i) -> const VoronoiCell2D& {
                return cells[i];
            });
    }

    /** @brief Read-only points in current volume order, then local face order.
     * @note This is a lazy range, not a contiguous array. Reacquire it after
     * renumbering or changing geometry. No duplicate coordinate storage exists.
     */
    [[nodiscard]] auto boundary_condition_points() const & noexcept {
        return cells | std::views::transform([](const auto& cell)
            -> const std::vector<VoronoiCellEdge2D>& { return cell.edges; })
            | std::views::join
            | std::views::filter([](const auto& edge) { return edge.is_boundary_edge; })
            | std::views::transform([](const auto& edge) -> const ::vmm::s2d::Point2& {
                return edge.boundary_condition_geometry().point();
            });
    }
    void boundary_condition_points() const && = delete;

    /** @brief Lazy traversal of constant pointers to boundary coordinates. */
    [[nodiscard]] auto boundary_condition_point_pointers() const & noexcept {
        return boundary_condition_points() | std::views::transform(
            [](const auto& point) { return &point; });
    }
    void boundary_condition_point_pointers() const && = delete;

    //--------------------------------------------------------------------------
    // Backward-compatible pointer range views, lazy and allocation-free
    //--------------------------------------------------------------------------

    /**
     * @brief Lazy range over pointers to all cells.
     *
     * Preserves compatibility with older code that expected
     * `all_volume_pointers()`, without storing a second array of raw pointers.
     *
     * @return Range yielding `VoronoiCell2D*`.
     */
    [[nodiscard]] auto all_volume_pointers() noexcept {
        return cells | std::views::transform(
            [](VoronoiCell2D& c) noexcept {
                return &c;
            });
    }

    /**
     * @brief Lazy range over pointers to all cells.
     *
     * Const overload preserving compatibility with older code.
     *
     * @return Range yielding `const VoronoiCell2D*`.
     */
    [[nodiscard]] auto all_volume_pointers() const noexcept {
        return cells | std::views::transform(
            [](const VoronoiCell2D& c) noexcept {
                return &c;
            });
    }

    /**
     * @brief Lazy range over pointers to internal cells.
     *
     * Preserves compatibility with older code that expected
     * `internal_volume_pointers()`, without storing a second array of raw
     * pointers.
     *
     * @return Range yielding `VoronoiCell2D*`.
     */
    [[nodiscard]] auto internal_volume_pointers() noexcept {
        return internal_volumes()
            | std::views::transform(
                [](VoronoiCell2D& c) noexcept {
                    return &c;
                });
    }

    /**
     * @brief Lazy range over pointers to internal cells.
     *
     * Const overload preserving compatibility with older code.
     *
     * @return Range yielding `const VoronoiCell2D*`.
     */
    [[nodiscard]] auto internal_volume_pointers() const noexcept {
        return internal_volumes()
            | std::views::transform(
                [](const VoronoiCell2D& c) noexcept {
                    return &c;
                });
    }

    /**
     * @brief Lazy range over pointers to boundary cells.
     *
     * Preserves compatibility with older code that expected
     * `boundary_volume_pointers()`, without storing a second array of raw
     * pointers.
     *
     * @return Range yielding `VoronoiCell2D*`.
     */
    [[nodiscard]] auto boundary_volume_pointers() noexcept {
        return boundary_volumes()
            | std::views::transform(
                [](VoronoiCell2D& c) noexcept {
                    return &c;
                });
    }

    /**
     * @brief Lazy range over pointers to boundary cells.
     *
     * Const overload preserving compatibility with older code.
     *
     * @return Range yielding `const VoronoiCell2D*`.
     */
    [[nodiscard]] auto boundary_volume_pointers() const noexcept {
        return boundary_volumes()
            | std::views::transform(
                [](const VoronoiCell2D& c) noexcept {
                    return &c;
                });
    }

    //--------------------------------------------------------------------------
    // Aggregate geometric queries
    //--------------------------------------------------------------------------

    /**
     * @brief Sum of all cell areas.
     *
     * Uses `std::transform_reduce` with `std::execution::par_unseq` for
     * parallel summation on multi-core hardware. Each cell is first mapped to
     * its scalar area, and the reduction then combines only `Real` values.
     *
     * @return Total area covered by the diagram cells.
     */
    [[nodiscard]] Real total_volume_area() const noexcept {
        return std::transform_reduce(
            std::execution::par_unseq,
            cells.begin(),
            cells.end(),
            Real{0},
            [](Real lhs, Real rhs) noexcept {
                return lhs + rhs;
            },
            [](const VoronoiCell2D& c) noexcept {
                return c.area();
            });
    }

    //--------------------------------------------------------------------------
    // Index management
    //--------------------------------------------------------------------------

    /**
     * @brief Rebuilds all auxiliary index structures after `cells` has changed.
     *
     * Must be called after:
     *  - `cells` is populated by the builder,
     *  - volume renumbering reorders `cells`,
     *  - any direct mutation of `cells`.
     *
     * The method is O(n) in the number of cells.
     */
    void rebuild_indices() {
        boundary_partitioned = std::is_partitioned(cells.begin(), cells.end(),
            [](const auto& cell) { return !cell.is_boundary_cell; });
        all_indices.clear();
        internal_indices.clear();
        boundary_indices.clear();
        site_to_cell_index.assign(sites.size(), cells.size());

        all_indices.reserve(cells.size());
        internal_indices.reserve(cells.size());
        boundary_indices.reserve(cells.size());

        for (std::size_t i = 0; i < cells.size(); ++i) {
            cells[i].volume_id = i;
            all_indices.push_back(i);

            const auto raw = cells[i].site_id.value;
            if (raw >= 0 &&
                static_cast<std::size_t>(raw) < site_to_cell_index.size()) {
                site_to_cell_index[static_cast<std::size_t>(raw)] = i;
            }

            if (cells[i].is_boundary_cell) {
                boundary_indices.push_back(i);
            } else {
                internal_indices.push_back(i);
            }
        }
    }

    /**
     * @brief Alias for `rebuild_indices()` kept for backward compatibility.
     * @deprecated Call `rebuild_indices()` directly.
     */
    void rebuild_classification_indices() {
        rebuild_indices();
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
