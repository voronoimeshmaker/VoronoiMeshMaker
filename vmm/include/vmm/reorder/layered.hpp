// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/layered.hpp>
#include <vmm/reorder/reorder.hpp>
namespace vmm {
/// @brief Renumbers mesh and provenance together; horizons remain unchanged.
/// @param mesh Original layered mesh.
/// @param permutation Cell permutation with matching size.
/// @return Renumbered geometry and metadata, or an invalid-permutation error.
/// @par Level
/// Intermediate
/// @par Location
/// vmm/reorder/layered.hpp
/// @sa LayeredMesh, Permutation
[[nodiscard]] Result<LayeredMesh> renumber(const LayeredMesh& mesh,const Permutation& permutation);
} // namespace vmm
