// ============================================================================
// File: partition3d.hpp
// Description: Explicit 3D partition (P15 §3): triangles with the region on
//              each side, a patch on the boundary triangles; triangles
//              between two regions are interfaces (P18).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <string>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/domain/declaration.hpp>
#include <vmm/geometry/surface.hpp>

namespace vmm {

/// Triangle v[0], v[1], v[2], oriented outward from `inside`. `outside` is the
/// region on the other side (invalid = outside the domain: boundary triangle,
/// which then carries a patch).
struct PartitionTriangle {
    Triangle v{};
    RegionId inside = RegionId::invalid();
    RegionId outside = RegionId::invalid();
    PatchId patch = PatchId::invalid();
};

class Partition3D {
public:
    Partition3D() = default;
    Partition3D(std::vector<Vec3> vertices, std::vector<PartitionTriangle> triangles, std::vector<RegionInfo> regions,
                std::vector<std::string> media, std::vector<std::string> patches);

    [[nodiscard]] const std::vector<Vec3>& vertices() const noexcept { return vertices_; }
    [[nodiscard]] const std::vector<PartitionTriangle>& triangles() const noexcept { return triangles_; }
    [[nodiscard]] const std::vector<RegionInfo>& regions() const noexcept { return regions_; }
    [[nodiscard]] const std::vector<std::string>& media() const noexcept { return media_; }
    [[nodiscard]] const std::vector<std::string>& patches() const noexcept { return patches_; }
    [[nodiscard]] std::size_t region_count() const noexcept { return regions_.size(); }

    /// Closed surface of a region, oriented outward from it (patches: the partition's patches plus
    /// "interface" for the triangles shared with another region).
    [[nodiscard]] Result<TriangleSurface> region_surface(RegionId r) const;
    [[nodiscard]] Real region_volume(RegionId r) const;
    [[nodiscard]] Real total_volume() const;
    [[nodiscard]] Real boundary_area() const;
    /// Area of the interface between two regions (order irrelevant).
    [[nodiscard]] Real interface_area(RegionId a, RegionId b) const;
    [[nodiscard]] Box3 bounding_box() const noexcept { return Box3::of(vertices_); }
    /// L = diagonal of the bounding box (R17).
    [[nodiscard]] Real length_scale() const noexcept { return bounding_box().diagonal(); }

private:
    std::vector<Vec3> vertices_;
    std::vector<PartitionTriangle> triangles_;
    std::vector<RegionInfo> regions_;
    std::vector<std::string> media_;
    std::vector<std::string> patches_;
};

}  // namespace vmm
