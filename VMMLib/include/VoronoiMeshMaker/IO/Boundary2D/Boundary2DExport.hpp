#pragma once
//==============================================================================
// Name        : Boundary2DExport.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO / Boundary2D
// Description : Public helpers to export Boundary2D data/shapes to VTK files.
// License     : GNU GPL v3
// Version     : 0.1.0
//==============================================================================

#include <filesystem>
#include <string_view>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Boundary2D/Policies/PolygonizePolicy.hpp>
#include <VoronoiMeshMaker/IO/Boundary2D/Writers/VTK_Legacy_PolyLines.hpp>
#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/IO/PathUtils.hpp>
#include <VoronoiMeshMaker/IO/Sinks.hpp>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

/**
 * @brief Write canonical Boundary2D data to a VTK Legacy POLYDATA file.
 *
 * The writer emits 2D points as `x y 0.0` and closed loops as VTK Legacy
 * `LINES`, which is accepted by VTK, ParaView and VisIt. This function is the
 * preferred path after a boundary has been transformed or composed.
 *
 * @return Final path, including `.vtk` extension when the filename has none.
 */
[[nodiscard]] inline std::filesystem::path write_boundary_vtk_legacy(
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
    VTK_Legacy_PolyLinesWriter{}(boundary, options, sink);
    return path;
}

/**
 * @brief Build a Boundary2DData from a shape and write it to VTK Legacy.
 *
 * This is a convenience overload for direct visualization. For workflows that
 * apply transforms, build the boundary first, transform it, and then call
 * `write_boundary_vtk_legacy`.
 */
template <class Shape>
[[nodiscard]] inline std::filesystem::path write_shape_vtk_legacy(
    const Shape& shape,
    std::string_view folder,
    std::string_view filename,
    VtkOptions options = VtkOptions{},
    const ::vmm::b2d::PolygonizePolicy& policy = ::vmm::b2d::PolygonizePolicy{},
    ::vmm::b2d::RegionId region = ::vmm::b2d::RegionId{0})
{
    const auto boundary = ::vmm::b2d::make_boundary(shape, policy, region);
    return write_boundary_vtk_legacy(boundary, folder, filename, options);
}

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
