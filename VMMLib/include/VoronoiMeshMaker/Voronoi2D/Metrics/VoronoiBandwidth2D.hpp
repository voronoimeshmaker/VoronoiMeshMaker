#pragma once
//==============================================================================
// Name        : VoronoiBandwidth2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Metrics
// Description : Matrix bandwidth metric for clipped 2D Voronoi diagrams.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VoronoiBandwidth2D.hpp
 * @brief Computes matrix bandwidth induced by Voronoi volume adjacency.
 */

#include <algorithm>
#include <cstddef>
#include <cstdlib>

#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Bandwidth information for the sparse matrix associated with a mesh.
 *
 * `max_volume_id_distance` is `max(abs(i - j))` over adjacent Voronoi
 * volumes. `matrix_bandwidth` is the conventional band width, equal to
 * `max_volume_id_distance + 1` when the diagram has at least one volume.
 */
struct VoronoiBandwidth2D {
    std::size_t max_volume_id_distance{0};
    std::size_t matrix_bandwidth{0};
    std::size_t adjacency_count{0};
};

/**
 * @brief Compute the matrix bandwidth using the current volume numbering.
 *
 * The function uses `volume_id`, not the storage position in `diagram.cells`.
 * Therefore, calling a renumbering routine before this function changes the
 * returned bandwidth, as expected for sparse matrix assembly.
 */
[[nodiscard]] inline VoronoiBandwidth2D compute_voronoi_bandwidth(
    const ClippedVoronoiDiagram2D& diagram)
{
    if (diagram.volume_count() == 0U) {
        return {};
    }

    std::size_t max_distance = 0;
    std::size_t unique_adjacencies = 0;

    for (const auto& volume : diagram.all_volumes()) {
        for (const auto neighbor_site_id : volume.neighbor_ids) {
            const auto& neighbor = diagram.cell(neighbor_site_id);

            const auto a = volume.volume_id;
            const auto b = neighbor.volume_id;

            if (a < b) {
                ++unique_adjacencies;
            }

            const auto distance = (a > b) ? (a - b) : (b - a);
            max_distance = std::max(max_distance, distance);
        }
    }

    return VoronoiBandwidth2D{
        max_distance,
        max_distance + 1U,
        unique_adjacencies
    };
}

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
