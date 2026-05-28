#pragma once
//==============================================================================
// Name        : VTK_Legacy_SitesWithBoundary.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO / Sites2D / Writers
// Description : Writer VTK Legacy (.vtk) for Site2D points plus Boundary2D.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VTK_Legacy_SitesWithBoundary.hpp
 * @brief Writes a Boundary2D together with its 2D generator sites.
 *
 * Boundary-only visualization remains in Boundary2DExport. This writer is the
 * Site2D visualization path: it emits the boundary loops as closed VTK `LINES`
 * and the generator sites as VTK `VERTICES` in the same POLYDATA file.
 * Site metadata is written as POINT_DATA so ParaView can color generator points
 * directly without confusing them with boundary line cells.
 */

#include <iomanip>
#include <sstream>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Core/namespace.h>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/IO/Boundary2D/Topology/PolyLinesView.hpp>
#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

/**
 * @brief Functor writer: VTK Legacy POLYDATA with boundary lines and site points.
 */
struct VTK_Legacy_SitesWithBoundaryWriter {
    template <class Sink>
    void operator()(const ::vmm::s2d::SiteSet& sites,
                    const ::vmm::b2d::Boundary2DData& boundary,
                    const VtkOptions& opt,
                    Sink& sink) const
    {
        (*this)(sites.span(), boundary, opt, sink);
    }

    template <class Sink>
    void operator()(std::span<const ::vmm::s2d::Site2D> sites,
                    const ::vmm::b2d::Boundary2DData& boundary,
                    const VtkOptions& opt,
                    Sink& sink) const
    {
        using ::vmm::error::CoreErr;

        if (!boundary.invariant_ok()) {
            VMM_THROW(CoreErr::AssertFailed, {{"where", "b2d.invariant_ok"}});
        }
        if (opt.encoding != Encoding::ASCII) {
            VMM_THROW(CoreErr::NotImplemented,
                      {{"where", "VTK_Legacy_SitesWithBoundary(non-ASCII)"}});
        }

        const PolyLinesView view{boundary};
        const std::size_t boundary_points = boundary.vertex_count();
        const std::size_t site_points = sites.size();
        const std::size_t total_points = boundary_points + site_points;

        sink.write("# vtk DataFile Version 4.2\n");
        sink.write("VMM Site2D with Boundary2D\n");
        sink.write("ASCII\n");
        sink.write("DATASET POLYDATA\n");

        std::stringstream ss;
        ss << std::fixed << std::setprecision(opt.precision);

        ss << "POINTS " << total_points << " double\n";
        for (const auto& p : boundary.points) {
            ss << static_cast<double>(p.x) << " "
               << static_cast<double>(p.y) << " 0.0\n";
        }
        for (const auto& site : sites) {
            ss << static_cast<double>(site.point.x) << " "
               << static_cast<double>(site.point.y) << " 0.0\n";
        }
        sink.write(ss.str());
        ss.str("");
        ss.clear();

        ss << "LINES " << view.num_lines() << " "
           << view.total_connectivity_size() << "\n";
        for (std::size_t r = 0; r < view.num_lines(); ++r) {
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
        sink.write(ss.str());
        ss.str("");
        ss.clear();

        ss << "VERTICES " << site_points << " " << (site_points * 2) << "\n";
        for (std::size_t i = 0; i < site_points; ++i) {
            ss << "1 " << (boundary_points + i) << "\n";
        }
        sink.write(ss.str());
        ss.str("");
        ss.clear();

        if (opt.cell_data && (view.num_lines() + site_points) > 0) {
            ss << "CELL_DATA " << (view.num_lines() + site_points) << "\n";

            ss << "SCALARS entity_kind int 1\nLOOKUP_TABLE default\n";
            for (std::size_t r = 0; r < view.num_lines(); ++r) ss << "0\n";
            for (std::size_t i = 0; i < site_points; ++i) ss << "1\n";

            ss << "SCALARS site_id int 1\nLOOKUP_TABLE default\n";
            for (std::size_t r = 0; r < view.num_lines(); ++r) ss << "-1\n";
            for (const auto& site : sites) {
                ss << static_cast<int>(site.id.value) << "\n";
            }

            ss << "SCALARS region_id int 1\nLOOKUP_TABLE default\n";
            for (const auto& region : boundary.regions) {
                ss << static_cast<int>(region.value) << "\n";
            }
            for (const auto& site : sites) {
                ss << static_cast<int>(site.region.value) << "\n";
            }

            ss << "SCALARS site_weight double 1\nLOOKUP_TABLE default\n";
            for (std::size_t r = 0; r < view.num_lines(); ++r) ss << "0.0\n";
            for (const auto& site : sites) {
                ss << static_cast<double>(site.weight) << "\n";
            }
            sink.write(ss.str());
            ss.str("");
            ss.clear();
        }

        if (total_points > 0) {
            ss << "POINT_DATA " << total_points << "\n";

            ss << "SCALARS point_entity_kind int 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < boundary_points; ++i) ss << "0\n";
            for (std::size_t i = 0; i < site_points; ++i) ss << "1\n";

            ss << "SCALARS point_site_id int 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < boundary_points; ++i) ss << "-1\n";
            for (const auto& site : sites) {
                ss << static_cast<int>(site.id.value) << "\n";
            }

            ss << "SCALARS point_region_id int 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < boundary_points; ++i) ss << "-1\n";
            for (const auto& site : sites) {
                ss << static_cast<int>(site.region.value) << "\n";
            }

            ss << "SCALARS point_site_weight double 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < boundary_points; ++i) ss << "0.0\n";
            for (const auto& site : sites) {
                ss << static_cast<double>(site.weight) << "\n";
            }
            sink.write(ss.str());
        }

        sink.flush();
    }
};

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
