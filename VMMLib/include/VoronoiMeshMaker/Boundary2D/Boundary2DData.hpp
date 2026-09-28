#pragma once
//==============================================================================
// Name        : Boundary2DData.hpp
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 0.4.0
// Description : Canonical polygon-with-holes (SoA/CSR) container for
//               Boundary2D. Fast path + safe APIs (throw/try_..)
//               wired into the project's ErrorHandling.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file Boundary2DData.hpp
 * @brief Canonical SoA/CSR storage for polygon-with-holes.
 *
 * Layout (CSR-like):
 *  - points[]     : all vertices, back-to-back.
 *  - ring_off[]   : offsets into points. Size == ring_count + 1 (sentinel).
 *  - kinds[]      : LoopKind per ring (Outer or Hole). Size == ring_count.
 *  - regions[]    : RegionId per ring (outers meaningful). Size == ring_count.
 *
 * Design:
 *  - Hot paths are guarded by VMM_ASSERT (debug-only throw).
 *  - Public safe APIs throw VMMException (ErrorHandling) or offer try_*.
 *  - No CGAL types here; adapters handle interop.
 */

//------------------------------------------------------------------------------
//      VoronoiMeshMaker includes
//------------------------------------------------------------------------------
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/ErrorHandling/Macros.h>      

VORMAKER_NAMESPACE_OPEN
BOUNDARY2D_NAMESPACE_OPEN

/**
 * @brief Canonical polygon-with-holes storage (SoA/CSR).
 *
 * Invariants:
 *  - ring_off[0] == 0
 *  - ring_off is non-decreasing
 *  - ring_off.back() == points.size()
 *  - kinds.size() == regions.size() == ring_count()
 */
struct Boundary2DData {
    //--- storage --------------------------------------------------------------
    std::vector<Point2>   points{};
    std::vector<Index>    ring_off{0};  // sentinel at index 0
    std::vector<LoopKind> kinds{};
    std::vector<RegionId> regions{};

    //--- basic queries (fast path) -------------------------------------------
    [[nodiscard]] Index ring_count() const noexcept {
        return static_cast<Index>(kinds.size());
    }

    [[nodiscard]] Index vertex_count() const noexcept {
        return static_cast<Index>(points.size());
    }

    /**
     * @brief View of ring i as a contiguous span of points.
     * @note  VMM_ASSERT (debug) guards; invalid 'i' is UB in Release.
     */
    [[nodiscard]] std::span<const Point2> ring(Index i) const {
        VMM_ASSERT(i >= 0 &&
                   static_cast<std::size_t>(i + 1) < ring_off.size());
        const auto b = static_cast<std::size_t>(
            ring_off[static_cast<std::size_t>(i)]);
        const auto e = static_cast<std::size_t>(
            ring_off[static_cast<std::size_t>(i + 1)]);
        return std::span<const Point2>(points.data() + b, e - b);
    }

    [[nodiscard]] std::span<Point2> ring(Index i) {
        VMM_ASSERT(i >= 0 &&
                   static_cast<std::size_t>(i + 1) < ring_off.size());
        const auto b = static_cast<std::size_t>(
            ring_off[static_cast<std::size_t>(i)]);
        const auto e = static_cast<std::size_t>(
            ring_off[static_cast<std::size_t>(i + 1)]);
        return std::span<Point2>(points.data() + b, e - b);
    }

    //--- safe accessors (throwing) -------------------------------------------
    /**
     * @brief Safe accessor: throws VMMException on invalid index.
     *        Uses CoreErr::OutOfRange and fills {index}.
     */
    [[nodiscard]] std::span<const Point2> ring_at(Index i) const {
        if (!in_bounds(i)) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"index", std::to_string(i)}});
        }
        return ring(i);
    }

    [[nodiscard]] std::span<Point2> ring_at(Index i) {
        if (!in_bounds(i)) {
            VMM_THROW(::vmm::error::CoreErr::OutOfRange,
                      {{"index", std::to_string(i)}});
        }
        return ring(i);
    }

    //--- non-throwing variant (useful in hot loops) --------------------------
    /**
     * @brief Non-throwing accessor. Returns false on invalid index.
     */
    [[nodiscard]] bool try_ring(Index i,
                                std::span<const Point2>& out) const noexcept {
        if (!in_bounds(i)) { out = {}; return false; }
        out = ring(i);
        return true;
    }

    [[nodiscard]] bool try_ring(Index i,
                                std::span<Point2>& out) noexcept {
        if (!in_bounds(i)) { out = {}; return false; }
        out = ring(i);
        return true;
    }

    //--- capacity -------------------------------------------------------------
    void reserve(Index points_cap, Index rings_cap) {
        if (points_cap > 0) {
            points.reserve(static_cast<std::size_t>(points_cap));
        }
        if (rings_cap > 0) {
            kinds.reserve(static_cast<std::size_t>(rings_cap));
            regions.reserve(static_cast<std::size_t>(rings_cap));
            const auto target = static_cast<std::size_t>(rings_cap) + 1U;
            if (ring_off.capacity() < target) ring_off.reserve(target);
        }
    }

    //--- append (fast) --------------------------------------------------------
    /**
     * @brief Append a ring, copying points and updating CSR offsets.
     * @warning No validation here (orientation/self-intersection).
     */
    void append_ring(std::span<const Point2> ring_pts,
                     LoopKind kind,
                     RegionId region) {
        const auto base = static_cast<Index>(points.size());
        points.insert(points.end(), ring_pts.begin(), ring_pts.end());
        kinds.push_back(kind);
        regions.push_back(region);
        ring_off.push_back(base + static_cast<Index>(ring_pts.size()));
        VMM_ASSERT(invariant_ok());
    }

    //--- append (checked) -----------------------------------------------------
    /**
     * @brief Checked append: validates minimal size, then appends.
     * @throws VMMException(CoreErr::InvalidArgument) if ring has < 3 vertices.
     */
    void append_ring_checked(std::span<const Point2> ring_pts,
                             LoopKind kind,
                             RegionId region) {
        if (ring_pts.size() < 3) {
            VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
                      {{"name", "ring(<3)"}});
        }
        append_ring(ring_pts, kind, region);
    }

    //--- clear/shrink ---------------------------------------------------------
    void clear() noexcept {
        points.clear();
        kinds.clear();
        regions.clear();
        ring_off.clear();
        ring_off.push_back(0);
    }

    void shrink_to_fit() {
        points.shrink_to_fit();
        kinds.shrink_to_fit();
        regions.shrink_to_fit();
        ring_off.shrink_to_fit();
    }

    //--- debug helpers --------------------------------------------------------
    [[nodiscard]] bool invariant_ok() const noexcept {
        const auto rc = ring_count();
        if (rc < 0) return false;
        if (ring_off.empty()) return false;
        if (static_cast<Index>(ring_off.size()) != rc + 1) return false;
        if (static_cast<Index>(kinds.size())    != rc)     return false;
        if (static_cast<Index>(regions.size())  != rc)     return false;
        // Non-decreasing offsets and final sentinel equals points.size()
        Index prev = 0;
        for (std::size_t i = 0; i < ring_off.size(); ++i) {
            const auto cur = ring_off[i];
            if (cur < prev) return false;
            prev = cur;
        }
        if (prev != static_cast<Index>(points.size())) return false;
        return true;
    }

private:
    [[nodiscard]] bool in_bounds(Index i) const noexcept {
        return (i >= 0) &&
               (static_cast<std::size_t>(i + 1) < ring_off.size());
    }
};

BOUNDARY2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
