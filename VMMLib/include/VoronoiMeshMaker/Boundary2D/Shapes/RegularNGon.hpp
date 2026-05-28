#pragma once
//==============================================================================
// Name        : RegularNGon.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D / Shapes
// Description : Regular polygonal 2D boundary.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file RegularNGon.hpp
 * @brief Value type representing a regular polygon inscribed in a circle.
 */

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
 * @brief Regular polygon with `sides` equally spaced CCW vertices.
 *
 * @invariant `sides >= 3`
 * @invariant `radius > 0`
 */
struct RegularNGon {
    ///< Center of the circumscribed circle.
    Point2 center{Real{0}, Real{0}};

    ///< Radius of the circumscribed circle.
    Real radius{Real{1}};

    ///< Number of polygon sides/vertices.
    Index sides{6};

    ///< Angle of the first vertex, in radians.
    Real start_angle{Real{0}};

    /**
     * @brief Construct centered at the origin.
     */
    RegularNGon(Index sides_, Real radius_, Real start_angle_ = Real{0})
        : radius{radius_}, sides{sides_}, start_angle{start_angle_}
    {
        validate();
    }

    /**
     * @brief Construct with explicit center.
     */
    RegularNGon(Point2 center_, Index sides_, Real radius_, Real start_angle_ = Real{0})
        : center{center_}, radius{radius_}, sides{sides_}, start_angle{start_angle_}
    {
        validate();
    }

    /**
     * @brief Return the CCW vertices of the regular polygon.
     */
    [[nodiscard]] std::vector<Point2>
    polygonize([[maybe_unused]] const PolygonizePolicy& policy) const
    {
        validate();
        std::vector<Point2> out(static_cast<std::size_t>(sides));
        polygonize_into(std::span<Point2>(out.data(), out.size()));
        return out;
    }

    /**
     * @brief Write the CCW vertices into `dst`.
     * @throws VMMException if `dst.size() < sides`.
     */
    void polygonize_into(std::span<Point2> dst) const {
        validate_destination(dst);
        const auto n = static_cast<std::size_t>(sides);
        for (std::size_t i = 0; i < n; ++i) {
            const Real t = start_angle + Real{2} * ::vmm::constants::kPi *
                           static_cast<Real>(i) / static_cast<Real>(n);
            dst[i] = Point2{
                center.x + radius * std::cos(static_cast<double>(t)),
                center.y + radius * std::sin(static_cast<double>(t))
            };
        }
    }

private:
    void validate() const {
        if (sides < 3) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "RegularNGon"}, {"what", "sides_lt_3"}});
        }
        if (radius <= Real{0}) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "RegularNGon"}, {"what", "non_positive_radius"}});
        }
    }

    void validate_destination(std::span<Point2> dst) const {
        validate();
        if (dst.size() < static_cast<std::size_t>(sides)) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"where", "RegularNGon::polygonize_into"},
                       {"need", std::to_string(sides)}});
        }
    }
};

static_assert(Shape2DLike<RegularNGon, PolygonizePolicy>);

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
