// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/layered.hpp>
#include <vmm/reorder/reorder.hpp>
namespace vmm {
/// Renumbers mesh and provenance together; the frozen horizons are unchanged.
[[nodiscard]] Result<LayeredMesh> renumber(const LayeredMesh& mesh,const Permutation& permutation);
} // namespace vmm
