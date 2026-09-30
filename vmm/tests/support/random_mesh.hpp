// ============================================================================
// File: random_mesh.hpp
// Description: Test helper: a random single-region mesh of the unit square.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/cgal.hpp>
#include <vmm/sites/sources.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace vmm::test {

inline Build2D random_square_mesh(std::size_t count, std::uint64_t seed) {
    const auto backend = cgal_backend_2d();
    Declaration2D d;
    (void)d.add_region("box", *d.media().add("m"), Rectangle(Vec2{0, 0}, Vec2{1, 1}, {"s", "e", "n", "w"}));
    const auto p = *backend.build_partition(d);
    std::vector<RegionSites> src{sites_for(RegionId{0}, RandomCountSource(count, 1e-3))};
    SiteGenerationOptions o;
    o.seed = seed;
    return *build_mesh_2d(p, *generate_sites(p, src, o), backend);
}

}  // namespace vmm::test
