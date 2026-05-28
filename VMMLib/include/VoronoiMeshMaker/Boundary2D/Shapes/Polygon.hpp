#pragma once
//==============================================================================
// Name        : Polygon.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D / Shapes
// Description : User-provided polygonal 2D boundary.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Polygon.hpp
 * @brief Value type for an explicit polygon supplied by the caller.
 *
 * `Polygon` stores a single outer ring. The constructor accepts clockwise or
 * counter-clockwise vertex order and normalizes valid input to CCW. Holes are
 * represented at the `Boundary2DData`/domain level, not inside this simple
 * single-ring shape.
 */

#include <algorithm>
#include <initializer_list>
#include <span>
#include <string>
#include <utility>
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
 * @brief Explicit polygon shape represented by one CCW outer ring.
 *
 * @invariant at least three vertices
 * @invariant non-zero signed area after orientation normalization
 */
struct Polygon {
    ///< Polygon vertices in CCW order after construction.
    std::vector<Point2> vertices{};

    /**
     * @brief Construct from an owning vertex vector.
     * @throws VMMException if the polygon has fewer than three vertices or zero area.
     */
    explicit Polygon(std::vector<Point2> vertices_)
        : vertices{std::move(vertices_)}
    {
        normalize_orientation();
        validate();
    }

    /**
     * @brief Construct from a non-owning span, copying vertices into the shape.
     */
    explicit Polygon(std::span<const Point2> vertices_)
        : vertices{vertices_.begin(), vertices_.end()}
    {
        normalize_orientation();
        validate();
    }

    /**
     * @brief Construct from an initializer list.
     */
    Polygon(std::initializer_list<Point2> vertices_)
        : vertices{vertices_}
    {
        normalize_orientation();
        validate();
    }

    /**
     * @brief Return the CCW vertices of the polygon.
     */
    [[nodiscard]] std::vector<Point2>
    polygonize([[maybe_unused]] const PolygonizePolicy& policy) const
    {
        validate();
        return vertices;
    }

    /**
     * @brief Copy polygon vertices into `dst`.
     * @throws VMMException if `dst` is smaller than the vertex count.
     */
    void polygonize_into(std::span<Point2> dst) const {
        validate();
        if (dst.size() < vertices.size()) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"where", "Polygon::polygonize_into"},
                       {"need", std::to_string(vertices.size())}});
        }
        std::copy(vertices.begin(), vertices.end(), dst.begin());
    }

private:
    [[nodiscard]] Real signed_area() const noexcept {
        if (vertices.size() < 3) return Real{0};

        long double twice_area = 0.0L;
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const auto& p = vertices[i];
            const auto& q = vertices[(i + 1U) % vertices.size()];
            twice_area += static_cast<long double>(p.x) * static_cast<long double>(q.y)
                        - static_cast<long double>(p.y) * static_cast<long double>(q.x);
        }
        return static_cast<Real>(twice_area * 0.5L);
    }

    void normalize_orientation() {
        if (signed_area() < Real{0}) {
            std::reverse(vertices.begin(), vertices.end());
        }
    }

    void validate() const {
        if (vertices.size() < 3) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "Polygon"}, {"what", "vertices_lt_3"}});
        }
        if (signed_area() <= ::vmm::constants::kZeroTol) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"shape", "Polygon"}, {"what", "degenerate"}});
        }
    }
};

static_assert(Shape2DLike<Polygon, PolygonizePolicy>);

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
