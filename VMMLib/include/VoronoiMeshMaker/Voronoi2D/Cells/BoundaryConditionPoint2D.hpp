#pragma once
//==============================================================================
// Name        : BoundaryConditionPoint2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Cells
// Description : Encapsulated boundary-condition application-point geometry.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file BoundaryConditionPoint2D.hpp
 * @brief Owns validated, equation-independent boundary application geometry.
 * @ingroup voronoi2d_cells
 */

#include <algorithm>
#include <cmath>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Core/constants.h>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Immutable boundary-condition geometry constructed from a support line.
 *
 * The application point is the unbounded normal projection of the generator.
 * It may lie outside both the finite face and the control volume.
 */
class BoundaryConditionPoint2D {
public:
    using Point2 = ::vmm::s2d::Point2;
    using Real = ::vmm::s2d::Real;

    BoundaryConditionPoint2D() = default;

    [[nodiscard]] static BoundaryConditionPoint2D from_support_line(
        Point2 generator,
        Point2 face_a,
        Point2 face_b,
        Point2 support_a,
        Point2 support_b) noexcept
    {
        BoundaryConditionPoint2D result;
        const Real sx = support_b.x - support_a.x;
        const Real sy = support_b.y - support_a.y;
        const Real support_length_squared = sx * sx + sy * sy;
        const Real support_length = std::hypot(sx, sy);
        const Real fx = face_b.x - face_a.x;
        const Real fy = face_b.y - face_a.y;
        const Real face_length = std::hypot(fx, fy);
        if (!(support_length > Real{0}) || !(face_length > Real{0})) {
            return result;
        }

        const Real tx = sx / support_length;
        const Real ty = sy / support_length;
        // Every valid boundary ring keeps the material domain on its left.
        result.outward_normal_ = {ty, -tx};
        result.perpendicular_distance_ =
            (support_a.x - generator.x) * result.outward_normal_.x
            + (support_a.y - generator.y) * result.outward_normal_.y;
        result.support_line_parameter_ =
            ((generator.x - support_a.x) * sx
             + (generator.y - support_a.y) * sy)
            / support_length_squared;
        // Preserve the historical arithmetic for projections on the finite
        // support segment, while deliberately leaving exterior projections
        // unclamped. This makes an equivalent V2 migration bit-inert.
        result.point_ = {
            support_a.x + result.support_line_parameter_ * sx,
            support_a.y + result.support_line_parameter_ * sy};
        result.face_parameter_ =
            ((result.point_.x - face_a.x) * fx
             + (result.point_.y - face_a.y) * fy)
            / (face_length * face_length);
        const Real face_a_offset =
            (face_a.x - support_a.x) * result.outward_normal_.x
            + (face_a.y - support_a.y) * result.outward_normal_.y;
        const Real face_b_offset =
            (face_b.x - support_a.x) * result.outward_normal_.x
            + (face_b.y - support_a.y) * result.outward_normal_.y;
        const Real tolerance = Real{64} * ::vmm::constants::kEpsilon;
        const Real support_tolerance = tolerance
            * std::max(support_length, Real{1});

        result.point_inside_local_face_ =
            result.face_parameter_ >= -tolerance
            && result.face_parameter_ <= Real{1} + tolerance;
        // Incidence against the identified parent support line remains stable
        // when a genuine clipped face is too short for a relative tangent test.
        result.normal_matches_face_ =
            std::abs(face_a_offset) <= support_tolerance
            && std::abs(face_b_offset) <= support_tolerance;
        result.valid_ = result.perpendicular_distance_ > Real{0}
            && result.normal_matches_face_;
        return result;
    }

    [[nodiscard]] const Point2& point() const & noexcept { return point_; }
    [[nodiscard]] Point2 point() const && noexcept { return point_; }
    [[nodiscard]] Point2 outward_normal() const noexcept { return outward_normal_; }
    [[nodiscard]] Real perpendicular_distance() const noexcept {
        return perpendicular_distance_;
    }
    [[nodiscard]] Real support_line_parameter() const noexcept {
        return support_line_parameter_;
    }
    [[nodiscard]] Real face_parameter() const noexcept {
        return face_parameter_;
    }
    [[nodiscard]] bool point_inside_local_face() const noexcept {
        return point_inside_local_face_;
    }
    [[nodiscard]] bool normal_matches_face() const noexcept {
        return normal_matches_face_;
    }
    [[nodiscard]] bool valid() const noexcept { return valid_; }

private:
    Point2 point_{};
    Point2 outward_normal_{};
    Real perpendicular_distance_{0};
    Real support_line_parameter_{0};
    Real face_parameter_{0};
    bool point_inside_local_face_{false};
    bool normal_matches_face_{false};
    bool valid_{false};
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
