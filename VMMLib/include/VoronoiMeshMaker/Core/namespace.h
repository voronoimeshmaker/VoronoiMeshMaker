#pragma once
//==============================================================================
// Name        : namespace.h
// Project     : VoronoiMeshMaker (VMM)
// Description : Centralized namespace helpers and Doxygen docs.
//               - Root   : vmm
//               - Module : vmm::b2d (Boundary2D)
//               - Geo    : vmm::gtp (GeoTypes)
//               - Const  : vmm::constants
//               - Error  : vmm::error
// License     : GNU GPL v3
// Version     : 1.4.0
//==============================================================================

/**
 * @file namespace.h
 * @brief Namespace utilities and macros for the VoronoiMeshMaker project.
 *
 * Root namespace: **vmm**.
 * Sub-namespaces:
 *  - **b2d**      : Boundary2D module
 *  - **s2d**      : Site2D module
 *  - **vd2d**     : Voronoi diagram 2D module
 *  - **gtp**      : GeoTypes (CGAL kernel/types)
 *  - **constants**: numerical constants
 *  - **error**    : error handling (exceptions, severities, catalogs)
 *
 * Provided macros:
 *  - VORMAKER_NAMESPACE_OPEN / VORMAKER_NAMESPACE_CLOSE
 *  - BOUNDARY2D_NAMESPACE_OPEN / BOUNDARY2D_NAMESPACE_CLOSE
 *  - SITE2D_NAMESPACE_OPEN / SITE2D_NAMESPACE_CLOSE
 *  - VORONOI2D_NAMESPACE_OPEN / VORONOI2D_NAMESPACE_CLOSE
 *  - DETAIL_NAMESPACE_OPEN / DETAIL_NAMESPACE_CLOSE
 *  - BOUNDARY2D_NAMESPACE_DETAIL_OPEN / BOUNDARY2D_NAMESPACE_DETAIL_CLOSE
 *  - GEOTYPES_NAMESPACE_OPEN / GEOTYPES_NAMESPACE_CLOSE
 *  - CONSTANTS_NAMESPACE_OPEN / CONSTANTS_NAMESPACE_CLOSE
 *  - ERROR_NAMESPACE_OPEN / ERROR_NAMESPACE_CLOSE
 */

//------------------------------------------------------------------------------
// Root: vmm
//------------------------------------------------------------------------------
#ifndef VORMAKER_NAMESPACE_OPEN
#  define VORMAKER_NAMESPACE_OPEN  namespace vmm {
#endif
#ifndef VORMAKER_NAMESPACE_CLOSE
#  define VORMAKER_NAMESPACE_CLOSE } /* namespace vmm */
#endif

//------------------------------------------------------------------------------
// Boundary2D: vmm::b2d
//------------------------------------------------------------------------------
#ifndef BOUNDARY2D_NAMESPACE_OPEN
#  define BOUNDARY2D_NAMESPACE_OPEN  namespace b2d {
#endif
#ifndef BOUNDARY2D_NAMESPACE_CLOSE
#  define BOUNDARY2D_NAMESPACE_CLOSE } /* namespace b2d */
#endif

//------------------------------------------------------------------------------
// Site2D: vmm::s2d
//------------------------------------------------------------------------------
#ifndef SITE2D_NAMESPACE_OPEN
#  define SITE2D_NAMESPACE_OPEN  namespace s2d {
#endif
#ifndef SITE2D_NAMESPACE_CLOSE
#  define SITE2D_NAMESPACE_CLOSE } /* namespace s2d */
#endif

//------------------------------------------------------------------------------
// Voronoi2D: vmm::vd2d
//------------------------------------------------------------------------------
#ifndef VORONOI2D_NAMESPACE_OPEN
#  define VORONOI2D_NAMESPACE_OPEN  namespace vd2d {
#endif
#ifndef VORONOI2D_NAMESPACE_CLOSE
#  define VORONOI2D_NAMESPACE_CLOSE } /* namespace vd2d */
#endif

//------------------------------------------------------------------------------
// detail helpers (generic)
//------------------------------------------------------------------------------
#ifndef DETAIL_NAMESPACE_OPEN
#  define DETAIL_NAMESPACE_OPEN  namespace detail {
#endif
#ifndef DETAIL_NAMESPACE_CLOSE
#  define DETAIL_NAMESPACE_CLOSE } /* namespace detail */
#endif

//------------------------------------------------------------------------------
// vmm::b2d::detail open/close in one step
//------------------------------------------------------------------------------
#ifndef BOUNDARY2D_NAMESPACE_DETAIL_OPEN
#  define BOUNDARY2D_NAMESPACE_DETAIL_OPEN \
     namespace vmm { namespace b2d { namespace detail {
#endif
#ifndef BOUNDARY2D_NAMESPACE_DETAIL_CLOSE
#  define BOUNDARY2D_NAMESPACE_DETAIL_CLOSE \
     } } } /* namespace vmm::b2d::detail */
#endif

//------------------------------------------------------------------------------
// GeoTypes: vmm::gtp
//------------------------------------------------------------------------------
#ifndef GEOTYPES_NAMESPACE_OPEN
#  define GEOTYPES_NAMESPACE_OPEN  namespace gtp {
#endif
#ifndef GEOTYPES_NAMESPACE_CLOSE
#  define GEOTYPES_NAMESPACE_CLOSE } /* namespace gtp */
#endif

//------------------------------------------------------------------------------
// Constants: vmm::constants
//------------------------------------------------------------------------------
#ifndef CONSTANTS_NAMESPACE_OPEN
#  define CONSTANTS_NAMESPACE_OPEN  namespace constants {
#endif
#ifndef CONSTANTS_NAMESPACE_CLOSE
#  define CONSTANTS_NAMESPACE_CLOSE } /* namespace constants */
#endif

//------------------------------------------------------------------------------
// Error handling: vmm::error
//------------------------------------------------------------------------------
#ifndef ERROR_NAMESPACE_OPEN
#  define ERROR_NAMESPACE_OPEN  namespace error {
#endif
#ifndef ERROR_NAMESPACE_CLOSE
#  define ERROR_NAMESPACE_CLOSE } /* namespace error */
#endif


//------------------------------------------------------------------------------
// Error handling: vmm::io
//------------------------------------------------------------------------------
#ifndef IO_NAMESPACE_OPEN
#  define IO_NAMESPACE_OPEN  namespace io {
#endif
#ifndef IO_NAMESPACE_CLOSE
#  define IO_NAMESPACE_CLOSE } /* namespace io */
#endif


//------------------------------------------------------------------------------
// I/O: vmm::io
//------------------------------------------------------------------------------
#ifndef IO_NAMESPACE_OPEN
#  define IO_NAMESPACE_OPEN  namespace io {
#endif
#ifndef IO_NAMESPACE_CLOSE
#  define IO_NAMESPACE_CLOSE } /* namespace io */
#endif
