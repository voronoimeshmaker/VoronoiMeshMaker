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
#include <memory>
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
// Surface index: bounding-volume hierarchy and pseudo-normals
// ----------------------------------------------------------------------------

namespace detail {

/// Closest point of a triangle and the feature that holds it: 0-2 the vertices
/// a, b, c; 3 edge ab; 4 edge bc; 5 edge ca; 6 the interior.
struct TrianglePoint {
    Vec3 point;
    int feature = 6;
};

/// Closest point on a triangle (Ericson, Real-Time Collision Detection, 5.1.5).
TrianglePoint closest_on_triangle(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) noexcept {
    const Vec3 ab = b - a;
    const Vec3 ac = c - a;
    const Vec3 ap = p - a;
    const Real d1 = dot(ab, ap);
    const Real d2 = dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return {a, 0};
    const Vec3 bp = p - b;
    const Real d3 = dot(ab, bp);
    const Real d4 = dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return {b, 1};
    const Real vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) return {a + (d1 / (d1 - d3)) * ab, 3};
    const Vec3 cp = p - c;
    const Real d5 = dot(ab, cp);
    const Real d6 = dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return {c, 2};
    const Real vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) return {a + (d2 / (d2 - d6)) * ac, 5};
    const Real va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) return {b + ((d4 - d3) / ((d4 - d3) + (d5 - d6))) * (c - b), 4};
    const Real den = 1 / (va + vb + vc);
    return {a + (vb * den) * ab + (vc * den) * ac, 6};
}

struct SurfaceIndex {
    struct Node {
        Box3 box;
        std::uint32_t first = 0;  ///< leaf: first triangle in `order`; inner: left child
        std::uint32_t count = 0;  ///< leaf: triangle count; inner: 0 (right child = first + 1 is not assumed)
        std::uint32_t right = 0;  ///< inner: right child
    };
    std::vector<Node> nodes;
    std::vector<std::uint32_t> order;
    std::vector<Vec3> face_normal;                         ///< unit
    std::unordered_map<std::uint64_t, Vec3> edge_normal;   ///< sum of the two face normals
    std::vector<Vec3> vertex_normal;                       ///< angle-weighted sum
};

struct Closest {
    Vec3 point;
    std::uint32_t triangle = 0;
    int feature = 6;
};

std::uint64_t undirected(std::uint32_t a, std::uint32_t b) {
    return (std::uint64_t{std::min(a, b)} << 32) | std::max(a, b);
}

std::shared_ptr<const SurfaceIndex> build_index(const std::vector<Vec3>& p, const std::vector<Triangle>& t) {
    auto ix = std::make_shared<SurfaceIndex>();
    ix->order.resize(t.size());
    std::iota(ix->order.begin(), ix->order.end(), 0u);
    std::vector<Vec3> centre(t.size());
    for (std::size_t k = 0; k < t.size(); ++k) centre[k] = (1.0 / 3.0) * (p[t[k][0]] + p[t[k][1]] + p[t[k][2]]);
    // Top-down build: median split along the longest axis of the centroid box.
    struct Task {
        std::uint32_t node, first, count;
    };
    ix->nodes.push_back({});
    std::vector<Task> stack{{0, 0, static_cast<std::uint32_t>(t.size())}};
    while (!stack.empty()) {
        const Task task = stack.back();
        stack.pop_back();
        Box3 box;
        Box3 centres;
        for (std::uint32_t k = task.first; k < task.first + task.count; ++k) {
            const Triangle& x = t[ix->order[k]];
            for (const std::uint32_t v : x) box.expand(p[v]);
            centres.expand(centre[ix->order[k]]);
        }
        ix->nodes[task.node].box = box;
        if (task.count <= 4) {
            ix->nodes[task.node].first = task.first;
            ix->nodes[task.node].count = task.count;
            continue;
        }
        const Vec3 extent = centres.hi() - centres.lo();
        const std::size_t axis = extent[0] >= extent[1] && extent[0] >= extent[2] ? 0 : (extent[1] >= extent[2] ? 1 : 2);
        const std::uint32_t half = task.count / 2;
        const auto begin = ix->order.begin() + task.first;
        std::nth_element(begin, begin + half, begin + task.count,
                         [&](std::uint32_t a, std::uint32_t b) { return centre[a][axis] < centre[b][axis]; });
        const auto left = static_cast<std::uint32_t>(ix->nodes.size());
        ix->nodes.push_back({});
        ix->nodes.push_back({});
        ix->nodes[task.node].first = left;
        ix->nodes[task.node].right = left + 1;
        stack.push_back({left, task.first, half});
        stack.push_back({left + 1, task.first + half, task.count - half});
    }
    // Pseudo-normals.
    ix->face_normal.resize(t.size());
    ix->vertex_normal.assign(p.size(), Vec3{});
    for (std::size_t k = 0; k < t.size(); ++k) {
        const Triangle& x = t[k];
        const Vec3 n = cross(p[x[1]] - p[x[0]], p[x[2]] - p[x[0]]);
        const Vec3 u = (1 / norm(n)) * n;
        ix->face_normal[k] = u;
        for (std::size_t e = 0; e < 3; ++e) {
            const std::uint32_t a = x[e];
            const std::uint32_t b = x[(e + 1) % 3];
            const std::uint32_t c = x[(e + 2) % 3];
            Vec3& en = ix->edge_normal[undirected(a, b)];
            en = en + u;
            const Vec3 ab = p[b] - p[a];
            const Vec3 ac = p[c] - p[a];
            const Real angle = std::atan2(norm(cross(ab, ac)), dot(ab, ac));
            ix->vertex_normal[a] = ix->vertex_normal[a] + angle * u;
        }
    }
    return ix;
}

