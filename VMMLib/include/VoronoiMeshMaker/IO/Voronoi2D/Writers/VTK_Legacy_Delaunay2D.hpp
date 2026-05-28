#pragma once
//==============================================================================
// Name        : VTK_Legacy_Delaunay2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO / Voronoi2D / Writers
// Description : Writer VTK Legacy (.vtk) for 2D Delaunay triangulations.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VTK_Legacy_Delaunay2D.hpp
 * @brief Writes Boundary2D, Site2D and Delaunay edges as VTK Legacy POLYDATA.
 */

#include <cstddef>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

#include <CGAL/number_utils.h>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/IO/Boundary2D/Topology/PolyLinesView.hpp>
#include <VoronoiMeshMaker/Core/namespace.h>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

/**
 * @brief Functor writer: VTK Legacy POLYDATA with boundary, sites and Delaunay.
 */
struct VTK_Legacy_Delaunay2DWriter {
    template <class Sink>
    void operator()(const ::vmm::s2d::SiteSet& sites,
                    const ::vmm::b2d::Boundary2DData& boundary,
                    const ::vmm::vd2d::DelaunayTriangulation2D& triangulation,
                    const VtkOptions& opt,
                    Sink& sink) const
    {
        using ::vmm::error::CoreErr;

        if (opt.encoding != Encoding::ASCII) {
            VMM_THROW(CoreErr::NotImplemented,
                      {{"where", "VTK_Legacy_Delaunay2D(non-ASCII)"}});
        }
        if (triangulation.dimension() != 2) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"where", "VTK_Legacy_Delaunay2D"},
                       {"reason", "triangulation_must_be_2d"}});
        }
        if (!boundary.invariant_ok()) {
            VMM_THROW(CoreErr::AssertFailed, {{"where", "b2d.invariant_ok"}});
        }
        if (!sites.ids_are_sequential()) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"where", "VTK_Legacy_Delaunay2D"},
                       {"reason", "site_ids_must_be_sequential"}});
        }

        std::size_t max_id = 0;
        bool has_vertices = false;
        for (auto vertex = triangulation.finite_vertices_begin();
             vertex != triangulation.finite_vertices_end();
             ++vertex) {
            if (vertex->info().value < 0) {
                VMM_THROW(CoreErr::InvalidArgument,
                          {{"where", "VTK_Legacy_Delaunay2D"},
                           {"reason", "negative_site_id"}});
            }
            max_id = std::max(max_id, static_cast<std::size_t>(vertex->info().value));
            has_vertices = true;
        }

        if (!has_vertices) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"where", "VTK_Legacy_Delaunay2D"},
                       {"reason", "empty_triangulation"}});
        }

        const std::size_t point_count = max_id + 1U;
        if (point_count != sites.size()) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"where", "VTK_Legacy_Delaunay2D"},
                       {"reason", "site_count_mismatch"}});
        }

        const PolyLinesView boundary_view{boundary};
        const std::size_t boundary_point_count = boundary.vertex_count();
        const std::size_t total_point_count = boundary_point_count + point_count;
        std::vector<::vmm::vd2d::CgalKernelTraits2D::CgalPoint2> points(point_count);
        std::vector<bool> present(point_count, false);

        for (auto vertex = triangulation.finite_vertices_begin();
             vertex != triangulation.finite_vertices_end();
             ++vertex) {
            const auto id = static_cast<std::size_t>(vertex->info().value);
            points[id] = vertex->point();
            present[id] = true;
        }

        for (std::size_t i = 0; i < point_count; ++i) {
            if (!present[i]) {
                VMM_THROW(CoreErr::InvalidArgument,
                          {{"where", "VTK_Legacy_Delaunay2D"},
                           {"reason", "site_ids_must_be_contiguous"}});
            }
        }

        std::vector<std::pair<std::size_t, std::size_t>> edges;
        for (auto edge = triangulation.finite_edges_begin();
             edge != triangulation.finite_edges_end();
             ++edge) {
            const auto face = edge->first;
            const int i = edge->second;
            const auto a = face->vertex((i + 1) % 3);
            const auto b = face->vertex((i + 2) % 3);
            if (triangulation.is_infinite(a) || triangulation.is_infinite(b)) {
                continue;
            }

            const auto ia = static_cast<std::size_t>(a->info().value);
            const auto ib = static_cast<std::size_t>(b->info().value);
            edges.emplace_back(ia, ib);
        }

        sink.write("# vtk DataFile Version 4.2\n");
        sink.write("VMM Boundary2D + Site2D + Delaunay2D\n");
        sink.write("ASCII\n");
        sink.write("DATASET POLYDATA\n");

        std::stringstream ss;
        ss << std::fixed << std::setprecision(opt.precision);

        ss << "POINTS " << total_point_count << " double\n";
        for (const auto& point : boundary.points) {
            ss << static_cast<double>(point.x) << " "
               << static_cast<double>(point.y) << " 0.0\n";
        }
        for (const auto& point : points) {
            ss << CGAL::to_double(point.x()) << " "
               << CGAL::to_double(point.y()) << " 0.0\n";
        }
        sink.write(ss.str());
        ss.str("");
        ss.clear();

        const std::size_t line_count = boundary_view.num_lines() + edges.size();
        const std::size_t line_connectivity_size =
            boundary_view.total_connectivity_size() + (edges.size() * 3U);

        ss << "LINES " << line_count << " " << line_connectivity_size << "\n";
        for (std::size_t r = 0; r < boundary_view.num_lines(); ++r) {
            const auto ring_view = boundary.ring(static_cast<::vmm::b2d::Index>(r));
            const auto n = ring_view.size();
            const auto base_index = boundary.ring_off[r];

            if (n < 3) {
                VMM_THROW(CoreErr::InvalidArgument, {{"ring", "<3_vertices"}});
            }

            ss << (n + 1);
            for (std::size_t i = 0; i < n; ++i) ss << " " << (base_index + i);
            ss << " " << base_index << "\n";
        }
        for (const auto& [a, b] : edges) {
            ss << "2 " << (boundary_point_count + a)
               << " " << (boundary_point_count + b) << "\n";
        }
        sink.write(ss.str());
        ss.str("");
        ss.clear();

        ss << "VERTICES " << point_count << " " << (point_count * 2U) << "\n";
        for (std::size_t i = 0; i < point_count; ++i) {
            ss << "1 " << (boundary_point_count + i) << "\n";
        }
        sink.write(ss.str());
        ss.str("");
        ss.clear();

        if (opt.cell_data && (line_count + point_count) > 0) {
            ss << "CELL_DATA " << (line_count + point_count) << "\n";

            ss << "SCALARS entity_kind int 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < boundary_view.num_lines(); ++i) ss << "0\n";
            for (std::size_t i = 0; i < edges.size(); ++i) ss << "2\n";
            for (std::size_t i = 0; i < point_count; ++i) ss << "1\n";

            ss << "SCALARS site_id int 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < boundary_view.num_lines(); ++i) ss << "-1\n";
            for (std::size_t i = 0; i < edges.size(); ++i) ss << "-1\n";
            for (const auto& site : sites) ss << site.id.value << "\n";

            ss << "SCALARS region_id int 1\nLOOKUP_TABLE default\n";
            for (const auto& region : boundary.regions) {
                ss << static_cast<int>(region.value) << "\n";
            }
            for (std::size_t i = 0; i < edges.size(); ++i) ss << "-1\n";
            for (const auto& site : sites) ss << site.region.value << "\n";

            ss << "SCALARS delaunay_edge int 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < boundary_view.num_lines(); ++i) ss << "0\n";
            for (std::size_t i = 0; i < edges.size(); ++i) ss << "1\n";
            for (std::size_t i = 0; i < point_count; ++i) ss << "0\n";
            sink.write(ss.str());
            ss.str("");
            ss.clear();
        }

        ss << "POINT_DATA " << total_point_count << "\n";

        ss << "SCALARS point_entity_kind int 1\nLOOKUP_TABLE default\n";
        for (std::size_t i = 0; i < boundary_point_count; ++i) ss << "0\n";
        for (std::size_t i = 0; i < point_count; ++i) ss << "1\n";

        ss << "SCALARS point_site_id int 1\nLOOKUP_TABLE default\n";
        for (std::size_t i = 0; i < boundary_point_count; ++i) ss << "-1\n";
        for (std::size_t i = 0; i < point_count; ++i) ss << i << "\n";

        ss << "SCALARS point_region_id int 1\nLOOKUP_TABLE default\n";
        for (std::size_t i = 0; i < boundary_point_count; ++i) ss << "-1\n";
        for (const auto& site : sites) ss << site.region.value << "\n";

        sink.write(ss.str());

        sink.flush();
    }
};

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
