#pragma once
//==============================================================================
// Name        : ClippedVoronoi2DExport.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO / Voronoi2D
// Description : Public helpers to export clipped 2D Voronoi diagrams.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file ClippedVoronoi2DExport.hpp
 * @brief Public VTK export helper for complete 2D Voronoi diagrams.
 */

#include <filesystem>
#include <span>
#include <string_view>

#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/IO/PathUtils.hpp>
#include <VoronoiMeshMaker/IO/Sinks.hpp>
#include <VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_Legacy_ClippedVoronoi2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

/**
 * @brief Write Boundary2D, Site2D, Delaunay edges and Voronoi cells to VTK.
 *
 * The output is one POLYDATA file with:
 *  - `entity_kind = 0`: boundary line
 *  - `entity_kind = 1`: site vertex
 *  - `entity_kind = 2`: Delaunay edge
 *  - `entity_kind = 3`: Voronoi volume polygon
 */
[[nodiscard]] inline std::filesystem::path write_clipped_voronoi_vtk_legacy(
    const ::vmm::vd2d::ClippedVoronoiDiagram2D& diagram,
    std::string_view folder,
    std::string_view filename,
    VtkOptions options = VtkOptions{})
{
    options.dialect = Dialect::Legacy;
    options.topology = Topology::Polys;
    options.encoding = Encoding::ASCII;

    const auto path = join_with_extension(folder, filename, options);
    FileSink sink(path.string());
    VTK_Legacy_ClippedVoronoi2DWriter{}(diagram, options, sink);
    return path;
}

[[nodiscard]] inline std::filesystem::path write_clipped_voronoi_vtk_legacy(
    const ::vmm::vd2d::ClippedVoronoiDiagram2D& diagram,
    std::span<const VtkVolumeScalarField> scalar_fields,
    std::span<const VtkVolumeVectorField> vector_fields,
    std::string_view folder,
    std::string_view filename,
    VtkOptions options = VtkOptions{})
{
    options.dialect = Dialect::Legacy;
    options.topology = Topology::Polys;
    options.encoding = Encoding::ASCII;

    const auto path = join_with_extension(folder, filename, options);
    FileSink sink(path.string());
    VTK_Legacy_ClippedVoronoi2DWriter{}(
        diagram,
        scalar_fields,
        vector_fields,
        options,
        sink);
    return path;
}

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
