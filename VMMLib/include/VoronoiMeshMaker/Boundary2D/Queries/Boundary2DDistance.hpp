#pragma once
//==============================================================================
// Name        : Boundary2DDistance.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D
// Description : Distance queries between points and polygon boundary loops.
// License     : GNU GPL v3
// Version     : 0.1.0
//==============================================================================

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>

VORMAKER_NAMESPACE_OPEN
BOUNDARY2D_NAMESPACE_OPEN

[[nodiscard]] inline Real squared_distance(Point2 a, Point2 b) noexcept {
    const Real dx = a.x - b.x;
    const Real dy = a.y - b.y;
    return dx * dx + dy * dy;
}

struct BoundaryProjection2D {
    Point2 point{};
    Real distance{std::numeric_limits<Real>::infinity()};
    Index ring_index{-1};
    std::size_t edge_index{0};
    Real segment_parameter{0};
    bool valid{false};
};

[[nodiscard]] inline BoundaryProjection2D project_to_segment(Point2 p,
                                                             Point2 a,
                                                             Point2 b,
                                                             Index ring_index = Index{-1},
                                                             std::size_t edge_index = 0U) noexcept
{
    const Real abx = b.x - a.x;
    const Real aby = b.y - a.y;
    const Real len2 = abx * abx + aby * aby;

    if (len2 <= Real{0}) {
        const Real distance = std::sqrt(squared_distance(p, a));
        return BoundaryProjection2D{
            a,
            distance,
            ring_index,
            edge_index,
            Real{0},
            true};
    }

    const Real apx = p.x - a.x;
    const Real apy = p.y - a.y;
    const Real t = std::clamp((apx * abx + apy * aby) / len2,
                              Real{0},
                              Real{1});
    const Point2 projection{a.x + t * abx, a.y + t * aby};
    return BoundaryProjection2D{
        projection,
        std::sqrt(squared_distance(p, projection)),
        ring_index,
        edge_index,
        t,
        true};
}

[[nodiscard]] inline Real distance_to_segment(Point2 p,
                                              Point2 a,
                                              Point2 b) noexcept
{
    return project_to_segment(p, a, b).distance;
}

[[nodiscard]] inline BoundaryProjection2D project_to_ring(
    Point2 p,
    std::span<const Point2> ring,
    Index ring_index = Index{-1}) noexcept
{
    BoundaryProjection2D best{};
    if (ring.empty()) {
        return best;
    }
    if (ring.size() == 1U) {
        return project_to_segment(p, ring.front(), ring.front(), ring_index, 0U);
    }

    for (std::size_t i = 0; i < ring.size(); ++i) {
        const auto candidate = project_to_segment(
            p,
            ring[i],
            ring[(i + 1U) % ring.size()],
            ring_index,
            i);
        if (!best.valid || candidate.distance < best.distance) {
            best = candidate;
        }
    }
    return best;
}

[[nodiscard]] inline Real distance_to_ring(Point2 p,
                                           std::span<const Point2> ring) noexcept
{
    return project_to_ring(p, ring).distance;
}

[[nodiscard]] inline BoundaryProjection2D project_to_boundary(
    Point2 p,
    const Boundary2DData& boundary) noexcept
{
    BoundaryProjection2D best{};
    if (!boundary.invariant_ok() || boundary.ring_count() <= 0) {
        best.distance = Real{0};
        return best;
    }

    for (Index i = 0; i < boundary.ring_count(); ++i) {
        const auto candidate = project_to_ring(p, boundary.ring(i), i);
        if (!best.valid || candidate.distance < best.distance) {
            best = candidate;
        }
    }
    return best;
}

[[nodiscard]] inline Real distance_to_boundary(Point2 p,
                                               const Boundary2DData& boundary) noexcept
{
    return project_to_boundary(p, boundary).distance;
}

[[nodiscard]] inline bool has_min_distance_to_boundary(
    Point2 p,
    const Boundary2DData& boundary,
    Real min_distance) noexcept
{
    if (min_distance < Real{0}) return false;
    return distance_to_boundary(p, boundary) >= min_distance;
}

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