Real box_distance2(const Box3& b, const Vec3& p) {
    Real d = 0;
    for (std::size_t k = 0; k < 3; ++k) {
        const Real e = std::max({b.lo()[k] - p[k], Real{0}, p[k] - b.hi()[k]});
        d += e * e;
    }
    return d;
}

Closest closest(const SurfaceIndex& ix, const std::vector<Vec3>& p, const std::vector<Triangle>& t, const Vec3& q) {
    Closest best;
    Real best2 = std::numeric_limits<Real>::infinity();
    std::vector<std::uint32_t> stack{0};
    while (!stack.empty()) {
        const SurfaceIndex::Node& node = ix.nodes[stack.back()];
        stack.pop_back();
        if (box_distance2(node.box, q) >= best2) continue;
        if (node.count > 0) {
            for (std::uint32_t k = node.first; k < node.first + node.count; ++k) {
                const std::uint32_t tri = ix.order[k];
                const auto c = closest_on_triangle(q, p[t[tri][0]], p[t[tri][1]], p[t[tri][2]]);
                const Vec3 d = q - c.point;
                const Real d2 = dot(d, d);
                if (d2 < best2) {
                    best2 = d2;
                    best = {c.point, tri, c.feature};
                }
            }
            continue;
        }
        // Nearer child last, so that it is visited first.
        const Real dl = box_distance2(ix.nodes[node.first].box, q);
        const Real dr = box_distance2(ix.nodes[node.right].box, q);
        if (dl < dr) {
            stack.push_back(node.right);
            stack.push_back(node.first);
        } else {
            stack.push_back(node.first);
            stack.push_back(node.right);
        }
    }
    return best;
}

Vec3 pseudo_normal(const SurfaceIndex& ix, const std::vector<Triangle>& t, const Closest& c) {
    const Triangle& x = t[c.triangle];
    if (c.feature <= 2) return ix.vertex_normal[x[static_cast<std::size_t>(c.feature)]];
    if (c.feature <= 5) {
        const std::size_t e = static_cast<std::size_t>(c.feature - 3);
        return ix.edge_normal.at(undirected(x[e], x[(e + 1) % 3]));
    }
    return ix.face_normal[c.triangle];
}

}  // namespace detail

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
    s.index_ = detail::build_index(s.points_, s.triangles_);
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
    if (!index_) return false;
    const auto c = detail::closest(*index_, points_, triangles_, p);
    const Vec3 d = p - c.point;
    if (norm(d) == 0) return false;  // on the surface
    return dot(d, detail::pseudo_normal(*index_, triangles_, c)) < 0;
}

Real TriangleSurface::distance(const Vec3& p) const noexcept {
    if (!index_) return std::numeric_limits<Real>::infinity();
    return norm(p - detail::closest(*index_, points_, triangles_, p).point);
}

Real distance_to_triangle(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) noexcept {
    return norm(p - detail::closest_on_triangle(p, a, b, c).point);
}

}  // namespace vmm
