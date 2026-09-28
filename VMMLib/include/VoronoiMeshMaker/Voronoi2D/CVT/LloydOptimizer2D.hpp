#pragma once
//==============================================================================
// Name        : LloydOptimizer2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / CVT
// Description : Lloyd relaxation for clipped 2D Voronoi diagrams.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file LloydOptimizer2D.hpp
 * @brief Computes centroidal Voronoi relaxations on clipped 2D domains.
 *
 * Lloyd relaxation repeatedly rebuilds the clipped Voronoi diagram and moves
 * each generator to the centroid of its clipped cell.  The implementation uses
 * the existing Yan-style clipped-cell builder and keeps CGAL details contained
 * in the existing tessellation layer.
 *
 * @ingroup voronoi2d
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DContains.hpp>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Options controlling Lloyd relaxation.
 */
struct LloydOptions2D {
    std::size_t max_iterations{20};
    ::vmm::s2d::Real tolerance{1.0e-10};
    bool stop_on_tolerance{true};
};

/**
 * @brief Diagnostic data for one Lloyd iteration.
 */
struct LloydIterationStats2D {
    std::size_t iteration{0};
    ::vmm::s2d::Real max_displacement{0};
    ::vmm::s2d::Real mean_displacement{0};
    ::vmm::s2d::Real cvt_energy{0};
};

/**
 * @brief Result of a Lloyd relaxation run.
 */
struct LloydResult2D {
    ::vmm::s2d::SiteSet sites{};
    ClippedVoronoiDiagram2D diagram{};
    std::vector<LloydIterationStats2D> history{};
    bool converged{false};

    [[nodiscard]] std::size_t iterations() const noexcept {
        return history.size();
    }
};

namespace detail {

[[nodiscard]] inline ::vmm::s2d::Real squared_distance(
    ::vmm::s2d::Point2 a,
    ::vmm::s2d::Point2 b) noexcept
{
    const auto dx = a.x - b.x;
    const auto dy = a.y - b.y;
    return dx * dx + dy * dy;
}

/**
 * @brief Integrates squared distance from a point over one polygonal cell.
 */
[[nodiscard]] inline ::vmm::s2d::Real cell_cvt_energy(
    const VoronoiCell2D& cell,
    ::vmm::s2d::Point2 site) noexcept
{
    using Real = ::vmm::s2d::Real;
    const auto n = cell.polygon.size();
    if (n < 3U) {
        return Real{0};
    }

    long double twice_area = 0.0L;
    long double int_x = 0.0L;
    long double int_y = 0.0L;
    long double int_x2 = 0.0L;
    long double int_y2 = 0.0L;

    for (std::size_t i = 0; i < n; ++i) {
        const auto& p = cell.polygon[i];
        const auto& q = cell.polygon[(i + 1U) % n];

        const long double px = static_cast<long double>(p.x);
        const long double py = static_cast<long double>(p.y);
        const long double qx = static_cast<long double>(q.x);
        const long double qy = static_cast<long double>(q.y);
        const long double cross = px * qy - qx * py;

        twice_area += cross;
        int_x += (px + qx) * cross;
        int_y += (py + qy) * cross;
        int_x2 += (px * px + px * qx + qx * qx) * cross;
        int_y2 += (py * py + py * qy + qy * qy) * cross;
    }

    if (twice_area == 0.0L) {
        return Real{0};
    }

    long double sign = 1.0L;
    if (twice_area < 0.0L) {
        sign = -1.0L;
    }

    const long double area = sign * twice_area / 2.0L;
    const long double first_x = sign * int_x / 6.0L;
    const long double first_y = sign * int_y / 6.0L;
    const long double second_x = sign * int_x2 / 12.0L;
    const long double second_y = sign * int_y2 / 12.0L;
    const long double sx = static_cast<long double>(site.x);
    const long double sy = static_cast<long double>(site.y);

    const long double value =
        second_x + second_y -
        2.0L * sx * first_x -
        2.0L * sy * first_y +
        (sx * sx + sy * sy) * area;

    return static_cast<Real>(std::max(0.0L, value));
}

inline void validate_lloyd_options_or_throw(const LloydOptions2D& options) {
    if (!std::isfinite(options.tolerance) || options.tolerance < ::vmm::s2d::Real{0}) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "LloydOptions2D"},
                   {"reason", "invalid_tolerance"}});
    }
}

} // namespace detail

