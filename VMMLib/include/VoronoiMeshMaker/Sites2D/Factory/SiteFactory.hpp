#pragma once
//==============================================================================
// Name        : SiteFactory.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Sites2D / Factory
// Description : Factory functions for 2D generator site distributions.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file SiteFactory.hpp
 * @brief Creates SiteSet objects from explicit distribution patterns.
 *
 * Factories are implemented as value-type patterns plus overloads, not through
 * inheritance. New distributions can be added by defining a new pattern type and
 * a corresponding `make_sites(boundary, pattern, ...)` overload.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <random>
#include <string>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Validation/Boundary2DValidation.hpp>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteValidation.hpp>

VORMAKER_NAMESPACE_OPEN
SITE2D_NAMESPACE_OPEN

/**
 * @brief Deterministic Cartesian grid distribution over a Boundary2D box.
 *
 * Candidate cell centroids are generated from the boundary bounding box. The
 * default generator placement is the centroid itself, which corresponds to
 * `epsilon = 0`.
 *
 * The paper's Cartesian eccentricity formula for theta = 0 is:
 * `x = x_c + epsilon*dx/2`, `y = y_c - epsilon*dy/2`, with
 * `-1 < epsilon < 1`.
 */
struct CartesianGrid2D {
    Real spacing_x{Real{1}};
    Real spacing_y{Real{1}};
    Real epsilon{Real{0}};
    Point2 origin{};
    Point2 custom_offset{Real{0.5}, Real{0.5}};
    bool use_origin{false};
    bool center_in_box{false};
    bool use_custom_offset{false};

    constexpr CartesianGrid2D() = default;

    explicit constexpr CartesianGrid2D(Real spacing)
        : spacing_x(spacing), spacing_y(spacing) {}

    constexpr CartesianGrid2D(Real dx, Real dy)
        : spacing_x(dx), spacing_y(dy) {}

    constexpr CartesianGrid2D(Real dx, Real dy, Point2 offset_fraction)
        : spacing_x(dx),
          spacing_y(dy),
          custom_offset(offset_fraction),
          use_custom_offset(true) {}

    [[nodiscard]] constexpr CartesianGrid2D with_origin(Point2 p) const noexcept {
        auto copy = *this;
        copy.origin = p;
        copy.use_origin = true;
        copy.center_in_box = false;
        return copy;
    }

    [[nodiscard]] constexpr CartesianGrid2D centered_in_box() const noexcept {
        auto copy = *this;
        copy.center_in_box = true;
        copy.use_origin = false;
        return copy;
    }

    [[nodiscard]] constexpr CartesianGrid2D with_epsilon(Real value) const noexcept {
        auto copy = *this;
        copy.epsilon = value;
        copy.use_custom_offset = false;
        return copy;
    }

    [[nodiscard]] constexpr CartesianGrid2D with_eccentricity(Real value) const noexcept {
        return with_epsilon(value);
    }
};

/**
 * @brief Deterministic Cartesian grid with a requested candidate count.
 *
 * The pattern creates `nx * ny` candidates at cell centers in the boundary
 * bounding box, then clips them by the actual Boundary2D.
 */
struct CartesianGridCount2D {
    std::size_t nx{1};
    std::size_t ny{1};

    constexpr CartesianGridCount2D() = default;
    constexpr CartesianGridCount2D(std::size_t nx_, std::size_t ny_)
        : nx(nx_), ny(ny_) {}
};

/**
 * @brief Axis-aligned generation box used by site patterns before clipping.
 *
 * Site distributions are intentionally independent from Boundary2D. The
 * boundary-aware factory overloads use the boundary bounding box only as a
 * convenient generation window, then validate and clip candidates afterwards.
 */
struct SiteGenerationBox2D {
    Point2 min{};
    Point2 max{};
};

/**
 * @brief Triangular II pattern from the reference paper.
 *
 * A square patch of side `spacing` is divided into two triangular control
 * volumes. Generator sites are placed from each triangle centroid using the
 * paper's eccentricity parameter `epsilon`.
 *
 * For theta = 0, Eq. (15) gives the central generator displacement
 * `(-k*h/6, +k*h/6)` for one triangle orientation. The complementary triangle
 * uses the opposite sign so each generator remains on the corresponding
 * admissible red line of the pattern.
 *
 * The paper defines `k = epsilon` for `0 <= epsilon < 1` and
 * `k = 2*epsilon` for `-1 < epsilon < 0`.
 */
