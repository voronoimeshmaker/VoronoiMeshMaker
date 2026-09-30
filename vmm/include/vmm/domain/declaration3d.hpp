// ============================================================================
// File: declaration3d.hpp
// Description: 3D domain declaration (P16): regions given by closed surfaces
//              (DEC-036), in declaration order as in 2D (DEC-018). Version 0.3
//              builds one region; precedence among several regions is P18.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/domain/declaration.hpp>
#include <vmm/domain/shapes3d.hpp>
#include <vmm/error/error.hpp>
#include <vmm/geometry/surface.hpp>

namespace vmm {

class Declaration3D {
public:
    /// A layer fills the inside of `surface` with `region`.
    struct Layer {
        RegionId region;
        TriangleSurface surface;
    };

    explicit Declaration3D(PolygonizeOptions3 options = {}) : options_(options) {}

    [[nodiscard]] MediumRegistry& media() noexcept { return media_; }
    [[nodiscard]] const MediumRegistry& media() const noexcept { return media_; }
    [[nodiscard]] const PolygonizeOptions3& polygonize_options() const noexcept { return options_; }

    template <Shape3D S>
    Result<RegionId> add_region(std::string name, MediumId medium, const S& shape) {
        auto surface = shape.surface(options_);
        if (!surface) return std::unexpected(surface.error());
        return add_region_surface(std::move(name), medium, std::move(*surface));
    }
    /// Fails on an empty or repeated name and on an unknown medium.
    Result<RegionId> add_region_surface(std::string name, MediumId medium, TriangleSurface surface);

    [[nodiscard]] const std::vector<Layer>& layers() const noexcept { return layers_; }
    [[nodiscard]] const std::vector<RegionInfo>& regions() const noexcept { return regions_; }

private:
    PolygonizeOptions3 options_;
    MediumRegistry media_;
    std::vector<Layer> layers_;
    std::vector<RegionInfo> regions_;
};

}  // namespace vmm
