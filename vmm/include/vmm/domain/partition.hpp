// ============================================================================
// File: partition.hpp
// Description: Explicit planar partition of the domain into regions
//              (DEC-018). Every boundary and interface segment is stored once
//              and shared by the regions on its two sides, so interfaces are
//              conforming by construction. Built by the backend.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/domain/declaration.hpp>
#include <vmm/geometry/polygon.hpp>

namespace vmm {

/// Segment v0 -> v1. `left`/`right` are the regions on each side (invalid =
/// outside the domain). Interface: both valid and different. Boundary: one
/// valid; it then carries a patch.
struct PartitionSegment {
    std::uint32_t v0 = 0;
    std::uint32_t v1 = 0;
    RegionId left = RegionId::invalid();
    RegionId right = RegionId::invalid();
    PatchId patch = PatchId::invalid();
};

/// A segment traversed forwards (v0 -> v1) or backwards.
struct SegmentUse {
    std::uint32_t segment = 0;
    bool reversed = false;
    friend bool operator==(const SegmentUse&, const SegmentUse&) = default;
};

/// One connected component of a region: loops[0] is the outer loop
/// (counter-clockwise), the others are holes (clockwise). The region lies on
/// the left of every traversed segment.
struct RegionComponent {
    std::vector<std::vector<SegmentUse>> loops;
};

class Partition2D {
public:
    Partition2D() = default;
    Partition2D(std::vector<Vec2> vertices, std::vector<PartitionSegment> segments, std::vector<RegionInfo> regions,
                std::vector<std::string> media, std::vector<std::string> patches,
                std::vector<std::vector<RegionComponent>> components, std::vector<PolygonWithHoles2> voids);

    [[nodiscard]] const std::vector<Vec2>& vertices() const noexcept { return vertices_; }
    [[nodiscard]] const std::vector<PartitionSegment>& segments() const noexcept { return segments_; }
    [[nodiscard]] const std::vector<RegionInfo>& regions() const noexcept { return regions_; }
    [[nodiscard]] const std::vector<std::string>& media() const noexcept { return media_; }
    [[nodiscard]] const std::vector<std::string>& patches() const noexcept { return patches_; }
    [[nodiscard]] const std::vector<RegionComponent>& components(RegionId r) const { return components_.at(r.index()); }
    /// Bounded parts of the hull covered by no layer (empty when a background region exists).
    [[nodiscard]] const std::vector<PolygonWithHoles2>& voids() const noexcept { return voids_; }

    [[nodiscard]] std::size_t region_count() const noexcept { return regions_.size(); }
    [[nodiscard]] bool is_interface(std::size_t s) const;
    [[nodiscard]] bool is_boundary(std::size_t s) const;

    [[nodiscard]] Vec2 start(const SegmentUse& u) const;
    [[nodiscard]] Vec2 end(const SegmentUse& u) const;
    [[nodiscard]] std::vector<Vec2> loop_points(const std::vector<SegmentUse>& loop) const;
    [[nodiscard]] PolygonWithHoles2 component_polygon(RegionId r, std::size_t c) const;
    [[nodiscard]] Real segment_length(std::size_t s) const;

    [[nodiscard]] Real region_area(RegionId r) const;
    [[nodiscard]] Real total_area() const;
    [[nodiscard]] Real boundary_length() const;
    [[nodiscard]] Real interface_length() const;
    /// Length of the interface between two regions (order irrelevant).
    [[nodiscard]] Real interface_length(RegionId a, RegionId b) const;
    [[nodiscard]] Box2 bounding_box() const;
    /// L = diagonal of the bounding box (R17).
    [[nodiscard]] Real length_scale() const { return bounding_box().diagonal(); }

private:
    std::vector<Vec2> vertices_;
    std::vector<PartitionSegment> segments_;
    std::vector<RegionInfo> regions_;
    std::vector<std::string> media_;
    std::vector<std::string> patches_;
    std::vector<std::vector<RegionComponent>> components_;
    std::vector<PolygonWithHoles2> voids_;
};

}  // namespace vmm
