// ============================================================================
// File: site_set.cpp
// Description: SiteSet and site validation.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <numeric>
#include <span>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/tolerance.hpp>
#include <vmm/sites/site_set.hpp>

namespace vmm {

SiteId SiteSet::add(Vec2 position, RegionId region) {
    positions_.push_back(position);
    regions_.push_back(region);
    return SiteId::from_index(positions_.size() - 1);
}

void SiteSet::append(std::span<const Vec2> positions, RegionId region) {
    for (const Vec2& p : positions) add(p, region);
}

std::size_t SiteSet::count(RegionId region) const noexcept {
    return static_cast<std::size_t>(std::ranges::count(regions_, region));
}

Status validate_sites(const Partition2D& partition, const SiteSet& sites) {
    const auto tol = Tolerance::from_length(partition.length_scale());
    if (!tol) return fail(ErrorCode::InvalidLengthScale, "empty partition");
    // Components as polygons, per region.
    std::vector<std::vector<PolygonWithHoles2>> polys(partition.region_count());
    for (std::size_t r = 0; r < partition.region_count(); ++r) {
        const RegionId id = RegionId::from_index(r);
        for (std::size_t c = 0; c < partition.components(id).size(); ++c) polys[r].push_back(partition.component_polygon(id, c));
    }
    std::vector<std::vector<char>> used(polys.size());
    for (std::size_t r = 0; r < polys.size(); ++r) used[r].assign(polys[r].size(), 0);

    const auto pos = sites.positions();
    const auto reg = sites.regions();
    for (std::size_t i = 0; i < sites.size(); ++i) {
        const SiteId id = SiteId::from_index(i);
        const Vec2& p = pos[i];
        if (!std::isfinite(p[0]) || !std::isfinite(p[1])) return fail(ErrorCode::SiteOutsideRegion, "not finite", id);
        if (!reg[i].valid() || reg[i].index() >= polys.size()) return fail(ErrorCode::SiteOutsideRegion, "bad region", id);
        bool inside = false;
        const auto& comps = polys[reg[i].index()];
        for (std::size_t c = 0; c < comps.size() && !inside; ++c) {
            if (comps[c].contains(p) && comps[c].distance_to_boundary(p) > tol->point()) {
                inside = true;
                used[reg[i].index()][c] = 1;
            }
        }
        if (!inside) {
            return fail(ErrorCode::SiteOutsideRegion, std::format("({}, {}) in {}", p[0], p[1],
                                                                  partition.regions()[reg[i].index()].name),
                        id);
        }
    }
    std::vector<std::size_t> order(sites.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) { return pos[a] < pos[b]; });
    for (std::size_t k = 1; k < order.size(); ++k) {
        if (pos[order[k]] == pos[order[k - 1]]) {
            return fail(ErrorCode::DuplicateSite, std::format("({}, {})", pos[order[k]][0], pos[order[k]][1]),
                        SiteId::from_index(order[k]));
        }
    }
    for (std::size_t r = 0; r < used.size(); ++r) {
        for (std::size_t c = 0; c < used[r].size(); ++c) {
            if (!used[r][c]) {
                return fail(ErrorCode::RegionWithoutSites,
                            std::format("{} component {}", partition.regions()[r].name, c), RegionId::from_index(r));
            }
        }
    }
    return {};
}

}  // namespace vmm
