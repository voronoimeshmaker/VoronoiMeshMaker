// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  C++ standard library
//==============================================================================
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/layered.hpp>
namespace vmm {
/// Generates columns over an existing 2D mesh using a frozen horizon grid.
/// Parameters: fractions per horizon interval, including 0 and 1; empty means
/// one cell per interval. Region identity is (base region, horizon interval).
/// Notes: pinch-outs omit absent cells; all faces are planar. Functions sampled
/// by HorizonGrid::sample are approximated once, not during reconstruction.
/// Level: Intermediate. See Also: HorizonGrid, LayeredMesh.
/// Location: vmm/layered.hpp.
[[nodiscard]] Result<LayeredMesh> generate_layered_mesh(const Mesh2D& base,
    const HorizonGrid& horizons, std::vector<std::vector<Real>> fractions = {});
} // namespace vmm
