#pragma once
//==============================================================================
// Name        : Capsule.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D / Shapes
// Description : Horizontal capsule / stadium 2D boundary.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Capsule.hpp
 * @brief Value type for a horizontal capsule shape.
 *
 * A capsule is the Minkowski sum of a line segment and a disk. In this initial
 * 2D implementation the segment is horizontal; arbitrary orientation is obtained
 * through `Boundary2DTransform`.
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
 * @brief Horizontal capsule polygonized as a CCW ring.
 *
 * @invariant `center_distance > 0`
 * @invariant `radius > 0`
 * @invariant `segments_per_arc >= 2`
 */
struct Capsule {
    ///< Midpoint between the two semicircle centers.
    Point2 center{Real{0}, Real{0}};

    ///< Distance between the left and right semicircle centers.
    Real center_distance{Real{1}};

    ///< Radius of both semicircles and straight side offset.
    Real radius{Real{0.25}};

    ///< Number of subdivisions per semicircular arc.
    Index segments_per_arc{16};

    /**
     * @brief Construct a capsule centered at the origin.
     */
    Capsule(Real center_distance_, Real radius_, Index segments_per_arc_ = 16)
        : center_distance{center_distance_},
          radius{radius_},
          segments_per_arc{segments_per_arc_}
    {
        validate();
    }

    /**
     * @brief Construct a capsule with explicit center.
     */
    Capsule(Point2 center_,
            Real center_distance_,
            Real radius_,
            Index segments_per_arc_ = 16)
        : center{center_},
          center_distance{center_distance_},
          radius{radius_},
          segments_per_arc{segments_per_arc_}
    {
        validate();
    }

    /**
     * @brief Number of vertices produced by polygonization.
     */
    [[nodiscard]] Index vertex_count() const noexcept {
        return 2 * (segments_per_arc + 1);
    }

    /**
     * @brief Return the CCW polygonal approximation of the capsule.
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

        const Real half = center_distance * Real{0.5};
        const Point2 right{center.x + half, center.y};
        const Point2 left{center.x - half, center.y};
        const auto n = static_cast<std::size_t>(segments_per_arc);

        std::size_t k = 0;
        for (std::size_t i = 0; i <= n; ++i) {
            const Real t = -::vmm::constants::kPi * Real{0.5} +
                           ::vmm::constants::kPi *
                           static_cast<Real>(i) / static_cast<Real>(n);
            dst[k++] = Point2{
                right.x + radius * std::cos(static_cast<double>(t)),
                right.y + radius * std::sin(static_cast<double>(t))
            };
        }
        for (std::size_t i = 0; i <= n; ++i) {
            const Real t = ::vmm::constants::kPi * Real{0.5} +
                           ::vmm::constants::kPi *
                           static_cast<Real>(i) / static_cast<Real>(n);
            dst[k++] = Point2{
                left.x + radius * std::cos(static_cast<double>(t)),
                left.y + radius * std::sin(static_cast<double>(t))
            };
        }
    }

private:
    void validate() const {
        if (center_distance <= Real{0}) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "Capsule"}, {"what", "non_positive_center_distance"}});
        }
        if (radius <= Real{0}) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "Capsule"}, {"what", "non_positive_radius"}});
        }
        if (segments_per_arc < 2) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "Capsule"}, {"what", "segments_per_arc_lt_2"}});
        }
    }

    void validate_destination(std::span<Point2> dst) const {
        validate();
        if (dst.size() < static_cast<std::size_t>(vertex_count())) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"where", "Capsule::polygonize_into"},
                       {"need", std::to_string(vertex_count())}});
        }
    }
};

static_assert(Shape2DLike<Capsule, PolygonizePolicy>);

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
