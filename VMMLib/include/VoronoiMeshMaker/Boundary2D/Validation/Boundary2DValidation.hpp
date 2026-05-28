#pragma once
//==============================================================================
// Name        : Boundary2DValidation.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D
// Description : Basic area, orientation and invariant validation helpers.
// License     : GNU GPL v3
// Version     : 0.1.0
//==============================================================================

#include <span>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Core/constants.h>

VORMAKER_NAMESPACE_OPEN
BOUNDARY2D_NAMESPACE_OPEN

[[nodiscard]] inline Real signed_area(std::span<const Point2> ring) noexcept {
    if (ring.size() < 3) return Real{0};

    long double twice_area = 0.0L;
    for (std::size_t i = 0; i < ring.size(); ++i) {
        const auto& p = ring[i];
        const auto& q = ring[(i + 1U) % ring.size()];
        twice_area += static_cast<long double>(p.x) * static_cast<long double>(q.y)
                    - static_cast<long double>(p.y) * static_cast<long double>(q.x);
    }
    return static_cast<Real>(twice_area * 0.5L);
}

[[nodiscard]] inline Real ring_area(std::span<const Point2> ring) noexcept {
    const Real area = signed_area(ring);
    return area < Real{0} ? -area : area;
}

[[nodiscard]] inline bool is_counter_clockwise(std::span<const Point2> ring) noexcept {
    return signed_area(ring) > ::vmm::constants::kZeroTol;
}

[[nodiscard]] inline bool is_clockwise(std::span<const Point2> ring) noexcept {
    return signed_area(ring) < -::vmm::constants::kZeroTol;
}

[[nodiscard]] inline Real boundary_area(const Boundary2DData& boundary) noexcept {
    if (!boundary.invariant_ok()) return Real{0};

    Real total{0};
    for (Index i = 0; i < boundary.ring_count(); ++i) {
        const Real area = ring_area(boundary.ring(i));
        total += (boundary.kinds[static_cast<std::size_t>(i)] == LoopKind::Hole)
                   ? -area
                   : area;
    }
    return total;
}

[[nodiscard]] inline Box2 bounding_box(const Boundary2DData& boundary) noexcept {
    Box2 box = Box2::empty();
    for (const auto& p : boundary.points) {
        box.expand(p);
    }
    return box;
}

[[nodiscard]] inline bool has_expected_orientation(const Boundary2DData& boundary) noexcept {
    if (!boundary.invariant_ok()) return false;

    for (Index i = 0; i < boundary.ring_count(); ++i) {
        const auto ring = boundary.ring(i);
        const auto kind = boundary.kinds[static_cast<std::size_t>(i)];
        if (kind == LoopKind::Outer && !is_counter_clockwise(ring)) return false;
        if (kind == LoopKind::Hole && !is_clockwise(ring)) return false;
    }
    return true;
}

[[nodiscard]] inline bool is_valid_boundary_minimal(const Boundary2DData& boundary) noexcept {
    if (!boundary.invariant_ok()) return false;
    if (boundary.ring_count() <= 0) return false;

    bool has_outer = false;
    for (Index i = 0; i < boundary.ring_count(); ++i) {
        if (boundary.ring(i).size() < 3) return false;
        if (boundary.kinds[static_cast<std::size_t>(i)] == LoopKind::Outer) {
            has_outer = true;
        }
    }
    return has_outer && has_expected_orientation(boundary) &&
           boundary_area(boundary) > ::vmm::constants::kZeroTol;
}

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
