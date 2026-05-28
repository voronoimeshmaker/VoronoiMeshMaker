#pragma once
//==============================================================================
// Name        : VoronoiBoundaryCellDetector2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Clipping
// Description : Boundary-cell detection by boundary-edge propagation.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VoronoiBoundaryCellDetector2D.hpp
 * @brief Detects boundary Voronoi cells using the Yan et al. FIFO propagation.
 *
 * The algorithm follows the 2D clipped Voronoi computation described by
 * Yan et al.:
 *  - each boundary edge starts from the nearest incident Voronoi cell;
 *  - a FIFO queue stores incident cell-boundary-edge pairs;
 *  - each boundary edge segment is clipped against the current Voronoi cell;
 *  - boundary-vertex endpoints propagate to adjacent boundary edges;
 *  - Voronoi-edge intersection endpoints propagate to neighboring cells.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <queue>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Clipping/BisectorHalfplane.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayNeighborProvider2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunaySiteIndex.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

enum class SegmentEndpointOrigin2D : unsigned char {
    BoundaryStart,
    BoundaryEnd,
    VoronoiEdge,
    Degenerate
};

struct SegmentEndpointEvent2D {
    SegmentEndpointOrigin2D origin{SegmentEndpointOrigin2D::Degenerate};
    ::vmm::s2d::SiteId neighbor{};
};

struct BoundaryCellIncidentEdge2D {
    ::vmm::s2d::SiteId site_id{};
    std::size_t edge_index{0};
};

struct BoundaryCellDetection2D {
    std::vector<bool> is_boundary_site{};
    std::vector<std::size_t> boundary_site_indices{};
    std::vector<BoundaryCellIncidentEdge2D> incident_edges{};

    [[nodiscard]] bool is_boundary(::vmm::s2d::SiteId site_id) const noexcept {
        const auto raw = site_id.value;
        return raw >= 0 &&
               static_cast<std::size_t>(raw) < is_boundary_site.size() &&
               is_boundary_site[static_cast<std::size_t>(raw)];
    }
};

/**
 * @brief Detects Voronoi cells that intersect Boundary2D edges.
 */
struct BoundaryCellDetector2D {
    using Point2 = ::vmm::s2d::Point2;
    using Real = ::vmm::s2d::Real;
    using SiteId = ::vmm::s2d::SiteId;
    using Index = ::vmm::s2d::Index;

    struct Options {
        Real eps;

        constexpr Options(Real eps_ = ::vmm::constants::kEpsilon) noexcept
            : eps{eps_}
        {}
    };

    [[nodiscard]] static BoundaryCellDetection2D detect(
        const ::vmm::s2d::SiteSet& sites,
        const ::vmm::b2d::Boundary2DData& boundary,
        const DelaunayTriangulation2D& triangulation,
        const DelaunaySiteIndex& index,
        const Options& options = Options{})
    {
        validate_inputs(sites, boundary);

        const auto edges = collect_edges(boundary);
        BoundaryCellDetection2D result;
        result.is_boundary_site.assign(sites.size(), false);

        if (edges.empty()) return result;

        std::vector<bool> edge_has_segment(edges.size(), false);
        std::queue<CellEdgePair> queue;
        std::unordered_set<std::uint64_t> processed_pairs;
        processed_pairs.reserve(edges.size() * 2U);

        for (std::size_t edge_index = 0; edge_index < edges.size(); ++edge_index) {
            if (!edge_has_segment[edge_index]) {
                queue.push(CellEdgePair{
                    nearest_site_to_midpoint(sites, edges[edge_index]),
                    edge_index
                });
            }

            while (!queue.empty()) {
                const auto current = queue.front();
                queue.pop();

                if (!is_valid_pair(current, sites.size(), edges.size())) continue;

                const auto key = pair_key(current, sites.size());
                if (processed_pairs.contains(key)) continue;
                processed_pairs.insert(key);

                const auto clipped = clip_boundary_edge_by_cell(
                    edges[current.edge_index],
                    current.site_id,
                    sites,
                    triangulation,
                    index,
                    options);

                if (!clipped.has_segment) continue;

                edge_has_segment[current.edge_index] = true;
                mark_boundary_site(result, current.site_id);
                result.incident_edges.push_back(
                    BoundaryCellIncidentEdge2D{current.site_id, current.edge_index});

                propagate_from_endpoint(
                    clipped.first,
                    current,
                    edges,
                    sites.size(),
                    queue);
                propagate_from_endpoint(
                    clipped.second,
                    current,
                    edges,
                    sites.size(),
                    queue);
            }
        }

        std::sort(result.boundary_site_indices.begin(),
                  result.boundary_site_indices.end());
        result.boundary_site_indices.erase(
            std::unique(result.boundary_site_indices.begin(),
                        result.boundary_site_indices.end()),
            result.boundary_site_indices.end());
        return result;
    }

private:
    struct BoundaryEdge {
        Point2 a{};
        Point2 b{};
        std::size_t previous_edge{0};
        std::size_t next_edge{0};
    };

