// ============================================================================
// File: simple_meshes.hpp
// Description: Test helpers: small hand-built meshes.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstdint>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/mesh.hpp>

namespace vmm::test {

/// Two unit squares [0,1]x[0,1] (cell 0) and [1,2]x[0,1] (cell 1), sites at
/// the centres; one internal face x = 1; patch "wall" (6 faces).
inline MeshData<2> two_squares_data() {
    MeshData<2> d;
    d.points = {{0, 0}, {1, 0}, {2, 0}, {0, 1}, {1, 1}, {2, 1}};
    auto face = [&](std::uint32_t a, std::uint32_t b, std::uint32_t owner) {
        const VertexId v[2] = {VertexId{a}, VertexId{b}};
        d.face_vertices.push_row(v);
        d.owner.push_back(CellId{owner});
    };
    face(1, 4, 0);  // internal: owner 0 on the left, normal +x
    d.neighbour.push_back(CellId{1});
    face(0, 1, 0);
    face(4, 3, 0);
    face(3, 0, 0);
    face(1, 2, 1);
    face(2, 5, 1);
    face(5, 4, 1);
    d.patches = {{"wall", 1, 6}};
    d.sites = {{0.5, 0.5}, {1.5, 0.5}};
    d.cell_region = {RegionId{0}, RegionId{0}};
    d.cell_input_site = {SiteId{1}, SiteId{0}};
    d.regions = {{"r", MediumId{0}}};
    d.media = {"m"};
    return d;
}

}  // namespace vmm::test
