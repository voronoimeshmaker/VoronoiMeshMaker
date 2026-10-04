// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  C++ standard library
//==============================================================================
#include <filesystem>
#include <iosfwd>
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/layered.hpp>
namespace vmm {
using HorizonSpecification = std::pair<HorizonGrid, std::vector<std::vector<Real>>>;
/// @brief Reads VMM_HORIZONS 1 axes, elevations and interval fractions.
/// @param in Input stream; heights use x-fast order, z positive upwards.
/// @return Frozen grid and fractions, or a parse/geometry error.
/// @par Level
/// Intermediate
/// @par Location
/// vmm/io/layered.hpp
/// @par Examples
/// horizons_flat.cfg (through vmm-mesh)
/// @sa HorizonGrid, generate_layered_mesh
[[nodiscard]] Result<HorizonSpecification> read_horizons(std::istream& in);
/// @brief Opens a horizon specification file and reads it as above.
[[nodiscard]] Result<HorizonSpecification> read_horizons(const std::filesystem::path& path);

/// @brief Writes self-contained .vlayers version 1.
/// @param mesh Mesh with frozen horizons and provenance.
/// @param out Destination stream; the embedded native mesh remains version 1.
/// @return Success or an output error.
/// @par Level
/// Intermediate
/// @par Location
/// vmm/io/layered.hpp
/// @par Examples
/// ex_horizons.cpp
/// @sa read_layered
[[nodiscard]] Status write_layered(const LayeredMesh& mesh,std::ostream& out);
/// @brief Opens the destination file and writes the layered representation.
[[nodiscard]] Status write_layered(const LayeredMesh& mesh,const std::filesystem::path& path);
/// @brief Reads geometry and metadata, validating them through LayeredMesh::from_data.
/// @param in Source stream.
/// @return Validated layered mesh or a parse/geometry error.
/// @par Level
/// Intermediate
/// @par Location
/// vmm/io/layered.hpp
/// @sa write_layered, LayeredMesh::from_data
[[nodiscard]] Result<LayeredMesh> read_layered(std::istream& in);
/// @brief Opens a layered mesh file and reads it as above.
[[nodiscard]] Result<LayeredMesh> read_layered(const std::filesystem::path& path);
} // namespace vmm
