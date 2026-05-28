#pragma once
//==============================================================================
// Name        : ClippedVoronoiDiagram2DTransform.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Diagram
// Description : Affine transforms for clipped Voronoi diagrams.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file ClippedVoronoiDiagram2DTransform.hpp
 * @brief Applies rigid/affine transforms to an already clipped Voronoi diagram.
 */

#include <cmath>
#include <span>

#include <VoronoiMeshMaker/Boundary2D/Transforms/Boundary2DTransform.hpp>
#include <VoronoiMeshMaker/Sites2D/Transforms/Site2DTransform.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

[[nodiscard]] inline Real distance_between(::vmm::b2d::Point2 a,
                                           ::vmm::b2d::Point2 b) noexcept
{
    const Real dx = a.x - b.x;
    const Real dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline void transform_in_place(ClippedVoronoiDiagram2D& diagram,
                               ::vmm::b2d::Affine2 transform)
{
    ::vmm::b2d::transform_in_place(diagram.boundary, transform);
    ::vmm::s2d::transform_in_place(diagram.sites, transform);

    for (auto& cell : diagram.cells) {
        ::vmm::b2d::transform_in_place(
            std::span<::vmm::b2d::Point2>(
                cell.polygon.data(),
                cell.polygon.size()),
            transform);

        const auto site_slot = static_cast<std::size_t>(cell.site_id.value);
        const bool have_site = site_slot < diagram.sites.size();
        const auto site_point = have_site
            ? diagram.sites[site_slot].point
            : ::vmm::b2d::Point2{};

        for (auto& edge : cell.edges) {
            edge.a = ::vmm::b2d::apply_transform(edge.a, transform);
            edge.b = ::vmm::b2d::apply_transform(edge.b, transform);
            edge.midpoint =
                ::vmm::b2d::apply_transform(edge.midpoint, transform);
            edge.representative_point =
                ::vmm::b2d::apply_transform(
                    edge.representative_point,
                    transform);

            edge.length = distance_between(edge.a, edge.b);

            if (edge.representative_valid && have_site) {
                edge.representative_distance =
                    distance_between(site_point, edge.representative_point);
            }
        }

        if (cell.boundary_projection_valid) {
            cell.boundary_projection_point =
                ::vmm::b2d::apply_transform(
                    cell.boundary_projection_point,
                    transform);

            if (have_site) {
                cell.boundary_projection_distance =
                    distance_between(site_point, cell.boundary_projection_point);
            }
        }
    }

    if (diagram.sites.size() >= 3U) {
        diagram.delaunay = DelaunayBuilder2D::build(diagram.sites);
    }
}

[[nodiscard]] inline ClippedVoronoiDiagram2D transformed(
    ClippedVoronoiDiagram2D diagram,
    ::vmm::b2d::Affine2 transform)
{
    transform_in_place(diagram, transform);
    return diagram;
}

inline void rotate_in_place(ClippedVoronoiDiagram2D& diagram,
                            Real angle_rad,
                            ::vmm::b2d::Point2 center = ::vmm::b2d::Point2{})
{
    transform_in_place(
        diagram,
        ::vmm::b2d::rotation_transform(angle_rad, center));
}

[[nodiscard]] inline ClippedVoronoiDiagram2D rotated(
    ClippedVoronoiDiagram2D diagram,
    Real angle_rad,
    ::vmm::b2d::Point2 center = ::vmm::b2d::Point2{})
{
    rotate_in_place(diagram, angle_rad, center);
    return diagram;
}

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE