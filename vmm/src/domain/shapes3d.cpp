// ============================================================================
// File: shapes3d.cpp
// Description: Surfaces of the 3D shapes and the 3D shape registry.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes3d.hpp>

namespace vmm {
namespace {

/// Patch names in order of first use; "" becomes "boundary" (as in 2D).
class PatchTable {
public:
    std::uint32_t id(const std::string& tag) {
        const std::string name = tag.empty() ? "boundary" : tag;
        const auto it = std::ranges::find(names_, name);
        if (it != names_.end()) return static_cast<std::uint32_t>(it - names_.begin());
        names_.push_back(name);
        return static_cast<std::uint32_t>(names_.size() - 1);
    }
    std::vector<std::string> take() { return std::move(names_); }

private:
    std::vector<std::string> names_;
};

bool finite(const Vec3& p) { return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]); }

/// Ear clipping of a simple counter-clockwise polygon; every vertex is used.
/// Returns an empty list when no ear is found (not expected for a simple polygon).
std::vector<Triangle> ear_clip(const std::vector<Vec2>& p) {
    const auto area2 = [&](std::uint32_t a, std::uint32_t b, std::uint32_t c) { return cross(p[b] - p[a], p[c] - p[a]); };
    std::vector<std::uint32_t> ring(p.size());
    std::iota(ring.begin(), ring.end(), 0u);
    std::vector<Triangle> out;
    while (ring.size() > 3) {
        bool cut = false;
        for (std::size_t k = 0; k < ring.size() && !cut; ++k) {
            const std::uint32_t a = ring[(k + ring.size() - 1) % ring.size()];
            const std::uint32_t b = ring[k];
            const std::uint32_t c = ring[(k + 1) % ring.size()];
            if (area2(a, b, c) <= 0) continue;
            const bool empty = std::ranges::none_of(ring, [&](std::uint32_t q) {
                if (q == a || q == b || q == c || p[q] == p[a] || p[q] == p[b] || p[q] == p[c]) return false;
                return area2(a, b, q) >= 0 && area2(b, c, q) >= 0 && area2(c, a, q) >= 0;
            });
            if (!empty) continue;
            out.push_back({a, b, c});
            ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(k));
            cut = true;
        }
        if (!cut) return {};
    }
    if (area2(ring[0], ring[1], ring[2]) <= 0) return {};
    out.push_back({ring[0], ring[1], ring[2]});
    return out;
}

Result<std::vector<Real>> numbers(const ShapeParameters& p, const std::string& key, std::size_t count) {
    const auto it = p.numbers.find(key);
    if (it == p.numbers.end() || it->second.size() != count) {
        return fail(ErrorCode::InvalidShapeParameter, std::format("'{}' needs {} value(s)", key, count));
    }
    return it->second;
}

std::string text(const ShapeParameters& p, const std::string& key) {
    const auto it = p.texts.find(key);
    return it == p.texts.end() ? std::string{} : it->second;
}

}  // namespace

Result<TriangleSurface> Cuboid::surface(const PolygonizeOptions3&) const {
    if (!finite(lo_) || !finite(hi_) || !(hi_[0] > lo_[0] && hi_[1] > lo_[1] && hi_[2] > lo_[2])) {
        return fail(ErrorCode::DegenerateShape, "cuboid needs finite lo < hi");
    }
    std::vector<Vec3> pts;
    for (int k = 0; k < 8; ++k) {
        pts.push_back({(k & 1) ? hi_[0] : lo_[0], (k & 2) ? hi_[1] : lo_[1], (k & 4) ? hi_[2] : lo_[2]});
    }
    // Corner k = x + 2y + 4z; quads counter-clockwise seen from outside, in tag order.
    constexpr std::array<std::array<std::uint32_t, 4>, 6> quads{
        {{0, 4, 6, 2}, {1, 3, 7, 5}, {0, 1, 5, 4}, {2, 6, 7, 3}, {0, 2, 3, 1}, {4, 5, 7, 6}}};
    PatchTable patches;
    std::vector<Triangle> tris;
    std::vector<std::uint32_t> tri_patch;
    for (std::size_t q = 0; q < 6; ++q) {
        const std::uint32_t id = patches.id(tags_[q]);
        tris.push_back({quads[q][0], quads[q][1], quads[q][2]});
        tris.push_back({quads[q][0], quads[q][2], quads[q][3]});
        tri_patch.insert(tri_patch.end(), {id, id});
    }
    return TriangleSurface::make(std::move(pts), std::move(tris), std::move(tri_patch), patches.take());
}

