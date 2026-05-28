#pragma once
//==============================================================================
// Name        : Boundary2DBuilder.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D
// Description : Builders for canonical Boundary2DData objects.
// License     : GNU GPL v3
// Version     : 0.1.0
//==============================================================================

#include <concepts>
#include <span>
#include <type_traits>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DConcepts.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Boundary2D/Policies/PolygonizePolicy.hpp>

VORMAKER_NAMESPACE_OPEN
BOUNDARY2D_NAMESPACE_OPEN

namespace detail {

template <class Shape>
concept BoundaryDataShape2DLike =
    requires(const Shape& shape,
             const PolygonizePolicy& policy,
             RegionId region)
{
    { shape.boundary_data(policy, region) } -> std::same_as<Boundary2DData>;
};

} // namespace detail

inline Boundary2DData make_boundary_from_outer(
    std::span<const Point2> outer,
    RegionId region = RegionId{0})
{
    Boundary2DData data;
    data.reserve(static_cast<Index>(outer.size()), 1);
    data.append_ring_checked(outer, LoopKind::Outer, region);
    return data;
}

inline Boundary2DData make_boundary_from_rings(
    std::span<const Point2> outer,
    std::span<const std::span<const Point2>> holes,
    RegionId region = RegionId{0})
{
    Index point_count = static_cast<Index>(outer.size());
    for (const auto hole : holes) {
        point_count += static_cast<Index>(hole.size());
    }

    Boundary2DData data;
    data.reserve(point_count, static_cast<Index>(holes.size() + 1U));
    data.append_ring_checked(outer, LoopKind::Outer, region);
    for (const auto hole : holes) {
        data.append_ring_checked(hole, LoopKind::Hole, region);
    }
    return data;
}

template <class Shape>
requires (Shape2DLike<Shape, PolygonizePolicy> ||
          detail::BoundaryDataShape2DLike<Shape>)
[[nodiscard]] Boundary2DData make_boundary(
    const Shape& shape,
    const PolygonizePolicy& policy = PolygonizePolicy{},
    RegionId region = RegionId{0})
{
    if constexpr (detail::BoundaryDataShape2DLike<Shape>) {
        return shape.boundary_data(policy, region);
    } else {
        const auto outer = shape.polygonize(policy);
        return make_boundary_from_outer(
            std::span<const Point2>(outer.data(), outer.size()),
            region);
    }
}

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
