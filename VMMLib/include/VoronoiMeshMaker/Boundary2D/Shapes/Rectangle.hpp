#pragma once
//==============================================================================
// Name        : Rectangle.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D / Shapes
// Description : Axis-aligned rectangle (default anchored at lower-left / LL)
// License     : GNU GPL v3
// Version     : 0.5.0
//==============================================================================

/**
 * @file Rectangle.hpp
 * @brief Axis-aligned rectangle with lower-left anchoring by default.
 *
 * Construction styles:
 *  1) Rectangle(w, h)                     -> anchored at LL = (0, 0)
 *  2) Rectangle(ll, w, h)                 -> anchored at user-provided LL
 *  3) Rectangle(w, h, Rectangle::Centered) -> centered at the origin
 *
 * Design notes:
 *  - No rotation/translation beyond LL anchoring; transforms live outside shapes.
 *  - polygonize(policy) returns std::vector<Point2> (4 CCW vertices).
 *  - polygonize_into(span<Point2>) writes the same 4 CCW vertices without
 *    allocations (requires dst.size() >= 4).
 *
 * Exceptions:
 *  - CoreErr::InvalidArgument when width/height are non-positive.
 *  - CoreErr::OutOfRange when destination span has size < 4.
 */

// -----------------------------------------------------------------------------
//  C++ Standard Library (alphabetical inside family)
// -----------------------------------------------------------------------------
#include <span>
#include <vector>

// -----------------------------------------------------------------------------
//  VoronoiMeshMaker (alphabetical inside family)
// -----------------------------------------------------------------------------
#include <VoronoiMeshMaker/Boundary2D/Boundary2DConcepts.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Boundary2D/Policies/PolygonizePolicy.hpp>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>

VORMAKER_NAMESPACE_OPEN
BOUNDARY2D_NAMESPACE_OPEN

/**
 * @brief Axis-aligned rectangle polygonized as four CCW vertices.
 *
 * `Rectangle` is deliberately not responsible for rotation. Build the boundary
 * with `make_boundary(rectangle)` and use `rotate_in_place` when an oriented
 * rectangle is needed.
 *
 * @invariant `w > 0`
 * @invariant `h > 0`
 */
struct Rectangle {
    // ----- Tags ----------------------------------------------------------------
    struct CenteredTag { };
    static constexpr CenteredTag Centered{};

    // ----- State ----------------------------------------------------------------
    ///< Lower-left anchor.
    Point2 ll{Real{0}, Real{0}};

    ///< Width. Must be strictly positive.
    Real   w{1.0};

    ///< Height. Must be strictly positive.
    Real   h{1.0};

    [[nodiscard]] constexpr bool has_positive_dims() const noexcept {
        return (w > Real{0}) && (h > Real{0});
    }

    // ----- Constructors ---------------------------------------------------------
    /// @brief Construct anchored at the origin (LL = (0,0)).
    explicit Rectangle(Real w_, Real h_)
    : ll{Point2{Real{0}, Real{0}}}, w{w_}, h{h_}
    {
        using ::vmm::error::CoreErr;
        if (w_ <= Real{0} || h_ <= Real{0}) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape","Rectangle"}, {"what","non_positive_dims"}});
        }
    }

    /// @brief Construct with explicit lower-left anchor.
    Rectangle(Point2 ll_, Real w_, Real h_)
    : ll{ll_}, w{w_}, h{h_}
    {
        using ::vmm::error::CoreErr;
        if (w_ <= Real{0} || h_ <= Real{0}) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape","Rectangle"}, {"what","non_positive_dims"}});
        }
    }

    /// @brief Construct centered at the origin via tag.
    Rectangle(Real w_, Real h_, CenteredTag)
    : ll{Point2{-w_ * Real{0.5}, -h_ * Real{0.5}}}, w{w_}, h{h_}
    {
        using ::vmm::error::CoreErr;
        if (w_ <= Real{0} || h_ <= Real{0}) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape","Rectangle"}, {"what","non_positive_dims"}});
        }
    }

    // ----- Geometry -------------------------------------------------------------
    /**
     * @brief Generates the 4 CCW vertices:
     *        (x0,y0), (x0+w,y0), (x0+w,y0+h), (x0,y0+h)
     */
    [[nodiscard]] std::vector<Point2>
    polygonize([[maybe_unused]] const ::vmm::b2d::PolygonizePolicy& pol) const
    {
        using ::vmm::error::CoreErr;
        if (!has_positive_dims()) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"shape","Rectangle"}, {"what","non_positive_dims"}});
        }

        const Real x0 = ll.x;
        const Real y0 = ll.y;

        std::vector<Point2> out;
        out.reserve(4);
        out.emplace_back(Point2{x0,       y0      });
        out.emplace_back(Point2{x0 + w,   y0      });
        out.emplace_back(Point2{x0 + w,   y0 + h  });
        out.emplace_back(Point2{x0,       y0 + h  });
        return out;
    }

    /**
     * @brief Writes the 4 CCW vertices into `dst` (must have size >= 4).
     */
    void polygonize_into(std::span<Point2> dst) const
    {
        using ::vmm::error::CoreErr;
        if (!has_positive_dims()) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"where","Rectangle::polygonize_into"},
                       {"what","non_positive_dims"}});
        }
        if (dst.size() < 4) {
            VMM_THROW(CoreErr::OutOfRange,
                      {{"where","Rectangle::polygonize_into"}, {"need","4"}});
        }

        const Real x0 = ll.x;
        const Real y0 = ll.y;
        dst[0] = Point2{x0,       y0      };
        dst[1] = Point2{x0 + w,   y0      };
        dst[2] = Point2{x0 + w,   y0 + h  };
        dst[3] = Point2{x0,       y0 + h  };
    }
};

// Concept check (must return std::vector<Point2> when polygonizing)
static_assert(Shape2DLike<Rectangle, ::vmm::b2d::PolygonizePolicy>);

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
