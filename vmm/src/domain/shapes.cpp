// ============================================================================
// File: shapes.cpp
// Description: Shape outlines and the shape registry.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/shapes.hpp>
#include <vmm/geometry/polygon.hpp>

namespace vmm {
namespace {

bool finite(const std::vector<Vec2>& ring) {
    return std::ranges::all_of(ring, [](const Vec2& p) { return std::isfinite(p[0]) && std::isfinite(p[1]); });
}

/// Reverses a ring and its per-edge tags consistently: edge k of the reversed
/// ring runs between the endpoints of original edge n-2-k.
void reverse_ring(std::vector<Vec2>& ring, std::vector<std::string>& tags) {
    std::ranges::reverse(ring);
    std::ranges::reverse(tags);
    if (!tags.empty()) std::rotate(tags.begin(), tags.begin() + 1, tags.end());
}

Result<std::vector<std::string>> tags_for(std::vector<std::string> tags, std::size_t n, const char* what) {
    if (tags.empty()) return std::vector<std::string>(n);
    if (tags.size() != n) {
        return fail(ErrorCode::InvalidShapeParameter, std::format("{}: {} tags for {} edges", what, tags.size(), n));
    }
    return tags;
}

int curve_segments(const PolygonizeOptions& o) { return std::max(8, o.segments_per_curve); }

}  // namespace

Result<ShapeOutline> ShapeOutline::make(std::vector<Vec2> outer, std::vector<std::string> outer_tags,
                                        std::vector<std::vector<Vec2>> holes,
                                        std::vector<std::vector<std::string>> hole_tags) {
    if (outer.size() < 3 || !finite(outer)) return fail(ErrorCode::InvalidPolygon, "outer ring");
    auto tags = tags_for(std::move(outer_tags), outer.size(), "outer ring");
    if (!tags) return std::unexpected(tags.error());
    if (!hole_tags.empty() && hole_tags.size() != holes.size()) {
        return fail(ErrorCode::InvalidShapeParameter, "one tag list per hole expected");
    }
    hole_tags.resize(holes.size());
    ShapeOutline s;
    s.outer_ = std::move(outer);
    s.outer_tags_ = std::move(*tags);
    if (signed_area(s.outer_) == 0) return fail(ErrorCode::DegenerateShape, "outer ring has zero area");
    if (signed_area(s.outer_) < 0) reverse_ring(s.outer_, s.outer_tags_);
    for (std::size_t h = 0; h < holes.size(); ++h) {
        if (holes[h].size() < 3 || !finite(holes[h])) return fail(ErrorCode::InvalidPolygon, std::format("hole {}", h));
        auto ht = tags_for(std::move(hole_tags[h]), holes[h].size(), "hole");
        if (!ht) return std::unexpected(ht.error());
        if (signed_area(holes[h]) == 0) return fail(ErrorCode::DegenerateShape, std::format("hole {} has zero area", h));
        if (signed_area(holes[h]) > 0) reverse_ring(holes[h], *ht);
        s.holes_.push_back(std::move(holes[h]));
        s.hole_tags_.push_back(std::move(*ht));
    }
    return s;
}

Real ShapeOutline::area() const noexcept {
    Real a = signed_area(outer_);
    for (const auto& h : holes_) a += signed_area(h);
    return a;
}

Result<ShapeOutline> Rectangle::outline(const PolygonizeOptions&) const {
    if (!(hi_[0] > lo_[0] && hi_[1] > lo_[1])) return fail(ErrorCode::DegenerateShape, "rectangle needs lo < hi");
    return ShapeOutline::make({lo_, Vec2{hi_[0], lo_[1]}, hi_, Vec2{lo_[0], hi_[1]}},
                              {tags_[0], tags_[1], tags_[2], tags_[3]});
}

Result<ShapeOutline> PolygonShape::outline(const PolygonizeOptions&) const {
    return ShapeOutline::make(points_, tags_, holes_);
}

Result<ShapeOutline> Ellipse::outline(const PolygonizeOptions& options) const {
    if (!(a_ > 0 && b_ > 0 && std::isfinite(a_) && std::isfinite(b_))) {
        return fail(ErrorCode::DegenerateShape, "ellipse semi-axes must be positive");
    }
    const int n = curve_segments(options);
    const Real c = std::cos(angle_);
    const Real s = std::sin(angle_);
    std::vector<Vec2> pts;
    pts.reserve(static_cast<std::size_t>(n));
    for (int k = 0; k < n; ++k) {
        const Real t = 2 * std::numbers::pi * static_cast<Real>(k) / static_cast<Real>(n);
        const Real x = a_ * std::cos(t);
        const Real y = b_ * std::sin(t);
        pts.push_back(Vec2{center_[0] + c * x - s * y, center_[1] + s * x + c * y});
    }
    return ShapeOutline::make(std::move(pts), std::vector<std::string>(static_cast<std::size_t>(n), tag_));
}

Result<ShapeOutline> RegularNGon::outline(const PolygonizeOptions&) const {
    if (n_ < 3 || !(radius_ > 0)) return fail(ErrorCode::DegenerateShape, "regular polygon needs n >= 3 and r > 0");
    std::vector<Vec2> pts;
    for (int k = 0; k < n_; ++k) {
        const Real t = rotation_ + 2 * std::numbers::pi * static_cast<Real>(k) / static_cast<Real>(n_);
        pts.push_back(Vec2{center_[0] + radius_ * std::cos(t), center_[1] + radius_ * std::sin(t)});
    }
    return ShapeOutline::make(std::move(pts), std::vector<std::string>(static_cast<std::size_t>(n_), tag_));
}

// ----------------------------------------------------------------------------
// ShapeRegistry
// ----------------------------------------------------------------------------
namespace {

Result<std::vector<Real>> numbers(const ShapeParameters& p, const std::string& key, std::size_t count) {
    const auto it = p.numbers.find(key);
    if (it == p.numbers.end() || (count > 0 && it->second.size() != count)) {
        return fail(ErrorCode::InvalidShapeParameter, std::format("'{}' needs {} value(s)", key, count));
    }
    return it->second;
}

std::string text(const ShapeParameters& p, const std::string& key) {
    const auto it = p.texts.find(key);
    return it == p.texts.end() ? std::string{} : it->second;
}

Real number_or(const ShapeParameters& p, const std::string& key, Real fallback) {
    const auto it = p.numbers.find(key);
    return it == p.numbers.end() || it->second.empty() ? fallback : it->second[0];
}

}  // namespace

ShapeRegistry ShapeRegistry::with_builtin_shapes() {
    ShapeRegistry r;
    r.factories_["rectangle"] = [](const ShapeParameters& p, const PolygonizeOptions& o) -> Result<ShapeOutline> {
        auto lo = numbers(p, "lo", 2);
        auto hi = numbers(p, "hi", 2);
        if (!lo) return std::unexpected(lo.error());
        if (!hi) return std::unexpected(hi.error());
        return Rectangle(Vec2{(*lo)[0], (*lo)[1]}, Vec2{(*hi)[0], (*hi)[1]},
                         {text(p, "bottom"), text(p, "right"), text(p, "top"), text(p, "left")})
            .outline(o);
    };
    r.factories_["polygon"] = [](const ShapeParameters& p, const PolygonizeOptions& o) -> Result<ShapeOutline> {
        auto xy = numbers(p, "xy", 0);
        if (!xy) return std::unexpected(xy.error());
        if (xy->size() % 2 != 0) return fail(ErrorCode::InvalidShapeParameter, "'xy' needs pairs");
        std::vector<Vec2> pts;
        for (std::size_t k = 0; k + 1 < xy->size(); k += 2) pts.push_back(Vec2{(*xy)[k], (*xy)[k + 1]});
        const std::string tag = text(p, "tag");
        return PolygonShape(pts, std::vector<std::string>(pts.size(), tag)).outline(o);
    };
    r.factories_["circle"] = [](const ShapeParameters& p, const PolygonizeOptions& o) -> Result<ShapeOutline> {
        auto c = numbers(p, "center", 2);
        auto rad = numbers(p, "radius", 1);
        if (!c) return std::unexpected(c.error());
        if (!rad) return std::unexpected(rad.error());
        return Circle(Vec2{(*c)[0], (*c)[1]}, (*rad)[0], text(p, "tag")).outline(o);
    };
    r.factories_["ellipse"] = [](const ShapeParameters& p, const PolygonizeOptions& o) -> Result<ShapeOutline> {
        auto c = numbers(p, "center", 2);
        auto ax = numbers(p, "axes", 2);
        if (!c) return std::unexpected(c.error());
        if (!ax) return std::unexpected(ax.error());
        return Ellipse(Vec2{(*c)[0], (*c)[1]}, (*ax)[0], (*ax)[1], number_or(p, "angle", 0), text(p, "tag")).outline(o);
    };
    r.factories_["regular_ngon"] = [](const ShapeParameters& p, const PolygonizeOptions& o) -> Result<ShapeOutline> {
        auto c = numbers(p, "center", 2);
        auto rad = numbers(p, "radius", 1);
        auto n = numbers(p, "n", 1);
        if (!c) return std::unexpected(c.error());
        if (!rad) return std::unexpected(rad.error());
        if (!n) return std::unexpected(n.error());
        return RegularNGon(Vec2{(*c)[0], (*c)[1]}, (*rad)[0], static_cast<int>((*n)[0]), number_or(p, "rotation", 0),
                           text(p, "tag"))
            .outline(o);
    };
    return r;
}

Status ShapeRegistry::add(std::string name, Factory factory) {
    if (name.empty() || !factory) return fail(ErrorCode::InvalidArgument, "shape name and factory are required");
    if (factories_.contains(name)) return fail(ErrorCode::DuplicateName, name);
    factories_.emplace(std::move(name), std::move(factory));
    return {};
}

Result<ShapeOutline> ShapeRegistry::make(const std::string& name, const ShapeParameters& parameters,
                                         const PolygonizeOptions& options) const {
    const auto it = factories_.find(name);
    if (it == factories_.end()) return fail(ErrorCode::UnknownShape, name);
    return it->second(parameters, options);
}

}  // namespace vmm
