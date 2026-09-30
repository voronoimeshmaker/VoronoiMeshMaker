// ============================================================================
// File: metrics.hpp
// Description: Finite-volume metrics (P11 task 3) and the quality report.
//              All quantities are derived from the mesh (base data) and kept
//              apart from it (R4).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/mesh/mesh.hpp>

namespace vmm {

template <std::size_t D>
struct Metrics {
    std::vector<Real> cell_measure;        ///< area (2D) / volume (3D)
    std::vector<Vec<D>> cell_centroid;
    std::vector<Real> cell_aspect_ratio;   ///< max centroid-vertex / min centroid-face distance
    std::vector<Vec<D>> face_area_vector;  ///< out of the owner
    std::vector<Vec<D>> face_centroid;
    std::vector<Real> face_measure;        ///< length (2D) / area (3D)
    /// Internal: |x_n - x_o| between generators. Boundary: distance from the
    /// owner's generator to the face line/plane.
    std::vector<Real> distance;
    /// Internal: point where x_o -> x_n crosses the face line/plane.
    /// Boundary: projection of x_o on the face line/plane.
    std::vector<Vec<D>> intersection;
    /// Angle between the area vector and x_n - x_o (boundary: face centroid - x_o).
    std::vector<Real> nonorthogonality;
    /// |face centroid - intersection| / distance.
    std::vector<Real> skewness;
};

/// @brief Finite-volume metrics of every cell and face.
/// @param mesh Mesh.
/// @return Cell measures, centroids and aspect ratios; face area vectors, centroids,
///         generator distances, intersection points, non-orthogonality and skewness.
/// @par Level
/// Beginner
/// @sa quality_report, check_invariants
/// @par Location
/// vmm/mesh/metrics.hpp
/// @par Examples
/// ex_anchor_a1.cpp, ex_anchor_a2.cpp
template <std::size_t D>
[[nodiscard]] Metrics<D> compute_metrics(const Mesh<D>& mesh);

struct QualityLimits {
    Real max_nonorthogonality = 1e-8;  ///< radians, internal faces of one region (DEC-020)
    Real max_skewness = 0.5;
    Real min_face_fraction = 1e-3;     ///< face measure / cell measure^((D-1)/D)
};

struct QualityRegion {
    Real max_nonortho_internal = 0;  ///< faces inside the region
    Real max_nonortho_interface = 0; ///< interface faces touching the region
    Real p99_nonortho_internal = 0;
    Real max_skewness = 0;
    Real p99_skewness = 0;
    Real max_aspect_ratio = 0;
    Real p99_aspect_ratio = 0;
    Real min_face_fraction = 0;
    std::size_t cells = 0;
};

struct QualityReport {
    std::vector<QualityRegion> regions;
    std::size_t nonortho_violations = 0;  ///< internal same-region faces above the limit
    std::size_t skewness_violations = 0;
    std::size_t short_faces = 0;
    [[nodiscard]] bool within_limits() const noexcept {
        return nonortho_violations == 0 && skewness_violations == 0 && short_faces == 0;
    }
};

template <std::size_t D>
[[nodiscard]] QualityReport quality_report(const Mesh<D>& mesh, const Metrics<D>& metrics,
                                           const QualityLimits& limits = {});

extern template Metrics<2> compute_metrics(const Mesh<2>&);
extern template Metrics<3> compute_metrics(const Mesh<3>&);
extern template QualityReport quality_report(const Mesh<2>&, const Metrics<2>&, const QualityLimits&);
extern template QualityReport quality_report(const Mesh<3>&, const Metrics<3>&, const QualityLimits&);

}  // namespace vmm
