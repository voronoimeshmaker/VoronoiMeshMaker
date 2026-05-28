#pragma once
//==============================================================================
// Name        : Boundary2DContains.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D
// Description : Point-in-boundary queries for polygon-with-holes domains.
// License     : GNU GPL v3
// Version     : 0.1.0
//==============================================================================

#include <span>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Core/constants.h>

VORMAKER_NAMESPACE_OPEN
BOUNDARY2D_NAMESPACE_OPEN

[[nodiscard]] inline bool point_on_segment(Point2 p,
                                           Point2 a,
                                           Point2 b,
                                           Real eps = ::vmm::constants::kEpsilon) noexcept
{
    const Real abx = b.x - a.x;
    const Real aby = b.y - a.y;
    const Real apx = p.x - a.x;
    const Real apy = p.y - a.y;
    const Real cross = abx * apy - aby * apx;
    if (cross > eps || cross < -eps) return false;

    const Real dot = apx * abx + apy * aby;
    if (dot < -eps) return false;

    const Real len2 = abx * abx + aby * aby;
    return dot <= len2 + eps;
}

[[nodiscard]] inline bool point_on_ring(Point2 p,
                                        std::span<const Point2> ring,
                                        Real eps = ::vmm::constants::kEpsilon) noexcept
{
    if (ring.size() < 2) return false;
    for (std::size_t i = 0; i < ring.size(); ++i) {
        if (point_on_segment(p, ring[i], ring[(i + 1U) % ring.size()], eps)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline bool point_in_ring_strict(Point2 p,
                                               std::span<const Point2> ring) noexcept
{
    if (ring.size() < 3) return false;
    if (point_on_ring(p, ring)) return false;

    bool inside = false;
    for (std::size_t i = 0, j = ring.size() - 1U; i < ring.size(); j = i++) {
        const auto& a = ring[i];
        const auto& b = ring[j];
        const bool crosses = ((a.y > p.y) != (b.y > p.y));
        if (crosses) {
            const Real x_at_y =
                (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x;
            if (p.x < x_at_y) {
                inside = !inside;
            }
        }
    }
    return inside;
}

[[nodiscard]] inline bool contains(Point2 p,
                                   const Boundary2DData& boundary,
                                   bool include_boundary = true) noexcept
{
    if (!boundary.invariant_ok()) return false;

    bool inside_outer = false;
    for (Index i = 0; i < boundary.ring_count(); ++i) {
        const auto ring = boundary.ring(i);
        const auto kind = boundary.kinds[static_cast<std::size_t>(i)];
        const bool on_ring = point_on_ring(p, ring);

        if (kind == LoopKind::Outer) {
            if (on_ring) return include_boundary;
            inside_outer = inside_outer || point_in_ring_strict(p, ring);
        } else {
            if (on_ring) return include_boundary;
            if (point_in_ring_strict(p, ring)) return false;
        }
    }
    return inside_outer;
}

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
