#pragma once
//==============================================================================
// Name        : Circle.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D / Shapes
// Description : Polygonizable 2D circle boundary.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Circle.hpp
 * @brief Value type representing a circular 2D boundary.
 *
 * `Circle` is a lightweight shape description: it stores only center, radius and
 * the number of polygonization segments. It does not own topology and it does
 * not depend on CGAL. Use `polygonize()` for a CCW ring, or `make_boundary()` to
 * obtain canonical `Boundary2DData` suitable for validation, transforms and VTK
 * export.
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
 * @brief Circular 2D shape polygonized as a counter-clockwise closed ring.
 *
 * @invariant `radius > 0`
 * @invariant `segments >= 3`
 * @note The generated ring is not explicitly closed by duplicating the first
 *       vertex; writers close the polyline when needed.
 */
struct Circle {
    ///< Center of the circle.
    Point2 center{Real{0}, Real{0}};

    ///< Circle radius. Must be strictly positive.
    Real radius{Real{1}};

    ///< Number of vertices used to approximate the circumference.
    Index segments{64};

    /**
     * @brief Construct a circle centered at the origin.
     * @throws VMMException if radius is non-positive or segments < 3.
     */
    explicit Circle(Real radius_, Index segments_ = 64)
        : radius{radius_}, segments{segments_}
    {
        validate();
    }

    /**
     * @brief Construct a circle with explicit center.
     * @throws VMMException if radius is non-positive or segments < 3.
     */
    Circle(Point2 center_, Real radius_, Index segments_ = 64)
        : center{center_}, radius{radius_}, segments{segments_}
    {
        validate();
    }

    /**
     * @brief Return the CCW polygonal approximation of the circle.
     */
    [[nodiscard]] std::vector<Point2>
    polygonize([[maybe_unused]] const PolygonizePolicy& policy) const
    {
        validate();
        std::vector<Point2> out(static_cast<std::size_t>(segments));
        polygonize_into(std::span<Point2>(out.data(), out.size()));
        return out;
    }

    /**
     * @brief Write the CCW polygonal approximation into a caller-owned buffer.
     * @throws VMMException if `dst.size() < segments`.
     */
    void polygonize_into(std::span<Point2> dst) const {
        validate_destination(dst);
        const auto n = static_cast<std::size_t>(segments);
        for (std::size_t i = 0; i < n; ++i) {
            const Real t = Real{2} * ::vmm::constants::kPi *
                           static_cast<Real>(i) / static_cast<Real>(n);
            dst[i] = Point2{
                center.x + radius * std::cos(static_cast<double>(t)),
                center.y + radius * std::sin(static_cast<double>(t))
            };
        }
    }

private:
    void validate() const {
        using ::vmm::error::CoreErr;
        if (radius <= Real{0}) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape", "Circle"}, {"what", "non_positive_radius"}});
        }
        if (segments < 3) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape", "Circle"}, {"what", "segments_lt_3"}});
        }
    }

    void validate_destination(std::span<Point2> dst) const {
        validate();
        if (dst.size() < static_cast<std::size_t>(segments)) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"where", "Circle::polygonize_into"},
                       {"need", std::to_string(segments)}});
        }
    }
};

static_assert(Shape2DLike<Circle, PolygonizePolicy>);

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