Result<TriangleSurface> Sphere::surface(const PolygonizeOptions3& options) const {
    if (!finite(center_) || !(radius_ > 0) || !std::isfinite(radius_)) {
        return fail(ErrorCode::DegenerateShape, "sphere needs a finite centre and radius > 0");
    }
    const int subdivisions = std::clamp(options.sphere_subdivisions, 0, 7);
    const Real t = (1 + std::sqrt(5.0)) / 2;
    std::vector<Vec3> p{{-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t},
                        {0, -1, -t}, {0, 1, -t}, {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
    std::vector<Triangle> f{{0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11}, {1, 5, 9},  {5, 11, 4},
                            {11, 10, 2}, {10, 7, 6}, {7, 1, 8},   {3, 9, 4},  {3, 4, 2},   {3, 2, 6},  {3, 6, 8},
                            {3, 8, 9},   {4, 9, 5},  {2, 4, 11},  {6, 2, 10}, {8, 6, 7},   {9, 8, 1}};
    for (Vec3& x : p) x = (1 / norm(x)) * x;
    for (int s = 0; s < subdivisions; ++s) {
        std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> mid;
        const auto midpoint = [&](std::uint32_t a, std::uint32_t b) {
            const auto key = std::minmax(a, b);
            if (const auto it = mid.find(key); it != mid.end()) return it->second;
            const Vec3 m = 0.5 * (p[a] + p[b]);
            p.push_back((1 / norm(m)) * m);
            return mid.emplace(key, static_cast<std::uint32_t>(p.size() - 1)).first->second;
        };
        std::vector<Triangle> g;
        g.reserve(4 * f.size());
        for (const Triangle& x : f) {
            const std::uint32_t a = midpoint(x[0], x[1]);
            const std::uint32_t b = midpoint(x[1], x[2]);
            const std::uint32_t c = midpoint(x[2], x[0]);
            g.insert(g.end(), {{x[0], a, c}, {x[1], b, a}, {x[2], c, b}, {a, b, c}});
        }
        f = std::move(g);
    }
    for (Vec3& x : p) x = center_ + radius_ * x;
    PatchTable patches;
    std::vector<std::uint32_t> tri_patch(f.size(), patches.id(tag_));
    return TriangleSurface::make(std::move(p), std::move(f), std::move(tri_patch), patches.take());
}

Result<TriangleSurface> Extrusion::surface(const PolygonizeOptions3&) const {
    if (!outline_.holes().empty()) {
        return fail(ErrorCode::InvalidShapeParameter, "extrusion of an outline with holes (planned with the 3D holes, P18)");
    }
    if (!std::isfinite(z0_) || !std::isfinite(z1_) || !(z1_ > z0_)) {
        return fail(ErrorCode::DegenerateShape, "extrusion needs finite z0 < z1");
    }
    const auto& ring = outline_.outer();  // counter-clockwise (ShapeOutline::make)
    if (ring.size() < 3) return fail(ErrorCode::InvalidPolygon, "empty outline");
    const auto caps = ear_clip(ring);
    if (caps.empty()) return fail(ErrorCode::InvalidPolygon, "outline could not be triangulated");
    const auto n = static_cast<std::uint32_t>(ring.size());
    std::vector<Vec3> pts;
    for (const Vec2& q : ring) pts.push_back({q[0], q[1], z0_});
    for (const Vec2& q : ring) pts.push_back({q[0], q[1], z1_});
    PatchTable patches;
    std::vector<Triangle> tris;
    std::vector<std::uint32_t> tri_patch;
    const std::uint32_t bottom = patches.id(bottom_);
    const std::uint32_t top = patches.id(top_);
    for (const Triangle& t : caps) {
        tris.push_back({t[0], t[2], t[1]});  // bottom faces -z
        tri_patch.push_back(bottom);
        tris.push_back({t[0] + n, t[1] + n, t[2] + n});
        tri_patch.push_back(top);
    }
    for (std::uint32_t k = 0; k < n; ++k) {
        const std::uint32_t a = k;
        const std::uint32_t b = (k + 1) % n;
        const std::uint32_t side = patches.id(outline_.outer_tags()[k]);
        tris.push_back({a, b, b + n});
        tris.push_back({a, b + n, a + n});
        tri_patch.insert(tri_patch.end(), {side, side});
    }
    return TriangleSurface::make(std::move(pts), std::move(tris), std::move(tri_patch), patches.take());
}

Result<TriangleSurface> Cylinder::surface(const PolygonizeOptions3& options) const {
    if (!finite(base_) || !(radius_ > 0) || !std::isfinite(radius_) || !(height_ > 0) || !std::isfinite(height_)) {
        return fail(ErrorCode::DegenerateShape, "cylinder needs a finite base, radius > 0 and height > 0");
    }
    const Circle circle(Vec2{base_[0], base_[1]}, radius_, tags_[0]);
    auto outline = circle.outline(PolygonizeOptions{std::max(8, options.segments_per_circle)});
    if (!outline) return std::unexpected(outline.error());
    return Extrusion(std::move(*outline), base_[2], base_[2] + height_, tags_[1], tags_[2]).surface(options);
}

ShapeRegistry3D ShapeRegistry3D::with_builtin_shapes() {
    ShapeRegistry3D r;
    (void)r.add("cuboid", [](const ShapeParameters& p, const PolygonizeOptions3& o) -> Result<TriangleSurface> {
        auto lo = numbers(p, "lo", 3);
        if (!lo) return std::unexpected(lo.error());
        auto hi = numbers(p, "hi", 3);
        if (!hi) return std::unexpected(hi.error());
        return Cuboid({(*lo)[0], (*lo)[1], (*lo)[2]}, {(*hi)[0], (*hi)[1], (*hi)[2]},
                      {text(p, "x-"), text(p, "x+"), text(p, "y-"), text(p, "y+"), text(p, "z-"), text(p, "z+")})
            .surface(o);
    });
    (void)r.add("sphere", [](const ShapeParameters& p, const PolygonizeOptions3& o) -> Result<TriangleSurface> {
        auto c = numbers(p, "center", 3);
        if (!c) return std::unexpected(c.error());
        auto radius = numbers(p, "radius", 1);
        if (!radius) return std::unexpected(radius.error());
        return Sphere({(*c)[0], (*c)[1], (*c)[2]}, (*radius)[0], text(p, "tag")).surface(o);
    });
    (void)r.add("cylinder", [](const ShapeParameters& p, const PolygonizeOptions3& o) -> Result<TriangleSurface> {
        auto b = numbers(p, "base", 3);
        if (!b) return std::unexpected(b.error());
        auto radius = numbers(p, "radius", 1);
        if (!radius) return std::unexpected(radius.error());
        auto height = numbers(p, "height", 1);
        if (!height) return std::unexpected(height.error());
        return Cylinder({(*b)[0], (*b)[1], (*b)[2]}, (*radius)[0], (*height)[0],
                        {text(p, "side"), text(p, "bottom"), text(p, "top")})
            .surface(o);
    });
    return r;
}

Status ShapeRegistry3D::add(std::string name, Factory factory) {
    if (name.empty() || !factory) return fail(ErrorCode::InvalidArgument, "shape needs a name and a factory");
    if (factories_.contains(name)) return fail(ErrorCode::DuplicateName, name);
    factories_.emplace(std::move(name), std::move(factory));
    return {};
}

Result<TriangleSurface> ShapeRegistry3D::make(const std::string& name, const ShapeParameters& parameters,
                                              const PolygonizeOptions3& options) const {
    const auto it = factories_.find(name);
    if (it == factories_.end()) return fail(ErrorCode::UnknownShape, name);
    return it->second(parameters, options);
}

}  // namespace vmm
