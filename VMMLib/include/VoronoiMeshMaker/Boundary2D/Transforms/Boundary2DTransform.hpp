#pragma once
//==============================================================================
// Name        : Boundary2DTransform.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Boundary2D
// Description : Translation, rotation about arbitrary centers, and affine
//               transforms for Boundary2D data.
// License     : GNU GPL v3
// Version     : 0.1.0
//==============================================================================

#include <algorithm>
#include <cmath>
#include <execution>
#include <span>
#include <type_traits>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>

VORMAKER_NAMESPACE_OPEN
BOUNDARY2D_NAMESPACE_OPEN

[[nodiscard]] constexpr Affine2 identity_transform() noexcept {
    return {};
}

[[nodiscard]] constexpr Affine2 translation_transform(Real dx,
                                                      Real dy) noexcept
{
    return Affine2{Real{1}, Real{0}, dx,
                   Real{0}, Real{1}, dy};
}

/**
 * @brief Build a rotation transform around an arbitrary center.
 *
 * @param angle_rad Rotation angle in radians, positive counter-clockwise.
 * @param center Fixed point of the rotation. Defaults to `(0,0)`, but any
 *        center can be supplied by the caller.
 */
[[nodiscard]] inline Affine2 rotation_transform(Real angle_rad,
                                                Point2 center = Point2{}) noexcept
{
    const Real c = std::cos(angle_rad);
    const Real s = std::sin(angle_rad);

    return Affine2{c, -s, center.x - c * center.x + s * center.y,
                   s,  c, center.y - s * center.x - c * center.y};
}

[[nodiscard]] constexpr Affine2 compose(Affine2 lhs, Affine2 rhs) noexcept {
    return Affine2{
        lhs.a00 * rhs.a00 + lhs.a01 * rhs.a10,
        lhs.a00 * rhs.a01 + lhs.a01 * rhs.a11,
        lhs.a00 * rhs.tx  + lhs.a01 * rhs.ty + lhs.tx,
        lhs.a10 * rhs.a00 + lhs.a11 * rhs.a10,
        lhs.a10 * rhs.a01 + lhs.a11 * rhs.a11,
        lhs.a10 * rhs.tx  + lhs.a11 * rhs.ty + lhs.ty
    };
}

[[nodiscard]] constexpr Point2 apply_transform(Point2 p,
                                                Affine2 transform) noexcept
{
    return Point2{
        transform.a00 * p.x + transform.a01 * p.y + transform.tx,
        transform.a10 * p.x + transform.a11 * p.y + transform.ty
    };
}

inline void transform_in_place(std::span<Point2> points,
                               Affine2 transform) noexcept
{
    std::for_each(points.begin(), points.end(), [transform](Point2& p) {
        p = apply_transform(p, transform);
    });
}

template <class ExecutionPolicy>
requires std::is_execution_policy_v<std::remove_cvref_t<ExecutionPolicy>>
inline void transform_in_place(ExecutionPolicy&& policy,
                               std::span<Point2> points,
                               Affine2 transform)
{
    std::for_each(std::forward<ExecutionPolicy>(policy),
                  points.begin(),
                  points.end(),
                  [transform](Point2& p) {
                      p = apply_transform(p, transform);
                  });
}

inline void transform_in_place(Boundary2DData& boundary,
                               Affine2 transform) noexcept
{
    transform_in_place(std::span<Point2>(boundary.points.data(),
                                         boundary.points.size()),
                       transform);
}

template <class ExecutionPolicy>
requires std::is_execution_policy_v<std::remove_cvref_t<ExecutionPolicy>>
inline void transform_in_place(ExecutionPolicy&& policy,
                               Boundary2DData& boundary,
                               Affine2 transform)
{
    transform_in_place(std::forward<ExecutionPolicy>(policy),
                       std::span<Point2>(boundary.points.data(),
                                         boundary.points.size()),
                       transform);
}

[[nodiscard]] inline Boundary2DData transformed(Boundary2DData boundary,
                                                Affine2 transform)
{
    transform_in_place(boundary, transform);
    return boundary;
}

template <class ExecutionPolicy>
requires std::is_execution_policy_v<std::remove_cvref_t<ExecutionPolicy>>
[[nodiscard]] inline Boundary2DData transformed(ExecutionPolicy&& policy,
                                                Boundary2DData boundary,
                                                Affine2 transform)
{
    transform_in_place(std::forward<ExecutionPolicy>(policy), boundary, transform);
    return boundary;
}

inline void translate_in_place(Boundary2DData& boundary,
                               Real dx,
                               Real dy) noexcept
{
    transform_in_place(boundary, translation_transform(dx, dy));
}

template <class ExecutionPolicy>
requires std::is_execution_policy_v<std::remove_cvref_t<ExecutionPolicy>>
inline void translate_in_place(ExecutionPolicy&& policy,
                               Boundary2DData& boundary,
                               Real dx,
                               Real dy)
{
    transform_in_place(std::forward<ExecutionPolicy>(policy),
                       boundary,
                       translation_transform(dx, dy));
}

[[nodiscard]] inline Boundary2DData translated(Boundary2DData boundary,
                                               Real dx,
                                               Real dy)
{
    translate_in_place(boundary, dx, dy);
    return boundary;
}

/**
 * @brief Rotate all vertices of a boundary around an arbitrary center.
 *
 * Topology, loop kinds and region ids are preserved. Only point coordinates are
 * modified. If `center` is omitted, the default rotation center is `(0,0)`.
 */
inline void rotate_in_place(Boundary2DData& boundary,
                            Real angle_rad,
                            Point2 center = Point2{}) noexcept
{
    transform_in_place(boundary, rotation_transform(angle_rad, center));
}

template <class ExecutionPolicy>
requires std::is_execution_policy_v<std::remove_cvref_t<ExecutionPolicy>>
inline void rotate_in_place(ExecutionPolicy&& policy,
                            Boundary2DData& boundary,
                            Real angle_rad,
                            Point2 center = Point2{})
{
    transform_in_place(std::forward<ExecutionPolicy>(policy),
                       boundary,
                       rotation_transform(angle_rad, center));
}

[[nodiscard]] inline Boundary2DData rotated(Boundary2DData boundary,
                                            Real angle_rad,
                                            Point2 center = Point2{})
{
    rotate_in_place(boundary, angle_rad, center);
    return boundary;
}

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
