// ============================================================================
// File: declaration.hpp
// Description: Domain declaration by precedence (DEC-018): an ordered list
//              of layers, each a region or a hole; a later layer paints over
//              the earlier ones. Media form an open run-time registry.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/domain/shapes.hpp>
#include <vmm/error/error.hpp>

namespace vmm {

/// Open registry of media (materials, fluids...). Ids follow insertion order.
class MediumRegistry {
public:
    Result<MediumId> add(std::string name);
    [[nodiscard]] std::optional<MediumId> find(std::string_view name) const;
    [[nodiscard]] const std::string& name(MediumId id) const { return names_.at(id.index()); }
    [[nodiscard]] const std::vector<std::string>& names() const noexcept { return names_; }
    [[nodiscard]] std::size_t size() const noexcept { return names_.size(); }

private:
    std::vector<std::string> names_;
};

struct RegionInfo {
    std::string name;
    MediumId medium;
};

class Declaration2D {
public:
    /// A layer paints `outline` with `region`; an invalid region means a hole.
    struct Layer {
        RegionId region;
        ShapeOutline outline;
    };

    explicit Declaration2D(PolygonizeOptions options = {}) : options_(options) {}

    [[nodiscard]] MediumRegistry& media() noexcept { return media_; }
    [[nodiscard]] const MediumRegistry& media() const noexcept { return media_; }
    [[nodiscard]] const PolygonizeOptions& polygonize_options() const noexcept { return options_; }

    template <Shape2D S>
    Result<RegionId> add_region(std::string name, MediumId medium, const S& shape) {
        auto outline = shape.outline(options_);
        if (!outline) return std::unexpected(outline.error());
        return add_region_outline(std::move(name), medium, std::move(*outline));
    }

    template <Shape2D S>
    Status add_hole(const S& shape) {
        auto outline = shape.outline(options_);
        if (!outline) return std::unexpected(outline.error());
        return add_hole_outline(std::move(*outline));
    }

    Result<RegionId> add_region_outline(std::string name, MediumId medium, ShapeOutline outline);
    Status add_hole_outline(ShapeOutline outline);

    /// Declares the region that fills every void inside the domain's hull
    /// (faces covered by no layer). Without it, voids are validation errors.
    Result<RegionId> set_background(std::string name, MediumId medium);

    [[nodiscard]] const std::vector<Layer>& layers() const noexcept { return layers_; }
    [[nodiscard]] const std::vector<RegionInfo>& regions() const noexcept { return regions_; }
    [[nodiscard]] RegionId background() const noexcept { return background_; }

private:
    Result<RegionId> new_region(std::string name, MediumId medium);

    PolygonizeOptions options_;
    MediumRegistry media_;
    std::vector<Layer> layers_;
    std::vector<RegionInfo> regions_;
    RegionId background_ = RegionId::invalid();
};

}  // namespace vmm
