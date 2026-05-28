#pragma once
//==============================================================================
// Name        : Delaunay2DExport.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO / Voronoi2D
// Description : Public helpers to export 2D Delaunay debug views.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Delaunay2DExport.hpp
 * @brief Public VTK export helpers for Boundary2D + Site2D + Delaunay2D.
 */

#include <filesystem>
#include <string_view>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/IO/PathUtils.hpp>
#include <VoronoiMeshMaker/IO/Sinks.hpp>
#include <VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_Legacy_Delaunay2D.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

/**
 * @brief Write Boundary2D, Site2D and Delaunay edges to VTK Legacy.
 *
 * The output is one POLYDATA file with closed boundary `LINES`, Delaunay edge
 * `LINES`, and site `VERTICES`. The `entity_kind` field uses:
 *  - 0: boundary line
 *  - 1: site vertex
 *  - 2: Delaunay edge
 *
 * @return Final path, including `.vtk` extension when the filename has none.
 */
[[nodiscard]] inline std::filesystem::path write_delaunay_vtk_legacy(
    const ::vmm::s2d::SiteSet& sites,
    const ::vmm::b2d::Boundary2DData& boundary,
    const ::vmm::vd2d::DelaunayTriangulation2D& triangulation,
    std::string_view folder,
    std::string_view filename,
    VtkOptions options = VtkOptions{})
{
    options.dialect = Dialect::Legacy;
    options.topology = Topology::PolyLines;
    options.encoding = Encoding::ASCII;

    const auto path = join_with_extension(folder, filename, options);
    FileSink sink(path.string());
    VTK_Legacy_Delaunay2DWriter{}(
        sites,
        boundary,
        triangulation,
        options,
        sink);
    return path;
}

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
