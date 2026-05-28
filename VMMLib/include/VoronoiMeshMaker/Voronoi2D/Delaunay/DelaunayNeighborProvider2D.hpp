#pragma once
//==============================================================================
// Name        : DelaunayNeighborProvider2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Delaunay
// Description : Site-neighbor queries over CGAL Delaunay triangulations.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file DelaunayNeighborProvider2D.hpp
 * @brief Queries Delaunay-adjacent generator sites.
 */

#include <algorithm>
#include <vector>

#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunaySiteIndex.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Neighbor query helper for a Delaunay triangulation.
 *
 * The provider does not own the triangulation or the index. Queries are
 * read-only and deterministic: returned SiteId values are sorted increasingly.
 */
struct DelaunayNeighborProvider2D {
    using SiteId = ::vmm::s2d::SiteId;
    using VertexHandle = CgalKernelTraits2D::VertexHandle;

    [[nodiscard]] static std::vector<SiteId> neighbor_site_ids(
        const DelaunayTriangulation2D& triangulation,
        const DelaunaySiteIndex& index,
        SiteId site_id)
    {
        const VertexHandle vertex = index.at(site_id);
        std::vector<SiteId> neighbors;

        const auto circulator = triangulation.incident_vertices(vertex);
        if (circulator == nullptr) return neighbors;

        auto current = circulator;
        do {
            if (!triangulation.is_infinite(current)) {
                neighbors.push_back(current->info());
            }
            ++current;
        } while (current != circulator);

        std::sort(neighbors.begin(), neighbors.end(), [](SiteId a, SiteId b) {
            return a.value < b.value;
        });
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()),
                        neighbors.end());
        return neighbors;
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
