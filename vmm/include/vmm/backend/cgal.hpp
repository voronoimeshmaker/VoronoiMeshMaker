// ============================================================================
// File: cgal.hpp
// Description: Factory of the CGAL backend. Declared without any CGAL type;
//              defined in vmm_backend_cgal (link that target to use it).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/backend2d.hpp>
#include <vmm/backend/backend3d.hpp>

namespace vmm {

[[nodiscard]] Backend2D cgal_backend_2d();
[[nodiscard]] Backend3D cgal_backend_3d();

}  // namespace vmm
