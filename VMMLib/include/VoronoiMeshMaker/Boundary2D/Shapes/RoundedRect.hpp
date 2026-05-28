#pragma once
//==============================================================================
// Name        : RoundedRect.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D / Shapes
// Description : Axis-aligned rounded rectangle 2D boundary.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file RoundedRect.hpp
 * @brief Value type for an axis-aligned rounded rectangle.
 */

#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DConcepts.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Boundary2D/Policies/PolygonizePolicy.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>

VORMAKER_NAMESPACE_OPEN
BOUNDARY2D_NAMESPACE_OPEN

/**
 * @brief Rounded rectangle polygonized as a CCW outer ring.
 *
 * @invariant `w > 0`
 * @invariant `h > 0`
 * @invariant `0 <= radius <= min(w, h) / 2`
 * @invariant `segments_per_corner >= 1`
 */
struct RoundedRect {
    ///< Lower-left corner of the rectangle before rounding.
    Point2 ll{Real{0}, Real{0}};

    ///< Rectangle width.
    Real w{Real{1}};

    ///< Rectangle height.
    Real h{Real{1}};

    ///< Corner radius.
    Real radius{Real{0.1}};

    ///< Number of subdivisions per quarter-circle corner.
    Index segments_per_corner{8};

    /**
     * @brief Construct anchored at the origin.
     */
    RoundedRect(Real w_, Real h_, Real radius_, Index segments_per_corner_ = 8)
        : w{w_}, h{h_}, radius{radius_}, segments_per_corner{segments_per_corner_}
    {
        validate();
    }

    /**
     * @brief Construct with explicit lower-left corner.
     */
    RoundedRect(Point2 ll_,
                Real w_,
                Real h_,
                Real radius_,
                Index segments_per_corner_ = 8)
        : ll{ll_}, w{w_}, h{h_}, radius{radius_}, segments_per_corner{segments_per_corner_}
    {
        validate();
    }

    /**
     * @brief Number of vertices produced by polygonization.
     */
    [[nodiscard]] Index vertex_count() const noexcept {
        return radius <= Real{0} ? 4 : 4 * (segments_per_corner + 1);
    }

    /**
     * @brief Return the CCW polygonal approximation.
     */
    [[nodiscard]] std::vector<Point2>
    polygonize([[maybe_unused]] const PolygonizePolicy& policy) const
    {
        validate();
        std::vector<Point2> out(static_cast<std::size_t>(vertex_count()));
        polygonize_into(std::span<Point2>(out.data(), out.size()));
        return out;
    }

    /**
     * @brief Write the CCW polygonal approximation into `dst`.
     */
    void polygonize_into(std::span<Point2> dst) const {
        validate_destination(dst);
        if (radius <= Real{0}) {
            dst[0] = Point2{ll.x, ll.y};
            dst[1] = Point2{ll.x + w, ll.y};
            dst[2] = Point2{ll.x + w, ll.y + h};
            dst[3] = Point2{ll.x, ll.y + h};
            return;
        }

        const Real x0 = ll.x;
        const Real y0 = ll.y;
        const Real x1 = ll.x + w;
        const Real y1 = ll.y + h;
        const Real r = radius;
        const auto n = static_cast<std::size_t>(segments_per_corner);
        std::size_t k = 0;

        write_corner(dst, k, Point2{x1 - r, y0 + r}, -::vmm::constants::kPi * Real{0.5}, Real{0}, n);
        write_corner(dst, k, Point2{x1 - r, y1 - r}, Real{0}, ::vmm::constants::kPi * Real{0.5}, n);
        write_corner(dst, k, Point2{x0 + r, y1 - r}, ::vmm::constants::kPi * Real{0.5}, ::vmm::constants::kPi, n);
        write_corner(dst, k, Point2{x0 + r, y0 + r}, ::vmm::constants::kPi, ::vmm::constants::kPi * Real{1.5}, n);
    }

private:
    void validate() const {
        if (w <= Real{0} || h <= Real{0}) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "RoundedRect"}, {"what", "non_positive_dims"}});
        }
        if (radius < Real{0}) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "RoundedRect"}, {"what", "negative_radius"}});
        }
        if (radius > std::min(w, h) * Real{0.5}) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "RoundedRect"}, {"what", "radius_too_large"}});
        }
        if (segments_per_corner < 1) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "RoundedRect"}, {"what", "segments_per_corner_lt_1"}});
        }
    }

    void validate_destination(std::span<Point2> dst) const {
        validate();
        if (dst.size() < static_cast<std::size_t>(vertex_count())) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"where", "RoundedRect::polygonize_into"},
                       {"need", std::to_string(vertex_count())}});
        }
    }

    void write_corner(std::span<Point2> dst,
                      std::size_t& k,
                      Point2 center,
                      Real a0,
                      Real a1,
                      std::size_t n) const
    {
        for (std::size_t i = 0; i <= n; ++i) {
            const Real t = a0 + (a1 - a0) * static_cast<Real>(i) / static_cast<Real>(n);
            dst[k++] = Point2{
                center.x + radius * std::cos(static_cast<double>(t)),
                center.y + radius * std::sin(static_cast<double>(t))
            };
        }
    }
};

static_assert(Shape2DLike<RoundedRect, PolygonizePolicy>);

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