struct TriangularIIGrid2D {
    Real spacing{Real{1}};
    Real epsilon{Real{0}};
    Point2 origin{};

    constexpr TriangularIIGrid2D() = default;

    explicit constexpr TriangularIIGrid2D(Real spacing_)
        : spacing(spacing_) {}

    constexpr TriangularIIGrid2D(Real spacing_, Real epsilon_)
        : spacing(spacing_), epsilon(epsilon_) {}

    [[nodiscard]] constexpr TriangularIIGrid2D with_epsilon(Real value) const noexcept {
        auto copy = *this;
        copy.epsilon = value;
        return copy;
    }

    [[nodiscard]] constexpr TriangularIIGrid2D with_origin(Point2 p) const noexcept {
        auto copy = *this;
        copy.origin = p;
        return copy;
    }
};

/**
 * @brief Triangular IV pattern from the reference paper.
 *
 * A square patch of side `spacing` is divided into four triangular control
 * volumes by connecting the square center to its four corners. Generator sites
 * are placed from each triangle centroid along the local altitude direction.
 *
 * For theta = 0, Eq. (20) gives the central generator displacement
 * `(0, +k*h/6)` for the reference orientation. The other three local
 * orientations are rotations of the same rule.
 *
 * This implementation follows the legacy generator convention, where positive
 * `epsilon` moves the reference point from the centroid toward the square
 * center and negative `epsilon` moves it toward the closest edge midpoint.
 */
struct TriangularIVGrid2D {
    Real spacing{Real{1}};
    Real epsilon{Real{0}};
    Point2 origin{};

    constexpr TriangularIVGrid2D() = default;

    explicit constexpr TriangularIVGrid2D(Real spacing_)
        : spacing(spacing_) {}

    constexpr TriangularIVGrid2D(Real spacing_, Real epsilon_)
        : spacing(spacing_), epsilon(epsilon_) {}

    [[nodiscard]] constexpr TriangularIVGrid2D with_epsilon(Real value) const noexcept {
        auto copy = *this;
        copy.epsilon = value;
        return copy;
    }

    [[nodiscard]] constexpr TriangularIVGrid2D with_origin(Point2 p) const noexcept {
        auto copy = *this;
        copy.origin = p;
        return copy;
    }
};

/**
 * @brief Staggered lattice that produces hexagonal Voronoi cells.
 *
 * A hexagonal Voronoi mesh is obtained by placing generator sites on a
 * triangular/staggered lattice. The input count is applied to the longest
 * generation-box direction; the shorter direction is filled using the same
 * lattice scale and centered so the first and last site layers have equal
 * distance to the nearest opposite boundaries. This pattern does not use
 * `epsilon`.
 */
struct HexagonalGrid2D {
    std::size_t count_on_longest_side{1};

    constexpr HexagonalGrid2D() = default;

    explicit constexpr HexagonalGrid2D(std::size_t count)
        : count_on_longest_side(count) {}
};

/**
 * @brief Uniform random rejection sampler inside Boundary2D.
 *
 * Sampling is reproducible through `seed`. Candidates are drawn in the boundary
 * bounding box and accepted only when they pass SiteValidation.
 */
struct UniformRandom2D {
    std::size_t target_count{0};
    std::uint32_t seed{5489U};
    std::size_t max_attempts{0};
    bool require_exact_count{true};

    constexpr UniformRandom2D() = default;
    explicit constexpr UniformRandom2D(std::size_t count)
        : target_count(count) {}
    constexpr UniformRandom2D(std::size_t count, std::uint32_t seed_)
        : target_count(count), seed(seed_) {}
};

[[nodiscard]] inline bool is_valid_pattern(const CartesianGrid2D& pattern) noexcept {
    return std::isfinite(pattern.spacing_x) &&
           std::isfinite(pattern.spacing_y) &&
           std::isfinite(pattern.epsilon) &&
           std::isfinite(pattern.custom_offset.x) &&
           std::isfinite(pattern.custom_offset.y) &&
           pattern.spacing_x > Real{0} &&
           pattern.spacing_y > Real{0} &&
           pattern.epsilon > Real{-1} &&
           pattern.epsilon < Real{1};
}

