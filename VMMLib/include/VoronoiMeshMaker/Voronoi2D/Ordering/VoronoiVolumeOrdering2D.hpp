#pragma once
//==============================================================================
// Name        : VoronoiVolumeOrdering2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Ordering
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 1.2
// Description : Pluggable volume renumbering policies for clipped 2D Voronoi
//               diagrams.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VoronoiVolumeOrdering2D.hpp
 * @brief Reorders Voronoi volumes with value-type ordering policies.
 *
 * Each policy is a callable struct that takes a `const ClippedVoronoiDiagram2D&`
 * and returns a `std::vector<std::size_t>` permutation (new_to_old mapping).
 * New policies can be added without modifying this header — just provide a
 * type that satisfies the same call signature and pass it to
 * `renumber_volumes()`.
 *
 * **Changes in v1.2:**
 *  - `LexicographicVolumeOrdering2D` and `HilbertVolumeOrdering2D` now
 *    pre-compute all cell centroids in a single parallel pass before sorting.
 *    Previously the comparator called `centroid()` on every comparison,
 *    resulting in O(n log n) polygon traversals instead of O(n).
 *  - `std::sort` is called with `std::execution::par_unseq` so the sort
 *    itself is also parallelised on multi-core hardware.
 *
 * @ingroup voronoi2d_ordering
 */

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstdint>
#include <execution>   // std::execution::par_unseq
#include <limits>
#include <numeric>     // std::iota
#include <span>
#include <string>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN



//==============================================================================
//  VolumeRenumbering2D — result type
//==============================================================================

/**
 * @brief Bidirectional mapping produced by renumber_volumes().
 * @ingroup voronoi2d_ordering
 */
struct VolumeRenumbering2D {
    std::vector<std::size_t> old_to_new{}; ///< `old_to_new[old_id] == new_id`.
    std::vector<std::size_t> new_to_old{}; ///< `new_to_old[new_id] == old_id`.
};

//==============================================================================
//  InputVolumeOrdering2D — identity permutation
//==============================================================================

/**
 * @brief Ordering policy that keeps the current volume order unchanged.
 *
 * @ingroup voronoi2d_ordering
 */
struct InputVolumeOrdering2D {
    /**
     * @param[in] diagram  Source diagram (unused — returns identity order).
     * @return Identity permutation `[0, 1, …, n-1]`.
     */
    [[nodiscard]] std::vector<std::size_t>
    operator()(const ClippedVoronoiDiagram2D& diagram) const {
        std::vector<std::size_t> order(diagram.cells.size());
        std::iota(order.begin(), order.end(), std::size_t{0});
        return order;
    }
};

//==============================================================================
//  LexicographicVolumeOrdering2D
//==============================================================================

/**
 * @brief Sorts volumes by centroid x-coordinate, then by y-coordinate.
 *
 * Ties in both coordinates are broken by `site_id.value` to guarantee
 * a deterministic total order.
 *
 * Centroids are pre-computed in a parallel O(n) pass before the sort to
 * avoid O(n log n) redundant polygon traversals inside the comparator.
 *
 * @ingroup voronoi2d_ordering
 */
struct LexicographicVolumeOrdering2D {
    /**
     * @param[in] diagram  Diagram whose cells are to be ordered.
     * @return Permutation vector `new_to_old` such that
     *         `diagram.cells[result[i]]` has rank `i` in lexicographic order.
     */
    [[nodiscard]] std::vector<std::size_t>
    operator()(const ClippedVoronoiDiagram2D& diagram) const {
        using Point2 = ::vmm::s2d::Point2;
        const std::size_t n = diagram.cells.size();

        // Pre-compute all centroids in one parallel pass: O(n).
        std::vector<Point2> centroids(n);
        std::transform(
            std::execution::par_unseq,
            diagram.cells.begin(), diagram.cells.end(),
            centroids.begin(),
            [](const VoronoiCell2D& cell) { return cell.centroid(); });

        // Build index array and sort in parallel: O(n log n).
        std::vector<std::size_t> order(n);
        std::iota(order.begin(), order.end(), std::size_t{0});

        std::sort(
            std::execution::par_unseq,
            order.begin(), order.end(),
            [&](std::size_t lhs, std::size_t rhs) noexcept {
                const auto& a = centroids[lhs];
                const auto& b = centroids[rhs];
                if (a.x != b.x) return a.x < b.x;
                if (a.y != b.y) return a.y < b.y;
                return diagram.cells[lhs].site_id < diagram.cells[rhs].site_id;
            });

        return order;
    }
};

