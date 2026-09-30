// ============================================================================
// File: mesh_checks.hpp
// Description: Test helpers: build a mesh from a declaration and sources,
//              check the DEC-011 invariants and compute a canonical topology
//              signature.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/cgal.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace vmm::test {

/// Checks every DEC-011 invariant; returns the report for further checks.
inline InvariantReport expect_invariants(const Partition2D& p, const Build2D& b, const std::string& label) {
    auto ref = invariant_reference(p);
    ref.cell_measure = b.cell_polygon_area;
    const auto r = check_invariants(b.mesh, ref);
    EXPECT_TRUE(r.passed(ref)) << label << ": total " << r.total_relative_error << " region "
                               << r.max_region_relative_error << " cell " << r.max_cell_measure_error << " boundary "
                               << r.boundary_relative_error << " interface " << r.max_interface_relative_error
                               << " closure " << r.max_closure << "/" << r.closure_tolerance << " bad " << r.bad_faces
                               << " nonpos " << r.nonpositive_cells << " nonconf " << r.nonconforming_faces
                               << " sym " << r.adjacency_symmetric << " nonortho " << r.max_nonortho_internal << " "
                               << r.first_problem;
    return r;
}

using Signature = std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>>;

/// Faces keyed by the input site ids of their cells: independent of the input
/// order and of a positive scaling.
inline Signature signature(const Mesh2D& m, const std::vector<std::uint32_t>& input_to_canonical_key) {
    constexpr std::uint32_t none = std::numeric_limits<std::uint32_t>::max();
    Signature s;
    for (const FaceId f : m.faces()) {
        const std::uint32_t o = input_to_canonical_key[m.cell_input_sites()[m.owner(f).index()].index()];
        if (m.is_internal(f)) {
            const std::uint32_t n = input_to_canonical_key[m.cell_input_sites()[m.neighbour(f).index()].index()];
            s.emplace_back(std::min(o, n), std::max(o, n), none);
        } else {
            s.emplace_back(o, none, m.patch(f).value);
        }
    }
    std::ranges::sort(s);
    return s;
}

}  // namespace vmm::test
