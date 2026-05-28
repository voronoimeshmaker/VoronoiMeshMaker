#pragma once
//==============================================================================
// Name        : Triangle.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D / Shapes
// Description : Polygonizable 2D triangle boundary.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Triangle.hpp
 * @brief Value type representing a non-degenerate triangle.
 */

#include <algorithm>
#include <array>
#include <span>
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
 * @brief Triangle shape stored as three counter-clockwise vertices.
 *
 * The constructor accepts either orientation and normalizes valid triangles to
 * CCW order. Collinear vertices are rejected.
 */
struct Triangle {
    ///< Triangle vertices in CCW order after construction.
    std::array<Point2, 3> vertices{};

    /**
     * @brief Construct from three vertices.
     * @throws VMMException if the triangle is degenerate.
     */
    Triangle(Point2 a, Point2 b, Point2 c)
        : vertices{a, b, c}
    {
        normalize_orientation();
        validate();
    }

    /**
     * @brief Return the three CCW vertices.
     */
    [[nodiscard]] std::vector<Point2>
    polygonize([[maybe_unused]] const PolygonizePolicy& policy) const
    {
        validate();
        return {vertices.begin(), vertices.end()};
    }

    /**
     * @brief Write the three CCW vertices into `dst`.
     * @throws VMMException if `dst.size() < 3`.
     */
    void polygonize_into(std::span<Point2> dst) const {
        validate();
        if (dst.size() < vertices.size()) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"where", "Triangle::polygonize_into"}, {"need", "3"}});
        }
        std::copy(vertices.begin(), vertices.end(), dst.begin());
    }

private:
    [[nodiscard]] Real signed_area() const noexcept {
        const auto& a = vertices[0];
        const auto& b = vertices[1];
        const auto& c = vertices[2];
        return Real{0.5} * ((a.x * b.y + b.x * c.y + c.x * a.y) -
                            (a.y * b.x + b.y * c.x + c.y * a.x));
    }

    void normalize_orientation() noexcept {
        if (signed_area() < Real{0}) {
            std::swap(vertices[1], vertices[2]);
        }
    }

    void validate() const {
        if (signed_area() <= ::vmm::constants::kZeroTol) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "Triangle"}, {"what", "degenerate"}});
        }
    }
};

static_assert(Shape2DLike<Triangle, PolygonizePolicy>);

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