[[nodiscard]] inline bool is_valid_pattern(const CartesianGridCount2D& pattern) noexcept {
    return pattern.nx > 0 && pattern.ny > 0;
}

[[nodiscard]] inline bool is_valid_generation_box(const SiteGenerationBox2D& box) noexcept {
    return std::isfinite(box.min.x) &&
           std::isfinite(box.min.y) &&
           std::isfinite(box.max.x) &&
           std::isfinite(box.max.y) &&
           box.max.x > box.min.x &&
           box.max.y > box.min.y;
}

[[nodiscard]] inline bool is_valid_pattern(const TriangularIIGrid2D& pattern) noexcept {
    return std::isfinite(pattern.spacing) &&
           std::isfinite(pattern.epsilon) &&
           std::isfinite(pattern.origin.x) &&
           std::isfinite(pattern.origin.y) &&
           pattern.spacing > Real{0} &&
           pattern.epsilon > Real{-1} &&
           pattern.epsilon < Real{1};
}

[[nodiscard]] inline bool is_valid_pattern(const TriangularIVGrid2D& pattern) noexcept {
    return std::isfinite(pattern.spacing) &&
           std::isfinite(pattern.epsilon) &&
           std::isfinite(pattern.origin.x) &&
           std::isfinite(pattern.origin.y) &&
           pattern.spacing > Real{0} &&
           pattern.epsilon > Real{-1} &&
           pattern.epsilon < Real{1};
}

[[nodiscard]] inline bool is_valid_pattern(const HexagonalGrid2D& pattern) noexcept {
    return pattern.count_on_longest_side > 0U;
}

[[nodiscard]] inline bool is_valid_pattern(const UniformRandom2D& pattern) noexcept {
    return pattern.target_count > 0 &&
           (pattern.max_attempts == 0U ||
            pattern.max_attempts >= pattern.target_count);
}

inline void validate_pattern_or_throw(const CartesianGrid2D& pattern) {
    if (!is_valid_pattern(pattern)) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "CartesianGrid2D"},
                   {"reason", "invalid_spacing_or_offset"}});
    }
}

inline void validate_pattern_or_throw(const CartesianGridCount2D& pattern) {
    if (!is_valid_pattern(pattern)) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "CartesianGridCount2D"},
                   {"reason", "invalid_counts"}});
    }
}

inline void validate_generation_box_or_throw(const SiteGenerationBox2D& box) {
    if (!is_valid_generation_box(box)) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "SiteGenerationBox2D"},
                   {"reason", "invalid_generation_box"}});
    }
}

inline void validate_pattern_or_throw(const TriangularIIGrid2D& pattern) {
    if (!is_valid_pattern(pattern)) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "TriangularIIGrid2D"},
                   {"reason", "invalid_spacing_or_epsilon"}});
    }
}

inline void validate_pattern_or_throw(const TriangularIVGrid2D& pattern) {
    if (!is_valid_pattern(pattern)) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "TriangularIVGrid2D"},
                   {"reason", "invalid_spacing_or_epsilon"}});
    }
}

inline void validate_pattern_or_throw(const HexagonalGrid2D& pattern) {
    if (!is_valid_pattern(pattern)) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "HexagonalGrid2D"},
                   {"reason", "invalid_count"}});
    }
}

inline void validate_pattern_or_throw(const UniformRandom2D& pattern) {
    if (!is_valid_pattern(pattern)) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "UniformRandom2D"},
                   {"reason", "invalid_count_or_attempts"}});
    }
}

[[nodiscard]] inline SiteGenerationBox2D generation_box_from_boundary(
    const ::vmm::b2d::Boundary2DData& boundary) noexcept
{
    const auto box = ::vmm::b2d::bounding_box(boundary);
    return SiteGenerationBox2D{box.min, box.max};
}

inline void validate_factory_inputs_or_throw(
    const ::vmm::b2d::Boundary2DData& boundary,
    const SiteValidationOptions& validation)
{
    if (!::vmm::b2d::is_valid_boundary_minimal(boundary)) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "SiteFactory"},
                   {"reason", "invalid_boundary"}});
    }
    if (!options_are_valid(validation)) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "SiteFactory"},
                   {"reason", "invalid_validation_options"}});
    }
}