/**
 * @brief Performs Lloyd relaxation from an existing site set.
 *
 * @param[in] initial_sites Initial generator sites.
 * @param[in] boundary Domain used to clip every Voronoi cell.
 * @param[in] options Iteration and convergence options.
 * @param[in] builder_options Options forwarded to the clipped Voronoi builder.
 * @return Final sites, final clipped diagram, and per-iteration diagnostics.
 */
[[nodiscard]] inline LloydResult2D lloyd_relax(
    const ::vmm::s2d::SiteSet& initial_sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const LloydOptions2D& options = LloydOptions2D{},
    const ClippedVoronoiBuildOptions2D& builder_options = ClippedVoronoiBuildOptions2D{})
{
    detail::validate_lloyd_options_or_throw(options);

    if (initial_sites.empty()) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "lloyd_relax"},
                   {"reason", "empty_site_set"}});
    }

    LloydResult2D result;
    result.sites = initial_sites;
    result.sites.renumber_sequential();
    result.history.reserve(options.max_iterations);

    for (std::size_t iter = 0; iter < options.max_iterations; ++iter) {
        auto diagram = ClippedVoronoiBuilder2D::build(
            result.sites,
            boundary,
            builder_options);

        LloydIterationStats2D stats;
        stats.iteration = iter + 1U;

        ::vmm::s2d::SiteSet next_sites;
        next_sites.reserve(result.sites.size());

        long double displacement_sum = 0.0L;
        long double energy_sum = 0.0L;
        for (std::size_t i = 0; i < result.sites.size(); ++i) {
            const auto& site = result.sites[i];
            const auto& cell = diagram.cell(site.id);
            auto next_point = site.point;

            if (!cell.empty()) {
                const auto centroid = cell.centroid();
                if (::vmm::b2d::contains(centroid, boundary, true)) {
                    next_point = centroid;
                }
            }

            const auto displacement =
                std::sqrt(detail::squared_distance(site.point, next_point));
            stats.max_displacement = std::max(stats.max_displacement, displacement);
            displacement_sum += static_cast<long double>(displacement);
            energy_sum += static_cast<long double>(
                detail::cell_cvt_energy(cell, site.point));

            next_sites.add(next_point, site.region, site.weight);
        }

        stats.mean_displacement = static_cast<::vmm::s2d::Real>(
            displacement_sum / static_cast<long double>(result.sites.size()));
        stats.cvt_energy = static_cast<::vmm::s2d::Real>(energy_sum);
        result.history.push_back(stats);
        result.sites = std::move(next_sites);

        if (options.stop_on_tolerance &&
            stats.max_displacement <= options.tolerance) {
            result.converged = true;
            break;
        }
    }

    result.diagram = ClippedVoronoiBuilder2D::build(
        result.sites,
        boundary,
        builder_options);

    if (options.max_iterations == 0U) {
        result.converged = true;
    }

    return result;
}

/**
 * @brief Performs one Lloyd step and returns the moved generator set.
 */
[[nodiscard]] inline ::vmm::s2d::SiteSet lloyd_step(
    const ::vmm::s2d::SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const ClippedVoronoiBuildOptions2D& builder_options = ClippedVoronoiBuildOptions2D{})
{
    LloydOptions2D options;
    options.max_iterations = 1U;
    options.stop_on_tolerance = false;
    return lloyd_relax(sites, boundary, options, builder_options).sites;
}

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
