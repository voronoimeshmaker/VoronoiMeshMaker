#pragma once
//==============================================================================
// Name        : DelaunaySiteIndex.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Delaunay
// Description : Maps SiteId values to CGAL Delaunay vertex handles.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file DelaunaySiteIndex.hpp
 * @brief Provides compact SiteId-to-vertex lookup for Delaunay triangulations.
 */

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Compact index from sequential SiteId values to CGAL vertex handles.
 *
 * The index does not own the triangulation. It must be rebuilt whenever the
 * referenced triangulation is rebuilt or modified.
 */
struct DelaunaySiteIndex {
    using SiteId = ::vmm::s2d::SiteId;
    using Index = ::vmm::s2d::Index;
    using VertexHandle = CgalKernelTraits2D::VertexHandle;

    std::vector<VertexHandle> vertices{};

    [[nodiscard]] std::size_t size() const noexcept { return vertices.size(); }
    [[nodiscard]] bool empty() const noexcept { return vertices.empty(); }

    [[nodiscard]] bool contains(SiteId id) const noexcept {
        const auto raw = static_cast<Index>(id);
        return raw >= 0 &&
               static_cast<std::size_t>(raw) < vertices.size() &&
               vertices[static_cast<std::size_t>(raw)] != VertexHandle{};
    }

    [[nodiscard]] VertexHandle at(SiteId id) const {
        if (!contains(id)) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"index", std::to_string(static_cast<Index>(id))}});
        }
        return vertices[static_cast<std::size_t>(static_cast<Index>(id))];
    }

    [[nodiscard]] static DelaunaySiteIndex from(
        DelaunayTriangulation2D& triangulation)
    {
        DelaunaySiteIndex index;
        Index max_id{-1};
        for (auto vertex = triangulation.finite_vertices_begin();
             vertex != triangulation.finite_vertices_end();
             ++vertex) {
            max_id = std::max(max_id, vertex->info().value);
        }

        if (max_id < 0) return index;
        index.vertices.resize(static_cast<std::size_t>(max_id) + 1U);

        for (auto vertex = triangulation.finite_vertices_begin();
             vertex != triangulation.finite_vertices_end();
             ++vertex) {
            const auto raw = vertex->info().value;
            if (raw < 0) {
                VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                          {{"where", "DelaunaySiteIndex"},
                           {"reason", "negative_site_id"}});
            }
            const auto slot = static_cast<std::size_t>(raw);
            if (index.vertices[slot] != VertexHandle{}) {
                VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                          {{"where", "DelaunaySiteIndex"},
                           {"reason", "duplicated_site_id"}});
            }
            index.vertices[slot] = vertex;
        }

        return index;
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