[[nodiscard]] inline SiteValidationOptions candidate_options(
    SiteValidationOptions validation) noexcept
{
    validation.min_distance_between_sites = Real{0};
    validation.require_sequential_ids = false;
    return validation;
}

[[nodiscard]] inline bool respects_existing_site_spacing(
    const SiteSet& sites,
    Point2 point,
    Real min_distance) noexcept
{
    if (min_distance <= Real{0}) return true;
    const Real min2 = min_distance * min_distance;
    for (const auto& site : sites) {
        if (site_squared_distance(site.point, point) < min2) return false;
    }
    return true;
}

[[nodiscard]] inline std::size_t effective_random_max_attempts(
    const UniformRandom2D& pattern) noexcept
{
    if (pattern.max_attempts > 0U) {
        return pattern.max_attempts;
    }

    constexpr std::size_t attempts_per_requested_site = 100U;
    constexpr std::size_t minimum_attempts = 100000U;
    constexpr std::size_t max_size = std::numeric_limits<std::size_t>::max();

    if (pattern.target_count >
        (max_size / attempts_per_requested_site)) {
        return max_size;
    }

    return std::max(minimum_attempts,
                    pattern.target_count * attempts_per_requested_site);
}

inline void append_site_if_valid(
    SiteSet& sites,
    Site2D candidate,
    const ::vmm::b2d::Boundary2DData& boundary,
    const SiteValidationOptions& candidate_validation,
    const SiteValidationOptions& final_validation)
{
    if (validate_site(candidate, boundary, candidate_validation) &&
        respects_existing_site_spacing(
            sites,
            candidate.point,
            final_validation.min_distance_between_sites)) {
        sites.add(candidate.point, candidate.region, candidate.weight);
    }
}

[[nodiscard]] inline Real first_grid_centroid_in_box(
    Real min_value,
    Real max_value,
    Real spacing,
    Real origin,
    bool use_origin,
    bool center_in_box) noexcept
{
    if (center_in_box) {
        const auto intervals = static_cast<std::size_t>(
            std::floor((max_value - min_value) / spacing));
        const Real span = static_cast<Real>(intervals) * spacing;
        return min_value + ((max_value - min_value) - span) * Real{0.5};
    }

    if (use_origin) {
        if (origin >= min_value) return origin;
        const Real steps = std::ceil((min_value - origin) / spacing);
        return origin + steps * spacing;
    }

    return min_value + Real{0.5} * spacing;
}

[[nodiscard]] inline Point2 cartesian_generator_from_centroid(
    Point2 centroid,
    const CartesianGrid2D& pattern) noexcept
{
    if (pattern.use_custom_offset) {
        return Point2{
            centroid.x + (pattern.custom_offset.x - Real{0.5}) * pattern.spacing_x,
            centroid.y + (pattern.custom_offset.y - Real{0.5}) * pattern.spacing_y
        };
    }

    return Point2{
        centroid.x + pattern.epsilon * pattern.spacing_x * Real{0.5},
        centroid.y - pattern.epsilon * pattern.spacing_y * Real{0.5}
    };
}

/**
 * @brief Convert Triangular II epsilon to the legacy point offset.
 */
[[nodiscard]] constexpr Real triangular_ii_unit_offset(Real epsilon) noexcept {
    return epsilon >= Real{0}
        ? Real{1} / Real{3} + epsilon / Real{6}
        : (Real{1} + epsilon) / Real{3};
}

/**
 * @brief Convert Triangular IV epsilon to the legacy point offset.
 */
[[nodiscard]] constexpr Real triangular_iv_unit_offset(Real epsilon) noexcept {
    return epsilon >= Real{0}
        ? Real{1} / Real{3} + epsilon / Real{6}
        : (Real{1} + epsilon) / Real{3};
}

/**
 * @brief First patch coordinate whose patch may intersect the generation box.
 */
[[nodiscard]] inline Real first_patch_coordinate(
    Real min_value,
    Real spacing,
    Real origin) noexcept
{
    const Real steps = std::floor((min_value - origin) / spacing);
    return origin + steps * spacing;
}

