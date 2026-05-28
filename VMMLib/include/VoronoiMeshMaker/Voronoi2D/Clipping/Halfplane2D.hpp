#pragma once
//==============================================================================
// Name        : Halfplane2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Clipping
// Description : Value type for 2D halfplanes.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Halfplane2D.hpp
 * @brief Defines a closed 2D halfplane `a*x + b*y <= c`.
 */

#include <cmath>

#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/Core/namespace.h>
#include <VoronoiMeshMaker/Sites2D/Site2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Closed 2D halfplane represented by `a*x + b*y <= c`.
 */
struct Halfplane2D {
    using Point2 = ::vmm::s2d::Point2;
    using Real = ::vmm::s2d::Real;

    Real a{0};
    Real b{0};
    Real c{0};

    constexpr Halfplane2D() = default;
    constexpr Halfplane2D(Real a_, Real b_, Real c_) noexcept
        : a(a_), b(b_), c(c_) {}

    [[nodiscard]] constexpr Real evaluate(Point2 p) const noexcept {
        return a * p.x + b * p.y - c;
    }

    [[nodiscard]] constexpr bool contains(
        Point2 p,
        Real eps = ::vmm::constants::kEpsilon) const noexcept
    {
        return evaluate(p) <= eps;
    }

    [[nodiscard]] bool is_valid(
        Real eps = ::vmm::constants::kEpsilon) const noexcept
    {
        return std::isfinite(a) &&
               std::isfinite(b) &&
               std::isfinite(c) &&
               (a * a + b * b) > eps * eps;
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
