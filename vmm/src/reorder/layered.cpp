// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <map>
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/reorder/layered.hpp>
namespace vmm {
Result<LayeredMesh> renumber(const LayeredMesh& input,const Permutation& permutation) {
    auto mesh=renumber(input.mesh(),permutation);
    if(!mesh) return std::unexpected(mesh.error());
    auto d=input.data();
    std::map<std::vector<VertexId>,std::size_t> levels;
    for(FaceId f:input.mesh().faces()) {
        const auto verts=input.mesh().face_vertices(f);
        std::vector<VertexId> key(verts.begin(),verts.end());
        std::ranges::sort(key); levels.emplace(std::move(key),d.face_level[f.index()]);
    }
    for(CellId c:mesh->cells()) {
        d.column[c.index()]=input.data().column[permutation.old_of(c).index()];
        d.layer[c.index()]=input.data().layer[permutation.old_of(c).index()];
    }
    for(FaceId f:mesh->faces()) {
        const auto verts=mesh->face_vertices(f);
        std::vector<VertexId> key(verts.begin(),verts.end());
        std::ranges::sort(key); d.face_level[f.index()]=levels.at(key);
    }
    d.mesh=std::move(*mesh);
    return LayeredMesh::from_data(std::move(d));
}
} // namespace vmm
