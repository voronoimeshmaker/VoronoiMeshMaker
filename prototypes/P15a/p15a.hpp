// ============================================================================
// File: p15a.hpp
// Description: P15a - throwaway proof of concept of the 3D clipping (DEC-003,
//              DEC-010): convex Voronoi cells by half-spaces for every site,
//              exact clipping against a closed triangulated domain only for
//              the cells whose box touches the boundary, assembled into the
//              generic vmm::Mesh<3> and checked with vmm::check_invariants.
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/mesh/mesh.hpp>

namespace p15a {

using vmm::Real;
using vmm::Vec3;

/// Closed, consistently oriented (outward) triangle surface; may have several
/// components. tri_patch[t] indexes patch_names.
struct Domain {
    std::vector<Vec3> points;
    std::vector<std::array<std::uint32_t, 3>> triangles;
    std::vector<std::uint32_t> tri_patch;
    std::vector<std::string> patch_names;
};

/// Axis-aligned box, 12 triangles, patches x-, x+, y-, y+, z-, z+.
Domain box(const Vec3& lo, const Vec3& hi);
/// Simple CCW polygon of the xy-plane extruded from z0 to z1 (ear clipping for
/// the caps); patches bottom, top, side.
Domain prism(const std::vector<std::array<Real, 2>>& polygon, Real z0, Real z1);
/// Sphere approximated by a subdivided icosahedron (convex, curved-like).
Domain icosphere(const Vec3& centre, Real radius, int subdivisions);
/// Several domains in one surface (disjoint components).
Domain merge(const std::vector<Domain>& parts);
/// "CGAL x.y.z; Boost ...".
std::string backend_versions();
/// Uniform scaling about the origin (R17 scale tests).
Domain scaled(const Domain& d, Real s);

/// Exact volume and area of the domain (CGAL, exact kernel).
struct DomainMeasures {
    Real volume = 0;
    Real area = 0;
    Real diagonal = 0;  ///< bounding-box diagonal L
};
DomainMeasures measures(const Domain& d);

/// Dart throwing strictly inside the domain: minimum spacing `spacing` and a
/// margin of `margin` to the surface. Deterministic for a seed.
std::vector<Vec3> random_sites(const Domain& d, std::size_t count, Real spacing, Real margin, std::uint64_t seed);
/// Points of `candidates` strictly inside the domain.
std::vector<Vec3> inside(const Domain& d, const std::vector<Vec3>& candidates);

struct BuildStats {
    std::size_t cells = 0;
    std::size_t boundary_cells = 0;         ///< clipped exactly
    std::size_t clip_failures = 0;          ///< corefinement refused or threw, or invalid cell mesh
    std::size_t invalid_cell_meshes = 0;    ///< cell mesh open or self-intersecting (not clipped)
    std::size_t disconnected_cells = 0;     ///< clipped cell with more than one piece
    std::size_t euler_failures = 0;         ///< unclipped convex cell with V - E + F != 2
    std::size_t unsnapped_vertices = 0;     ///< bisector vertex whose 4 sites are coplanar
    std::size_t merged_vertices = 0;        ///< vertices moved by the merge within 1e-12 L
    std::size_t t_vertices = 0;             ///< vertices inserted into a neighbouring face edge (T-junctions)
    std::size_t unlabelled_triangles = 0;   ///< clipped triangle on no domain triangle and no cell face
    std::size_t ambiguous_triangles = 0;    ///< clipped triangle close to a domain triangle and a cell face
    std::size_t faces_with_holes = 0;       ///< face piece bounded by more than one loop
    std::size_t leftover_box_faces = 0;     ///< enclosing-box face surviving the clipping
    std::size_t unmatched_faces = 0;        ///< internal face seen by one cell only, above the tiny area
    std::size_t dropped_tiny_faces = 0;     ///< the same, at or below the tiny area
    std::size_t piece_count_mismatch = 0;   ///< pair (i, j) with a different number of pieces on each side
    Real max_partner_mismatch = 0;          ///< max |S_ij + S_ji| / |S_ij| over matched pairs
    Real seconds_delaunay = 0;
    Real seconds_convex = 0;
    Real seconds_classify = 0;
    Real seconds_clip = 0;
    Real seconds_assembly = 0;
};

struct Build {
    vmm::Mesh3D mesh;
    std::vector<Real> cell_volume;  ///< independent: exact for clipped cells, tetrahedra for the others
    BuildStats stats;
    std::string error;              ///< Mesh::from_data failure, if any
};

/// Builds the clipped Voronoi mesh of `sites` (inside `domain`). Cells are
/// numbered in canonical (lexicographic) site order; cell_input_site maps back.
Build build_mesh(const Domain& domain, const std::vector<Vec3>& sites);

/// Topology in the canonical cell numbering: (owner, neighbour) of the internal
/// faces and (owner, patch) of the boundary faces, sorted.
std::vector<std::array<std::uint64_t, 2>> topology(const vmm::Mesh3D& mesh);
/// Bitwise checksum of points, faces, owner and neighbour (not the input map).
std::uint64_t checksum(const vmm::Mesh3D& mesh);

}  // namespace p15a
