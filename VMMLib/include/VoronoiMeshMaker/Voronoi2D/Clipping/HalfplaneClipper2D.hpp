#pragma once
//==============================================================================
// Name        : HalfplaneClipper2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Clipping
// Description : Polygon clipping by a 2D halfplane.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file HalfplaneClipper2D.hpp
 * @brief Sutherland-Hodgman style clipping by a closed halfplane.
 */

#include <span>
#include <vector>

#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/ClippingWorkspace2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/Halfplane2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Clips a polygon by `a*x + b*y <= c`.
 */
struct HalfplaneClipper2D {
    using Point2 = ::vmm::s2d::Point2;
    using Real = ::vmm::s2d::Real;

    [[nodiscard]] static Point2 intersection(Point2 p,
                                             Point2 q,
                                             const Halfplane2D& halfplane)
    {
        const Real fp = halfplane.evaluate(p);
        const Real fq = halfplane.evaluate(q);
        const Real denom = fp - fq;
        if (std::abs(denom) <= ::vmm::constants::kEpsilon) {
            return p;
        }

        const Real t = fp / denom;
        return Point2{
            p.x + t * (q.x - p.x),
            p.y + t * (q.y - p.y)
        };
    }

    [[nodiscard]] static std::vector<Point2> clip(
        std::span<const Point2> polygon,
        const Halfplane2D& halfplane)
    {
        ClippingWorkspace2D workspace;
        return clip(polygon, halfplane, workspace);
    }

    [[nodiscard]] static std::vector<Point2> clip(
        std::span<const Point2> polygon,
        const Halfplane2D& halfplane,
        ClippingWorkspace2D& workspace)
    {
        if (!halfplane.is_valid()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where", "HalfplaneClipper2D"},
                       {"reason", "invalid_halfplane"}});
        }

        workspace.output.clear();
        if (polygon.empty()) return {};

        workspace.output.reserve(polygon.size() + 1U);
        Point2 previous = polygon.back();
        bool previous_inside = halfplane.contains(previous);

        for (const auto& current : polygon) {
            const bool current_inside = halfplane.contains(current);
            if (current_inside) {
                if (!previous_inside) {
                    workspace.output.push_back(
                        intersection(previous, current, halfplane));
                }
                workspace.output.push_back(current);
            } else if (previous_inside) {
                workspace.output.push_back(
                    intersection(previous, current, halfplane));
            }

            previous = current;
            previous_inside = current_inside;
        }

        return workspace.output;
    }

    [[nodiscard]] static std::vector<Point2> clip_all(
        std::span<const Point2> polygon,
        std::span<const Halfplane2D> halfplanes,
        ClippingWorkspace2D& workspace)
    {
        workspace.input.assign(polygon.begin(), polygon.end());

        for (const auto& halfplane : halfplanes) {
            auto clipped = clip(std::span<const Point2>(workspace.input),
                                halfplane,
                                workspace);
            workspace.input = std::move(clipped);
            if (workspace.input.empty()) break;
        }

        return workspace.input;
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
