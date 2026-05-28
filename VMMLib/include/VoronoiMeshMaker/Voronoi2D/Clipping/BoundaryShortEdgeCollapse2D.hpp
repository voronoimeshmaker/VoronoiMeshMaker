#pragma once
//==============================================================================
// Name        : BoundaryShortEdgeCollapse2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Clipping
// Description : Collapses short clipped Voronoi edges that lie on Boundary2D.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file BoundaryShortEdgeCollapse2D.hpp
 * @brief Optional clean-up for very short Voronoi edges on the clipped boundary.
 *
 * The operation is intentionally conservative: it only changes a polygon edge
 * when both of its vertices lie on the same Boundary2D segment and the edge is
 * shorter than the requested threshold. Interior Voronoi edges are not touched.
 *
 * @ingroup voronoi2d_clipping
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DContains.hpp>
#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DDistance.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

enum class BoundaryShortEdgePolicy2D : unsigned char {
    Keep,
    CollapseToMidpoint,
    CollapseToBoundaryProjection
};

struct BoundaryShortEdgeCollapseOptions2D {
    Real min_length{Real{0}};
    BoundaryShortEdgePolicy2D policy{BoundaryShortEdgePolicy2D::Keep};
};

struct BoundaryShortEdgeCollapseResult2D {
    std::size_t collapsed_count{0};
    Real minimum_length_before{std::numeric_limits<Real>::infinity()};
    Real minimum_length_after{std::numeric_limits<Real>::infinity()};

    [[nodiscard]] bool changed() const noexcept {
        return collapsed_count > 0U;
    }
};

namespace detail {

[[nodiscard]] inline Real boundary_short_edge_squared_distance(
    ::vmm::s2d::Point2 a,
    ::vmm::s2d::Point2 b) noexcept
{
    const Real dx = a.x - b.x;
    const Real dy = a.y - b.y;
    return dx * dx + dy * dy;
}

[[nodiscard]] inline Real boundary_short_edge_min_length(
    const std::vector<::vmm::s2d::Point2>& polygon) noexcept
{
    if (polygon.size() < 2U) {
        return std::numeric_limits<Real>::infinity();
    }

    Real best = std::numeric_limits<Real>::infinity();
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const auto& a = polygon[i];
        const auto& b = polygon[(i + 1U) % polygon.size()];
        best = std::min(best, std::sqrt(
            boundary_short_edge_squared_distance(a, b)));
    }
    return best;
}

inline void remove_consecutive_duplicate_boundary_vertices(
    std::vector<::vmm::s2d::Point2>& polygon) noexcept
{
    if (polygon.empty()) {
        return;
    }

    constexpr Real eps2 =
        ::vmm::constants::kEpsilon * ::vmm::constants::kEpsilon;

    std::vector<::vmm::s2d::Point2> cleaned;
    cleaned.reserve(polygon.size());

    for (const auto point : polygon) {
        if (cleaned.empty() ||
            boundary_short_edge_squared_distance(cleaned.back(), point) > eps2) {
            cleaned.push_back(point);
        }
    }

    if (cleaned.size() > 1U &&
        boundary_short_edge_squared_distance(cleaned.front(), cleaned.back()) <= eps2) {
        cleaned.pop_back();
    }

    polygon = std::move(cleaned);
}

[[nodiscard]] inline bool find_boundary_parent_segment(
    ::vmm::s2d::Point2 a,
    ::vmm::s2d::Point2 b,
    const ::vmm::b2d::Boundary2DData& boundary,
    ::vmm::b2d::Index& ring_index,
    std::size_t& edge_index,
    ::vmm::s2d::Point2& segment_a,
    ::vmm::s2d::Point2& segment_b) noexcept
{
    constexpr Real eps = ::vmm::constants::kEpsilon;

    for (::vmm::b2d::Index r = 0; r < boundary.ring_count(); ++r) {
        const auto ring = boundary.ring(r);
        if (ring.size() < 2U) {
            continue;
        }

        for (std::size_t e = 0; e < ring.size(); ++e) {
            const auto& p = ring[e];
            const auto& q = ring[(e + 1U) % ring.size()];

            if (::vmm::b2d::point_on_segment(a, p, q, eps) &&
                ::vmm::b2d::point_on_segment(b, p, q, eps)) {
                ring_index = r;
                edge_index = e;
                segment_a = p;
                segment_b = q;
                return true;
            }
        }
    }

    ring_index = ::vmm::b2d::kInvalid;
    edge_index = 0U;
    segment_a = {};
    segment_b = {};
    return false;
}

