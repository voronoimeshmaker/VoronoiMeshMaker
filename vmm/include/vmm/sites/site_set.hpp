// ============================================================================
// File: site_set.hpp
// Description: Sites (Voronoi generators) with their regions, in SoA, and
//              their validation against a partition.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/domain/partition.hpp>
#include <vmm/error/error.hpp>

namespace vmm {

class SiteSet {
public:
    SiteId add(Vec2 position, RegionId region);
    void append(std::span<const Vec2> positions, RegionId region);
    /// Optional weights (power diagrams, future): empty means all zero, at no cost.
    void set_weights(std::vector<Real> weights) { weights_ = std::move(weights); }

    [[nodiscard]] std::size_t size() const noexcept { return positions_.size(); }
    [[nodiscard]] bool empty() const noexcept { return positions_.empty(); }
    [[nodiscard]] std::span<const Vec2> positions() const noexcept { return positions_; }
    [[nodiscard]] std::span<const RegionId> regions() const noexcept { return regions_; }
    [[nodiscard]] std::span<const Real> weights() const noexcept { return weights_; }
    [[nodiscard]] std::size_t count(RegionId region) const noexcept;

private:
    std::vector<Vec2> positions_;
    std::vector<RegionId> regions_;
    std::vector<Real> weights_;
};

/// Checks every site: finite, strictly inside its region (farther than the
/// point tolerance from the region boundary), no duplicates, and at least one
/// site in every component of every region.
[[nodiscard]] Status validate_sites(const Partition2D& partition, const SiteSet& sites);

}  // namespace vmm
