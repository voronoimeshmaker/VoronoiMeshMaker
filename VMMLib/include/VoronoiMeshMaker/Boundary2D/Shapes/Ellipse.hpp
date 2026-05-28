#pragma once
//==============================================================================
// Name        : Ellipse.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D / Shapes
// Description : Axis-aligned polygonizable 2D ellipse boundary.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Ellipse.hpp
 * @brief Value type representing an axis-aligned elliptical boundary.
 *
 * The ellipse is centered at `center` and sampled with `segments` vertices in
 * counter-clockwise order. Rotation is intentionally handled by
 * `Boundary2DTransform`, keeping the shape itself simple and free of inheritance
 * or virtual dispatch.
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
 * @brief Axis-aligned ellipse polygonized as a CCW ring.
 *
 * @invariant `radius_x > 0`
 * @invariant `radius_y > 0`
 * @invariant `segments >= 3`
 */
struct Ellipse {
    ///< Ellipse center.
    Point2 center{Real{0}, Real{0}};

    ///< Semi-axis along x. Must be strictly positive.
    Real radius_x{Real{1}};

    ///< Semi-axis along y. Must be strictly positive.
    Real radius_y{Real{0.5}};

    ///< Number of vertices used to approximate the ellipse.
    Index segments{64};

    /**
     * @brief Construct an ellipse centered at the origin.
     */
    Ellipse(Real radius_x_, Real radius_y_, Index segments_ = 64)
        : radius_x{radius_x_}, radius_y{radius_y_}, segments{segments_}
    {
        validate();
    }

    /**
     * @brief Construct an ellipse with explicit center.
     */
    Ellipse(Point2 center_, Real radius_x_, Real radius_y_, Index segments_ = 64)
        : center{center_}, radius_x{radius_x_}, radius_y{radius_y_}, segments{segments_}
    {
        validate();
    }

    /**
     * @brief Return a CCW polygonal approximation of the ellipse.
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
     * @brief Write the CCW polygonal approximation into `dst`.
     * @throws VMMException if `dst.size() < segments`.
     */
    void polygonize_into(std::span<Point2> dst) const {
        validate_destination(dst);
        const auto n = static_cast<std::size_t>(segments);
        for (std::size_t i = 0; i < n; ++i) {
            const Real t = Real{2} * ::vmm::constants::kPi *
                           static_cast<Real>(i) / static_cast<Real>(n);
            dst[i] = Point2{
                center.x + radius_x * std::cos(static_cast<double>(t)),
                center.y + radius_y * std::sin(static_cast<double>(t))
            };
        }
    }

private:
    void validate() const {
        using ::vmm::error::CoreErr;
        if (radius_x <= Real{0} || radius_y <= Real{0}) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape", "Ellipse"}, {"what", "non_positive_radius"}});
        }
        if (segments < 3) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape", "Ellipse"}, {"what", "segments_lt_3"}});
        }
    }

    void validate_destination(std::span<Point2> dst) const {
        validate();
        if (dst.size() < static_cast<std::size_t>(segments)) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"where", "Ellipse::polygonize_into"},
                       {"need", std::to_string(segments)}});
        }
    }
};

static_assert(Shape2DLike<Ellipse, PolygonizePolicy>);

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
