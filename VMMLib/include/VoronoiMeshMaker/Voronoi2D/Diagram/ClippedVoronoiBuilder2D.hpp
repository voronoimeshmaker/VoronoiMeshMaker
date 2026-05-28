#pragma once
//==============================================================================
// Name        : ClippedVoronoiBuilder2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Diagram
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 1.4
// Description : Builds complete clipped 2D Voronoi diagrams.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file ClippedVoronoiBuilder2D.hpp
 * @brief Builds all clipped cells for a Boundary2D / SiteSet pair.
 *
 * The clipped cell polygon remains unchanged. This builder now also fills
 * per-edge metadata so downstream numerical methods can distinguish the actual
 * clipped edge from the representative point on Boundary2D used by boundary
 * formulae.
 *
 * @ingroup voronoi2d_diagram
 */

#include <algorithm>
#include <cmath>
#include <execution>
#include <numeric>
#include <string>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DContains.hpp>
#include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DDistance.hpp>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCellBuilder2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/BoundaryShortEdgeCollapse2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/ClippingWorkspace2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunaySiteIndex.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

struct ClippedVoronoiBuildOptions2D {
    DelaunayBuildOptions2D    delaunay{};
    VoronoiCellBuildOptions2D cells{};
    bool allow_parallel_cell_build{true};
    Real min_boundary_edge_length{Real{0}};
    BoundaryShortEdgePolicy2D boundary_short_edge_policy{
        BoundaryShortEdgePolicy2D::Keep};
};

