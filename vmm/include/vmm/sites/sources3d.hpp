// ============================================================================
// File: sources3d.hpp
// Description: 3D sites (P16): the site set, site sources per region
//              (deterministic by seed with the portable generator) and the
//              site checks. Point-in-domain tests use the generalized winding
//              number in double: for generating sites only, never topology.
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
#include <vmm/domain/partition3d.hpp>
#include <vmm/error/error.hpp>
#include <vmm/geometry/surface.hpp>

namespace vmm {

/// Sites of a 3D mesh with their regions (the 3D counterpart of SiteSet).
class SiteSet3D {
public:
    SiteId add(Vec3 position, RegionId region);
    void append(std::span<const Vec3> positions, RegionId region);

    [[nodiscard]] std::size_t size() const noexcept { return positions_.size(); }
    [[nodiscard]] bool empty() const noexcept { return positions_.empty(); }
    [[nodiscard]] std::span<const Vec3> positions() const noexcept { return positions_; }
    [[nodiscard]] std::span<const RegionId> regions() const noexcept { return regions_; }
    [[nodiscard]] std::size_t count(RegionId region) const noexcept;

private:
    std::vector<Vec3> positions_;
    std::vector<RegionId> regions_;
};

/// A source fills one region, given by its closed surface.
template <class S>
concept SiteSource3D = requires(const S& s, const TriangleSurface& region, Random& rng) {
    { s.generate(region, rng) } -> std::same_as<Result<std::vector<Vec3>>>;
};

/// Dart throwing with constant spacing h: a candidate is kept when it lies
/// inside, at least margin * h from the surface and at least min_distance * h
/// from every kept site.
class UniformRandomSource3D {
public:
    explicit UniformRandomSource3D(Real spacing) : spacing_(spacing) {}

    UniformRandomSource3D& min_distance_fraction(Real f) noexcept { min_distance_ = f; return *this; }
    UniformRandomSource3D& boundary_margin_fraction(Real f) noexcept { margin_ = f; return *this; }
    /// Stop after this many consecutive rejected candidates.
    UniformRandomSource3D& failure_limit(std::size_t n) noexcept { failure_limit_ = n; return *this; }

    [[nodiscard]] Result<std::vector<Vec3>> generate(const TriangleSurface& region, Random& rng) const;

private:
    Real spacing_;
    Real min_distance_ = 0.6;
    Real margin_ = 0.25;
    std::size_t failure_limit_ = 3000;
};

/// Exactly `count` uniformly distributed points inside (no minimum distance);
/// points closer than `margin` to the surface are redrawn.
class RandomCountSource3D {
public:
    explicit RandomCountSource3D(std::size_t count, Real margin = 0) : count_(count), margin_(margin) {}
    [[nodiscard]] Result<std::vector<Vec3>> generate(const TriangleSurface& region, Random& rng) const;

private:
    std::size_t count_;
    Real margin_;
};

/// Cubic lattice with the given spacing, kept at margin * spacing from the surface.
class CartesianGridSource3D {
public:
    explicit CartesianGridSource3D(Real spacing, Vec3 origin = {}, Real margin_fraction = 0.25)
        : spacing_(spacing), origin_(origin), margin_(margin_fraction) {}
    [[nodiscard]] Result<std::vector<Vec3>> generate(const TriangleSurface& region, Random& rng) const;

private:
    Real spacing_;
    Vec3 origin_;
    Real margin_;
};

/// User-given sites; the region keeps the ones inside it.
class ExplicitSites3D {
public:
    explicit ExplicitSites3D(std::vector<Vec3> sites) : sites_(std::move(sites)) {}
    [[nodiscard]] Result<std::vector<Vec3>> generate(const TriangleSurface& region, Random& rng) const;

private:
    std::vector<Vec3> sites_;
};

/// A source bound to a region (type-erased, open to user sources).
struct RegionSites3D {
    RegionId region;
    std::function<Result<std::vector<Vec3>>(const TriangleSurface&, Random&)> source;
};

template <SiteSource3D S>
[[nodiscard]] RegionSites3D sites_for_3d(RegionId region, S source) {
    return {region, [s = std::move(source)](const TriangleSurface& r, Random& rng) { return s.generate(r, rng); }};
}

/// Mirrored site pairs across the interfaces (DEC-028, E2; P18): points every
/// `spacing` on each interface triangle, one site on each side at about
/// offset_fraction * spacing along the normal (varied by up to 20 % from pair to
/// pair, deterministically, so that the sites of a curved interface are not cospherical). Their bisector is the triangle
/// plane, so the interface face between the two sites is orthogonal. A pair is
/// kept only where the interface is the nearest surface of both sites.
class InterfacePairs3D {
public:
    struct Pair {
        Vec3 inside;
        RegionId inside_region;
        Vec3 outside;
        RegionId outside_region;
    };
    explicit InterfacePairs3D(Real spacing, Real offset_fraction = 0.25) : spacing_(spacing), offset_(offset_fraction) {}
    [[nodiscard]] Real spacing() const noexcept { return spacing_; }
    [[nodiscard]] Result<std::vector<Pair>> generate(const Partition3D& partition) const;

private:
    Real spacing_;
    Real offset_;
};

struct SiteGenerationOptions3D {
    std::uint64_t seed = 0;
    std::optional<InterfacePairs3D> interface_pairs;
    /// Sites of the region sources closer than this * pair spacing to a pair site are dropped.
    Real pair_exclusion_fraction = 0.75;
};

/// @brief Generates the sites of every region of a 3D partition.
/// @param partition Partition (from Backend3D::build_partition).
/// @param sources One or more sources per region; each region needs at least one site.
/// @param options Seed of the portable generator (same seed, same sites on every platform).
/// @return The sites, checked by validate_sites_3d, or the first error.
/// @par Level
/// Beginner
/// @sa validate_sites_3d, build_mesh_3d, generate_mesh_3d
/// @par Location
/// vmm/sites/sources3d.hpp
[[nodiscard]] Result<SiteSet3D> generate_sites_3d(const Partition3D& partition, std::span<const RegionSites3D> sources,
                                                  const SiteGenerationOptions3D& options = {});

/// Checks every site: finite, strictly inside its region (farther than the
/// point tolerance from its surface), no duplicates, every region with sites.
[[nodiscard]] Status validate_sites_3d(const Partition3D& partition, const SiteSet3D& sites);

}  // namespace vmm
