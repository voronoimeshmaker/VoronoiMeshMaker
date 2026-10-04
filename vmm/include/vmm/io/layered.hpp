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
/// Reads VMM_HORIZONS 1 followed by axes, elevations and interval fractions.
[[nodiscard]] Result<HorizonSpecification> read_horizons(std::istream& in);
[[nodiscard]] Result<HorizonSpecification> read_horizons(const std::filesystem::path& path);

/// Self-contained .vlayers version 1: frozen horizons, provenance and embedded
/// native mesh. The existing .vmesh version 1 is unchanged.
[[nodiscard]] Status write_layered(const LayeredMesh& mesh,std::ostream& out);
[[nodiscard]] Status write_layered(const LayeredMesh& mesh,const std::filesystem::path& path);
[[nodiscard]] Result<LayeredMesh> read_layered(std::istream& in);
[[nodiscard]] Result<LayeredMesh> read_layered(const std::filesystem::path& path);
} // namespace vmm
