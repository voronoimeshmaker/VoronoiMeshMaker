// ============================================================================
// File: sources.hpp
// Description: Site sources per region (P10 task 1), deterministic by seed
//              with the portable generator, and the optional interface-pair
//              policy (mirrored sites across interfaces, DEC-028).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/random.hpp>
#include <vmm/core/types.hpp>
#include <vmm/domain/partition.hpp>
#include <vmm/error/error.hpp>
#include <vmm/geometry/polygon.hpp>
#include <vmm/sites/site_set.hpp>

namespace vmm {

/// Local spacing h(x) of the sites.
using SpacingField = std::function<Real(const Vec2&)>;

/// A source fills one connected component of a region.
template <class S>
concept SiteSource = requires(const S& s, const PolygonWithHoles2& component, Random& rng) {
    { s.generate(component, rng) } -> std::same_as<Result<std::vector<Vec2>>>;
};

/// Dart throwing with constant spacing h: a candidate is kept when it lies
/// inside, at least margin * h from the boundary and at least
/// min_distance * h from every kept site.
class UniformRandomSource {
public:
    explicit UniformRandomSource(Real spacing) : spacing_(spacing) {}

    UniformRandomSource& min_distance_fraction(Real f) noexcept { min_distance_ = f; return *this; }
    UniformRandomSource& boundary_margin_fraction(Real f) noexcept { margin_ = f; return *this; }
    /// Stop after this many consecutive rejected candidates.
    UniformRandomSource& failure_limit(std::size_t n) noexcept { failure_limit_ = n; return *this; }

    [[nodiscard]] Result<std::vector<Vec2>> generate(const PolygonWithHoles2& component, Random& rng) const;

private:
    Real spacing_;
    Real min_distance_ = 0.6;
    Real margin_ = 0.25;
    std::size_t failure_limit_ = 3000;
};

/// Variable spacing h(x) by an adaptive quadtree: squares are split until
/// their side is at most h(center); each leaf gets one site at its centre
/// moved by up to jitter * side in each direction. O(N) and deterministic.
class AdaptiveQuadtreeSource {
public:
    AdaptiveQuadtreeSource(SpacingField spacing, Real min_spacing, Real jitter = 0.3, Real margin_fraction = 0.25)
        : spacing_(std::move(spacing)), min_spacing_(min_spacing), jitter_(jitter), margin_(margin_fraction) {}

    [[nodiscard]] Result<std::vector<Vec2>> generate(const PolygonWithHoles2& component, Random& rng) const;

private:
    SpacingField spacing_;
    Real min_spacing_;
    Real jitter_;
    Real margin_;
};

/// Exactly `count` uniformly distributed points (no minimum distance), as the
/// VMMLib UniformRandom2D generator; points closer than `margin` to the
/// boundary are redrawn.
class RandomCountSource {
public:
    explicit RandomCountSource(std::size_t count, Real margin = 0) : count_(count), margin_(margin) {}
    [[nodiscard]] Result<std::vector<Vec2>> generate(const PolygonWithHoles2& component, Random& rng) const;

private:
    std::size_t count_;
    Real margin_;
};

/// Square lattice with the given spacing, kept at margin * spacing from the boundary.
class CartesianGridSource {
public:
    explicit CartesianGridSource(Real spacing, Vec2 origin = {}, Real margin_fraction = 0.25)
        : spacing_(spacing), origin_(origin), margin_(margin_fraction) {}
    [[nodiscard]] Result<std::vector<Vec2>> generate(const PolygonWithHoles2& component, Random& rng) const;

private:
    Real spacing_;
    Vec2 origin_;
    Real margin_;
};

/// Triangular (hexagonal-cell) lattice with the given spacing.
class HexagonalGridSource {
public:
    explicit HexagonalGridSource(Real spacing, Vec2 origin = {}, Real margin_fraction = 0.25)
        : spacing_(spacing), origin_(origin), margin_(margin_fraction) {}
    [[nodiscard]] Result<std::vector<Vec2>> generate(const PolygonWithHoles2& component, Random& rng) const;

private:
    Real spacing_;
    Vec2 origin_;
    Real margin_;
};

/// User-given sites; each component keeps the ones inside it.
class ExplicitSites {
public:
    explicit ExplicitSites(std::vector<Vec2> sites) : sites_(std::move(sites)) {}
    [[nodiscard]] Result<std::vector<Vec2>> generate(const PolygonWithHoles2& component, Random& rng) const;

private:
    std::vector<Vec2> sites_;
};

/// A source bound to a region (type-erased, open to user sources).
struct RegionSites {
    RegionId region;
    std::function<Result<std::vector<Vec2>>(const PolygonWithHoles2&, Random&)> source;
};

template <SiteSource S>
[[nodiscard]] RegionSites sites_for(RegionId region, S source) {
    return {region, [s = std::move(source)](const PolygonWithHoles2& c, Random& rng) { return s.generate(c, rng); }};
}

/// Mirrored site pairs across every interface segment: points every `spacing`
/// along the segment, sites at +/- offset_fraction * spacing along the normal,
/// none closer than end_clearance_fraction * spacing to a segment end.
class InterfacePairs {
public:
    struct Pair {
        Vec2 left;
        RegionId left_region;
        Vec2 right;
        RegionId right_region;
    };

    explicit InterfacePairs(Real spacing, Real offset_fraction = 0.3, Real end_clearance_fraction = 1.0)
        : spacing_(spacing), offset_(offset_fraction), clearance_(end_clearance_fraction) {}

    [[nodiscard]] Result<std::vector<Pair>> generate(const Partition2D& partition) const;
    [[nodiscard]] Real spacing() const noexcept { return spacing_; }

private:
    Real spacing_;
    Real offset_;
    Real clearance_;
};

struct SiteGenerationOptions {
    std::uint64_t seed = 0;
    std::optional<InterfacePairs> interface_pairs;
    /// Sites of the region sources closer than this * pair spacing to a pair site are dropped.
    Real pair_exclusion_fraction = 0.75;
};

/// Runs the sources of every region (each component with its own seeded
/// generator), adds the interface pairs if requested, and validates the set.
[[nodiscard]] Result<SiteSet> generate_sites(const Partition2D& partition, std::span<const RegionSites> sources,
                                             const SiteGenerationOptions& options = {});

}  // namespace vmm