    struct CellEdgePair {
        SiteId site_id{};
        std::size_t edge_index{0};
    };

    struct CellHalfplane {
        Halfplane2D halfplane{};
        SiteId neighbor{};
    };

    struct ClippedSegment {
        bool has_segment{false};
        SegmentEndpointEvent2D first{};
        SegmentEndpointEvent2D second{};
    };

    static void validate_inputs(const ::vmm::s2d::SiteSet& sites,
                                const ::vmm::b2d::Boundary2DData& boundary)
    {
        if (sites.empty()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where", "BoundaryCellDetector2D"},
                       {"reason", "empty_sites"}});
        }
        if (!sites.ids_are_sequential()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where", "BoundaryCellDetector2D"},
                       {"reason", "site_ids_must_be_sequential"}});
        }
        if (!boundary.invariant_ok()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where", "BoundaryCellDetector2D"},
                       {"reason", "invalid_boundary"}});
        }
    }

    [[nodiscard]] static std::vector<BoundaryEdge> collect_edges(
        const ::vmm::b2d::Boundary2DData& boundary)
    {
        std::vector<BoundaryEdge> edges;
        edges.reserve(static_cast<std::size_t>(boundary.vertex_count()));

        for (Index ring_id = 0; ring_id < boundary.ring_count(); ++ring_id) {
            const auto ring = boundary.ring(ring_id);
            if (ring.size() < 2U) continue;

            const auto ring_edge_begin = edges.size();
            for (std::size_t i = 0; i < ring.size(); ++i) {
                edges.push_back(BoundaryEdge{
                    ring[i],
                    ring[(i + 1U) % ring.size()],
                    0U,
                    0U
                });
            }

            const auto ring_edge_count = ring.size();
            for (std::size_t i = 0; i < ring_edge_count; ++i) {
                const auto global = ring_edge_begin + i;
                edges[global].previous_edge =
                    ring_edge_begin + ((i + ring_edge_count - 1U) % ring_edge_count);
                edges[global].next_edge =
                    ring_edge_begin + ((i + 1U) % ring_edge_count);
            }
        }
        return edges;
    }

    [[nodiscard]] static SiteId nearest_site_to_midpoint(
        const ::vmm::s2d::SiteSet& sites,
        const BoundaryEdge& edge)
    {
        const Point2 midpoint{
            Real{0.5} * (edge.a.x + edge.b.x),
            Real{0.5} * (edge.a.y + edge.b.y)
        };

        std::size_t best = 0;
        Real best_distance = squared_distance(midpoint, sites[0].point);
        for (std::size_t i = 1; i < sites.size(); ++i) {
            const Real distance = squared_distance(midpoint, sites[i].point);
            if (distance < best_distance) {
                best = i;
                best_distance = distance;
            }
        }
        return sites[best].id;
    }

    [[nodiscard]] static Real squared_distance(Point2 a, Point2 b) noexcept {
        const Real dx = a.x - b.x;
        const Real dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    [[nodiscard]] static bool is_valid_pair(CellEdgePair pair,
                                            std::size_t site_count,
                                            std::size_t edge_count) noexcept
    {
        return pair.site_id.value >= 0 &&
               static_cast<std::size_t>(pair.site_id.value) < site_count &&
               pair.edge_index < edge_count;
    }

    [[nodiscard]] static std::uint64_t pair_key(CellEdgePair pair,
                                                std::size_t site_count) noexcept
    {
        return (static_cast<std::uint64_t>(pair.edge_index) *
                static_cast<std::uint64_t>(site_count)) +
               static_cast<std::uint64_t>(pair.site_id.value);
    }

    [[nodiscard]] static std::vector<CellHalfplane> cell_halfplanes(
        const ::vmm::s2d::SiteSet& sites,
        const DelaunayTriangulation2D& triangulation,
        const DelaunaySiteIndex& index,
        SiteId site_id)
    {
        const auto owner = sites.at(static_cast<std::size_t>(site_id.value));
        const auto neighbors =
            DelaunayNeighborProvider2D::neighbor_site_ids(
                triangulation,
                index,
                site_id);

        std::vector<CellHalfplane> halfplanes;
        halfplanes.reserve(neighbors.size());
        for (const auto neighbor_id : neighbors) {
            const auto& neighbor =
                sites.at(static_cast<std::size_t>(neighbor_id.value));
            halfplanes.push_back(CellHalfplane{
                BisectorHalfplane::between(owner.point, neighbor.point),
                neighbor_id
            });
        }
        return halfplanes;
    }

    [[nodiscard]] static ClippedSegment clip_boundary_edge_by_cell(
        const BoundaryEdge& edge,
        SiteId site_id,
        const ::vmm::s2d::SiteSet& sites,
        const DelaunayTriangulation2D& triangulation,
        const DelaunaySiteIndex& index,
        const Options& options)
    {
        Real t_min{0};
        Real t_max{1};
        SegmentEndpointEvent2D lower{
            SegmentEndpointOrigin2D::BoundaryStart,
            SiteId{-1}
        };
        SegmentEndpointEvent2D upper{
            SegmentEndpointOrigin2D::BoundaryEnd,
            SiteId{-1}
        };

        const auto halfplanes =
            cell_halfplanes(sites, triangulation, index, site_id);

        for (const auto& item : halfplanes) {
            const Real fa = item.halfplane.evaluate(edge.a);
            const Real fb = item.halfplane.evaluate(edge.b);

            if (fa <= options.eps && fb <= options.eps) continue;
            if (fa > options.eps && fb > options.eps) {
                return ClippedSegment{};
            }

            const Real denom = fa - fb;
            if (std::abs(denom) <= options.eps) {
                return ClippedSegment{};
            }

            const Real t = fa / denom;
            const SegmentEndpointEvent2D event{
                SegmentEndpointOrigin2D::VoronoiEdge,
                item.neighbor
            };

            if (fa > options.eps && fb <= options.eps) {
                if (t > t_min) {
                    t_min = t;
                    lower = event;
                }
            } else if (fa <= options.eps && fb > options.eps) {
                if (t < t_max) {
                    t_max = t;
                    upper = event;
                }
            }

            if (t_min > t_max + options.eps) return ClippedSegment{};
        }

        if ((t_max - t_min) <= options.eps) return ClippedSegment{};
        return ClippedSegment{true, lower, upper};
    }

    static void mark_boundary_site(BoundaryCellDetection2D& result,
                                   SiteId site_id)
    {
        const auto slot = static_cast<std::size_t>(site_id.value);
        if (!result.is_boundary_site[slot]) {
            result.is_boundary_site[slot] = true;
            result.boundary_site_indices.push_back(slot);
        }
    }

    static void propagate_from_endpoint(
        const SegmentEndpointEvent2D& endpoint,
        CellEdgePair current,
        std::span<const BoundaryEdge> edges,
        std::size_t site_count,
        std::queue<CellEdgePair>& queue)
    {
        switch (endpoint.origin) {
            case SegmentEndpointOrigin2D::BoundaryStart:
                queue.push(CellEdgePair{
                    current.site_id,
                    edges[current.edge_index].previous_edge
                });
                break;
            case SegmentEndpointOrigin2D::BoundaryEnd:
                queue.push(CellEdgePair{
                    current.site_id,
                    edges[current.edge_index].next_edge
                });
                break;
            case SegmentEndpointOrigin2D::VoronoiEdge:
                if (endpoint.neighbor.value >= 0 &&
                    static_cast<std::size_t>(endpoint.neighbor.value) < site_count) {
                    queue.push(CellEdgePair{
                        endpoint.neighbor,
                        current.edge_index
                    });
                }
                break;
            case SegmentEndpointOrigin2D::Degenerate:
                break;
        }
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
