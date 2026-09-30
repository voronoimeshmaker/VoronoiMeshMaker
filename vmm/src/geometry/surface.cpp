// ============================================================================
// File: surface.cpp
// Description: Box3 and TriangleSurface.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <numbers>
#include <numeric>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/geometry/surface.hpp>

namespace vmm {

// ----------------------------------------------------------------------------
// Box3
// ----------------------------------------------------------------------------

Box3 Box3::of(std::span<const Vec3> points) noexcept {
    Box3 b;
    for (const Vec3& p : points) b.expand(p);
    return b;
}

void Box3::expand(const Vec3& p) noexcept {
    if (empty()) {
        lo_ = p;
        hi_ = p;
        return;
    }
    for (std::size_t k = 0; k < 3; ++k) {
        lo_[k] = std::min(lo_[k], p[k]);
        hi_[k] = std::max(hi_[k], p[k]);
    }
}

void Box3::expand(const Box3& b) noexcept {
    if (b.empty()) return;
    expand(b.lo_);
    expand(b.hi_);
}

Box3 Box3::inflated(Real margin) const noexcept {
    if (empty()) return *this;
    return {lo_ - Vec3{margin, margin, margin}, hi_ + Vec3{margin, margin, margin}};
}

bool Box3::contains(const Vec3& p) const noexcept {
    for (std::size_t k = 0; k < 3; ++k) {
        if (p[k] < lo_[k] || p[k] > hi_[k]) return false;
    }
    return !empty();
}

bool Box3::overlaps(const Box3& b) const noexcept {
    if (empty() || b.empty()) return false;
    for (std::size_t k = 0; k < 3; ++k) {
        if (b.hi_[k] < lo_[k] || b.lo_[k] > hi_[k]) return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// TriangleSurface
// ----------------------------------------------------------------------------

namespace {

std::uint64_t edge_key(std::uint32_t a, std::uint32_t b) { return (std::uint64_t{a} << 32) | b; }

Real signed_volume(const std::vector<Vec3>& p, const std::vector<Triangle>& t) {
    Real v = 0;
    for (const Triangle& x : t) v += dot(p[x[0]], cross(p[x[1]], p[x[2]])) / 6;
    return v;
}

}  // namespace

Result<TriangleSurface> TriangleSurface::make(std::vector<Vec3> points, std::vector<Triangle> triangles,
                                              std::vector<std::uint32_t> triangle_patch,
                                              std::vector<std::string> patches) {
    if (triangles.size() < 4) return fail(ErrorCode::InvalidSurface, std::format("{} triangles", triangles.size()));
    if (triangle_patch.size() != triangles.size()) {
        return fail(ErrorCode::InvalidArgument, "one patch per triangle expected");
    }
    for (const Vec3& p : points) {
        if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) {
            return fail(ErrorCode::InvalidSurface, "non-finite point");
        }
    }
    std::unordered_map<std::uint64_t, std::uint32_t> directed;
    directed.reserve(3 * triangles.size());
    for (std::size_t t = 0; t < triangles.size(); ++t) {
        const Triangle& x = triangles[t];
        if (std::ranges::any_of(x, [&](std::uint32_t v) { return v >= points.size(); })) {
            return fail(ErrorCode::InvalidArgument, std::format("triangle {}: vertex index", t));
        }
        if (triangle_patch[t] >= patches.size()) {
            return fail(ErrorCode::InvalidArgument, std::format("triangle {}: patch index", t));
        }
        if (norm(cross(points[x[1]] - points[x[0]], points[x[2]] - points[x[0]])) == 0) {
            return fail(ErrorCode::InvalidSurface, std::format("triangle {} is degenerate", t));
        }
        for (std::size_t k = 0; k < 3; ++k) {
            if (++directed[edge_key(x[k], x[(k + 1) % 3])] > 1) {
                return fail(ErrorCode::InvalidSurface,
                            std::format("edge ({}, {}) used twice in one direction", x[k], x[(k + 1) % 3]));
            }
        }
    }
    for (const auto& [key, count] : directed) {
        const auto a = static_cast<std::uint32_t>(key >> 32);
        const auto b = static_cast<std::uint32_t>(key & 0xffffffffu);
        if (!directed.contains(edge_key(b, a))) {
            return fail(ErrorCode::InvalidSurface, std::format("edge ({}, {}) has no opposite (open surface)", a, b));
        }
    }
    const Real v = signed_volume(points, triangles);
    if (!(v != 0) || !std::isfinite(v)) return fail(ErrorCode::InvalidSurface, "zero volume");
    if (v < 0) {
        for (Triangle& x : triangles) std::swap(x[1], x[2]);
    }
    TriangleSurface s;
    s.points_ = std::move(points);
    s.triangles_ = std::move(triangles);
    s.triangle_patch_ = std::move(triangle_patch);
    s.patches_ = std::move(patches);
    return s;
}

Real TriangleSurface::area() const noexcept {
    Real a = 0;
    for (const Triangle& x : triangles_) {
        a += 0.5 * norm(cross(points_[x[1]] - points_[x[0]], points_[x[2]] - points_[x[0]]));
    }
    return a;
}

Real TriangleSurface::volume() const noexcept { return signed_volume(points_, triangles_); }

std::size_t TriangleSurface::component_count() const {
    std::vector<std::uint32_t> parent(points_.size());
    std::iota(parent.begin(), parent.end(), 0u);
    const auto find = [&](std::uint32_t x) {
        while (parent[x] != x) x = parent[x] = parent[parent[x]];
        return x;
    };
    for (const Triangle& x : triangles_) {
        parent[find(x[1])] = find(x[0]);
        parent[find(x[2])] = find(x[0]);
    }
    std::vector<char> root_seen(points_.size(), 0);
    std::size_t roots = 0;
    for (const Triangle& x : triangles_) {
        const std::uint32_t r = find(x[0]);
        if (!root_seen[r]) {
            root_seen[r] = 1;
            ++roots;
        }
    }
    return roots;
}

bool TriangleSurface::contains(const Vec3& p) const noexcept {
    // Van Oosterom-Strackee solid angle of every triangle seen from p.
    Real omega = 0;
    for (const Triangle& x : triangles_) {
        const Vec3 a = points_[x[0]] - p;
        const Vec3 b = points_[x[1]] - p;
        const Vec3 c = points_[x[2]] - p;
        const Real la = norm(a);
        const Real lb = norm(b);
        const Real lc = norm(c);
        if (la == 0 || lb == 0 || lc == 0) return false;  // on a vertex
        const Real num = dot(a, cross(b, c));
        const Real den = la * lb * lc + dot(a, b) * lc + dot(a, c) * lb + dot(b, c) * la;
        omega += 2 * std::atan2(num, den);
    }
    return omega / (4 * std::numbers::pi) > 0.5;
}

Real TriangleSurface::distance(const Vec3& p) const noexcept {
    Real d = std::numeric_limits<Real>::infinity();
    for (const Triangle& x : triangles_) d = std::min(d, distance_to_triangle(p, points_[x[0]], points_[x[1]], points_[x[2]]));
    return d;
}

Real distance_to_triangle(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) noexcept {
    // Closest point on a triangle (Ericson, Real-Time Collision Detection, 5.1.5).
    const Vec3 ab = b - a;
    const Vec3 ac = c - a;
    const Vec3 ap = p - a;
    const Real d1 = dot(ab, ap);
    const Real d2 = dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return norm(ap);
    const Vec3 bp = p - b;
    const Real d3 = dot(ab, bp);
    const Real d4 = dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return norm(bp);
    const Real vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) return norm(p - (a + (d1 / (d1 - d3)) * ab));
    const Vec3 cp = p - c;
    const Real d5 = dot(ab, cp);
    const Real d6 = dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return norm(cp);
    const Real vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) return norm(p - (a + (d2 / (d2 - d6)) * ac));
    const Real va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        return norm(p - (b + ((d4 - d3) / ((d4 - d3) + (d5 - d6))) * (c - b)));
    }
    const Real den = 1 / (va + vb + vc);
    return norm(p - (a + (vb * den) * ab + (vc * den) * ac));
}

}  // namespace vmm