//==============================================================================
//  HilbertVolumeOrdering2D
//==============================================================================

/**
 * @brief Sorts volumes along a Hilbert space-filling curve over cell centroids.
 *
 * Hilbert ordering is the standard technique for reducing the matrix bandwidth
 * of sparse systems assembled from unstructured meshes.  The integer grid
 * resolution is controlled by `bits` (1–31); a value of 16 gives
 * 65 536 × 65 536 grid cells — sufficient for any practical mesh size.
 *
 * Centroids are pre-computed in a parallel O(n) pass before the sort
 * (same optimisation as `LexicographicVolumeOrdering2D`).
 *
 * @ingroup voronoi2d_ordering
 */
struct HilbertVolumeOrdering2D {
    unsigned bits{16}; ///< Hilbert curve resolution (1–31, default 16).

    /**
     * @param[in] diagram  Diagram whose cells are to be ordered.
     * @return Permutation vector `new_to_old` sorted by Hilbert key of the
     *         cell centroid.
     *
     * @throws vmm::error::VMMException CoreErr::InvalidArgument
     *         if `bits` is 0 or greater than 31.
     */
    [[nodiscard]] std::vector<std::size_t>
    operator()(const ClippedVoronoiDiagram2D& diagram) const {
        if (bits == 0U || bits > 31U) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where",  "HilbertVolumeOrdering2D"},
                       {"reason", "bits_must_be_between_1_and_31"}});
        }

        using Point2 = ::vmm::s2d::Point2;
        const std::size_t n = diagram.cells.size();

        std::vector<std::size_t> order(n);
        std::iota(order.begin(), order.end(), std::size_t{0});
        if (n < 2U) return order;

        // Pre-compute all centroids in one parallel pass: O(n).
        std::vector<Point2> centroids(n);
        std::transform(
            std::execution::par_unseq,
            diagram.cells.begin(), diagram.cells.end(),
            centroids.begin(),
            [](const VoronoiCell2D& cell) { return cell.centroid(); });

        // Compute bounding box of centroids.
        const auto box = centroid_box(centroids);

        // Sort in parallel: O(n log n).
        const unsigned b = bits;
        std::sort(
            std::execution::par_unseq,
            order.begin(), order.end(),
            [&](std::size_t lhs, std::size_t rhs) noexcept {
                const auto& a = centroids[lhs];
                const auto& bpt = centroids[rhs];
                const std::uint64_t ka = hilbert_key(
                    normalized_grid_coordinate(a.x,   box.min_x, box.max_x, b),
                    normalized_grid_coordinate(a.y,   box.min_y, box.max_y, b), b);
                const std::uint64_t kb = hilbert_key(
                    normalized_grid_coordinate(bpt.x, box.min_x, box.max_x, b),
                    normalized_grid_coordinate(bpt.y, box.min_y, box.max_y, b), b);
                if (ka != kb) return ka < kb;
                return diagram.cells[lhs].site_id < diagram.cells[rhs].site_id;
            });

        return order;
    }

