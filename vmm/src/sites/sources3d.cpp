// ============================================================================
// File: sources3d.cpp
// Description: SiteSet3D, the 3D site sources and the 3D site checks.
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
#include <numeric>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/tolerance.hpp>
#include <vmm/sites/sources3d.hpp>

namespace vmm {
namespace {

/// Uniform hash grid of points for minimum-distance queries.
class PointGrid3 {
public:
    explicit PointGrid3(Real cell) : cell_(cell) {}
    void insert(const Vec3& p) { cells_[key(cx(p[0]), cx(p[1]), cx(p[2]))].push_back(p); }
    [[nodiscard]] bool any_within(const Vec3& p, Real radius) const {
        const auto reach = static_cast<std::int64_t>(std::ceil(radius / cell_));
        const std::int64_t x0 = cx(p[0]);
        const std::int64_t y0 = cx(p[1]);
        const std::int64_t z0 = cx(p[2]);
        for (std::int64_t k = z0 - reach; k <= z0 + reach; ++k) {
            for (std::int64_t j = y0 - reach; j <= y0 + reach; ++j) {
                for (std::int64_t i = x0 - reach; i <= x0 + reach; ++i) {
                    const auto it = cells_.find(key(i, j, k));
                    if (it == cells_.end()) continue;
                    for (const Vec3& q : it->second) {
                        if (norm(q - p) < radius) return true;
                    }
                }
            }
        }
        return false;
    }

private:
    [[nodiscard]] std::int64_t cx(Real v) const { return static_cast<std::int64_t>(std::floor(v / cell_)); }
    [[nodiscard]] static std::uint64_t key(std::int64_t i, std::int64_t j, std::int64_t k) {
        const auto u = [](std::int64_t v) { return static_cast<std::uint64_t>(v) & 0x1fffffULL; };
        return (u(i) << 42) | (u(j) << 21) | u(k);
    }
    Real cell_;
    std::unordered_map<std::uint64_t, std::vector<Vec3>> cells_;
};

bool keeps(const TriangleSurface& s, const Vec3& p, Real margin) {
    return s.contains(p) && s.distance(p) >= margin;
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

Vec3 uniform_in(const Box3& b, Random& rng) {
    return {rng.uniform(b.lo()[0], b.hi()[0]), rng.uniform(b.lo()[1], b.hi()[1]), rng.uniform(b.lo()[2], b.hi()[2])};
}

}  // namespace

// ----------------------------------------------------------------------------
// SiteSet3D
// ----------------------------------------------------------------------------

SiteId SiteSet3D::add(Vec3 position, RegionId region) {
    positions_.push_back(position);
    regions_.push_back(region);
    return SiteId::from_index(positions_.size() - 1);
}

void SiteSet3D::append(std::span<const Vec3> positions, RegionId region) {
    for (const Vec3& p : positions) add(p, region);
}

std::size_t SiteSet3D::count(RegionId region) const noexcept {
    return static_cast<std::size_t>(std::ranges::count(regions_, region));
}

// ----------------------------------------------------------------------------
// Sources
// ----------------------------------------------------------------------------

Result<std::vector<Vec3>> UniformRandomSource3D::generate(const TriangleSurface& region, Random& rng) const {
    if (auto s = positive(spacing_, "spacing"); !s) return std::unexpected(s.error());
    if (auto s = positive(min_distance_, "min_distance_fraction"); !s) return std::unexpected(s.error());
    const Real r = min_distance_ * spacing_;
    PointGrid3 grid(r);
    const Box3 box = region.bounding_box();
    std::vector<Vec3> out;
    std::size_t failures = 0;
    while (failures < failure_limit_) {
        const Vec3 p = uniform_in(box, rng);
        if (grid.any_within(p, r) || !keeps(region, p, margin_ * spacing_)) {
            ++failures;
            continue;
        }
        failures = 0;
        grid.insert(p);
        out.push_back(p);
    }
    return out;
}

Result<std::vector<Vec3>> AdaptiveOctreeSource3D::generate(const TriangleSurface& region, Random& rng) const {
    if (auto s = positive(min_spacing_, "min_spacing"); !s) return std::unexpected(s.error());
    if (!spacing_) return fail(ErrorCode::InvalidSpacing, "no spacing field");
    const Box3 box = region.bounding_box();
    const Vec3 extent = box.hi() - box.lo();
    const Real side0 = std::max({extent[0], extent[1], extent[2]});
    struct Cube {
        Vec3 lo;
        Real side;
    };
    std::vector<Cube> stack{{box.lo(), side0}};
    std::vector<Vec3> out;
    while (!stack.empty()) {
        const Cube c = stack.back();
        stack.pop_back();
        if (c.lo[0] > box.hi()[0] || c.lo[1] > box.hi()[1] || c.lo[2] > box.hi()[2]) continue;  // outside the box
        const Vec3 centre = c.lo + Vec3{0.5 * c.side, 0.5 * c.side, 0.5 * c.side};
        const Real h = spacing_(centre);
        if (!(h > 0) || !std::isfinite(h)) return fail(ErrorCode::InvalidSpacing, std::format("h = {}", h));
        if (c.side > h && c.side > min_spacing_) {
            const Real half = 0.5 * c.side;
            // Pushed in reverse so that the children are visited in a fixed order.
            for (int k = 7; k >= 0; --k) {
                stack.push_back({c.lo + Vec3{(k & 1) ? half : 0, (k & 2) ? half : 0, (k & 4) ? half : 0}, half});
            }
            continue;
        }
        Vec3 p = centre;
        for (std::size_t a = 0; a < 3; ++a) p[a] += jitter_ * c.side * (2 * rng.uniform() - 1);
        if (keeps(region, p, margin_ * c.side)) out.push_back(p);
    }
    return out;
}

Result<std::vector<Vec3>> RandomCountSource3D::generate(const TriangleSurface& region, Random& rng) const {
    if (!(margin_ >= 0) || !std::isfinite(margin_)) return fail(ErrorCode::InvalidSpacing, std::format("margin = {}", margin_));
    const Box3 box = region.bounding_box();
    std::vector<Vec3> out;
    out.reserve(count_);
    const std::size_t limit = 1000 + 1000 * count_;
    for (std::size_t attempt = 0; out.size() < count_; ++attempt) {
        if (attempt == limit) {
            return fail(ErrorCode::SiteGenerationFailed, std::format("{} of {} sites", out.size(), count_));
        }
        const Vec3 p = uniform_in(box, rng);
        if (keeps(region, p, margin_)) out.push_back(p);
    }
    return out;
}

Result<std::vector<Vec3>> CartesianGridSource3D::generate(const TriangleSurface& region, Random&) const {
    if (auto s = positive(spacing_, "spacing"); !s) return std::unexpected(s.error());
    if (!(margin_ >= 0) || !std::isfinite(margin_)) {
        return fail(ErrorCode::InvalidSpacing, std::format("margin_fraction = {}", margin_));
    }
    const Box3 box = region.bounding_box();
    std::array<long long, 3> lo{};
    std::array<long long, 3> hi{};
    for (std::size_t k = 0; k < 3; ++k) {
        lo[k] = static_cast<long long>(std::floor((box.lo()[k] - origin_[k]) / spacing_));
        hi[k] = static_cast<long long>(std::ceil((box.hi()[k] - origin_[k]) / spacing_));
    }
    std::vector<Vec3> out;
    for (long long k = lo[2]; k <= hi[2]; ++k) {
        for (long long j = lo[1]; j <= hi[1]; ++j) {
            for (long long i = lo[0]; i <= hi[0]; ++i) {
                const Vec3 p = origin_ + spacing_ * Vec3{static_cast<Real>(i), static_cast<Real>(j), static_cast<Real>(k)};
                if (keeps(region, p, margin_ * spacing_)) out.push_back(p);
            }
        }
    }
    return out;
}

Result<std::vector<Vec3>> ExplicitSites3D::generate(const TriangleSurface& region, Random&) const {
    std::vector<Vec3> out;
    for (const Vec3& p : sites_) {
        if (region.contains(p)) out.push_back(p);
    }
    return out;
}

// ----------------------------------------------------------------------------
// Interface pairs
// ----------------------------------------------------------------------------

Result<std::vector<InterfacePairs3D::Pair>> InterfacePairs3D::generate(const Partition3D& partition) const {
    if (auto s = positive(spacing_, "pair spacing"); !s) return std::unexpected(s.error());
    if (!(offset_ > 0 && offset_ < 0.5)) return fail(ErrorCode::InvalidSpacing, std::format("offset_fraction = {}", offset_));
    std::vector<TriangleSurface> surfaces;
    for (std::size_t r = 0; r < partition.region_count(); ++r) {
        auto s = partition.region_surface(RegionId::from_index(r));
        if (!s) return std::unexpected(s.error());
        surfaces.push_back(std::move(*s));
    }
    const auto& p = partition.vertices();
    PointGrid3 taken(0.5 * spacing_);
    std::vector<Pair> pairs;
    for (const PartitionTriangle& t : partition.triangles()) {
        if (!t.inside.valid() || !t.outside.valid()) continue;
        const Vec3& a = p[t.v[0]];
        const Vec3& b = p[t.v[1]];
        const Vec3& c = p[t.v[2]];
        const Vec3 n = cross(b - a, c - a);
        const Vec3 u = (1 / norm(n)) * n;  // out of the inside region
        const Real longest = std::max({norm(b - a), norm(c - b), norm(a - c)});
        const int k = std::max(1, static_cast<int>(std::ceil(longest / spacing_)));
        // Centroids of the upright sub-triangles of a k x k barycentric subdivision.
        for (int i = 0; i < k; ++i) {
            for (int j = 0; i + j < k; ++j) {
                const Real wb = (i + 1.0 / 3) / k;
                const Real wc = (j + 1.0 / 3) / k;
                const Vec3 x = a + wb * (b - a) + wc * (c - a);
                if (taken.any_within(x, 0.5 * spacing_)) continue;
                // Each pair stays mirrored (orthogonal face), but its offset varies by up to 20 %
                // (deterministic): equal offsets put the sites of a curved interface on one
                // sphere around its centre, a massive cospherical degeneracy.
                const Real jitter = static_cast<Real>(splitmix64(pairs.size() + 1) >> 11) * 0x1.0p-53;
                const Real delta = offset_ * spacing_ * (0.8 + 0.4 * jitter);
                const Vec3 in = x - delta * u;
                const Vec3 out = x + delta * u;
                const TriangleSurface& si = surfaces[t.inside.index()];
                const TriangleSurface& so = surfaces[t.outside.index()];
                // The interface must be the nearest surface of both sites.
                if (!si.contains(in) || !so.contains(out) || si.distance(in) < 0.99 * delta || so.distance(out) < 0.99 * delta) {
                    continue;
                }
                taken.insert(x);
                pairs.push_back({in, t.inside, out, t.outside});
            }
        }
    }
    return pairs;
}

// ----------------------------------------------------------------------------
// Generation and checks
// ----------------------------------------------------------------------------

Result<SiteSet3D> generate_sites_3d(const Partition3D& partition, std::span<const RegionSites3D> sources,
                                    const SiteGenerationOptions3D& options) {
    SiteSet3D sites;
    std::vector<InterfacePairs3D::Pair> pairs;
    if (options.interface_pairs) {
        auto made = options.interface_pairs->generate(partition);
        if (!made) return std::unexpected(made.error());
        pairs = std::move(*made);
    }
    const Real exclusion = options.interface_pairs ? options.pair_exclusion_fraction * options.interface_pairs->spacing() : 0;
    PointGrid3 near_pairs(exclusion > 0 ? exclusion : 1);
    for (const auto& pr : pairs) {
        sites.add(pr.inside, pr.inside_region);
        sites.add(pr.outside, pr.outside_region);
        near_pairs.insert(pr.inside);
        near_pairs.insert(pr.outside);
    }
    for (const auto& spec : sources) {
        if (!spec.region.valid() || spec.region.index() >= partition.region_count() || !spec.source) {
            return fail(ErrorCode::InvalidArgument, "site source for an unknown region", spec.region);
        }
        auto surface = partition.region_surface(spec.region);
        if (!surface) return std::unexpected(surface.error());
        Random rng(splitmix64(options.seed ^ splitmix64(spec.region.index() << 32)));
        auto pts = spec.source(*surface, rng);
        if (!pts) return std::unexpected(pts.error());
        for (const Vec3& x : *pts) {
            if (exclusion > 0 && near_pairs.any_within(x, exclusion)) continue;
            sites.add(x, spec.region);
        }
    }
    if (auto ok = validate_sites_3d(partition, sites); !ok) return std::unexpected(ok.error());
    return sites;
}

Status validate_sites_3d(const Partition3D& partition, const SiteSet3D& sites) {
    const auto tol = Tolerance::from_length(partition.length_scale());
    if (!tol) return fail(ErrorCode::InvalidLengthScale, "empty partition");
    std::vector<TriangleSurface> surfaces;
    for (std::size_t r = 0; r < partition.region_count(); ++r) {
        auto s = partition.region_surface(RegionId::from_index(r));
        if (!s) return std::unexpected(s.error());
        surfaces.push_back(std::move(*s));
    }
    const auto pos = sites.positions();
    const auto reg = sites.regions();
    for (std::size_t i = 0; i < sites.size(); ++i) {
        const SiteId id = SiteId::from_index(i);
        const Vec3& p = pos[i];
        if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) {
            return fail(ErrorCode::SiteOutsideRegion, "not finite", id);
        }
        if (!reg[i].valid() || reg[i].index() >= surfaces.size()) return fail(ErrorCode::SiteOutsideRegion, "bad region", id);
        const TriangleSurface& s = surfaces[reg[i].index()];
        if (!s.contains(p) || s.distance(p) <= tol->point()) {
            return fail(ErrorCode::SiteOutsideRegion,
                        std::format("({}, {}, {}) in {}", p[0], p[1], p[2], partition.regions()[reg[i].index()].name), id);
        }
    }
    std::vector<std::size_t> order(sites.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) { return pos[a] < pos[b]; });
    for (std::size_t k = 1; k < order.size(); ++k) {
        if (pos[order[k]] == pos[order[k - 1]]) {
            const Vec3& p = pos[order[k]];
            return fail(ErrorCode::DuplicateSite, std::format("({}, {}, {})", p[0], p[1], p[2]), SiteId::from_index(order[k]));
        }
    }
    for (std::size_t r = 0; r < partition.region_count(); ++r) {
        if (sites.count(RegionId::from_index(r)) == 0) {
            return fail(ErrorCode::RegionWithoutSites, partition.regions()[r].name, RegionId::from_index(r));
        }
    }
    return {};
}

}  // namespace vmm
