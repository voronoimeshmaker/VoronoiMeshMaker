#pragma once
//==============================================================================
// Name        : ClippingWorkspace2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Clipping
// Description : Reusable buffers for halfplane clipping.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file ClippingWorkspace2D.hpp
 * @brief Reusable polygon buffers used while clipping Voronoi cells.
 */

#include <vector>

#include <VoronoiMeshMaker/Sites2D/Site2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Scratch buffers for repeated polygon clipping.
 */
struct ClippingWorkspace2D {
    using Point2 = ::vmm::s2d::Point2;

    std::vector<Point2> input{};
    std::vector<Point2> output{};

    void clear() noexcept {
        input.clear();
        output.clear();
    }

    void reserve(std::size_t n) {
        input.reserve(n);
        output.reserve(n);
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
