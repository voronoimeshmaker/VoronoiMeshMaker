#pragma once
//==============================================================================
// Name        : SiteValidation.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Sites2D
// Description : Validation rules for 2D Voronoi generator sites.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file SiteValidation.hpp
 * @brief Validates 2D generator sites against a Boundary2D domain.
 *
 * The initial project rule is strict: generator sites must not lie on the
 * boundary. A valid site is inside the domain with boundary excluded and has at
 * least the configured minimum distance to any boundary loop, including holes.
 */

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DContains.hpp>
#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DDistance.hpp>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>

VORMAKER_NAMESPACE_OPEN
SITE2D_NAMESPACE_OPEN

/**
 * @brief Options controlling validation of generator sites.
 */
struct SiteValidationOptions {
    /**
     * @brief Minimum allowed Euclidean distance from a site to any boundary loop.
     *
     * The boundary itself is always rejected because containment is evaluated
     * with `include_boundary=false`.
     */
    Real min_distance_to_boundary{Real{0}};

    /**
     * @brief Minimum allowed distance between two distinct sites.
     *
     * A value of zero disables duplicate/spacing checks. This is separate from
     * the boundary distance rule.
     */
    Real min_distance_between_sites{Real{0}};

    /**
     * @brief Require `site[i].id == SiteId{i}`.
     *
     * Generation algorithms may temporarily validate raw coordinates before the
     * final numbering pass. The final SiteSet handed to mesh/MVF assembly should
     * keep this enabled.
     */
    bool require_sequential_ids{true};
};

/**
 * @brief Failure category for site validation.
 */
enum class SiteValidationError {
    None,
    InvalidOptions,
    NonFiniteCoordinate,
    NonSequentialSiteId,
    OutsideDomainOrOnBoundary,
    TooCloseToBoundary,
    TooCloseToAnotherSite
};

/**
 * @brief Lightweight validation report.
 */
struct SiteValidationReport {
    bool ok{true};
    SiteValidationError error{SiteValidationError::None};
    std::size_t index{0};
    std::size_t other_index{0};

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return ok;
    }
};

[[nodiscard]] inline bool is_finite(Point2 p) noexcept {
    return std::isfinite(p.x) && std::isfinite(p.y);
}

[[nodiscard]] inline bool options_are_valid(const SiteValidationOptions& options) noexcept {
    return options.min_distance_to_boundary >= Real{0} &&
           options.min_distance_between_sites >= Real{0};
}

[[nodiscard]] inline SiteValidationReport validate_site(
    const Site2D& site,
    const ::vmm::b2d::Boundary2DData& boundary,
    const SiteValidationOptions& options = SiteValidationOptions{},
    std::size_t index = 0) noexcept
{
    if (!options_are_valid(options)) {
        return {false, SiteValidationError::InvalidOptions, index, 0};
    }
    if (options.require_sequential_ids &&
        site.id.value != static_cast<Index>(index)) {
        return {false, SiteValidationError::NonSequentialSiteId, index, 0};
    }
    if (!is_finite(site.point)) {
        return {false, SiteValidationError::NonFiniteCoordinate, index, 0};
    }
    if (!::vmm::b2d::contains(site.point, boundary, false)) {
        return {false, SiteValidationError::OutsideDomainOrOnBoundary, index, 0};
    }
    if (!::vmm::b2d::has_min_distance_to_boundary(
            site.point,
            boundary,
            options.min_distance_to_boundary)) {
        return {false, SiteValidationError::TooCloseToBoundary, index, 0};
    }
    return {};
}

[[nodiscard]] inline Real site_squared_distance(Point2 a, Point2 b) noexcept {
    const Real dx = a.x - b.x;
    const Real dy = a.y - b.y;
    return dx * dx + dy * dy;
}

namespace detail {

struct SiteSpacingCell2D {
    std::int64_t x{0};
    std::int64_t y{0};

    [[nodiscard]] friend constexpr bool operator==(
        SiteSpacingCell2D a,
        SiteSpacingCell2D b) noexcept
    {
        return a.x == b.x && a.y == b.y;
    }
};

struct SiteSpacingCellHash2D {
    [[nodiscard]] std::size_t operator()(SiteSpacingCell2D cell) const noexcept {
        const auto ux = static_cast<std::uint64_t>(cell.x);
        const auto uy = static_cast<std::uint64_t>(cell.y);
        return static_cast<std::size_t>(
            (ux * 0x9E3779B185EBCA87ULL) ^
            (uy + 0xC2B2AE3D27D4EB4FULL + (ux << 6U) + (ux >> 2U)));
    }
};

} // namespace detail