/**
 * @brief Append raw Triangular II candidates inside a generation box.
 *
 * This overload is independent from Boundary2D. It generates the deterministic
 * pattern in a rectangular window and does not clip candidates against a domain.
 * Use the boundary-aware `append_sites` overload when the final site set must be
 * inside a Boundary2D.
 */
inline void append_raw_sites(
    SiteSet& sites,
    const SiteGenerationBox2D& box,
    const TriangularIIGrid2D& pattern,
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    validate_generation_box_or_throw(box);
    validate_pattern_or_throw(pattern);

    const Real h = pattern.spacing;
    const Real a = h * triangular_ii_unit_offset(pattern.epsilon);
    const Real block = Real{2} * h;
    const Real x_start = first_patch_coordinate(box.min.x, block, pattern.origin.x);
    const Real y_start = first_patch_coordinate(box.min.y, block, pattern.origin.y);

    sites.renumber_sequential();
    for (Real y = y_start; y < box.max.y; y += block) {
        if (y + block <= box.min.y) continue;
        for (Real x = x_start; x < box.max.x; x += block) {
            if (x + block <= box.min.x) continue;

            // Legacy TriangleII: four squares per 2h x 2h block, two sites in
            // each square. The alternating placement reproduces the original
            // generator pattern while keeping this function boundary-free.
            sites.add(Point2{x + a,     y + h - a}, region, weight);
            sites.add(Point2{x + h - a, y + a},     region, weight);

            sites.add(Point2{x + a,     y + h + a},     region, weight);
            sites.add(Point2{x + h - a, y + block - a}, region, weight);

            sites.add(Point2{x + h + a,     y + a},     region, weight);
            sites.add(Point2{x + block - a, y + h - a}, region, weight);

            sites.add(Point2{x + h + a,     y + block - a}, region, weight);
            sites.add(Point2{x + block - a, y + h + a},     region, weight);
        }
    }
    sites.renumber_sequential();
}

/**
 * @brief Append raw Triangular IV candidates inside a generation box.
 *
 * This overload is independent from Boundary2D. It generates the deterministic
 * pattern in a rectangular window and does not clip candidates against a domain.
 * Use the boundary-aware `append_sites` overload when the final site set must be
 * inside a Boundary2D.
 */
inline void append_raw_sites(
    SiteSet& sites,
    const SiteGenerationBox2D& box,
    const TriangularIVGrid2D& pattern,
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    validate_generation_box_or_throw(box);
    validate_pattern_or_throw(pattern);

    const Real h = pattern.spacing;
    const Real a = h * triangular_iv_unit_offset(pattern.epsilon);
    const Real x_start = first_patch_coordinate(box.min.x, h, pattern.origin.x);
    const Real y_start = first_patch_coordinate(box.min.y, h, pattern.origin.y);

    sites.renumber_sequential();
    for (Real y = y_start; y < box.max.y; y += h) {
        if (y + h > box.min.y) {
            for (Real x = x_start; x < box.max.x; x += h) {
                if (x + h <= box.min.x) continue;

                // Legacy TriangleIV: four sites per square, obtained from one
                // reference point and its rotations/reflections.
                sites.add(Point2{x + a, y + Real{0.5} * h}, region, weight);
                sites.add(Point2{x + Real{0.5} * h, y + a}, region, weight);
                sites.add(Point2{x + h - a, y + Real{0.5} * h}, region, weight);
                sites.add(Point2{x + Real{0.5} * h, y + h - a}, region, weight);
            }
        }
    }
    sites.renumber_sequential();
}

/**
 * @brief Append raw hexagonal-lattice candidates inside a generation box.
 *
 * This pattern computes the lattice spacing from the longest box side and
 * centers the site layers along the shorter side.
 */
