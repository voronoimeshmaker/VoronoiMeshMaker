// ============================================================================
// File: facade.cpp
// Description: generate_mesh_2d().
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <format>
#include <string>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/cgal.hpp>
#include <vmm/vmm.hpp>

namespace vmm {

Result<MeshResult2D> generate_mesh_2d(const MeshRequest2D& request) {
    const Backend2D backend = cgal_backend_2d();
    auto partition = backend.build_partition(request.declaration);
    if (!partition) return std::unexpected(partition.error());
    ValidationReport validation = validate_partition(*partition, request.validation);
    if (!validation.ok()) return std::unexpected(validation.errors().front());
    auto sites = generate_sites(*partition, request.sources, request.sites);
    if (!sites) return std::unexpected(sites.error());
    auto build = build_mesh_2d(*partition, *sites, backend, request.build);
    if (!build) return std::unexpected(build.error());
    if (request.reject_fragmented && !build->fragmented_cells.empty()) {
        std::string cells;
        for (const CellId cell : build->fragmented_cells) cells += std::format("{}{}", cells.empty() ? "" : ", ", cell.index());
        return fail(ErrorCode::FragmentedCells, "cells: " + cells);
    }
    auto reference = invariant_reference(*partition);
    reference.cell_measure = build->cell_polygon_area;
    InvariantReport invariants = check_invariants(build->mesh, reference);
    if (!invariants.passed(reference)) return fail(ErrorCode::InvariantViolated, invariants.first_problem);
    return MeshResult2D{std::move(build->mesh), std::move(*partition), build->stats, std::move(validation), invariants,
                        std::move(build->fragmented_cells)};
}

Result<MeshResult3D> generate_mesh_3d(const MeshRequest3D& request) {
    const Backend3D backend = cgal_backend_3d();
    auto partition = backend.build_partition(request.declaration);
    if (!partition) return std::unexpected(partition.error());
    auto sites = generate_sites_3d(*partition, request.sources, request.sites);
    if (!sites) return std::unexpected(sites.error());
    auto build = build_mesh_3d(*partition, *sites, backend, request.build);
    if (!build) return std::unexpected(build.error());
    if (request.reject_fragmented && !build->fragmented_cells.empty()) {
        std::string cells;
        for (const CellId cell : build->fragmented_cells) cells += std::format("{}{}", cells.empty() ? "" : ", ", cell.index());
        return fail(ErrorCode::FragmentedCells, "cells: " + cells);
    }
    auto reference = invariant_reference(*partition);
    reference.cell_measure = build->cell_volume;
    InvariantReport invariants = check_invariants(build->mesh, reference);
    if (!invariants.passed(reference)) return fail(ErrorCode::InvariantViolated, invariants.first_problem);
    return MeshResult3D{std::move(build->mesh), std::move(*partition), build->stats, invariants, std::move(build->fragmented_cells)};
}

}  // namespace vmm
