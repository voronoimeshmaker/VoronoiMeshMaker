#pragma once
//==============================================================================
// Name        : Sites2DExport.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO / Sites2D
// Description : Public helpers to export Site2D data with Boundary2D context.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Sites2DExport.hpp
 * @brief Public VTK export helpers for 2D generator sites.
 *
 * A Site2D visualization includes both the generator points and the Boundary2D
 * used to validate them. Boundary-only export intentionally remains available
 * through `Boundary2DExport.hpp`.
 */

#include <filesystem>
#include <span>
#include <string_view>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/IO/PathUtils.hpp>
#include <VoronoiMeshMaker/IO/Sinks.hpp>
#include <VoronoiMeshMaker/IO/Sites2D/Writers/VTK_Legacy_SitesWithBoundary.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

/**
 * @brief Write Site2D points plus their Boundary2D domain to VTK Legacy.
 *
 * The output is one POLYDATA file: boundary loops are closed `LINES` and sites
 * are `VERTICES`. This is meant for visual inspection in VTK, ParaView or VisIt.
 *
 * @return Final path, including `.vtk` extension when the filename has none.
 */
[[nodiscard]] inline std::filesystem::path write_sites_vtk_legacy(
    std::span<const ::vmm::s2d::Site2D> sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    std::string_view folder,
    std::string_view filename,
    VtkOptions options = VtkOptions{})
{
    options.dialect = Dialect::Legacy;
    options.topology = Topology::PolyLines;
    options.encoding = Encoding::ASCII;

    const auto path = join_with_extension(folder, filename, options);
    FileSink sink(path.string());
    VTK_Legacy_SitesWithBoundaryWriter{}(sites, boundary, options, sink);
    return path;
}

/**
 * @brief Convenience overload for SiteSet.
 */
[[nodiscard]] inline std::filesystem::path write_sites_vtk_legacy(
    const ::vmm::s2d::SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    std::string_view folder,
    std::string_view filename,
    VtkOptions options = VtkOptions{})
{
    return write_sites_vtk_legacy(sites.span(), boundary, folder, filename, options);
}

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