inline void append_raw_sites(
    SiteSet& sites,
    const SiteGenerationBox2D& box,
    const HexagonalGrid2D& pattern,
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    validate_generation_box_or_throw(box);
    validate_pattern_or_throw(pattern);

    constexpr Real sqrt3_over_2 = Real{0.86602540378443864676372317075293618};
    const Real width = box.max.x - box.min.x;
    const Real height = box.max.y - box.min.y;
    const auto count = static_cast<Real>(pattern.count_on_longest_side);

    Real h = Real{1};
    Real dy = sqrt3_over_2;
    if (width >= height) {
        h = width / count;
        dy = sqrt3_over_2 * h;
    } else {
        dy = height / count;
        h = dy / sqrt3_over_2;
    }

    const auto centered_start = [](Real min_value,
                                   Real max_value,
                                   Real pitch) noexcept {
        const Real length = max_value - min_value;
        const auto layers = std::max<std::size_t>(
            1U,
            static_cast<std::size_t>(std::floor(length / pitch)));
        const Real span = static_cast<Real>(layers - 1U) * pitch;
        return min_value + (length - span) * Real{0.5};
    };

    const Real x_start = width >= height
        ? box.min.x
        : centered_start(box.min.x, box.max.x, h);
    const Real y_start = width >= height
        ? centered_start(box.min.y, box.max.y, dy)
        : box.min.y;

    sites.renumber_sequential();
    for (std::size_t row = 0; ; ++row) {
        const Real y = y_start + static_cast<Real>(row) * dy;
        if (y > box.max.y) break;
        const Real shift = (row % 2U == 0U) ? Real{0} : Real{0.5} * h;
        const Real first_x = x_start + shift;
        const auto n_cols = first_x > box.max.x ? std::size_t{0}
            : static_cast<std::size_t>(std::floor((box.max.x - first_x) / h)) + 1U;
        for (std::size_t column = 0; column < n_cols; ++column) {
            const Real x = first_x + static_cast<Real>(column) * h;
            if (x >= box.min.x) {
                sites.add(Point2{x, y}, region, weight);
            }
        }
    }
    sites.renumber_sequential();
}

/**
 * @brief Create raw Triangular II candidates in a generation box.
 *
 * Returned sites are sequentially numbered, but they are not checked against a
 * Boundary2D. This is useful for testing pattern formulas and for workflows that
 * choose a generation window explicitly.
 */
[[nodiscard]] inline SiteSet make_raw_sites(
    const SiteGenerationBox2D& box,
    const TriangularIIGrid2D& pattern,
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    SiteSet sites;
    append_raw_sites(sites, box, pattern, region, weight);
    return sites;
}

/**
 * @brief Create raw Triangular IV candidates in a generation box.
 *
 * Returned sites are sequentially numbered, but they are not checked against a
 * Boundary2D. This is useful for testing pattern formulas and for workflows that
 * choose a generation window explicitly.
 */
[[nodiscard]] inline SiteSet make_raw_sites(
    const SiteGenerationBox2D& box,
    const TriangularIVGrid2D& pattern,
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    SiteSet sites;
    append_raw_sites(sites, box, pattern, region, weight);
    return sites;
}

/**
 * @brief Create raw hexagonal-lattice candidates in a generation box.
 */
[[nodiscard]] inline SiteSet make_raw_sites(
    const SiteGenerationBox2D& box,
    const HexagonalGrid2D& pattern,
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    SiteSet sites;
    append_raw_sites(sites, box, pattern, region, weight);
    return sites;
}

inline void append_sites(
    SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const CartesianGrid2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    validate_pattern_or_throw(pattern);
    validate_factory_inputs_or_throw(boundary, validation);

    const auto box = ::vmm::b2d::bounding_box(boundary);
    const Real x0 = first_grid_centroid_in_box(box.min.x,
                                               box.max.x,
                                               pattern.spacing_x,
                                               pattern.origin.x,
                                               pattern.use_origin,
                                               pattern.center_in_box);
    const Real y0 = first_grid_centroid_in_box(box.min.y,
                                               box.max.y,
                                               pattern.spacing_y,
                                               pattern.origin.y,
                                               pattern.use_origin,
                                               pattern.center_in_box);
    const auto candidate_validation = candidate_options(validation);

    sites.renumber_sequential();
    for (Real y = y0; y <= box.max.y; y += pattern.spacing_y) {
        for (Real x = x0; x <= box.max.x; x += pattern.spacing_x) {
            const Point2 generator = cartesian_generator_from_centroid(
                Point2{x, y},
                pattern);
            append_site_if_valid(sites,
                                 Site2D(generator, SiteId{0}, region, weight),
                                 boundary,
                                 candidate_validation,
                                 validation);
        }
    }

    sites.renumber_sequential();
    validate_sites_or_throw(sites, boundary, validation);
}