struct ClippedVoronoiBuilder2D {
    [[nodiscard]] static ClippedVoronoiDiagram2D build(
        const ::vmm::s2d::SiteSet&          sites,
        const ::vmm::b2d::Boundary2DData&   boundary,
        const ClippedVoronoiBuildOptions2D& options = {})
    {
        validate_inputs_or_throw(sites, boundary, options);

        auto delaunay = DelaunayBuilder2D::build(sites, options.delaunay);
        auto index = DelaunaySiteIndex::from(delaunay);

        ClippedVoronoiDiagram2D diagram;
        diagram.boundary = boundary;
        diagram.sites    = sites;
        diagram.delaunay = delaunay;
        diagram.cells.resize(sites.size());

        std::vector<std::size_t> idx(sites.size());
        std::iota(idx.begin(), idx.end(), std::size_t{0});

        const auto& cell_opts = options.cells;

        auto build_one_cell = [&](std::size_t i) {
            thread_local ClippingWorkspace2D ws;

            auto& cell = diagram.cells[i];
            cell = VoronoiCellBuilder2D::build_unchecked(
                sites, boundary, delaunay, index, sites[i].id, ws, cell_opts);

            collapse_short_boundary_edges(
                cell.polygon,
                boundary,
                BoundaryShortEdgeCollapseOptions2D{
                    options.min_boundary_edge_length,
                    options.boundary_short_edge_policy});

            populate_cell_edges(cell, sites[i].point, boundary);
            cell.is_boundary_cell = cell_has_boundary_edge(cell);

            if (cell.is_boundary_cell) {
                const auto projection = ::vmm::b2d::project_to_boundary(
                    sites[i].point,
                    boundary);

                cell.boundary_projection_point = projection.point;
                cell.boundary_projection_distance = projection.distance;
                cell.boundary_projection_valid = projection.valid;
                cell.boundary_projection_inside_cell =
                    point_in_or_on_polygon(projection.point, cell.polygon);
            } else {
                cell.boundary_projection_point = {};
                cell.boundary_projection_distance = Real{0};
                cell.boundary_projection_valid = false;
                cell.boundary_projection_inside_cell = false;
            }
        };

        if (options.allow_parallel_cell_build) {
            std::for_each(std::execution::par_unseq,
                          idx.begin(), idx.end(),
                          build_one_cell);
        } else {
            std::for_each(std::execution::seq,
                          idx.begin(), idx.end(),
                          build_one_cell);
        }

        diagram.rebuild_indices();
        return diagram;
    }

private:
    [[nodiscard]] static Real squared_distance(
        ::vmm::s2d::Point2 a,
        ::vmm::s2d::Point2 b) noexcept
    {
        const Real dx = a.x - b.x;
        const Real dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    [[nodiscard]] static bool cell_has_boundary_edge(
        const VoronoiCell2D& cell) noexcept
    {
        return std::any_of(
            cell.edges.begin(),
            cell.edges.end(),
            [](const VoronoiCellEdge2D& edge) noexcept {
                return edge.is_boundary_edge;
            });
    }

    [[nodiscard]] static bool find_parent_boundary_segment(
        ::vmm::s2d::Point2 a,
        ::vmm::s2d::Point2 b,
        const ::vmm::b2d::Boundary2DData& boundary,
        ::vmm::b2d::Index& ring_index,
        std::size_t& edge_index,
        ::vmm::s2d::Point2& segment_a,
        ::vmm::s2d::Point2& segment_b) noexcept
    {
        constexpr Real eps = ::vmm::constants::kEpsilon;

        for (::vmm::b2d::Index r = 0; r < boundary.ring_count(); ++r) {
            const auto ring = boundary.ring(r);
            if (ring.size() < 2U) {
                continue;
            }

            for (std::size_t e = 0; e < ring.size(); ++e) {
                const auto& p = ring[e];
                const auto& q = ring[(e + 1U) % ring.size()];

                if (::vmm::b2d::point_on_segment(a, p, q, eps) &&
                    ::vmm::b2d::point_on_segment(b, p, q, eps)) {
                    ring_index = r;
                    edge_index = e;
                    segment_a = p;
                    segment_b = q;
                    return true;
                }
            }
        }

        ring_index = ::vmm::b2d::kInvalid;
        edge_index = 0U;
        segment_a = {};
        segment_b = {};
        return false;
    }

    static void populate_cell_edges(
        VoronoiCell2D& cell,
        ::vmm::s2d::Point2 site,
        const ::vmm::b2d::Boundary2DData& boundary)
    {
        cell.edges.clear();

        const std::size_t n = cell.polygon.size();
        if (n < 2U) {
            return;
        }

        cell.edges.reserve(n);

        for (std::size_t i = 0; i < n; ++i) {
            const auto& a = cell.polygon[i];
            const auto& b = cell.polygon[(i + 1U) % n];

            VoronoiCellEdge2D edge;
            edge.a = a;
            edge.b = b;
            edge.midpoint = ::vmm::s2d::Point2{
                (a.x + b.x) * Real{0.5},
                (a.y + b.y) * Real{0.5}
            };

            const Real dx = b.x - a.x;
            const Real dy = b.y - a.y;
            edge.length = std::sqrt(dx * dx + dy * dy);

            edge.representative_point = edge.midpoint;
            edge.representative_distance =
                std::sqrt(squared_distance(site, edge.representative_point));
            edge.representative_valid = edge.length > Real{0};
            edge.representative_inside_local_edge = true;

            ::vmm::b2d::Index ring_id = ::vmm::b2d::kInvalid;
            std::size_t boundary_edge_id = 0U;
            ::vmm::s2d::Point2 boundary_a{};
            ::vmm::s2d::Point2 boundary_b{};

            if (find_parent_boundary_segment(
                    a, b, boundary,
                    ring_id, boundary_edge_id,
                    boundary_a, boundary_b)) {
                const auto projection = ::vmm::b2d::project_to_segment(
                    site,
                    boundary_a,
                    boundary_b,
                    ring_id,
                    boundary_edge_id);

                edge.is_boundary_edge = true;
                edge.representative_point = projection.point;
                edge.representative_distance = projection.distance;
                edge.representative_valid = projection.valid;
                edge.boundary_ring_index = projection.ring_index;
                edge.boundary_edge_index = projection.edge_index;
                edge.boundary_segment_parameter = projection.segment_parameter;
                edge.representative_inside_local_edge =
                    ::vmm::b2d::point_on_segment(
                        projection.point,
                        a,
                        b,
                        ::vmm::constants::kEpsilon);
            }

            cell.edges.push_back(edge);
        }
    }

    [[nodiscard]] static bool point_in_or_on_polygon(
        ::vmm::s2d::Point2 point,
        const std::vector<::vmm::s2d::Point2>& polygon) noexcept
    {
        if (polygon.size() < 3U) {
            return false;
        }

        bool has_positive = false;
        bool has_negative = false;
        constexpr Real tol = static_cast<Real>(1.0e-10);

        for (std::size_t i = 0; i < polygon.size(); ++i) {
            const auto& a = polygon[i];
            const auto& b = polygon[(i + 1U) % polygon.size()];
            const Real cross =
                (b.x - a.x) * (point.y - a.y) -
                (b.y - a.y) * (point.x - a.x);

            if (cross > tol) {
                has_positive = true;
            } else if (cross < -tol) {
                has_negative = true;
            }

            if (has_positive && has_negative) {
                return false;
            }
        }

        return true;
    }

    static void validate_inputs_or_throw(
        const ::vmm::s2d::SiteSet&        sites,
        const ::vmm::b2d::Boundary2DData& boundary,
        const ClippedVoronoiBuildOptions2D& options)
    {
        if (sites.size() < 3U) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where",  "ClippedVoronoiBuilder2D"},
                       {"reason", "at_least_three_sites_required"},
                       {"count",  std::to_string(sites.size())}});
        }

        if (!sites.ids_are_sequential()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where",  "ClippedVoronoiBuilder2D"},
                       {"reason", "site_ids_must_be_sequential"}});
        }

        if (!boundary.invariant_ok()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where",  "ClippedVoronoiBuilder2D"},
                       {"reason", "invalid_boundary_invariant"}});
        }

        validate_boundary_short_edge_options_or_throw(
            BoundaryShortEdgeCollapseOptions2D{
                options.min_boundary_edge_length,
                options.boundary_short_edge_policy});
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
