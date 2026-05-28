#pragma once
//==============================================================================
// Name        : type.h
// Project     : VoronoiMeshMaker (VMM)
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 1.3
// Description : Common types used across the library (CGAL kernel, points,
//               segments, vectors, polygons, smart pointers, etc.).
// License     : GNU GPL v3
//==============================================================================

/**
 * @file type.h
 * @brief Defines common geometric and numeric types used in the library.
 *
 * This header centralises key computational-geometry types (2D/3D points,
 * segments, rays, vectors, polygons) and smart-pointer aliases.
 *
 * **Migration notes (v1.3):**
 *  - `LstPoint2D`, `LstPoint3D`, `LstInt`, `LstReal` are deprecated.
 *    Prefer the corresponding `Vec*` aliases — contiguous storage has far
 *    better cache behaviour for geometric data.
 *  - `GeometryType` enum has been removed; it was defined but never used
 *    inside the library. If you were using it externally, copy the definition
 *    into your own code.
 *  - 3D types (`Point3D`, `Segment3D`, `Ray3D`, `Vector3D`) remain available
 *    but the `Polyhedron3D` alias is still disabled pending the 3D module.
 *
 * @ingroup core
 */

//==============================================================================
//  C++ standard library
//==============================================================================
#include <memory>
#include <vector>

//==============================================================================
//  CGAL
//==============================================================================
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
// 3D support is intentionally disabled while the library is being stabilised
// around the 2D Voronoi mesh pipeline.  Re-enable with the 3D module and
// dedicated tests.
// #include <CGAL/Polyhedron_3.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/Core/namespace.h>

VORMAKER_NAMESPACE_OPEN

//------------------------------------------------------------------------------
// Geometric kernel and types  (vmm::gtp sub-namespace)
//------------------------------------------------------------------------------

GEOTYPES_NAMESPACE_OPEN

/// Kernel used in CGAL computations (robust predicates, fast constructions).
using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;

//--- 2D primitives ------------------------------------------------------------

using Point2D   = Kernel::Point_2;     ///< 2D point.
using Segment2D = Kernel::Segment_2;   ///< 2D directed segment.
using Ray2D     = Kernel::Ray_2;       ///< 2D ray.
using Vector2D  = Kernel::Vector_2;    ///< 2D vector.

//--- 3D primitives ------------------------------------------------------------

using Point3D   = Kernel::Point_3;     ///< 3D point.
using Segment3D = Kernel::Segment_3;   ///< 3D directed segment.
using Ray3D     = Kernel::Ray_3;       ///< 3D ray.
using Vector3D  = Kernel::Vector_3;    ///< 3D vector.

//--- Composite shapes ---------------------------------------------------------

using Polygon2D    = CGAL::Polygon_2<Kernel>; ///< Simple 2D polygon.
// 3D polyhedral storage will be restored with the future 3D module.
// using Polyhedron3D = CGAL::Polyhedron_3<Kernel>;

//--- Smart pointers -----------------------------------------------------------

using PtrPolygon2DUnique      = std::unique_ptr<Polygon2D>;
using PtrConstPolygon2DUnique = std::unique_ptr<const Polygon2D>;

using PtrPolygon2DShared      = std::shared_ptr<Polygon2D>;
using PtrConstPolygon2DShared = std::shared_ptr<const Polygon2D>;

//--- Vector containers (preferred) -------------------------------------------

using VecPoint2D = std::vector<Point2D>; ///< Contiguous collection of 2D points.
using VecPoint3D = std::vector<Point3D>; ///< Contiguous collection of 3D points.

//--- List containers (deprecated — prefer Vec* for cache efficiency) ----------

// /** @deprecated Use `VecPoint2D` instead. `std::list` has poor cache locality
//  *              for geometric data and is rarely the right container. */
// [[deprecated("Use VecPoint2D — std::list has poor cache locality for geometric data")]]
// using LstPoint2D = std::list<Point2D>;

// /** @deprecated Use `VecPoint3D` instead. */
// [[deprecated("Use VecPoint3D — std::list has poor cache locality for geometric data")]]
// using LstPoint3D = std::list<Point3D>;

GEOTYPES_NAMESPACE_CLOSE


//------------------------------------------------------------------------------
// Numeric types at the vmm level
//------------------------------------------------------------------------------

using Real = vmm::gtp::Kernel::FT;   ///< Floating-point type matching the CGAL kernel.
using Int  = vmm::gtp::Kernel::RT;   ///< Integer / exact ring type of the CGAL kernel.

//--- Numeric containers (preferred) ------------------------------------------

using VecInt  = std::vector<Int>;   ///< Contiguous collection of integers.
using VecReal = std::vector<Real>;  ///< Contiguous collection of reals.

//--- Numeric list containers (deprecated) ------------------------------------

// /** @deprecated Use `VecInt` instead. */
// [[deprecated("Use VecInt — std::list has poor cache locality")]]
// using LstInt  = std::list<Int>;

// /** @deprecated Use `VecReal` instead. */
// [[deprecated("Use VecReal — std::list has poor cache locality")]]
// using LstReal = std::list<Real>;

VORMAKER_NAMESPACE_CLOSE
