// ============================================================================
// File: polygon.cpp
// Description: Floating-point polygon utilities.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/geometry/polygon.hpp>

namespace vmm {

// ----------------------------------------------------------------------------
// Box2
// ----------------------------------------------------------------------------
Box2 Box2::of(std::span<const Vec2> points) noexcept {
    Box2 b;
    for (const Vec2& p : points) b.expand(p);
    return b;
}

void Box2::expand(const Vec2& p) noexcept {
    if (empty()) {
        lo_ = p;
        hi_ = p;
        return;
    }
    for (std::size_t k = 0; k < 2; ++k) {
        lo_[k] = std::min(lo_[k], p[k]);
        hi_[k] = std::max(hi_[k], p[k]);
    }
}

void Box2::expand(const Box2& b) noexcept {
    if (b.empty()) return;
    expand(b.lo_);
    expand(b.hi_);
}

Box2 Box2::inflated(Real margin) const noexcept {
    if (empty()) return *this;
    return {Vec2{lo_[0] - margin, lo_[1] - margin}, Vec2{hi_[0] + margin, hi_[1] + margin}};
}

bool Box2::contains(const Vec2& p) const noexcept {
    return !empty() && p[0] >= lo_[0] && p[0] <= hi_[0] && p[1] >= lo_[1] && p[1] <= hi_[1];
}

bool Box2::overlaps(const Box2& b) const noexcept {
    return !empty() && !b.empty() && lo_[0] <= b.hi_[0] && b.lo_[0] <= hi_[0] && lo_[1] <= b.hi_[1] &&
           b.lo_[1] <= hi_[1];
}

// ----------------------------------------------------------------------------
// Rings
// ----------------------------------------------------------------------------
Real signed_area(std::span<const Vec2> ring) noexcept {
    if (ring.size() < 3) return 0;
    const Vec2 o = ring[0];
    Real a = 0;
    for (std::size_t k = 1; k + 1 < ring.size(); ++k) a += cross(ring[k] - o, ring[k + 1] - o);
    return 0.5 * a;
}

Vec2 ring_centroid(std::span<const Vec2> ring) noexcept {
    if (ring.empty()) return {0, 0};
    const Vec2 o = ring[0];
    Real area = 0;
    Vec2 acc{0, 0};
    for (std::size_t k = 1; k + 1 < ring.size(); ++k) {
        const Real t = cross(ring[k] - o, ring[k + 1] - o);
        area += t;
        acc = acc + (t / 3.0) * (ring[k] - o + (ring[k + 1] - o));
    }
    if (area == 0) {
        Vec2 mean{0, 0};
        for (const Vec2& p : ring) mean = mean + p;
        return (1.0 / static_cast<Real>(ring.size())) * mean;
    }
    return o + (1.0 / area) * acc;
}

Real perimeter(std::span<const Vec2> ring) noexcept {
    Real p = 0;
    for (std::size_t k = 0; k < ring.size(); ++k) p += norm(ring[(k + 1) % ring.size()] - ring[k]);
    return p;
}

bool ring_contains(std::span<const Vec2> ring, const Vec2& p) noexcept {
    bool inside = false;
    const std::size_t n = ring.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const Vec2& a = ring[i];
        const Vec2& b = ring[j];
        if ((a[1] > p[1]) != (b[1] > p[1])) {
            const Real x = a[0] + (p[1] - a[1]) * (b[0] - a[0]) / (b[1] - a[1]);
            if (p[0] < x) inside = !inside;
        }
    }
    return inside;
}

Real distance_to_segment(const Vec2& p, const Vec2& a, const Vec2& b) noexcept {
    const Vec2 e = b - a;
    const Real ee = dot(e, e);
    const Real t = ee > 0 ? std::clamp(dot(p - a, e) / ee, Real{0}, Real{1}) : Real{0};
    return norm(p - (a + t * e));
}

// ----------------------------------------------------------------------------
// PolygonWithHoles2
// ----------------------------------------------------------------------------
PolygonWithHoles2::PolygonWithHoles2(std::vector<Vec2> outer, std::vector<std::vector<Vec2>> holes)
    : outer_(std::move(outer)), holes_(std::move(holes)) {
    if (signed_area(outer_) < 0) std::ranges::reverse(outer_);
    for (auto& h : holes_) {
        if (signed_area(h) > 0) std::ranges::reverse(h);
    }
}

