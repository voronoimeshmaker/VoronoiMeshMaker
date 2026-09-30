// ============================================================================
// File: sources.cpp
// Description: Site sources, interface pairs and generate_sites().
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
#include <limits>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/tolerance.hpp>
#include <vmm/sites/sources.hpp>

namespace vmm {
namespace {

/// Uniform hash grid of points for minimum-distance queries.
class PointGrid {
public:
    explicit PointGrid(Real cell) : cell_(cell) {}

    void insert(const Vec2& p) {
        cells_[key(cx(p[0]), cx(p[1]))].push_back(p);
    }

    [[nodiscard]] bool any_within(const Vec2& p, Real radius) const {
        const auto reach = static_cast<std::int64_t>(std::ceil(radius / cell_));
        const std::int64_t x0 = cx(p[0]);
        const std::int64_t y0 = cx(p[1]);
        for (std::int64_t j = y0 - reach; j <= y0 + reach; ++j) {
            for (std::int64_t i = x0 - reach; i <= x0 + reach; ++i) {
                const auto it = cells_.find(key(i, j));
                if (it == cells_.end()) continue;
                for (const Vec2& q : it->second) {
                    if (norm(q - p) < radius) return true;
                }
            }
        }
        return false;
    }

private:
    [[nodiscard]] std::int64_t cx(Real v) const { return static_cast<std::int64_t>(std::floor(v / cell_)); }
    [[nodiscard]] static std::uint64_t key(std::int64_t i, std::int64_t j) {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(i)) << 32) | static_cast<std::uint32_t>(j);
    }

    Real cell_;
    std::unordered_map<std::uint64_t, std::vector<Vec2>> cells_;
};

bool keeps(const PolygonWithHoles2& c, const Vec2& p, Real margin) {
    return c.contains(p) && c.distance_to_boundary(p) >= margin;
}

Status positive(Real v, const char* what) {
    if (!(v > 0) || !std::isfinite(v)) return fail(ErrorCode::InvalidSpacing, std::format("{} = {}", what, v));
    return {};
}

std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31);
}

std::vector<Vec2> lattice(const PolygonWithHoles2& c, Vec2 origin, Vec2 a, Vec2 b, Real margin) {
    // Points origin + i a + j b inside the component's box (a, b non-collinear).
    const Box2 box = c.bounding_box();
    const Real det = cross(a, b);
    constexpr Real inf = std::numeric_limits<Real>::infinity();
    std::array<Real, 2> lo_ij{inf, inf};
    std::array<Real, 2> hi_ij{-inf, -inf};
    for (const Vec2& corner : {box.lo(), box.hi(), Vec2{box.lo()[0], box.hi()[1]}, Vec2{box.hi()[0], box.lo()[1]}}) {
        const Vec2 d = corner - origin;
        const Real i = cross(d, b) / det;
        const Real j = cross(a, d) / det;
        lo_ij = {std::min(lo_ij[0], i), std::min(lo_ij[1], j)};
        hi_ij = {std::max(hi_ij[0], i), std::max(hi_ij[1], j)};
    }
    std::vector<Vec2> out;
    for (auto j = static_cast<long long>(std::floor(lo_ij[1])); j <= static_cast<long long>(std::ceil(hi_ij[1])); ++j) {
        for (auto i = static_cast<long long>(std::floor(lo_ij[0])); i <= static_cast<long long>(std::ceil(hi_ij[0])); ++i) {
            const Vec2 p = origin + static_cast<Real>(i) * a + static_cast<Real>(j) * b;
            if (keeps(c, p, margin)) out.push_back(p);
        }
    }
    return out;
}

}  // namespace

