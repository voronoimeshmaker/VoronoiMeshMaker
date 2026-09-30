// ============================================================================
// File: registries.hpp
// Description: Run-time registries used by configuration files (DEC-040):
//              site sources by name (open, like ShapeRegistry) and the set
//              of shape and source registries a configuration is read with.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <functional>
#include <map>
#include <string>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/domain/shapes.hpp>
#include <vmm/domain/shapes3d.hpp>
#include <vmm/error/error.hpp>
#include <vmm/sites/sources.hpp>
#include <vmm/sites/sources3d.hpp>

namespace vmm {

/// @brief Open registry of 2D site sources by name. Pre-filled with:
///        uniform (spacing; min_distance, margin), count (count; margin), grid (spacing; origin, margin),
///        hexagonal (spacing; origin, margin) and explicit (xy: x0 y0 x1 y1 ...).
/// @par Level
/// Intermediate
/// @sa ShapeRegistry, ConfigRegistries
/// @par Location
/// vmm/app/registries.hpp
class SiteSourceRegistry2D {
public:
    using Factory = std::function<Result<RegionSites>(RegionId, const ShapeParameters&)>;

    [[nodiscard]] static SiteSourceRegistry2D with_builtin_sources();

    /// Fails on an empty name, an empty factory or a name already present.
    Status add(std::string name, Factory factory);
    [[nodiscard]] bool contains(const std::string& name) const { return factories_.contains(name); }
    /// The source `name` for `region`; InvalidArgument for an unknown name.
    [[nodiscard]] Result<RegionSites> make(const std::string& name, RegionId region,
                                           const ShapeParameters& parameters) const;

private:
    std::map<std::string, Factory> factories_;
};

/// @brief Open registry of 3D site sources by name. Pre-filled with:
///        uniform (spacing; min_distance, margin), count (count; margin), grid (spacing; origin, margin) and
///        explicit (xyz: x0 y0 z0 x1 y1 z1 ...).
/// @par Level
/// Intermediate
/// @sa ShapeRegistry3D, ConfigRegistries
/// @par Location
/// vmm/app/registries.hpp
class SiteSourceRegistry3D {
public:
    using Factory = std::function<Result<RegionSites3D>(RegionId, const ShapeParameters&)>;

    [[nodiscard]] static SiteSourceRegistry3D with_builtin_sources();

    Status add(std::string name, Factory factory);
    [[nodiscard]] bool contains(const std::string& name) const { return factories_.contains(name); }
    [[nodiscard]] Result<RegionSites3D> make(const std::string& name, RegionId region,
                                             const ShapeParameters& parameters) const;

private:
    std::map<std::string, Factory> factories_;
};

/// @brief The registries a configuration file is read with. Add shapes or site sources to them to use
///        your own names in configuration files.
/// @note The 3D shapes are the built-in ones plus `stl` (file; patch: name of the patch of a binary file) and
///       `extrusion` (xy, z: bottom and top; bottom, top, tag: patch names).
/// @par Level
/// Intermediate
/// @sa run_config
/// @par Location
/// vmm/app/registries.hpp
class ConfigRegistries {
public:
    [[nodiscard]] static ConfigRegistries with_builtins();

    ShapeRegistry shapes_2d;
    ShapeRegistry3D shapes_3d;
    SiteSourceRegistry2D sites_2d;
    SiteSourceRegistry3D sites_3d;
};

}  // namespace vmm
