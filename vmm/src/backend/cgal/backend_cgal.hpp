// ============================================================================
// File: backend_cgal.hpp
// Description: Private declarations of the CGAL backend (not installed).
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <span>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/backend2d.hpp>

namespace vmm::cgal_detail {

[[nodiscard]] Result<Partition2D> build_partition(const Declaration2D& declaration);
[[nodiscard]] std::vector<CellPair> delaunay_pairs(std::span<const Vec2> sites);
[[nodiscard]] RegionClip2 clip_by_region(const LabelledLoop2& cell, std::span<const LabelledPolygon2> components);
[[nodiscard]] BackendInfo info();

}  // namespace vmm::cgal_detail