Result<std::vector<Vec2>> UniformRandomSource::generate(const PolygonWithHoles2& c, Random& rng) const {
    if (auto s = positive(spacing_, "spacing"); !s) return std::unexpected(s.error());
    // Without a minimum distance every candidate is kept and the loop never ends;
    // RandomCountSource is the source for unconstrained random sites.
    if (auto s = positive(min_distance_, "min_distance_fraction"); !s) return std::unexpected(s.error());
    const Real r = min_distance_ * spacing_;
    PointGrid grid(r);
    const Box2 box = c.bounding_box();
    std::vector<Vec2> out;
    std::size_t failures = 0;
    while (failures < failure_limit_) {
        const Vec2 p{rng.uniform(box.lo()[0], box.hi()[0]), rng.uniform(box.lo()[1], box.hi()[1])};
        if (!keeps(c, p, margin_ * spacing_) || grid.any_within(p, r)) {
            ++failures;
            continue;
        }
        failures = 0;
        grid.insert(p);
        out.push_back(p);
    }
    return out;
}

Result<std::vector<Vec2>> AdaptiveQuadtreeSource::generate(const PolygonWithHoles2& c, Random& rng) const {
    if (auto s = positive(min_spacing_, "min_spacing"); !s) return std::unexpected(s.error());
    if (!spacing_) return fail(ErrorCode::InvalidSpacing, "no spacing field");
    const Box2 box = c.bounding_box();
    const Real side0 = std::max(box.hi()[0] - box.lo()[0], box.hi()[1] - box.lo()[1]);
    struct Square {
        Vec2 lo;
        Real side;
    };
    std::vector<Square> stack{{box.lo(), side0}};
    std::vector<Vec2> out;
    while (!stack.empty()) {
        const Square s = stack.back();
        stack.pop_back();
        const Vec2 center = s.lo + Vec2{0.5 * s.side, 0.5 * s.side};
        const Real h = spacing_(center);
        if (!(h > 0) || !std::isfinite(h)) return fail(ErrorCode::InvalidSpacing, std::format("h = {}", h));
        if (s.side > h && s.side > min_spacing_) {
            const Real half = 0.5 * s.side;
            // Pushed in reverse so that children are visited in a fixed order.
            stack.push_back({s.lo + Vec2{half, half}, half});
            stack.push_back({s.lo + Vec2{0, half}, half});
            stack.push_back({s.lo + Vec2{half, 0}, half});
            stack.push_back({s.lo, half});
            continue;
        }
        const Vec2 p = center + Vec2{jitter_ * s.side * (2 * rng.uniform() - 1), jitter_ * s.side * (2 * rng.uniform() - 1)};
        if (keeps(c, p, margin_ * s.side)) out.push_back(p);
    }
    return out;
}

Result<std::vector<Vec2>> RandomCountSource::generate(const PolygonWithHoles2& c, Random& rng) const {
    const Box2 box = c.bounding_box();
    std::vector<Vec2> out;
    out.reserve(count_);
    const std::size_t limit = 1000 * count_ + 1000;
    for (std::size_t attempt = 0; out.size() < count_; ++attempt) {
        if (attempt >= limit) return fail(ErrorCode::SiteGenerationFailed, std::format("{} of {} sites", out.size(), count_));
        const Vec2 p{rng.uniform(box.lo()[0], box.hi()[0]), rng.uniform(box.lo()[1], box.hi()[1])};
        if (keeps(c, p, margin_)) out.push_back(p);
    }
    return out;
}

Result<std::vector<Vec2>> CartesianGridSource::generate(const PolygonWithHoles2& c, Random&) const {
    if (auto s = positive(spacing_, "spacing"); !s) return std::unexpected(s.error());
    return lattice(c, origin_, Vec2{spacing_, 0}, Vec2{0, spacing_}, margin_ * spacing_);
}

Result<std::vector<Vec2>> HexagonalGridSource::generate(const PolygonWithHoles2& c, Random&) const {
    if (auto s = positive(spacing_, "spacing"); !s) return std::unexpected(s.error());
    return lattice(c, origin_, Vec2{spacing_, 0}, Vec2{0.5 * spacing_, 0.5 * std::sqrt(3.0) * spacing_}, margin_ * spacing_);
}

