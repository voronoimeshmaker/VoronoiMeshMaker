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

[[nodiscard]] Status write_vtu(const Mesh2D& mesh, std::ostream& out, const VtuOptions& options = {});
[[nodiscard]] Status write_vtu(const Mesh2D& mesh, const std::filesystem::path& path, const VtuOptions& options = {});

}  // namespace vmm