private:
    using Point2 = ::vmm::s2d::Point2;
    using Real   = ::vmm::s2d::Real;

    struct CentroidBox {
        Real min_x{+std::numeric_limits<Real>::infinity()};
        Real min_y{+std::numeric_limits<Real>::infinity()};
        Real max_x{-std::numeric_limits<Real>::infinity()};
        Real max_y{-std::numeric_limits<Real>::infinity()};
    };

    /**
     * @brief Computes the axis-aligned bounding box of a set of centroids.
     * @param[in] pts  Centroid points.
     * @return Tight bounding box.
     */
    [[nodiscard]] static CentroidBox
    centroid_box(const std::vector<Point2>& pts) noexcept {
        CentroidBox box;
        for (const auto& p : pts) {
            box.min_x = std::min(box.min_x, p.x);
            box.min_y = std::min(box.min_y, p.y);
            box.max_x = std::max(box.max_x, p.x);
            box.max_y = std::max(box.max_y, p.y);
        }
        return box;
    }

    /**
     * @brief Maps a real coordinate to an unsigned grid cell in [0, 2^bits).
     *
     * @param[in] value      Coordinate to map.
     * @param[in] min_value  Box minimum.
     * @param[in] max_value  Box maximum.
     * @param[in] b          Number of bits (grid resolution).
     * @return Grid cell index in [0, 2^b - 1].
     */
    [[nodiscard]] static std::uint32_t
    normalized_grid_coordinate(Real value, Real min_value, Real max_value,
                               unsigned b) noexcept {
        const std::uint32_t max_grid =
            static_cast<std::uint32_t>((std::uint64_t{1} << b) - 1U);
        if (!(max_value > min_value)) return 0U;

        const long double t =
            (static_cast<long double>(value) -
             static_cast<long double>(min_value)) /
            (static_cast<long double>(max_value) -
             static_cast<long double>(min_value));
        const long double clamped = std::clamp(t, 0.0L, 1.0L);
        return static_cast<std::uint32_t>(
            clamped * static_cast<long double>(max_grid));
    }

    static void rotate(std::uint32_t n,
                       std::uint32_t& x,
                       std::uint32_t& y,
                       std::uint32_t rx,
                       std::uint32_t ry) noexcept {
        if (ry == 0U) {
            if (rx == 1U) {
                x = n - 1U - x;
                y = n - 1U - y;
            }
            std::swap(x, y);
        }
    }

    /**
     * @brief Computes the Hilbert curve key for grid cell (x, y).
     *
     * Standard in-place rotation algorithm (see Wikipedia "Hilbert curve").
     *
     * @param[in] x   Grid x-coordinate in [0, 2^b).
     * @param[in] y   Grid y-coordinate in [0, 2^b).
     * @param[in] b   Curve resolution bits.
     * @return 64-bit Hilbert key.
     */
    [[nodiscard]] static std::uint64_t
    hilbert_key(std::uint32_t x, std::uint32_t y, unsigned b) noexcept {
        const std::uint32_t n =
            static_cast<std::uint32_t>(std::uint64_t{1} << b);
        std::uint64_t d = 0;
        for (std::uint32_t s = n / 2U; s > 0U; s /= 2U) {
            const std::uint32_t rx = (x & s) ? 1U : 0U;
            const std::uint32_t ry = (y & s) ? 1U : 0U;
            d += static_cast<std::uint64_t>(s) *
                 static_cast<std::uint64_t>(s) *
                 static_cast<std::uint64_t>((3U * rx) ^ ry);
            rotate(s, x, y, rx, ry);
        }
        return d;
    }
};

//==============================================================================
//  Validation helper
//==============================================================================

/**
 * @brief Verifies that `order` is a valid permutation of `[0, expected_size)`.
 *
 * @param[in] order          Permutation to validate.
 * @param[in] expected_size  Expected number of elements.
 *
 * @throws vmm::error::VMMException CoreErr::InvalidArgument
 *         if `order.size() != expected_size` or if any element is out of
 *         range or duplicated.
 *
 * @ingroup voronoi2d_ordering
 */
