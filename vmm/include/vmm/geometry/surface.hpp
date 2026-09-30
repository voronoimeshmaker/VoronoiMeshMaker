// ============================================================================
// File: surface.hpp
// Description: 3D geometry of the domain (DEC-036): axis-aligned box and the
//              closed, outward-oriented triangle surface with one patch per
//              triangle. Double precision only: exact checks (self-
//              intersection) belong to the backend.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/error/error.hpp>

namespace vmm {

/// Axis-aligned box; empty until the first point is added.
class Box3 {
public:
    Box3() = default;
    Box3(Vec3 lo, Vec3 hi) noexcept : lo_(lo), hi_(hi) {}

    [[nodiscard]] static Box3 of(std::span<const Vec3> points) noexcept;

    [[nodiscard]] bool empty() const noexcept { return lo_[0] > hi_[0] || lo_[1] > hi_[1] || lo_[2] > hi_[2]; }
    [[nodiscard]] const Vec3& lo() const noexcept { return lo_; }
    [[nodiscard]] const Vec3& hi() const noexcept { return hi_; }
    [[nodiscard]] Real diagonal() const noexcept { return empty() ? 0 : norm(hi_ - lo_); }

    void expand(const Vec3& p) noexcept;
    void expand(const Box3& b) noexcept;
    /// Grows every side by `margin` (may be negative).
    [[nodiscard]] Box3 inflated(Real margin) const noexcept;
    [[nodiscard]] bool contains(const Vec3& p) const noexcept;
    [[nodiscard]] bool overlaps(const Box3& b) const noexcept;

private:
    Vec3 lo_{1, 1, 1};
    Vec3 hi_{-1, -1, -1};
};

using Triangle = std::array<std::uint32_t, 3>;

/// Closed triangle surface, possibly with several components, oriented
/// outward; triangle t carries patch triangle_patch()[t] (an index into
/// patches()).
class TriangleSurface {
public:
    TriangleSurface() = default;

    /// @brief Builds and checks a closed surface.
    /// @param points Vertex coordinates (finite).
    /// @param triangles Vertex indices; each triangle with non-zero area.
    /// @param triangle_patch Patch of each triangle (index into `patches`).
    /// @param patches Patch names.
    /// @return The surface, re-oriented outward when its volume was negative, or InvalidSurface when a
    ///         triangle is degenerate or an edge is not shared by exactly two triangles in opposite
    ///         directions (open or inconsistently oriented surface), InvalidArgument for bad indices.
    /// @note Self-intersection is checked exactly by the backend (Backend3D::build_partition).
    /// @par Level
    /// Intermediate
    /// @sa Declaration3D, Partition3D
    /// @par Location
    /// vmm/geometry/surface.hpp
    [[nodiscard]] static Result<TriangleSurface> make(std::vector<Vec3> points, std::vector<Triangle> triangles,
                                                      std::vector<std::uint32_t> triangle_patch,
                                                      std::vector<std::string> patches);

    [[nodiscard]] const std::vector<Vec3>& points() const noexcept { return points_; }
    [[nodiscard]] const std::vector<Triangle>& triangles() const noexcept { return triangles_; }
    [[nodiscard]] const std::vector<std::uint32_t>& triangle_patch() const noexcept { return triangle_patch_; }
    [[nodiscard]] const std::vector<std::string>& patches() const noexcept { return patches_; }
    [[nodiscard]] std::size_t triangle_count() const noexcept { return triangles_.size(); }

    [[nodiscard]] Real area() const noexcept;
    [[nodiscard]] Real volume() const noexcept;
    [[nodiscard]] Box3 bounding_box() const noexcept { return Box3::of(points_); }
    /// Number of connected components (triangles linked through shared vertices).
    [[nodiscard]] std::size_t component_count() const;
    /// Generalized winding number above 1/2. Double precision: for site
    /// generation only, never for the topology of the mesh.
    [[nodiscard]] bool contains(const Vec3& p) const noexcept;
    /// Distance from p to the surface.
    [[nodiscard]] Real distance(const Vec3& p) const noexcept;

private:
    std::vector<Vec3> points_;
    std::vector<Triangle> triangles_;
    std::vector<std::uint32_t> triangle_patch_;
    std::vector<std::string> patches_;
};

/// Distance from p to the triangle (a, b, c).
[[nodiscard]] Real distance_to_triangle(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) noexcept;

}  // namespace vmm
