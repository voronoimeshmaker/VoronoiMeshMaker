// ============================================================================
// File: polygon.hpp
// Description: Floating-point polygon utilities: boxes, rings, polygons with
//              holes and a uniform-grid segment index. These are used for
//              measures and site placement, never to decide topology (the
//              topology comes from the exact backend, DEC-028).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>

namespace vmm {

/// Axis-aligned box. An empty box has lo > hi.
class Box2 {
public:
    Box2() = default;
    Box2(Vec2 lo, Vec2 hi) noexcept : lo_(lo), hi_(hi) {}

    [[nodiscard]] static Box2 of(std::span<const Vec2> points) noexcept;

    [[nodiscard]] bool empty() const noexcept { return lo_[0] > hi_[0] || lo_[1] > hi_[1]; }
    [[nodiscard]] const Vec2& lo() const noexcept { return lo_; }
    [[nodiscard]] const Vec2& hi() const noexcept { return hi_; }
    [[nodiscard]] Real diagonal() const noexcept { return empty() ? 0 : norm(hi_ - lo_); }

    void expand(const Vec2& p) noexcept;
    void expand(const Box2& b) noexcept;
    /// Grows every side by `margin` (may be negative).
    [[nodiscard]] Box2 inflated(Real margin) const noexcept;
    [[nodiscard]] bool contains(const Vec2& p) const noexcept;
    [[nodiscard]] bool overlaps(const Box2& b) const noexcept;

private:
    Vec2 lo_{1, 1};
    Vec2 hi_{-1, -1};
};

/// Signed area of a closed ring (counter-clockwise > 0).
[[nodiscard]] Real signed_area(std::span<const Vec2> ring) noexcept;

/// Area centroid of a closed ring (vertex average if the area is zero).
[[nodiscard]] Vec2 ring_centroid(std::span<const Vec2> ring) noexcept;

[[nodiscard]] Real perimeter(std::span<const Vec2> ring) noexcept;

/// Even-odd point-in-ring test; points on the boundary may go either way.
[[nodiscard]] bool ring_contains(std::span<const Vec2> ring, const Vec2& p) noexcept;

[[nodiscard]] Real distance_to_segment(const Vec2& p, const Vec2& a, const Vec2& b) noexcept;

/// Polygon with holes: outer ring counter-clockwise, holes clockwise.
class PolygonWithHoles2 {
public:
    PolygonWithHoles2() = default;
    /// Rings are re-oriented as needed; rings with fewer than 3 points are kept as given.
    explicit PolygonWithHoles2(std::vector<Vec2> outer, std::vector<std::vector<Vec2>> holes = {});

    [[nodiscard]] const std::vector<Vec2>& outer() const noexcept { return outer_; }
    [[nodiscard]] const std::vector<std::vector<Vec2>>& holes() const noexcept { return holes_; }

    [[nodiscard]] Real area() const noexcept;
    [[nodiscard]] Real perimeter() const noexcept;
    [[nodiscard]] Box2 bounding_box() const noexcept;
    /// Inside the outer ring and outside every hole (boundary ambiguous).
    [[nodiscard]] bool contains(const Vec2& p) const noexcept;
    /// Distance to the nearest edge of any ring.
    [[nodiscard]] Real distance_to_boundary(const Vec2& p) const noexcept;
    /// Number of vertices in all rings.
    [[nodiscard]] std::size_t vertex_count() const noexcept;

private:
    std::vector<Vec2> outer_;
    std::vector<std::vector<Vec2>> holes_;
};

/// Uniform grid over a set of segments, answering "does this box touch a
/// segment?" conservatively (bounding boxes only). Used to skip the exact
/// region clipping for cells far from any boundary.
class SegmentIndex2 {
public:
    SegmentIndex2() = default;
    /// `cells_per_side` <= 0 chooses about sqrt(segments) cells per side.
    SegmentIndex2(std::span<const Vec2> a, std::span<const Vec2> b, int cells_per_side = 0);

    /// True if the box overlaps the bounding box of at least one segment.
    [[nodiscard]] bool may_touch(const Box2& box) const noexcept;
    [[nodiscard]] std::size_t segment_count() const noexcept { return boxes_.size(); }

private:
    [[nodiscard]] std::size_t cell_x(Real x) const noexcept;
    [[nodiscard]] std::size_t cell_y(Real y) const noexcept;

    Box2 extent_;
    std::size_t nx_ = 0;
    std::size_t ny_ = 0;
    std::vector<Box2> boxes_;
    std::vector<std::size_t> offsets_;
    std::vector<std::uint32_t> items_;
};

}  // namespace vmm
