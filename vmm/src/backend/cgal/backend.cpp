// ============================================================================
// File: backend.cpp
// Description: cgal_backend_2d() factory and version information.
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <string>

//==============================================================================
//  External libraries
//==============================================================================
#include <boost/version.hpp>
#include <CGAL/config.h>
#include <CGAL/version.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "backend_cgal.hpp"
#include <vmm/backend/cgal.hpp>

#if defined(VMM_EXPECTED_CGAL_VERSION_NR)
static_assert(CGAL_VERSION_NR == VMM_EXPECTED_CGAL_VERSION_NR,
              "CGAL headers differ from the CGAL package selected by CMake");
#endif

namespace vmm {

namespace cgal_detail {

BackendInfo info() {
#if defined(CGAL_USE_GMP)
    const char* exact = "GMP";
#else
    const char* exact = "Boost.Multiprecision";
#endif
    return {"cgal", std::string("CGAL ") + CGAL_VERSION_STR + "; Boost " + BOOST_LIB_VERSION + "; exact numbers " + exact};
}

}  // namespace cgal_detail

Backend2D cgal_backend_2d() {
    Backend2D b;
    b.build_partition = &cgal_detail::build_partition;
    b.delaunay_pairs = &cgal_detail::delaunay_pairs;
    b.clip_by_region = &cgal_detail::clip_by_region;
    b.info = &cgal_detail::info;
    return b;
}

Backend3D cgal_backend_3d() {
    Backend3D b;
    b.build_partition = &cgal_detail::build_partition_3d;
    b.prepare = &cgal_detail::prepare_3d;
    b.delaunay_pairs = &cgal_detail::delaunay_pairs_3d;
    b.circumcentre = &cgal_detail::circumcentre_3d;
    b.touches_boundary = &cgal_detail::touches_boundary_3d;
    b.clip_cell = &cgal_detail::clip_cell_3d;
    b.info = &cgal_detail::info;
    return b;
}

}  // namespace vmm
