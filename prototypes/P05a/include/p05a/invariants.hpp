// ============================================================================
// File: invariants.hpp
// Description: P05a prototype - DEC-011 geometric invariants, written once
//              for any dimension; only face_geometry() depends on D.
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
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

/// Reference values supplied by the caller. Tolerances are relative (R17,
/// DEC-020): measures to the reference total, points and area vectors to L.
struct InvariantReference {
    Real length_scale = 1;                  ///< L, diagonal of the bounding box
    Real total_measure = 0;                 ///< area (2D) or volume (3D) of the domain
    std::vector<Real> region_measure;       ///< one per region
    Real boundary_measure = -1;             ///< length/area of the boundary; < 0 skips the check
    Real interface_measure = -1;            ///< length/area of the interfaces; < 0 skips the check
    std::span<const Real> cell_measure;     ///< optional independent measure of each cell
    Real relative_tolerance = 1e-12;
    Real nonorthogonality_limit = 1e-8;     ///< radians, internal faces of one region (DEC-020)
};

struct InvariantReport {
    std::size_t cells = 0;
    std::size_t internal_faces = 0;
    std::size_t interface_faces = 0;
    std::size_t boundary_faces = 0;
    Real total_measure = 0;
    Real total_relative_error = 0;
    Real max_region_relative_error = 0;
    Real max_closure = 0;                   ///< max |sum of signed area vectors| over cells
    Real closure_tolerance = 0;             ///< relative_tolerance * L^(D-1)
    Real max_cell_measure_error = 0;        ///< vs cell_measure, relative to total_measure
    Real boundary_relative_error = 0;
    Real interface_relative_error = 0;
    std::size_t bad_owner_neighbour = 0;    ///< invalid ids, owner >= neighbour, patch mismatch
    std::size_t nonpositive_cells = 0;      ///< cells with no face or measure <= 0
    bool adjacency_symmetric = false;
    Real max_nonortho_internal = 0;         ///< radians, faces inside one region
    Real max_nonortho_interface = 0;        ///< radians, faces between regions

    [[nodiscard]] bool measures_ok(Real tol) const noexcept {
        return total_relative_error <= tol && max_region_relative_error <= tol && max_cell_measure_error <= tol &&
               boundary_relative_error <= tol && interface_relative_error <= tol;
    }
};

template <std::size_t D>
[[nodiscard]] InvariantReport check_invariants(const PolyMesh<D>& m, const InvariantReference& ref) {
    InvariantReport rep;
    rep.cells = m.cell_count();
    rep.closure_tolerance = ref.relative_tolerance * std::pow(ref.length_scale, static_cast<Real>(D - 1));

    std::vector<Real> measure(m.cell_count(), 0);
    std::vector<Vec<D>> closure(m.cell_count(), Vec<D>{});
    std::vector<std::size_t> face_count(m.cell_count(), 0);
    Real boundary = 0;
    Real interface = 0;

    for (std::size_t f = 0; f < m.face_count(); ++f) {
        const CellId o = m.owner[f];
        const CellId n = m.neighbour[f];
        const bool ids_ok = o.valid() && o.index() < m.cell_count() &&
                            (!n.valid() || (n.index() < m.cell_count() && o < n)) && (n.valid() != m.patch[f].valid()) &&
                            m.face_points.row(f).size() >= D;
        if (!ids_ok) {
            ++rep.bad_owner_neighbour;
            continue;
        }
        const FaceGeometry<D> g = face_geometry(m, f);
        const Real area = norm(g.area_vector);
        measure[o.index()] += dot(g.centroid - m.sites[o.index()], g.area_vector) / static_cast<Real>(D);
        closure[o.index()] = closure[o.index()] + g.area_vector;
        ++face_count[o.index()];
        if (!n.valid()) {
            ++rep.boundary_faces;
            boundary += area;
            continue;
        }
        measure[n.index()] -= dot(g.centroid - m.sites[n.index()], g.area_vector) / static_cast<Real>(D);
        closure[n.index()] = closure[n.index()] - g.area_vector;
        ++face_count[n.index()];
        const Real theta = angle_between(g.area_vector, m.sites[n.index()] - m.sites[o.index()]);
        if (m.cell_region[o.index()] == m.cell_region[n.index()]) {
            ++rep.internal_faces;
            rep.max_nonortho_internal = std::max(rep.max_nonortho_internal, theta);
        } else {
            ++rep.interface_faces;
            interface += area;
            rep.max_nonortho_interface = std::max(rep.max_nonortho_interface, theta);
        }
    }

    std::vector<Real> region(ref.region_measure.size(), 0);
    for (std::size_t c = 0; c < m.cell_count(); ++c) {
        rep.total_measure += measure[c];
        region[m.cell_region[c].index()] += measure[c];
        rep.max_closure = std::max(rep.max_closure, norm(closure[c]));
        if (face_count[c] == 0 || measure[c] <= 0) ++rep.nonpositive_cells;
        if (!ref.cell_measure.empty()) {
            rep.max_cell_measure_error =
                std::max(rep.max_cell_measure_error, std::abs(measure[c] - ref.cell_measure[c]) / ref.total_measure);
        }
    }
    rep.total_relative_error = std::abs(rep.total_measure - ref.total_measure) / ref.total_measure;
    for (std::size_t r = 0; r < region.size(); ++r) {
        rep.max_region_relative_error = std::max(
            rep.max_region_relative_error, std::abs(region[r] - ref.region_measure[r]) / ref.region_measure[r]);
    }
    if (ref.boundary_measure >= 0) {
        rep.boundary_relative_error = std::abs(boundary - ref.boundary_measure) / ref.boundary_measure;
    }
    if (ref.interface_measure >= 0) {
        rep.interface_relative_error =
            ref.interface_measure > 0 ? std::abs(interface - ref.interface_measure) / ref.interface_measure : interface;
    }
    rep.adjacency_symmetric = is_structurally_symmetric(cell_adjacency(m));
    return rep;
}

}  // namespace vmm::p05a