inline void append_sites(
    SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const CartesianGridCount2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    validate_pattern_or_throw(pattern);
    validate_factory_inputs_or_throw(boundary, validation);

    const auto box = ::vmm::b2d::bounding_box(boundary);
    const Real width = box.max.x - box.min.x;
    const Real height = box.max.y - box.min.y;
    if (!(width > Real{0}) || !(height > Real{0})) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "CartesianGridCount2D"},
                   {"reason", "empty_bounding_box"}});
    }

    const Real dx = width / static_cast<Real>(pattern.nx);
    const Real dy = height / static_cast<Real>(pattern.ny);
    const auto candidate_validation = candidate_options(validation);

    sites.renumber_sequential();
    sites.reserve(sites.size() + pattern.nx * pattern.ny);
    for (std::size_t j = 0; j < pattern.ny; ++j) {
        const Real y = box.min.y + (static_cast<Real>(j) + Real{0.5}) * dy;
        for (std::size_t i = 0; i < pattern.nx; ++i) {
            const Real x = box.min.x + (static_cast<Real>(i) + Real{0.5}) * dx;
            append_site_if_valid(sites,
                                 Site2D(Point2{x, y}, SiteId{0}, region, weight),
                                 boundary,
                                 candidate_validation,
                                 validation);
        }
    }

    sites.renumber_sequential();
    validate_sites_or_throw(sites, boundary, validation);
}

/**
 * @brief Append Triangular II sites clipped by a Boundary2D.
 *
 * The Triangular II pattern is generated first in the boundary bounding box and
 * then filtered by SiteValidation. This keeps the mathematical site pattern
 * independent from the domain while still guaranteeing that the accepted sites
 * are inside the boundary and respect the configured distances.
 */
inline void append_sites(
    SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const TriangularIIGrid2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    validate_pattern_or_throw(pattern);
    validate_factory_inputs_or_throw(boundary, validation);

    const auto candidate_validation = candidate_options(validation);
    const auto raw_sites = make_raw_sites(
        generation_box_from_boundary(boundary),
        pattern,
        region,
        weight);

    sites.renumber_sequential();
    sites.reserve(sites.size() + raw_sites.size());
    for (const auto& candidate : raw_sites) {
        append_site_if_valid(sites,
                             Site2D(candidate.point, SiteId{0}, candidate.region, candidate.weight),
                             boundary,
                             candidate_validation,
                             validation);
    }

    sites.renumber_sequential();
    validate_sites_or_throw(sites, boundary, validation);
}

/**
 * @brief Append Triangular IV sites clipped by a Boundary2D.
 *
 * The Triangular IV pattern is generated first in the boundary bounding box and
 * then filtered by SiteValidation. This keeps the mathematical site pattern
 * independent from the domain while still guaranteeing that the accepted sites
 * are inside the boundary and respect the configured distances.
 */
inline void append_sites(
    SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const TriangularIVGrid2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    validate_pattern_or_throw(pattern);
    validate_factory_inputs_or_throw(boundary, validation);

    const auto candidate_validation = candidate_options(validation);
    const auto raw_sites = make_raw_sites(
        generation_box_from_boundary(boundary),
        pattern,
        region,
        weight);

    sites.renumber_sequential();
    sites.reserve(sites.size() + raw_sites.size());
    for (const auto& candidate : raw_sites) {
        append_site_if_valid(sites,
                             Site2D(candidate.point, SiteId{0}, candidate.region, candidate.weight),
                             boundary,
                             candidate_validation,
                             validation);
    }

    sites.renumber_sequential();
    validate_sites_or_throw(sites, boundary, validation);
}

/**
 * @brief Append hexagonal-lattice sites clipped by a Boundary2D.
 *
 * The staggered lattice is generated first in the boundary bounding box and then
 * filtered by SiteValidation. This pattern intentionally has no eccentricity
 * parameter.
 */
