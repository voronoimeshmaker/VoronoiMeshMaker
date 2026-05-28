#pragma once
//==============================================================================
// Name        : DelaunayBuilder2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Delaunay
// Description : Builds CGAL Delaunay triangulations from SiteSet.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file DelaunayBuilder2D.hpp
 * @brief Builds the 2D Delaunay triangulation used by the Voronoi pipeline.
 */

#include <cstddef>
#include <string>

#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Traits/CgalKernelTraits2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Options controlling Delaunay construction from SiteSet.
 */
struct DelaunayBuildOptions2D {
    bool require_sequential_ids{true};
    bool require_unique_points{true};
    bool require_two_dimensional{true};
};

using DelaunayTriangulation2D = CgalKernelTraits2D::DelaunayTriangulation;

/**
 * @brief Value-type builder for CGAL Delaunay triangulations.
 *
 * The builder inserts every Site2D point into a CGAL triangulation and stores
 * the corresponding SiteId in the CGAL vertex info field. The returned
 * triangulation is the first CGAL-backed data structure in the Voronoi pipeline.
 */
struct DelaunayBuilder2D {
    [[nodiscard]] static DelaunayTriangulation2D build(
        const ::vmm::s2d::SiteSet& sites,
        const DelaunayBuildOptions2D& options = DelaunayBuildOptions2D{})
    {
        if (sites.size() < 3U) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where", "DelaunayBuilder2D"},
                       {"reason", "at_least_three_sites_required"}});
        }
        if (options.require_sequential_ids && !sites.ids_are_sequential()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where", "DelaunayBuilder2D"},
                       {"reason", "non_sequential_site_ids"}});
        }

        DelaunayTriangulation2D triangulation;
        for (const auto& site : sites) {
            auto handle = triangulation.insert(
                CgalKernelTraits2D::to_cgal(site.point));
            handle->info() = site.id;
        }

        if (options.require_unique_points &&
            triangulation.number_of_vertices() != sites.size()) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where", "DelaunayBuilder2D"},
                       {"reason", "duplicate_site_points"},
                       {"input_count", std::to_string(sites.size())},
                       {"vertex_count", std::to_string(
                           static_cast<std::size_t>(
                               triangulation.number_of_vertices()))}});
        }

        if (options.require_two_dimensional && triangulation.dimension() != 2) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"where", "DelaunayBuilder2D"},
                       {"reason", "sites_do_not_span_2d"}});
        }

        return triangulation;
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
