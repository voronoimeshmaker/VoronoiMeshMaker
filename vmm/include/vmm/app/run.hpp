// ============================================================================
// File: run.hpp
// Description: From a configuration file to a mesh on disk (DEC-040): the
//              facade requests built from a MeshConfig, and run_config,
//              which the vmm-mesh executable calls.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <filesystem>
#include <optional>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/config.hpp>
#include <vmm/app/registries.hpp>
#include <vmm/cvt.hpp>
#include <vmm/error/error.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/vmm.hpp>

namespace vmm {

/// @brief Builds the 2D facade request of a configuration.
/// @param config A configuration with dimension 2.
/// @param registries Shapes and site sources by name.
/// @return The request, or the first error, whose context names the line of the configuration.
/// @par Level
/// Intermediate
/// @sa generate_mesh_2d, run_config
/// @par Location
/// vmm/app/run.hpp
[[nodiscard]] Result<MeshRequest2D> make_request_2d(const MeshConfig& config,
                                                    const ConfigRegistries& registries = ConfigRegistries::with_builtins());

/// @brief Builds the 3D facade request of a configuration (dimension 3).
/// @sa generate_mesh_3d, make_request_2d
/// @par Location
/// vmm/app/run.hpp
[[nodiscard]] Result<MeshRequest3D> make_request_3d(const MeshConfig& config,
                                                    const ConfigRegistries& registries = ConfigRegistries::with_builtins());

/// What run_config did.
struct ConfigRunReport {
    int dimension = 0;
    std::size_t cells = 0;
    std::size_t internal_faces = 0;
    std::size_t boundary_faces = 0;
    InvariantReport invariants;
    std::vector<std::filesystem::path> written;
    std::optional<CvtReport> cvt;
};

/// @brief Generates the mesh of a configuration and writes it in the requested formats.
/// @param config The configuration (MeshConfig::read).
/// @param registries Shapes and site sources by name.
/// @return Counts, invariants and the files written, or the first error (unknown format, configuration,
///         meshing or writing).
/// @par Level
/// Beginner
/// @sa MeshConfig, generate_mesh_2d, generate_mesh_3d
/// @par Location
/// vmm/app/run.hpp
[[nodiscard]] Result<ConfigRunReport> run_config(const MeshConfig& config,
                                                 const ConfigRegistries& registries = ConfigRegistries::with_builtins());

}  // namespace vmm