inline void append_sites(
    SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const HexagonalGrid2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    validate_pattern_or_throw(pattern);
    validate_factory_inputs_or_throw(boundary, validation);

    const auto candidate_validation = candidate_options(validation);
    const auto raw_sites = make_raw_sites(
        generation_box_from_boundary(boundary),
        pattern,
        region,
        weight);

    sites.renumber_sequential();
    sites.reserve(sites.size() + raw_sites.size());
    for (const auto& candidate : raw_sites) {
        append_site_if_valid(sites,
                             Site2D(candidate.point, SiteId{0}, candidate.region, candidate.weight),
                             boundary,
                             candidate_validation,
                             validation);
    }

    sites.renumber_sequential();
    validate_sites_or_throw(sites, boundary, validation);
}

inline void append_sites(
    SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const UniformRandom2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    validate_pattern_or_throw(pattern);
    validate_factory_inputs_or_throw(boundary, validation);

    const auto box = ::vmm::b2d::bounding_box(boundary);
    std::mt19937 rng(pattern.seed);
    std::uniform_real_distribution<Real> x_dist(box.min.x, box.max.x);
    std::uniform_real_distribution<Real> y_dist(box.min.y, box.max.y);
    const auto candidate_validation = candidate_options(validation);
    const auto max_attempts = effective_random_max_attempts(pattern);

    SiteSpacingIndex2D spacing_index(validation.min_distance_between_sites);
    spacing_index.reserve(sites.size() + pattern.target_count);
    if (spacing_index.enabled()) {
        for (const auto& site : sites) {
            spacing_index.add(site.point);
        }
    }

    sites.renumber_sequential();
    const auto initial_size = sites.size();
    sites.reserve(initial_size + pattern.target_count);
    for (std::size_t attempt = 0;
         attempt < max_attempts &&
         (sites.size() - initial_size) < pattern.target_count;
         ++attempt) {
        const Point2 point{x_dist(rng), y_dist(rng)};
        const Site2D candidate(point, SiteId{0}, region, weight);
        if (!validate_site(candidate, boundary, candidate_validation)) {
            continue;
        }
        if (spacing_index.enabled() && !spacing_index.try_add(point)) {
            continue;
        }
        sites.add(point, region, weight);
    }

    const auto accepted = sites.size() - initial_size;
    if (pattern.require_exact_count && accepted != pattern.target_count) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "UniformRandom2D"},
                   {"reason", "could_not_reach_target_count"},
                   {"target_count", std::to_string(pattern.target_count)},
                   {"accepted_count", std::to_string(accepted)}});
    }

    sites.renumber_sequential();
    validate_sites_or_throw(sites, boundary, validation);
}

/**
 * @brief Create a SiteSet from a distribution pattern clipped by Boundary2D.
 */
[[nodiscard]] inline SiteSet make_sites(
    const ::vmm::b2d::Boundary2DData& boundary,
    const CartesianGrid2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    SiteSet sites;
    append_sites(sites, boundary, pattern, validation, region, weight);
    return sites;
}

[[nodiscard]] inline SiteSet make_sites(
    const ::vmm::b2d::Boundary2DData& boundary,
    const CartesianGridCount2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    SiteSet sites;
    append_sites(sites, boundary, pattern, validation, region, weight);
    return sites;
}

[[nodiscard]] inline SiteSet make_sites(
    const ::vmm::b2d::Boundary2DData& boundary,
    const TriangularIIGrid2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    SiteSet sites;
    append_sites(sites, boundary, pattern, validation, region, weight);
    return sites;
}

[[nodiscard]] inline SiteSet make_sites(
    const ::vmm::b2d::Boundary2DData& boundary,
    const TriangularIVGrid2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    SiteSet sites;
    append_sites(sites, boundary, pattern, validation, region, weight);
    return sites;
}

[[nodiscard]] inline SiteSet make_sites(
    const ::vmm::b2d::Boundary2DData& boundary,
    const HexagonalGrid2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    SiteSet sites;
    append_sites(sites, boundary, pattern, validation, region, weight);
    return sites;
}

[[nodiscard]] inline SiteSet make_sites(
    const ::vmm::b2d::Boundary2DData& boundary,
    const UniformRandom2D& pattern,
    const SiteValidationOptions& validation = SiteValidationOptions{},
    RegionId region = RegionId{0},
    Real weight = Real{0})
{
    SiteSet sites;
    append_sites(sites, boundary, pattern, validation, region, weight);
    return sites;
}

SITE2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
