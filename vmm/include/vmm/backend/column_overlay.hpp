// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <cstddef>
#include <span>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/error/error.hpp>
#include <vmm/geometry/horizons.hpp>
namespace vmm {
/// Internal backend transport; oriented edges keep their base-cell incidence.
struct ColumnEdge {
    Vec2 a, b;
    CellId owner, neighbour;
    PatchId patch;
};
struct ColumnTriangle {
    std::array<std::size_t,3> vertices;
    CellId column;
    std::array<PatchId,3> patches;
};
struct ColumnOverlay {
    std::vector<Vec2> points;
    std::vector<std::vector<Real>> elevations;
    std::vector<ColumnTriangle> triangles;
};
/// Exact common triangulation of footprint edges and the fixed horizon grid.
/// Internal API. No geometry outside the footprint is returned.
[[nodiscard]] Result<ColumnOverlay> cgal_column_overlay(std::span<const ColumnEdge> edges,
    std::size_t columns, const HorizonGrid& grid);
} // namespace vmm