Real PolygonWithHoles2::area() const noexcept {
    Real a = signed_area(outer_);
    for (const auto& h : holes_) a += signed_area(h);
    return a;
}

Real PolygonWithHoles2::perimeter() const noexcept {
    Real p = vmm::perimeter(outer_);
    for (const auto& h : holes_) p += vmm::perimeter(h);
    return p;
}

Box2 PolygonWithHoles2::bounding_box() const noexcept { return Box2::of(outer_); }

bool PolygonWithHoles2::contains(const Vec2& p) const noexcept {
    if (!ring_contains(outer_, p)) return false;
    return std::ranges::none_of(holes_, [&](const auto& h) { return ring_contains(h, p); });
}

Real PolygonWithHoles2::distance_to_boundary(const Vec2& p) const noexcept {
    Real d = std::numeric_limits<Real>::infinity();
    auto ring_distance = [&](const std::vector<Vec2>& r) {
        for (std::size_t k = 0; k < r.size(); ++k) d = std::min(d, distance_to_segment(p, r[k], r[(k + 1) % r.size()]));
    };
    ring_distance(outer_);
    for (const auto& h : holes_) ring_distance(h);
    return d;
}

std::size_t PolygonWithHoles2::vertex_count() const noexcept {
    std::size_t n = outer_.size();
    for (const auto& h : holes_) n += h.size();
    return n;
}

// ----------------------------------------------------------------------------
// SegmentIndex2
// ----------------------------------------------------------------------------
SegmentIndex2::SegmentIndex2(std::span<const Vec2> a, std::span<const Vec2> b, int cells_per_side) {
    const std::size_t n = std::min(a.size(), b.size());
    boxes_.reserve(n);
    for (std::size_t k = 0; k < n; ++k) {
        Box2 box;
        box.expand(a[k]);
        box.expand(b[k]);
        boxes_.push_back(box);
        extent_.expand(box);
    }
    const auto side = cells_per_side > 0
                          ? static_cast<std::size_t>(cells_per_side)
                          : std::max<std::size_t>(1, static_cast<std::size_t>(std::sqrt(static_cast<Real>(n))));
    nx_ = side;
    ny_ = side;
    std::vector<std::vector<std::uint32_t>> cells(nx_ * ny_);
    for (std::size_t k = 0; k < n; ++k) {
        for (std::size_t j = cell_y(boxes_[k].lo()[1]); j <= cell_y(boxes_[k].hi()[1]); ++j) {
            for (std::size_t i = cell_x(boxes_[k].lo()[0]); i <= cell_x(boxes_[k].hi()[0]); ++i) {
                cells[j * nx_ + i].push_back(static_cast<std::uint32_t>(k));
            }
        }
    }
    offsets_.assign(1, 0);
    for (const auto& c : cells) {
        items_.insert(items_.end(), c.begin(), c.end());
        offsets_.push_back(items_.size());
    }
}

std::size_t SegmentIndex2::cell_x(Real x) const noexcept {
    const Real w = extent_.hi()[0] - extent_.lo()[0];
    if (!(w > 0)) return 0;
    const Real t = (x - extent_.lo()[0]) / w * static_cast<Real>(nx_);
    return std::min(nx_ - 1, static_cast<std::size_t>(std::max(Real{0}, t)));
}

std::size_t SegmentIndex2::cell_y(Real y) const noexcept {
    const Real h = extent_.hi()[1] - extent_.lo()[1];
    if (!(h > 0)) return 0;
    const Real t = (y - extent_.lo()[1]) / h * static_cast<Real>(ny_);
    return std::min(ny_ - 1, static_cast<std::size_t>(std::max(Real{0}, t)));
}

bool SegmentIndex2::may_touch(const Box2& box) const noexcept {
    if (boxes_.empty() || !box.overlaps(extent_)) return false;
    for (std::size_t j = cell_y(box.lo()[1]); j <= cell_y(box.hi()[1]); ++j) {
        for (std::size_t i = cell_x(box.lo()[0]); i <= cell_x(box.hi()[0]); ++i) {
            const std::size_t c = j * nx_ + i;
            for (std::size_t k = offsets_[c]; k < offsets_[c + 1]; ++k) {
                if (boxes_[items_[k]].overlaps(box)) return true;
            }
        }
    }
    return false;
}

}  // namespace vmm
