// ============================================================================
// File: invariants.hpp
// Description: DEC-011 invariants for any dimension: measures, closure,
//              owner/neighbour consistency, boundary and interface measures,
//              conformity, non-orthogonality and symmetric adjacency.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <map>
#include <span>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/mesh/mesh.hpp>

namespace vmm {

/// Expected values. Tolerances are relative (R17): measures to the reference,
/// closure to L^(D-1).
struct InvariantReference {
    Real length_scale = 1;
    Real total_measure = 0;
    std::vector<Real> region_measure;
    Real boundary_measure = -1;  ///< < 0 skips
    /// Interface measure per region pair (first < second); empty map skips.
    std::map<std::pair<std::size_t, std::size_t>, Real> interface_measure;
    std::span<const Real> cell_measure;  ///< optional independent measure per cell
    Real relative_tolerance = 1e-12;
};

struct InvariantReport {
    Real total_relative_error = 0;
    Real max_region_relative_error = 0;
    Real max_cell_measure_error = 0;  ///< relative to the total measure
    Real boundary_relative_error = 0;
    Real max_interface_relative_error = 0;
    Real max_closure = 0;
    Real closure_tolerance = 0;
    std::size_t bad_faces = 0;        ///< ids, orientation, ordering
    std::size_t nonpositive_cells = 0;
    std::size_t interface_faces = 0;
    std::size_t nonconforming_faces = 0;  ///< interface faces between cells of one region (none by construction)
    bool adjacency_symmetric = false;
    /// Measured non-orthogonality, reported only: faces inside a region are
    /// orthogonal by construction (DEC-032); the value reflects vertex rounding.
    Real max_nonortho_internal = 0;
    Real max_nonortho_interface = 0;  ///< reported only (interfaces are not Voronoi faces)
    std::string first_problem;

    [[nodiscard]] bool passed(const InvariantReference& ref) const noexcept {
        const Real t = ref.relative_tolerance;
        return total_relative_error <= t && max_region_relative_error <= t && max_cell_measure_error <= t &&
               boundary_relative_error <= t && max_interface_relative_error <= t && max_closure <= closure_tolerance &&
               bad_faces == 0 && nonpositive_cells == 0 && nonconforming_faces == 0 && adjacency_symmetric;
    }
};

template <std::size_t D>
[[nodiscard]] InvariantReport check_invariants(const Mesh<D>& mesh, const InvariantReference& reference);

extern template InvariantReport check_invariants(const Mesh<2>&, const InvariantReference&);
extern template InvariantReport check_invariants(const Mesh<3>&, const InvariantReference&);

}  // namespace vmm
