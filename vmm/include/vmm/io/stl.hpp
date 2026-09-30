// ============================================================================
// File: stl.hpp
// Description: STL reader and writer (P17). ASCII files give one patch per
//              "solid" block (named after the solid); binary files have one
//              patch. The reader returns a triangle soup: repair_surface
//              turns it into a checked TriangleSurface.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <filesystem>
#include <istream>
#include <ostream>
#include <string>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/error.hpp>
#include <vmm/geometry/repair.hpp>
#include <vmm/geometry/surface.hpp>

namespace vmm {

struct StlReadOptions {
    std::string patch_name = "stl";  ///< patch of a binary file and of unnamed ASCII solids
};

struct StlWriteOptions {
    bool binary = false;  ///< binary files keep no patch names
};

/// @brief Reads an ASCII or binary STL file (detected from the content).
/// @param in Source stream (opened in binary mode for binary files).
/// @param options Patch name for binary files and unnamed solids.
/// @return The triangles, not welded: three points per triangle; ParseError on malformed input.
/// @note Facet normals are ignored: the orientation comes from the vertex order and is made
///       consistent by repair_surface.
/// @par Level
/// Beginner
/// @sa repair_surface, read_stl_surface, write_stl
/// @par Location
/// vmm/io/stl.hpp
[[nodiscard]] Result<TriangleSoup> read_stl(std::istream& in, const StlReadOptions& options = {});
[[nodiscard]] Result<TriangleSoup> read_stl(const std::filesystem::path& path, const StlReadOptions& options = {});

/// @brief Reads an STL file and repairs it into a closed surface, ready for Declaration3D.
/// @param path STL file.
/// @param options Patch name for binary files and unnamed solids.
/// @param repair Welding tolerance of repair_surface.
/// @param report Optional repair counters.
/// @return The surface, or the first error of the reading or of the repair.
/// @par Level
/// Beginner
/// @sa read_stl, repair_surface, SurfaceShape
/// @par Location
/// vmm/io/stl.hpp
/// @par Examples
/// ex_stl_domain.cpp
[[nodiscard]] Result<TriangleSurface> read_stl_surface(const std::filesystem::path& path,
                                                       const StlReadOptions& options = {},
                                                       const SurfaceRepairOptions& repair = {},
                                                       SurfaceRepairReport* report = nullptr);

/// Writes a surface as STL: ASCII with one solid per patch, or binary.
[[nodiscard]] Status write_stl(const TriangleSurface& surface, std::ostream& out, const StlWriteOptions& options = {});
[[nodiscard]] Status write_stl(const TriangleSurface& surface, const std::filesystem::path& path,
                               const StlWriteOptions& options = {});

}  // namespace vmm
