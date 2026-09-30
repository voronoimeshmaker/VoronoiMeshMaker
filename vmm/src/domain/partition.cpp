// ============================================================================
// File: partition.cpp
// Description: Partition2D queries.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/partition.hpp>

namespace vmm {

Partition2D::Partition2D(std::vector<Vec2> vertices, std::vector<PartitionSegment> segments,
                         std::vector<RegionInfo> regions, std::vector<std::string> media,
                         std::vector<std::string> patches, std::vector<std::vector<RegionComponent>> components,
                         std::vector<PolygonWithHoles2> voids)
    : vertices_(std::move(vertices)),
      segments_(std::move(segments)),
      regions_(std::move(regions)),
      media_(std::move(media)),
      patches_(std::move(patches)),
      components_(std::move(components)),
      voids_(std::move(voids)) {
    components_.resize(regions_.size());
}

bool Partition2D::is_interface(std::size_t s) const {
    const auto& seg = segments_.at(s);
    return seg.left.valid() && seg.right.valid() && seg.left != seg.right;
}

bool Partition2D::is_boundary(std::size_t s) const {
    const auto& seg = segments_.at(s);
    return seg.left.valid() != seg.right.valid();
}

Vec2 Partition2D::start(const SegmentUse& u) const {
    const auto& s = segments_.at(u.segment);
    return vertices_.at(u.reversed ? s.v1 : s.v0);
}

Vec2 Partition2D::end(const SegmentUse& u) const {
    const auto& s = segments_.at(u.segment);
    return vertices_.at(u.reversed ? s.v0 : s.v1);
}

std::vector<Vec2> Partition2D::loop_points(const std::vector<SegmentUse>& loop) const {
    std::vector<Vec2> pts;
    pts.reserve(loop.size());
    for (const auto& u : loop) pts.push_back(start(u));
    return pts;
}

PolygonWithHoles2 Partition2D::component_polygon(RegionId r, std::size_t c) const {
    const auto& comp = components(r).at(c);
    std::vector<std::vector<Vec2>> holes;
    for (std::size_t k = 1; k < comp.loops.size(); ++k) holes.push_back(loop_points(comp.loops[k]));
    return PolygonWithHoles2(comp.loops.empty() ? std::vector<Vec2>{} : loop_points(comp.loops[0]), std::move(holes));
}

Real Partition2D::segment_length(std::size_t s) const {
    const auto& seg = segments_.at(s);
    return norm(vertices_.at(seg.v1) - vertices_.at(seg.v0));
}

Real Partition2D::region_area(RegionId r) const {
    Real a = 0;
    for (const auto& comp : components(r)) {
        for (const auto& loop : comp.loops) a += signed_area(loop_points(loop));
    }
    return a;
}

Real Partition2D::total_area() const {
    Real a = 0;
    for (std::size_t r = 0; r < regions_.size(); ++r) a += region_area(RegionId::from_index(r));
    return a;
}

Real Partition2D::boundary_length() const {
    Real l = 0;
    for (std::size_t s = 0; s < segments_.size(); ++s) {
        if (is_boundary(s)) l += segment_length(s);
    }
    return l;
}

Real Partition2D::interface_length() const {
    Real l = 0;
    for (std::size_t s = 0; s < segments_.size(); ++s) {
        if (is_interface(s)) l += segment_length(s);
    }
    return l;
}

Real Partition2D::interface_length(RegionId a, RegionId b) const {
    Real l = 0;
    for (std::size_t s = 0; s < segments_.size(); ++s) {
        const auto& seg = segments_[s];
        if (is_interface(s) && ((seg.left == a && seg.right == b) || (seg.left == b && seg.right == a))) {
            l += segment_length(s);
        }
    }
    return l;
}

Box2 Partition2D::bounding_box() const { return Box2::of(vertices_); }

}  // namespace vmm
