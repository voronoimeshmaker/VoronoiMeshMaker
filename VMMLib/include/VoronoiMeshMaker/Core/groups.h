#pragma once
//==============================================================================
// Name        : groups.h
// Project     : VoronoiMeshMaker (VMM)
// Description : Central Doxygen group definitions for the entire library.
//               Every module header should add @ingroup <group-id> to its
//               @file block and to each public symbol.
// License     : GNU GPL v3
// Version     : 1.0.0
//==============================================================================

/**
 * @file groups.h
 * @brief Defines the complete Doxygen group hierarchy for VoronoiMeshMaker.
 *
 * Include this file in your Doxyfile INPUT list.  It contains no code —
 * only Doxygen documentation comments that establish the module groups used
 * throughout the library.
 *
 * Sphinx users: configure the `breathe` extension and reference groups with
 * @code{.rst}
 *   .. doxygengroup:: voronoi2d
 *      :project: VoronoiMeshMaker
 *      :members:
 * @endcode
 */

//------------------------------------------------------------------------------
// Root library group
//------------------------------------------------------------------------------

/**
 * @defgroup vmm VoronoiMeshMaker
 * @brief Root group for the entire VoronoiMeshMaker (VMM) library.
 *
 * VoronoiMeshMaker generates 2D clipped Voronoi meshes suitable for
 * finite-volume discretisations. The library is organised as a layered
 * pipeline:
 *
 *  1. **Core**        — primitive types, constants, namespace macros.
 *  2. **Boundary2D**  — 2D domain boundary (polygon-with-holes, shapes,
 *                       queries).
 *  3. **Sites2D**     — generator site containers and distribution
 *                       factories.
 *  4. **Voronoi2D**   — complete Delaunay-based clipped Voronoi pipeline.
 *  5. **IO**          — VTK and other format writers.
 *
 * @{
 */

//------------------------------------------------------------------------------
// Core
//------------------------------------------------------------------------------

/**
 * @defgroup core Core
 * @ingroup vmm
 * @brief Primitive types, numerical constants and namespace utilities.
 *
 * This group contains the building blocks shared by every other module:
 *  - `type.h`      — CGAL kernel aliases, point/vector/polygon types and
 *                    smart-pointer aliases.
 *  - `constants.h` — numerical thresholds (kZeroTol, kEpsilon, kPi …).
 *  - `namespace.h` — macro helpers for the vmm / b2d / s2d / vd2d namespaces.
 *  - `groups.h`    — this file (Doxygen group definitions only).
 */

//------------------------------------------------------------------------------
// Boundary2D
//------------------------------------------------------------------------------

/**
 * @defgroup boundary2d Boundary2D
 * @ingroup vmm
 * @brief 2D domain boundary representation (polygon-with-holes).
 *
 * Provides:
 *  - **Boundary2DData** — compact CSR storage for multi-ring polygons.
 *  - **Shapes**         — parametric shapes (Rectangle, Circle, Ellipse, …).
 *  - **Builders**       — Boundary2DBuilder assembles data from shapes.
 *  - **Queries**        — point-in-polygon, distance-to-boundary.
 *  - **Validation**     — orientation, self-intersection, minimum-vertex
 *                         checks.
 *  - **Transforms**     — translation, rotation, scaling of boundary data.
 *  - **Concepts**       — C++20 concepts (Point2Like, Shape2DLike).
 *
 * @{
 */

/** @defgroup boundary2d_shapes    Shapes
 *  @ingroup  boundary2d
 *  @brief    Parametric 2D shape types (Rectangle, Circle, Ellipse, …). */

/** @defgroup boundary2d_builders  Builders
 *  @ingroup  boundary2d
 *  @brief    Factory helpers that assemble Boundary2DData from shapes. */

/** @defgroup boundary2d_queries   Queries
 *  @ingroup  boundary2d
 *  @brief    Point-containment and distance queries on Boundary2DData. */

/** @defgroup boundary2d_validation Validation
 *  @ingroup  boundary2d
 *  @brief    Invariant checks on Boundary2DData instances. */

/** @defgroup boundary2d_transforms Transforms
 *  @ingroup  boundary2d
 *  @brief    Rigid and affine transforms applied to Boundary2DData. */

/** @} */ // boundary2d

//------------------------------------------------------------------------------
// Sites2D
//------------------------------------------------------------------------------