Result<std::vector<Vec2>> ExplicitSites::generate(const PolygonWithHoles2& c, Random&) const {
    std::vector<Vec2> out;
    for (const Vec2& p : sites_) {
        if (c.contains(p)) out.push_back(p);
    }
    return out;
}

Result<std::vector<InterfacePairs::Pair>> InterfacePairs::generate(const Partition2D& p) const {
    if (auto s = positive(spacing_, "pair spacing"); !s) return std::unexpected(s.error());
    std::vector<std::vector<PolygonWithHoles2>> comps(p.region_count());
    for (std::size_t r = 0; r < p.region_count(); ++r) {
        for (std::size_t c = 0; c < p.components(RegionId::from_index(r)).size(); ++c) {
            comps[r].push_back(p.component_polygon(RegionId::from_index(r), c));
        }
    }
    const Real offset = offset_ * spacing_;
    auto inside = [&](RegionId r, const Vec2& x) {
        return std::ranges::any_of(comps[r.index()], [&](const auto& c) { return keeps(c, x, 0.5 * offset); });
    };
    std::vector<Pair> pairs;
    for (std::size_t s = 0; s < p.segments().size(); ++s) {
        if (!p.is_interface(s)) continue;
        const auto& seg = p.segments()[s];
        const Vec2 a = p.vertices()[seg.v0];
        const Vec2 b = p.vertices()[seg.v1];
        const Real length = norm(b - a);
        const Real usable = length - 2 * clearance_ * spacing_;
        if (usable <= 0) continue;
        const auto n = static_cast<std::size_t>(std::max(1.0, std::floor(usable / spacing_)));
        const Vec2 t = (1.0 / length) * (b - a);
        const Vec2 left_normal{-t[1], t[0]};
        for (std::size_t k = 0; k < n; ++k) {
            const Real d = clearance_ * spacing_ + (static_cast<Real>(k) + 0.5) * usable / static_cast<Real>(n);
            const Vec2 x = a + d * t;
            const Vec2 l = x + offset * left_normal;
            const Vec2 r = x - offset * left_normal;
            if (inside(seg.left, l) && inside(seg.right, r)) pairs.push_back({l, seg.left, r, seg.right});
        }
    }
    return pairs;
}

Result<SiteSet> generate_sites(const Partition2D& partition, std::span<const RegionSites> sources,
                               const SiteGenerationOptions& options) {
    std::vector<std::pair<Vec2, RegionId>> generated;
    for (const auto& spec : sources) {
        if (!spec.region.valid() || spec.region.index() >= partition.region_count() || !spec.source) {
            return fail(ErrorCode::InvalidArgument, "site source for an unknown region", spec.region);
        }
        const auto& comps = partition.components(spec.region);
        for (std::size_t c = 0; c < comps.size(); ++c) {
            Random rng(splitmix64(options.seed ^ splitmix64((spec.region.index() << 32) | c)));
            auto pts = spec.source(partition.component_polygon(spec.region, c), rng);
            if (!pts) return std::unexpected(pts.error());
            for (const Vec2& x : *pts) generated.emplace_back(x, spec.region);
        }
    }
    SiteSet sites;
    if (options.interface_pairs) {
        auto pairs = options.interface_pairs->generate(partition);
        if (!pairs) return std::unexpected(pairs.error());
        const Real exclusion = options.pair_exclusion_fraction * options.interface_pairs->spacing();
        PointGrid grid(exclusion > 0 ? exclusion : options.interface_pairs->spacing());
        for (const auto& pr : *pairs) {
            grid.insert(pr.left);
            grid.insert(pr.right);
            sites.add(pr.left, pr.left_region);
            sites.add(pr.right, pr.right_region);
        }
        std::erase_if(generated, [&](const auto& g) { return exclusion > 0 && grid.any_within(g.first, exclusion); });
    }
    for (const auto& [x, r] : generated) sites.add(x, r);
    if (auto ok = validate_sites(partition, sites); !ok) return std::unexpected(ok.error());
    return sites;
}

}  // namespace vmm
