#pragma once
//==============================================================================
// Name        : Site2DTransform.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Sites2D / Transforms
// Description : Affine transforms for SiteSet coordinates.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Site2DTransform.hpp
 * @brief Applies Boundary2D affine transforms to Site2D/SiteSet coordinates.
 */

#include <algorithm>
#include <execution>
#include <span>
#include <type_traits>

#include <VoronoiMeshMaker/Boundary2D/Transforms/Boundary2DTransform.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>

VORMAKER_NAMESPACE_OPEN
SITE2D_NAMESPACE_OPEN

/**
 * @brief Apply an affine transform to all site coordinates in-place.
 *
 * Site ids, regions and weights are preserved.
 */
inline void transform_in_place(SiteSet& sites,
                               ::vmm::b2d::Affine2 transform) noexcept
{
    for (auto& site : sites) {
        site.point = ::vmm::b2d::apply_transform(site.point, transform);
    }
}

template <class ExecutionPolicy>
requires std::is_execution_policy_v<std::remove_cvref_t<ExecutionPolicy>>
inline void transform_in_place(ExecutionPolicy&& policy,
                               SiteSet& sites,
                               ::vmm::b2d::Affine2 transform)
{
    auto view = sites.span();
    std::for_each(std::forward<ExecutionPolicy>(policy),
                  view.begin(),
                  view.end(),
                  [transform](Site2D& site) {
                      site.point = ::vmm::b2d::apply_transform(site.point, transform);
                  });
}

[[nodiscard]] inline SiteSet transformed(SiteSet sites,
                                         ::vmm::b2d::Affine2 transform)
{
    transform_in_place(sites, transform);
    return sites;
}

/**
 * @brief Rotate all sites around an arbitrary center.
 */
inline void rotate_in_place(SiteSet& sites,
                            Real angle_rad,
                            Point2 center = Point2{}) noexcept
{
    transform_in_place(
        sites,
        ::vmm::b2d::rotation_transform(angle_rad, center));
}

template <class ExecutionPolicy>
requires std::is_execution_policy_v<std::remove_cvref_t<ExecutionPolicy>>
inline void rotate_in_place(ExecutionPolicy&& policy,
                            SiteSet& sites,
                            Real angle_rad,
                            Point2 center = Point2{})
{
    transform_in_place(
        std::forward<ExecutionPolicy>(policy),
        sites,
        ::vmm::b2d::rotation_transform(angle_rad, center));
}

[[nodiscard]] inline SiteSet rotated(SiteSet sites,
                                    Real angle_rad,
                                    Point2 center = Point2{})
{
    rotate_in_place(sites, angle_rad, center);
    return sites;
}

SITE2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
