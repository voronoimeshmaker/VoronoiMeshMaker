// ============================================================================
// File: shapes.hpp
// Description: 2D shapes of the domain declaration. A shape produces a
//              polygonal outline whose edges may carry a patch tag (the name
//              of the boundary patch they create). Extension is open: any
//              type satisfying the Shape2D concept is accepted, and the
//              ShapeRegistry builds shapes by name at run time.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/error/error.hpp>

namespace vmm {

struct PolygonizeOptions {
    int segments_per_curve = 64;  ///< segments used for a full circle or ellipse
};

/// Polygonal outline: outer ring (any orientation) and holes, each edge k of a
/// ring going from point k to point k+1 and carrying tag k ("" = no tag).
class ShapeOutline {
public:
    ShapeOutline() = default;
    /// Validates: at least 3 finite points per ring, tags sized like rings,
    /// non-zero area, and no edge touching or crossing another edge of the
    /// outline (rings simple, holes disjoint from the outer ring and from each
    /// other). Rings are re-oriented (outer CCW, holes CW) with their tags.
    [[nodiscard]] static Result<ShapeOutline> make(std::vector<Vec2> outer, std::vector<std::string> outer_tags,
                                                   std::vector<std::vector<Vec2>> holes = {},
                                                   std::vector<std::vector<std::string>> hole_tags = {});

    [[nodiscard]] const std::vector<Vec2>& outer() const noexcept { return outer_; }
    [[nodiscard]] const std::vector<std::string>& outer_tags() const noexcept { return outer_tags_; }
    [[nodiscard]] const std::vector<std::vector<Vec2>>& holes() const noexcept { return holes_; }
    [[nodiscard]] const std::vector<std::vector<std::string>>& hole_tags() const noexcept { return hole_tags_; }
    [[nodiscard]] Real area() const noexcept;

private:
    std::vector<Vec2> outer_;
    std::vector<std::string> outer_tags_;
    std::vector<std::vector<Vec2>> holes_;
    std::vector<std::vector<std::string>> hole_tags_;
};

template <class S>
concept Shape2D = requires(const S& s, const PolygonizeOptions& o) {
    { s.outline(o) } -> std::same_as<Result<ShapeOutline>>;
};

/// Axis-aligned rectangle. Tags: bottom, right, top, left.
class Rectangle {
public:
    Rectangle(Vec2 lo, Vec2 hi, std::array<std::string, 4> tags = {}) : lo_(lo), hi_(hi), tags_(std::move(tags)) {}
    [[nodiscard]] Result<ShapeOutline> outline(const PolygonizeOptions& options) const;

private:
    Vec2 lo_;
    Vec2 hi_;
    std::array<std::string, 4> tags_;
};

/// Simple polygon (with optional holes) and one tag per edge (or none).
class PolygonShape {
public:
    explicit PolygonShape(std::vector<Vec2> points, std::vector<std::string> tags = {},
                          std::vector<std::vector<Vec2>> holes = {})
        : points_(std::move(points)), tags_(std::move(tags)), holes_(std::move(holes)) {}
    [[nodiscard]] Result<ShapeOutline> outline(const PolygonizeOptions& options) const;

private:
    std::vector<Vec2> points_;
    std::vector<std::string> tags_;
    std::vector<std::vector<Vec2>> holes_;
};

/// Ellipse with semi-axes a, b rotated by `angle` (radians); one tag for all edges.
class Ellipse {
public:
    Ellipse(Vec2 center, Real a, Real b, Real angle = 0, std::string tag = {})
        : center_(center), a_(a), b_(b), angle_(angle), tag_(std::move(tag)) {}
    [[nodiscard]] Result<ShapeOutline> outline(const PolygonizeOptions& options) const;

private:
    Vec2 center_;
    Real a_;
    Real b_;
    Real angle_;
    std::string tag_;
};

class Circle {
public:
    Circle(Vec2 center, Real radius, std::string tag = {}) : ellipse_(center, radius, radius, 0, std::move(tag)) {}
    [[nodiscard]] Result<ShapeOutline> outline(const PolygonizeOptions& options) const {
        return ellipse_.outline(options);
    }

private:
    Ellipse ellipse_;
};

/// Regular polygon with n >= 3 vertices on a circle of the given radius.
class RegularNGon {
public:
    RegularNGon(Vec2 center, Real radius, int n, Real rotation = 0, std::string tag = {})
        : center_(center), radius_(radius), n_(n), rotation_(rotation), tag_(std::move(tag)) {}
    [[nodiscard]] Result<ShapeOutline> outline(const PolygonizeOptions& options) const;

private:
    Vec2 center_;
    Real radius_;
    int n_;
    Real rotation_;
    std::string tag_;
};

/// Numeric and textual parameters of a shape built by name.
struct ShapeParameters {
    std::map<std::string, std::vector<Real>> numbers;
    std::map<std::string, std::string> texts;
};

/// Open run-time registry: name -> factory. Pre-filled with rectangle, polygon,
/// circle, ellipse and regular_ngon; users may add their own.
class ShapeRegistry {
public:
    using Factory = std::function<Result<ShapeOutline>(const ShapeParameters&, const PolygonizeOptions&)>;

    [[nodiscard]] static ShapeRegistry with_builtin_shapes();

    Status add(std::string name, Factory factory);
    [[nodiscard]] bool contains(const std::string& name) const { return factories_.contains(name); }
    [[nodiscard]] Result<ShapeOutline> make(const std::string& name, const ShapeParameters& parameters,
                                            const PolygonizeOptions& options = {}) const;

private:
    std::map<std::string, Factory> factories_;
};

}  // namespace vmm
