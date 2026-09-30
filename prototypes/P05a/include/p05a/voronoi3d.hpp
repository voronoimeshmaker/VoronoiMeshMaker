// ============================================================================
// File: voronoi3d.hpp
// Description: P05a prototype - minimal 3D proof: Voronoi cells of a box as
//              intersections of half-spaces (neighbours from the backend's
//              Delaunay 3D), stored in the same PolyMesh used in 2D.
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <span>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/csr.hpp>
#include <p05a/mesh.hpp>
#include <p05a/types.hpp>

namespace vmm::p05a {

struct Backend3D {
    std::vector<CellPair> (*delaunay_pairs)(std::span<const Vec3>) = nullptr;
};

struct Build3DStats {
    std::size_t unmatched_faces = 0;     ///< face seen from one cell only, above the tiny-area threshold
    std::size_t dropped_tiny_faces = 0;  ///< face seen from one cell only, below the threshold
    std::size_t euler_failures = 0;      ///< cells whose polyhedron has V - E + F != 2
    Real max_partner_mismatch = 0;       ///< max |S_ij + S_ji| / |S_ij| between the two copies of a face
    Real min_face_area = 0;              ///< smallest internal face kept, absolute
};

struct Build3D {
    PolyMesh<3> mesh;
    std::vector<Real> cell_polyhedron_volume;  ///< volume of each cell from its own polyhedron
    Build3DStats stats;
};

/// Box patches: 0 x-, 1 x+, 2 y-, 3 y+, 4 z-, 5 z+. tiny_area is absolute and
/// must be derived from the box scale by the caller (R17).
[[nodiscard]] Build3D build_box_mesh_3d(std::span<const Vec3> sites, const Vec3& lo, const Vec3& hi,
                                        const Backend3D& backend, Real tiny_area);

}  // namespace vmm::p05a
