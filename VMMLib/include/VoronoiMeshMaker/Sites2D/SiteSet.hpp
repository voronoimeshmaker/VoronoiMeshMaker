#pragma once
//==============================================================================
// Name        : SiteSet.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Sites2D
// Description : Contiguous container for 2D Voronoi generator sites.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file SiteSet.hpp
 * @brief Defines the canonical container for 2D generator sites.
 *
 * `SiteSet` is intentionally a thin value container over `std::vector<Site2D>`.
 * It gives the Voronoi pipeline a stable public API while preserving contiguous
 * storage, `std::span` access and compatibility with STL algorithms.
 */

#include <span>
#include <string>
#include <vector>

#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Sites2D/Site2D.hpp>

VORMAKER_NAMESPACE_OPEN
SITE2D_NAMESPACE_OPEN

/**
 * @brief Contiguous collection of 2D Voronoi generator sites.
 *
 * Site ids are owned by the container. Every insertion assigns the next
 * sequential id, starting at zero, because finite-volume matrix assembly will
 * use those ids as compact row/column indices.
 */
struct SiteSet {
    std::vector<Site2D> sites{};

    [[nodiscard]] bool empty() const noexcept { return sites.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return sites.size(); }

    void clear() noexcept { sites.clear(); }
    void reserve(std::size_t n) { sites.reserve(n); }

    /**
     * @brief Append a fully formed site and assign the next sequential SiteId.
     */
    void push_back(Site2D site) {
        site.id = SiteId{static_cast<Index>(sites.size())};
        sites.push_back(site);
    }

    /**
     * @brief Append a point and assign the next sequential SiteId.
     *
     * @return Reference to the newly inserted site.
     */
    Site2D& add(Point2 point,
                RegionId region = RegionId{0},
                Real weight = Real{0})
    {
        const SiteId id{static_cast<Index>(sites.size())};
        sites.emplace_back(point, id, region, weight);
        return sites.back();
    }

    [[nodiscard]] std::span<const Site2D> span() const noexcept {
        return std::span<const Site2D>(sites.data(), sites.size());
    }

    [[nodiscard]] std::span<Site2D> span() noexcept {
        return std::span<Site2D>(sites.data(), sites.size());
    }

    [[nodiscard]] const Site2D& at(std::size_t i) const {
        if (i >= sites.size()) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"index", std::to_string(i)}});
        }
        return sites[i];
    }

    [[nodiscard]] Site2D& at(std::size_t i) {
        if (i >= sites.size()) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"index", std::to_string(i)}});
        }
        return sites[i];
    }

    [[nodiscard]] const Site2D& operator[](std::size_t i) const noexcept {
        return sites[i];
    }

    [[nodiscard]] Site2D& operator[](std::size_t i) noexcept {
        return sites[i];
    }

    auto begin() noexcept { return sites.begin(); }
    auto end() noexcept { return sites.end(); }
    auto begin() const noexcept { return sites.begin(); }
    auto end() const noexcept { return sites.end(); }

    /**
     * @brief Return true when `sites[i].id == SiteId{i}` for every site.
     */
    [[nodiscard]] bool ids_are_sequential() const noexcept {
        for (std::size_t i = 0; i < sites.size(); ++i) {
            if (sites[i].id.value != static_cast<Index>(i)) return false;
        }
        return true;
    }

    /**
     * @brief Rebuild ids as `0..size-1` without changing points or metadata.
     */
    void renumber_sequential() noexcept {
        for (std::size_t i = 0; i < sites.size(); ++i) {
            sites[i].id = SiteId{static_cast<Index>(i)};
        }
    }
};

SITE2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