[[nodiscard]] inline ::vmm::s2d::Point2 collapsed_boundary_point(
    ::vmm::s2d::Point2 a,
    ::vmm::s2d::Point2 b,
    ::vmm::s2d::Point2 segment_a,
    ::vmm::s2d::Point2 segment_b,
    ::vmm::b2d::Index ring_index,
    std::size_t edge_index,
    BoundaryShortEdgePolicy2D policy) noexcept
{
    const ::vmm::s2d::Point2 midpoint{
        (a.x + b.x) * Real{0.5},
        (a.y + b.y) * Real{0.5}
    };

    if (policy == BoundaryShortEdgePolicy2D::CollapseToBoundaryProjection) {
        return ::vmm::b2d::project_to_segment(
            midpoint,
            segment_a,
            segment_b,
            ring_index,
            edge_index).point;
    }

    return midpoint;
}

[[nodiscard]] inline bool collapse_one_short_boundary_edge(
    std::vector<::vmm::s2d::Point2>& polygon,
    const ::vmm::b2d::Boundary2DData& boundary,
    Real min_length,
    BoundaryShortEdgePolicy2D policy) noexcept
{
    if (polygon.size() <= 3U ||
        min_length <= Real{0} ||
        policy == BoundaryShortEdgePolicy2D::Keep) {
        return false;
    }

    const Real min_length2 = min_length * min_length;

    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const std::size_t j = (i + 1U) % polygon.size();
        const auto a = polygon[i];
        const auto b = polygon[j];

        if (boundary_short_edge_squared_distance(a, b) >= min_length2) {
            continue;
        }

        ::vmm::b2d::Index ring_index = ::vmm::b2d::kInvalid;
        std::size_t edge_index = 0U;
        ::vmm::s2d::Point2 segment_a{};
        ::vmm::s2d::Point2 segment_b{};

        if (!find_boundary_parent_segment(
                a, b, boundary,
                ring_index, edge_index,
                segment_a, segment_b)) {
            continue;
        }

        const auto replacement = collapsed_boundary_point(
            a, b, segment_a, segment_b, ring_index, edge_index, policy);

        std::vector<::vmm::s2d::Point2> collapsed;
        collapsed.reserve(polygon.size() - 1U);
        for (std::size_t k = 0; k < polygon.size(); ++k) {
            if (k == j) {
                continue;
            }
            if (k == i) {
                collapsed.push_back(replacement);
            } else {
                collapsed.push_back(polygon[k]);
            }
        }

        polygon = std::move(collapsed);
        remove_consecutive_duplicate_boundary_vertices(polygon);
        return true;
    }

    return false;
}

} // namespace detail

inline void validate_boundary_short_edge_options_or_throw(
    BoundaryShortEdgeCollapseOptions2D options)
{
    if (!std::isfinite(options.min_length) || options.min_length < Real{0}) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where",  "BoundaryShortEdgeCollapse2D"},
                   {"reason", "min_length_must_be_finite_and_non_negative"}});
    }
}

inline BoundaryShortEdgeCollapseResult2D collapse_short_boundary_edges(
    std::vector<::vmm::s2d::Point2>& polygon,
    const ::vmm::b2d::Boundary2DData& boundary,
    BoundaryShortEdgeCollapseOptions2D options)
{
    validate_boundary_short_edge_options_or_throw(options);

    BoundaryShortEdgeCollapseResult2D result;
    result.minimum_length_before =
        detail::boundary_short_edge_min_length(polygon);

    if (!boundary.invariant_ok() ||
        polygon.size() <= 3U ||
        options.min_length <= Real{0} ||
        options.policy == BoundaryShortEdgePolicy2D::Keep) {
        result.minimum_length_after =
            detail::boundary_short_edge_min_length(polygon);
        return result;
    }

    while (detail::collapse_one_short_boundary_edge(
        polygon, boundary, options.min_length, options.policy)) {
        ++result.collapsed_count;
    }

    result.minimum_length_after =
        detail::boundary_short_edge_min_length(polygon);
    return result;
}

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
