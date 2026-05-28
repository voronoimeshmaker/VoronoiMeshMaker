#pragma once
//==============================================================================
// Name        : VoronoiEdgeLengthDiagnostics2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Metrics
// Description : Diagnostics for short edges in clipped Voronoi diagrams.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VoronoiEdgeLengthDiagnostics2D.hpp
 * @brief Finds Voronoi cell edges shorter than a user supplied limit.
 */

#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCell2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief One Voronoi edge whose length is below the requested threshold.
 */
struct ShortVoronoiEdge2D {
    using Real = ::vmm::s2d::Real;
    using SiteId = ::vmm::s2d::SiteId;

    std::size_t volume_id{0};
    SiteId site_id{};
    std::size_t edge_index{0};
    Real length{0};
    bool is_boundary_edge{false};
    ::vmm::s2d::Point2 a{};
    ::vmm::s2d::Point2 b{};
};

/**
 * @brief Summary of edge-length checks over a clipped Voronoi diagram.
 */
struct VoronoiEdgeLengthDiagnostics2D {
    using Real = ::vmm::s2d::Real;

    Real requested_min_edge_length{0};
    Real minimum_edge_length{0};
    std::size_t edge_count{0};
    std::size_t short_edge_count{0};
    std::vector<ShortVoronoiEdge2D> short_edges{};

    [[nodiscard]] bool has_short_edges() const noexcept {
        return short_edge_count > 0U;
    }
};

[[nodiscard]] inline ::vmm::s2d::Real voronoi_edge_length(
    ::vmm::s2d::Point2 a,
    ::vmm::s2d::Point2 b) noexcept
{
    const auto dx = b.x - a.x;
    const auto dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * @brief Find all per-cell Voronoi edges shorter than `min_edge_length`.
 *
 * The function does not mutate the diagram. It uses the current volume
 * numbering and reports `volume_id` for direct use in matrix/finite-volume
 * diagnostics. Shared internal faces are reported once for each incident cell,
 * which is usually what is desired when checking the local control volumes.
 */
[[nodiscard]] inline VoronoiEdgeLengthDiagnostics2D
diagnose_voronoi_edge_lengths(const ClippedVoronoiDiagram2D& diagram,
                              ::vmm::s2d::Real min_edge_length)
{
    using Real = ::vmm::s2d::Real;

    if (!std::isfinite(min_edge_length) || min_edge_length < Real{0}) {
        VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                  {{"where", "diagnose_voronoi_edge_lengths"},
                   {"reason", "invalid_min_edge_length"},
                   {"value", std::to_string(static_cast<double>(min_edge_length))}});
    }

    VoronoiEdgeLengthDiagnostics2D report;
    report.requested_min_edge_length = min_edge_length;

    Real observed_min = std::numeric_limits<Real>::infinity();

    auto record_edge = [&](const VoronoiCell2D& cell,
                           std::size_t edge_index,
                           Real length,
                           bool is_boundary_edge,
                           ::vmm::s2d::Point2 a,
                           ::vmm::s2d::Point2 b) {
        ++report.edge_count;
        if (length < observed_min) {
            observed_min = length;
        }

        if (length < min_edge_length) {
            report.short_edges.push_back(ShortVoronoiEdge2D{
                cell.volume_id,
                cell.site_id,
                edge_index,
                length,
                is_boundary_edge,
                a,
                b
            });
        }
    };

    for (const auto& volume : diagram.all_volumes()) {
        if (!volume.edges.empty()) {
            for (std::size_t i = 0; i < volume.edges.size(); ++i) {
                const auto& edge = volume.edges[i];
                record_edge(volume,
                            i,
                            edge.length,
                            edge.is_boundary_edge,
                            edge.a,
                            edge.b);
            }
            continue;
        }

        const auto n = volume.polygon.size();
        if (n < 2U) {
            continue;
        }

        for (std::size_t i = 0; i < n; ++i) {
            const auto& a = volume.polygon[i];
            const auto& b = volume.polygon[(i + 1U) % n];
            record_edge(volume,
                        i,
                        voronoi_edge_length(a, b),
                        false,
                        a,
                        b);
        }
    }

    report.minimum_edge_length =
        (report.edge_count == 0U) ? Real{0} : observed_min;
    report.short_edge_count = report.short_edges.size();

    return report;
}

[[nodiscard]] inline bool voronoi_has_edges_shorter_than(
    const ClippedVoronoiDiagram2D& diagram,
    ::vmm::s2d::Real min_edge_length)
{
    return diagnose_voronoi_edge_lengths(
        diagram,
        min_edge_length).has_short_edges();
}

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