/**
 * @brief Spatial index for minimum-distance checks between sites.
 *
 * The grid cell side is equal to the requested minimum distance, so any
 * conflicting point must lie in the candidate cell or in one of its eight
 * neighbours. This keeps random large-site generation close to linear time
 * instead of repeatedly scanning all accepted sites.
 */
struct SiteSpacingIndex2D {
    Real min_distance{Real{0}};
    Real min_distance_squared{Real{0}};
    std::vector<Point2> points{};
    std::unordered_map<
        detail::SiteSpacingCell2D,
        std::vector<std::size_t>,
        detail::SiteSpacingCellHash2D> cells{};

    SiteSpacingIndex2D() = default;

    explicit SiteSpacingIndex2D(Real min_distance_)
        : min_distance(min_distance_),
          min_distance_squared(min_distance_ * min_distance_) {}

    [[nodiscard]] bool enabled() const noexcept {
        return min_distance > Real{0};
    }

    void reserve(std::size_t count) {
        points.reserve(count);
        if (enabled()) {
            cells.reserve(count);
        }
    }

    [[nodiscard]] detail::SiteSpacingCell2D cell_for(Point2 point) const noexcept {
        return detail::SiteSpacingCell2D{
            static_cast<std::int64_t>(std::floor(point.x / min_distance)),
            static_cast<std::int64_t>(std::floor(point.y / min_distance))
        };
    }

    [[nodiscard]] std::optional<std::size_t> find_conflict(Point2 point) const {
        if (!enabled()) {
            return std::nullopt;
        }

        const auto center = cell_for(point);
        for (std::int64_t dy = -1; dy <= 1; ++dy) {
            for (std::int64_t dx = -1; dx <= 1; ++dx) {
                const auto it = cells.find(
                    detail::SiteSpacingCell2D{center.x + dx, center.y + dy});
                if (it == cells.end()) {
                    continue;
                }

                for (const auto index : it->second) {
                    if (site_squared_distance(points[index], point) <
                        min_distance_squared) {
                        return index;
                    }
                }
            }
        }

        return std::nullopt;
    }

    void add(Point2 point) {
        const auto index = points.size();
        points.push_back(point);
        if (enabled()) {
            cells[cell_for(point)].push_back(index);
        }
    }

    [[nodiscard]] bool try_add(Point2 point) {
        if (find_conflict(point)) {
            return false;
        }
        add(point);
        return true;
    }
};

[[nodiscard]] inline SiteValidationReport validate_sites(
    std::span<const Site2D> sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const SiteValidationOptions& options = SiteValidationOptions{}) noexcept
{
    if (!options_are_valid(options)) {
        return {false, SiteValidationError::InvalidOptions, 0, 0};
    }

    for (std::size_t i = 0; i < sites.size(); ++i) {
        const auto report = validate_site(sites[i], boundary, options, i);
        if (!report) return report;
    }

    if (options.min_distance_between_sites > Real{0}) {
        SiteSpacingIndex2D spacing_index(options.min_distance_between_sites);
        spacing_index.reserve(sites.size());
        for (std::size_t i = 0; i < sites.size(); ++i) {
            if (const auto conflict =
                    spacing_index.find_conflict(sites[i].point)) {
                return {false,
                        SiteValidationError::TooCloseToAnotherSite,
                        i,
                        *conflict};
            }
            spacing_index.add(sites[i].point);
        }
    }

    return {};
}

[[nodiscard]] inline SiteValidationReport validate_sites(
    const SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const SiteValidationOptions& options = SiteValidationOptions{}) noexcept
{
    return validate_sites(sites.span(), boundary, options);
}

inline void validate_sites_or_throw(
    std::span<const Site2D> sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const SiteValidationOptions& options = SiteValidationOptions{})
{
    const auto report = validate_sites(sites, boundary, options);
    if (report) return;

    VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
              {{"where", "SiteValidation"},
               {"index", std::to_string(report.index)},
               {"other_index", std::to_string(report.other_index)},
               {"reason", std::to_string(static_cast<int>(report.error))}});
}

inline void validate_sites_or_throw(
    const SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const SiteValidationOptions& options = SiteValidationOptions{})
{
    validate_sites_or_throw(sites.span(), boundary, options);
}

SITE2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
