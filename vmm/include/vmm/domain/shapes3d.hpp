// ============================================================================
// File: shapes3d.hpp
// Description: 3D shapes (P16, DEC-036): each shape gives a closed triangle
//              surface with one patch per triangle. Curved shapes are
//              polygonized, as the curves in 2D. Open registry by name.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <concepts>
#include <functional>
#include <map>
#include <string>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/domain/shapes.hpp>
#include <vmm/error/error.hpp>
#include <vmm/geometry/surface.hpp>

namespace vmm {

struct PolygonizeOptions3 {
    int segments_per_circle = 64;  ///< segments of a full circle (cylinders, extruded curves)
    int sphere_subdivisions = 3;   ///< subdivisions of the icosahedron (20 * 4^k triangles)
};

template <class S>
concept Shape3D = requires(const S& s, const PolygonizeOptions3& o) {
    { s.surface(o) } -> std::same_as<Result<TriangleSurface>>;
};

/// Axis-aligned box. Tags (patches): x-, x+, y-, y+, z-, z+ ("" = boundary).
class Cuboid {
public:
    Cuboid(Vec3 lo, Vec3 hi, std::array<std::string, 6> tags = {}) : lo_(lo), hi_(hi), tags_(std::move(tags)) {}
    [[nodiscard]] Result<TriangleSurface> surface(const PolygonizeOptions3& options) const;

private:
    Vec3 lo_;
    Vec3 hi_;
    std::array<std::string, 6> tags_;
};

/// Sphere approximated by a subdivided icosahedron (vertices on the sphere).
class Sphere {
public:
    Sphere(Vec3 center, Real radius, std::string tag = {}) : center_(center), radius_(radius), tag_(std::move(tag)) {}
    [[nodiscard]] Result<TriangleSurface> surface(const PolygonizeOptions3& options) const;

private:
    Vec3 center_;
    Real radius_;
    std::string tag_;
};

/// A 2D outline (without holes) extruded along z from z0 to z1. The side
/// faces keep the tags of the 2D edges; the caps get their own tags.
class Extrusion {
public:
    Extrusion(ShapeOutline outline, Real z0, Real z1, std::string bottom_tag = {}, std::string top_tag = {})
        : outline_(std::move(outline)), z0_(z0), z1_(z1), bottom_(std::move(bottom_tag)), top_(std::move(top_tag)) {}
    [[nodiscard]] Result<TriangleSurface> surface(const PolygonizeOptions3& options) const;

private:
    ShapeOutline outline_;
    Real z0_;
    Real z1_;
    std::string bottom_;
    std::string top_;
};

/// Circular cylinder along z. Tags: side, bottom, top.
class Cylinder {
public:
    Cylinder(Vec3 base_center, Real radius, Real height, std::array<std::string, 3> tags = {})
        : base_(base_center), radius_(radius), height_(height), tags_(std::move(tags)) {}
    [[nodiscard]] Result<TriangleSurface> surface(const PolygonizeOptions3& options) const;

private:
    Vec3 base_;
    Real radius_;
    Real height_;
    std::array<std::string, 3> tags_;
};

/// A surface given by the user (already checked by TriangleSurface::make).
class SurfaceShape {
public:
    explicit SurfaceShape(TriangleSurface surface) : surface_(std::move(surface)) {}
    [[nodiscard]] Result<TriangleSurface> surface(const PolygonizeOptions3&) const { return surface_; }

private:
    TriangleSurface surface_;
};

/// Open run-time registry of 3D shapes: name -> factory. Pre-filled with
/// cuboid (lo, hi), sphere (center, radius) and cylinder (base, radius, height).
class ShapeRegistry3D {
public:
    using Factory = std::function<Result<TriangleSurface>(const ShapeParameters&, const PolygonizeOptions3&)>;
    [[nodiscard]] static ShapeRegistry3D with_builtin_shapes();
    Status add(std::string name, Factory factory);
    [[nodiscard]] bool contains(const std::string& name) const { return factories_.contains(name); }
    [[nodiscard]] Result<TriangleSurface> make(const std::string& name, const ShapeParameters& parameters,
                                               const PolygonizeOptions3& options = {}) const;

private:
    std::map<std::string, Factory> factories_;
};

}  // namespace vmm