inline void validate_volume_order_or_throw(std::span<const std::size_t> order,
                                           std::size_t expected_size)
{
    if (order.size() != expected_size) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where",  "validate_volume_order_or_throw"},
                   {"reason", "order_size_mismatch"}});
    }

    std::vector<bool> seen(expected_size, false);
    for (const auto value : order) {
        if (value >= expected_size || seen[value]) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where",  "validate_volume_order_or_throw"},
                       {"reason", "order_is_not_a_permutation"},
                       {"index",  std::to_string(value)}});
        }
        seen[value] = true;
    }
}

//==============================================================================
//  renumber_volumes — generic overload
//==============================================================================

/**
 * @brief Reorders cells in `diagram` according to the permutation returned by
 *        `ordering`, then rebuilds the internal index structures.
 *
 * @tparam Ordering  Any callable that maps `const ClippedVoronoiDiagram2D&`
 *                   to `std::vector<std::size_t>` (new_to_old permutation).
 *
 * @param[in,out] diagram   Diagram to renumber in place.
 * @param[in]     ordering  Ordering policy instance.
 * @param[in]     method    Numbering method tag written to `diagram.numbering`.
 *
 * @return `VolumeRenumbering2D` with both the `old_to_new` and `new_to_old`
 *         maps.
 *
 * @throws vmm::error::VMMException CoreErr::InvalidArgument
 *         if the ordering policy returns an invalid permutation.
 *
 * @ingroup voronoi2d_ordering
 */
template <class Ordering>
VolumeRenumbering2D renumber_volumes(
    ClippedVoronoiDiagram2D& diagram,
    const Ordering& ordering,
    VolumeNumberingMethod2D method = VolumeNumberingMethod2D::Custom)
{
    auto new_to_old = ordering(diagram);
    validate_volume_order_or_throw(new_to_old, diagram.cells.size());

    VolumeRenumbering2D result;
    result.new_to_old = std::move(new_to_old);
    result.old_to_new.resize(result.new_to_old.size());
    for (std::size_t new_id = 0; new_id < result.new_to_old.size(); ++new_id) {
        result.old_to_new[result.new_to_old[new_id]] = new_id;
    }

    // Reorder cells in place (avoids a full extra copy).
    std::vector<VoronoiCell2D> reordered;
    reordered.reserve(diagram.cells.size());
    for (const auto old_id : result.new_to_old) {
        reordered.push_back(std::move(diagram.cells[old_id]));
    }
    diagram.cells = std::move(reordered);
    diagram.rebuild_indices();
    diagram.numbering = VolumeNumberingState2D{
        method,
        method != VolumeNumberingMethod2D::Original
    };
    return result;
}

//==============================================================================
//  renumber_volumes — typed overloads (set the correct method tag automatically)
//==============================================================================

/**
 * @brief Renumbers volumes using Hilbert ordering.
 * @ingroup voronoi2d_ordering
 */
inline VolumeRenumbering2D renumber_volumes(
    ClippedVoronoiDiagram2D& diagram,
    const HilbertVolumeOrdering2D& ordering)
{
    return renumber_volumes(diagram, ordering, VolumeNumberingMethod2D::Hilbert);
}

/**
 * @brief Renumbers volumes using lexicographic (centroid x, then y) ordering.
 * @ingroup voronoi2d_ordering
 */
inline VolumeRenumbering2D renumber_volumes(
    ClippedVoronoiDiagram2D& diagram,
    const LexicographicVolumeOrdering2D& ordering)
{
    return renumber_volumes(diagram, ordering, VolumeNumberingMethod2D::Lexicographic);
}

/**
 * @brief Renumbers volumes using the identity ordering (no-op reorder).
 * @ingroup voronoi2d_ordering
 */
inline VolumeRenumbering2D renumber_volumes(
    ClippedVoronoiDiagram2D& diagram,
    const InputVolumeOrdering2D& ordering)
{
    return renumber_volumes(diagram, ordering, VolumeNumberingMethod2D::Original);
}

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
