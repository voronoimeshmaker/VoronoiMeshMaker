// ============================================================================
// File: backend_cgal.hpp
// Description: Private declarations of the CGAL backend (not installed).
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <optional>
#include <span>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/backend2d.hpp>
#include <vmm/backend/backend3d.hpp>

namespace vmm::cgal_detail {

[[nodiscard]] Result<Partition2D> build_partition(const Declaration2D& declaration);
[[nodiscard]] std::vector<CellPair> delaunay_pairs(std::span<const Vec2> sites);
[[nodiscard]] RegionClip2 clip_by_region(const LabelledLoop2& cell, std::span<const LabelledPolygon2> components);
[[nodiscard]] BackendInfo info();

[[nodiscard]] Result<Partition3D> build_partition_3d(const Declaration3D& declaration);
[[nodiscard]] Result<PreparedDomain3> prepare_3d(const Partition3D& partition, RegionId region);
[[nodiscard]] std::vector<CellPair> delaunay_pairs_3d(std::span<const Vec3> sites);
[[nodiscard]] std::optional<Vec3> circumcentre_3d(const std::array<Vec3, 4>& sites);
[[nodiscard]] bool touches_boundary_3d(const PreparedDomain3& domain, const Box3& box);
[[nodiscard]] CellClip3 clip_cell_3d(const LabelledPolyhedron3& cell, const PreparedDomain3& domain);

}  // namespace vmm::cgal_detail