/**
 * @defgroup sites2d Sites2D
 * @ingroup vmm
 * @brief Generator site containers and distribution factories.
 *
 * Provides:
 *  - **Site2D**        — lightweight value type (point, id, region, weight).
 *  - **SiteSet**       — contiguous container with sequential-id management.
 *  - **SiteFactory**   — pattern-based factories (CartesianGrid, Hexagonal,
 *                        TriangularII/IV, UniformRandom, …).
 *  - **SiteValidation** — boundary-containment and spacing checks.
 *  - **Transforms**    — site coordinate transformations.
 *
 * @{
 */

/** @defgroup sites2d_factory    Factory
 *  @ingroup  sites2d
 *  @brief    Pattern types and make_sites/append_sites overloads. */

/** @defgroup sites2d_validation Validation
 *  @ingroup  sites2d
 *  @brief    Checks that sites are inside the boundary and respect spacings. */

/** @defgroup sites2d_transforms Transforms
 *  @ingroup  sites2d
 *  @brief    Coordinate transforms applied to SiteSet instances. */

/** @} */ // sites2d

//------------------------------------------------------------------------------
// Voronoi2D
//------------------------------------------------------------------------------

/**
 * @defgroup voronoi2d Voronoi2D
 * @ingroup vmm
 * @brief Complete 2D clipped Voronoi diagram construction pipeline.
 *
 * The pipeline proceeds in the following stages:
 *
 *  1. **Traits**   — canonical CGAL type aliases (`CgalKernelTraits2D`).
 *  2. **Delaunay** — triangulation builder and site-index.
 *  3. **Clipping** — halfplane clipping (Sutherland-Hodgman) and boundary-cell
 *                    detection (Yan et al. FIFO propagation).
 *  4. **Cells**    — per-cell polygon builder (`VoronoiCellBuilder2D`).
 *  5. **Diagram**  — full diagram container and builder
 *                    (`ClippedVoronoiDiagram2D`, `ClippedVoronoiBuilder2D`).
 *  6. **Ordering** — volume renumbering policies (Hilbert, Lexicographic).
 *  7. **Metrics**  — matrix bandwidth, generation timer.
 *
 * @{
 */

/** @defgroup voronoi2d_traits   Traits
 *  @ingroup  voronoi2d
 *  @brief    Canonical CGAL kernel and triangulation type aliases. */

/** @defgroup voronoi2d_delaunay Delaunay
 *  @ingroup  voronoi2d
 *  @brief    Delaunay triangulation builder, site index and neighbor queries. */

/** @defgroup voronoi2d_clipping Clipping
 *  @ingroup  voronoi2d
 *  @brief    Halfplane clipping and boundary-cell detection. */

/** @defgroup voronoi2d_cells    Cells
 *  @ingroup  voronoi2d
 *  @brief    Per-cell Voronoi polygon construction. */

/** @defgroup voronoi2d_diagram  Diagram
 *  @ingroup  voronoi2d
 *  @brief    Complete clipped Voronoi diagram container and builder. */

/** @defgroup voronoi2d_ordering Ordering
 *  @ingroup  voronoi2d
 *  @brief    Volume renumbering policies (Hilbert, Lexicographic, Custom). */

/** @defgroup voronoi2d_metrics  Metrics
 *  @ingroup  voronoi2d
 *  @brief    Matrix bandwidth and pipeline timing utilities. */

/** @} */ // voronoi2d

//------------------------------------------------------------------------------
// IO
//------------------------------------------------------------------------------

/**
 * @defgroup io IO
 * @ingroup vmm
 * @brief Writers for external mesh and visualisation formats.
 *
 * Currently provided formats:
 *  - **VTK Legacy ASCII POLYDATA** — boundary, sites, Delaunay edges, and
 *    Voronoi cells (with optional scalar / vector fields per volume).
 *
 * The IO layer depends only on the Voronoi2D public API and CGAL number
 * utilities; it has no reverse dependency on the core pipeline.
 *
 * @{
 */

/** @defgroup io_boundary2d IO / Boundary2D
 *  @ingroup  io
 *  @brief    Writers for Boundary2D data (VTK poly-lines and polygons). */

/** @defgroup io_voronoi2d  IO / Voronoi2D
 *  @ingroup  io
 *  @brief    Writers for the complete clipped Voronoi diagram. */

/** @defgroup io_sites2d    IO / Sites2D
 *  @ingroup  io
 *  @brief    Writers for generator site sets. */

/** @} */ // io

/** @} */ // vmm
