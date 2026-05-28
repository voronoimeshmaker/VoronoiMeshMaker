#pragma once
//==============================================================================
// Name        : Ring2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D / Shapes
// Description : Circular annulus represented by an outer ring and one hole.
// License     : GNU GPL v3
// Version     : 0.1.0
//==============================================================================

/**
 * @file Ring2D.hpp
 * @brief Circular annulus shape for Boundary2D.
 *
 * Ring2D stores a center, an inner radius, an outer radius and a polygonization
 * segment count. The outer contour is generated counter-clockwise and the inner
 * contour is generated clockwise, matching Boundary2DData's Outer/Hole
 * convention.
 */

#include <cmath>
#include <span>
#include <string>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DConcepts.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Boundary2D/Policies/PolygonizePolicy.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>

VORMAKER_NAMESPACE_OPEN
BOUNDARY2D_NAMESPACE_OPEN

/**
 * @brief Circular annulus represented by one outer loop and one hole loop.
 *
 * `Ring2D` is the first Boundary2D shape that naturally produces multiple
 * loops. `polygonize()` returns only the outer loop to satisfy the simple shape
 * concept, while `boundary_data()` returns the full annulus: an outer CCW ring
 * and an inner CW hole ring.
 *
 * @invariant `0 < inner_radius < outer_radius`
 * @invariant `segments >= 3`
 */
struct Ring2D {
    ///< Annulus center.
    Point2 center{Real{0}, Real{0}};

    ///< Radius of the inner hole. Must be strictly positive.
    Real inner_radius{Real{0.5}};

    ///< Radius of the outer loop. Must be larger than inner_radius.
    Real outer_radius{Real{1.0}};

    ///< Number of vertices in each loop.
    Index segments{64};

    /**
     * @brief Construct centered at the origin.
     */
    explicit Ring2D(Real inner_radius_,
                    Real outer_radius_,
                    Index segments_ = 64)
        : center{Real{0}, Real{0}},
          inner_radius{inner_radius_},
          outer_radius{outer_radius_},
          segments{segments_}
    {
        validate();
    }

    /**
     * @brief Construct with explicit center.
     */
    Ring2D(Point2 center_,
           Real inner_radius_,
           Real outer_radius_,
           Index segments_ = 64)
        : center{center_},
          inner_radius{inner_radius_},
          outer_radius{outer_radius_},
          segments{segments_}
    {
        validate();
    }

    [[nodiscard]] constexpr bool has_valid_radii() const noexcept {
        return inner_radius > Real{0} &&
               outer_radius > Real{0} &&
               inner_radius < outer_radius;
    }

    [[nodiscard]] constexpr bool has_valid_segments() const noexcept {
        return segments >= 3;
    }

    /**
     * @brief Return the outer CCW ring for simple-shape compatibility.
     */
    [[nodiscard]] std::vector<Point2>
    polygonize([[maybe_unused]] const PolygonizePolicy& policy) const
    {
        return outer_ring();
    }

    /**
     * @brief Return the outer loop in CCW orientation.
     */
    [[nodiscard]] std::vector<Point2> outer_ring() const {
        validate();
        std::vector<Point2> out;
        out.resize(static_cast<std::size_t>(segments));
        write_circle(out, outer_radius, Orientation::CounterClockwise);
        return out;
    }

    /**
     * @brief Return the inner loop in CW orientation, suitable as a hole.
     */
    [[nodiscard]] std::vector<Point2> hole_ring() const {
        validate();
        std::vector<Point2> out;
        out.resize(static_cast<std::size_t>(segments));
        write_circle(out, inner_radius, Orientation::Clockwise);
        return out;
    }

    /**
     * @brief Write the outer CCW loop into `dst`.
     */
    void outer_ring_into(std::span<Point2> dst) const {
        validate_destination(dst);
        write_circle(dst, outer_radius, Orientation::CounterClockwise);
    }

    /**
     * @brief Write the inner CW hole loop into `dst`.
     */
    void hole_ring_into(std::span<Point2> dst) const {
        validate_destination(dst);
        write_circle(dst, inner_radius, Orientation::Clockwise);
    }

    /**
     * @brief Return canonical Boundary2DData with one outer loop and one hole.
     */
    [[nodiscard]] Boundary2DData
    boundary_data([[maybe_unused]] const PolygonizePolicy& policy,
                  RegionId region = RegionId{0}) const
    {
        auto outer = outer_ring();
        auto hole = hole_ring();

        Boundary2DData data;
        data.reserve(static_cast<Index>(outer.size() + hole.size()), 2);
        data.append_ring_checked(std::span<const Point2>(outer.data(), outer.size()),
                                 LoopKind::Outer,
                                 region);
        data.append_ring_checked(std::span<const Point2>(hole.data(), hole.size()),
                                 LoopKind::Hole,
                                 region);
        return data;
    }

private:
    enum class Orientation {
        CounterClockwise,
        Clockwise
    };

    void validate() const {
        using ::vmm::error::CoreErr;
        if (!has_valid_radii()) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape", "Ring2D"}, {"what", "invalid_radii"}});
        }
        if (!has_valid_segments()) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape", "Ring2D"}, {"what", "segments_lt_3"}});
        }
    }

    void validate_destination(std::span<Point2> dst) const {
        validate();
        if (dst.size() < static_cast<std::size_t>(segments)) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"where", "Ring2D::ring_into"},
                       {"need", std::to_string(segments)}});
        }
    }

    void write_circle(std::span<Point2> dst,
                      Real radius,
                      Orientation orientation) const
    {
        const auto n = static_cast<std::size_t>(segments);
        const auto sign = (orientation == Orientation::CounterClockwise)
                            ? Real{1}
                            : Real{-1};

        for (std::size_t i = 0; i < n; ++i) {
            const Real t = sign *
                (Real{2} * ::vmm::constants::kPi *
                 static_cast<Real>(i) / static_cast<Real>(n));
            dst[i] = Point2{
                center.x + radius * std::cos(static_cast<double>(t)),
                center.y + radius * std::sin(static_cast<double>(t))
            };
        }
    }
};

static_assert(Shape2DLike<Ring2D, PolygonizePolicy>);

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
