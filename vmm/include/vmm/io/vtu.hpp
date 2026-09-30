// ============================================================================
// File: vtu.hpp
// Description: VTK XML unstructured grid (.vtu) writer for 2D meshes, for
//              visualisation (DEC-019); persistence uses the native format.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <filesystem>
#include <iosfwd>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/error/error.hpp>
#include <vmm/mesh/mesh.hpp>

namespace vmm {

struct VtuOptions {
    bool include_metrics = true;  ///< area, aspect ratio, max non-orthogonality per cell
};

/// Counter-clockwise vertex loop of every cell, rebuilt from its faces. A
/// cell with several loops (hole or several pieces) returns the loop of
/// largest area first.
[[nodiscard]] Result<std::vector<std::vector<std::vector<VertexId>>>> cell_loops(const Mesh2D& mesh);

/// @brief Writes a 2D mesh as VTK XML for visualisation.
/// @param mesh Mesh.
/// @param out Destination stream.
/// @param options Include the per-cell metrics (area, aspect ratio, non-orthogonality).
/// @par Level
/// Beginner
/// @sa write_native, cell_loops
/// @par Location
/// vmm/io/vtu.hpp
/// @par Examples
/// ex_quickstart.cpp, ex_anchor_a1.cpp, ex_anchor_a2.cpp
[[nodiscard]] Status write_vtu(const Mesh2D& mesh, std::ostream& out, const VtuOptions& options = {});
[[nodiscard]] Status write_vtu(const Mesh2D& mesh, const std::filesystem::path& path, const VtuOptions& options = {});

/// @brief Writes a 3D mesh as VTK XML (VTK_POLYHEDRON cells) for visualisation.
/// @param mesh Mesh.
/// @param out Destination stream.
/// @param options Include the per-cell metrics (volume, aspect ratio, non-orthogonality).
/// @note Each cell lists its faces with outward normals (the neighbour's copy of a face is reversed).
/// @par Level
/// Beginner
/// @sa write_native, generate_mesh_3d
/// @par Location
/// vmm/io/vtu.hpp
/// @par Examples
/// ex_voronoi3d.cpp
[[nodiscard]] Status write_vtu(const Mesh3D& mesh, std::ostream& out, const VtuOptions& options = {});
[[nodiscard]] Status write_vtu(const Mesh3D& mesh, const std::filesystem::path& path, const VtuOptions& options = {});

}  // namespace vmm
