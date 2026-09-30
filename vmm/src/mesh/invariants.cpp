// ============================================================================
// File: invariants.cpp
// Description: DEC-011 invariant checker.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <map>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/invariants.hpp>

namespace vmm {

template <std::size_t D>
InvariantReport check_invariants(const Mesh<D>& m, const InvariantReference& ref) {
    InvariantReport rep;
    rep.closure_tolerance = ref.relative_tolerance * std::pow(ref.length_scale, static_cast<Real>(D - 1));
    const std::size_t nc = m.cell_count();
    std::vector<Real> measure(nc, 0);
    std::vector<Vec<D>> closure(nc, Vec<D>{});
    std::vector<std::size_t> faces(nc, 0);
    Real boundary = 0;
    std::map<std::pair<std::size_t, std::size_t>, Real> interface;
    auto problem = [&](std::string what) {
        if (rep.first_problem.empty()) rep.first_problem = std::move(what);
    };

    for (const FaceId f : m.faces()) {
        const auto g = face_geometry(m, f);
        const CellId o = m.owner(f);
        const Real area = norm(g.area_vector);
        measure[o.index()] += dot(g.centroid - m.site(o), g.area_vector) / static_cast<Real>(D);
        closure[o.index()] = closure[o.index()] + g.area_vector;
        ++faces[o.index()];
        if (!m.is_internal(f)) {
            boundary += area;
            if (!m.patch(f).valid()) {
                ++rep.bad_faces;
                problem(std::format("boundary face {} without patch", f.value));
            }
            continue;
        }
        const CellId n = m.neighbour(f);
        if (!(o < n)) {
            ++rep.bad_faces;
            problem(std::format("face {}: owner >= neighbour", f.value));
        }
        measure[n.index()] -= dot(g.centroid - m.site(n), g.area_vector) / static_cast<Real>(D);
        closure[n.index()] = closure[n.index()] - g.area_vector;
        ++faces[n.index()];
        const Vec<D> d = m.site(n) - m.site(o);
        const Real theta = angle_between(g.area_vector, d);
        const std::size_t ro = m.region(o).index();
        const std::size_t rn = m.region(n).index();
        if (ro == rn) {
            // Voronoi face inside one region: S is parallel to x_n - x_o.
            if (dot(d, g.area_vector) <= 0) {
                ++rep.bad_faces;
                problem(std::format("face {}: area vector does not point from owner to neighbour", f.value));
            }
            // Orthogonal by construction (DEC-032): the face is a piece of the
            // bisector chosen by the exact backend. The measured angle only
            // reflects the rounding of the stored vertices; reported, not checked.
            rep.max_nonortho_internal = std::max(rep.max_nonortho_internal, theta);
        } else {
            ++rep.interface_faces;
            rep.max_nonortho_interface = std::max(rep.max_nonortho_interface, theta);
            interface[{std::min(ro, rn), std::max(ro, rn)}] += area;
        }
    }

    Real total = 0;
    std::vector<Real> region(m.regions().size(), 0);
    for (std::size_t c = 0; c < nc; ++c) {
        total += measure[c];
        region[m.cell_regions()[c].index()] += measure[c];
        rep.max_closure = std::max(rep.max_closure, norm(closure[c]));
        if (faces[c] == 0 || measure[c] <= 0) {
            ++rep.nonpositive_cells;
            problem(std::format("cell {} has no faces or non-positive measure", c));
        }
        if (!ref.cell_measure.empty()) {
            rep.max_cell_measure_error =
                std::max(rep.max_cell_measure_error, std::abs(measure[c] - ref.cell_measure[c]) / ref.total_measure);
        }
    }
    rep.total_relative_error = std::abs(total - ref.total_measure) / ref.total_measure;
    for (std::size_t r = 0; r < std::min(region.size(), ref.region_measure.size()); ++r) {
        if (ref.region_measure[r] > 0) {
            rep.max_region_relative_error =
                std::max(rep.max_region_relative_error, std::abs(region[r] - ref.region_measure[r]) / ref.region_measure[r]);
        }
    }
    if (ref.boundary_measure > 0) rep.boundary_relative_error = std::abs(boundary - ref.boundary_measure) / ref.boundary_measure;
    for (const auto& [key, expected] : ref.interface_measure) {
        const auto it = interface.find(key);
        const Real got = it == interface.end() ? 0 : it->second;
        rep.max_interface_relative_error =
            std::max(rep.max_interface_relative_error, expected > 0 ? std::abs(got - expected) / expected : got);
    }
    for (const auto& [key, got] : interface) {
        if (!ref.interface_measure.empty() && !ref.interface_measure.contains(key)) {
            ++rep.nonconforming_faces;
            problem(std::format("interface between regions {} and {} not in the partition", key.first, key.second));
        }
    }
    rep.adjacency_symmetric = is_structurally_symmetric(cell_adjacency(m));
    return rep;
}

template InvariantReport check_invariants(const Mesh<2>&, const InvariantReference&);
template InvariantReport check_invariants(const Mesh<3>&, const InvariantReference&);

}  // namespace vmm
