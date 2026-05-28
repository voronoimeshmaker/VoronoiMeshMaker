#pragma once
//==============================================================================
// Name        : VoronoiGenerationTimer2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Metrics
// Description : Timing helper for the 2D Voronoi generation pipeline.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VoronoiGenerationTimer2D.hpp
 * @brief Measures Boundary2D, Site2D and Voronoi2D generation phases.
 */

#include <chrono>
#include <functional>
#include <type_traits>
#include <utility>

#include <VoronoiMeshMaker/Core/namespace.h>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Generation times in milliseconds.
 */
struct VoronoiGenerationTimings2D {
    double boundary2d_ms{0.0};
    double sites2d_ms{0.0};
    double voronoi2d_ms{0.0};
};

/**
 * @brief Result of a timed 2D generation pipeline.
 */
template <class Boundary, class Sites, class Diagram>
struct TimedVoronoiGeneration2D {
    Boundary boundary;
    Sites sites;
    Diagram diagram;
    VoronoiGenerationTimings2D timings{};
};

/**
 * @brief Measures the three logical phases of a 2D Voronoi mesh pipeline.
 *
 * The factories are user-provided callables so the timer can be reused by
 * examples, paper programs, benchmarks and external applications without
 * coupling it to one specific boundary or site pattern.
 *
 * Expected signatures:
 * - `boundary_factory() -> Boundary2DData`
 * - `sites_factory(boundary) -> SiteSet`
 * - `diagram_factory(boundary, sites) -> ClippedVoronoiDiagram2D`
 *
 * File writing/export is intentionally outside this class.
 */
struct VoronoiGenerationTimer2D {
    using Clock = std::chrono::steady_clock;

    template <class BoundaryFactory,
              class SitesFactory,
              class DiagramFactory>
    [[nodiscard]] static auto measure(BoundaryFactory&& boundary_factory,
                                      SitesFactory&& sites_factory,
                                      DiagramFactory&& diagram_factory)
    {
        VoronoiGenerationTimings2D timings{};

        const auto boundary_start = Clock::now();
        auto boundary = std::invoke(
            std::forward<BoundaryFactory>(boundary_factory));
        timings.boundary2d_ms = elapsed_ms(boundary_start, Clock::now());

        const auto sites_start = Clock::now();
        auto sites = std::invoke(
            std::forward<SitesFactory>(sites_factory),
            boundary);
        timings.sites2d_ms = elapsed_ms(sites_start, Clock::now());

        const auto voronoi_start = Clock::now();
        auto diagram = std::invoke(
            std::forward<DiagramFactory>(diagram_factory),
            boundary,
            sites);
        timings.voronoi2d_ms = elapsed_ms(voronoi_start, Clock::now());

        using Boundary = std::decay_t<decltype(boundary)>;
        using Sites = std::decay_t<decltype(sites)>;
        using Diagram = std::decay_t<decltype(diagram)>;

        return TimedVoronoiGeneration2D<Boundary, Sites, Diagram>{
            std::move(boundary),
            std::move(sites),
            std::move(diagram),
            timings
        };
    }

private:
    [[nodiscard]] static double elapsed_ms(Clock::time_point start,
                                           Clock::time_point stop)
    {
        return std::chrono::duration<double, std::milli>(stop - start).count();
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
