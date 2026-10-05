// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/cvt.hpp>
namespace vmm {
Result<LayeredCvtDomain> cvt_domain(const LayeredMesh& source) {
    auto valid=LayeredMesh::from_data(source.data());
    if(!valid) return std::unexpected(valid.error());
    const auto& mesh=source.mesh();
    std::vector<bool> present(mesh.regions().size(),false);
    for(CellId c:mesh.cells()) present[mesh.region(c).index()]=true;
    std::vector<RegionId> compact(present.size());
    std::vector<RegionId> original;
    std::vector<RegionInfo> regions;
    for(std::size_t r=0;r<present.size();++r) if(present[r]) {
        compact[r]=RegionId::from_index(regions.size());
        original.push_back(RegionId::from_index(r));
        regions.push_back({mesh.regions()[r].name,mesh.regions()[r].medium});
    }
    std::vector<std::string> patches;
    for(const auto& patch:mesh.patches()) patches.push_back(patch.name);
    std::vector<PartitionTriangle> triangles;
    for(FaceId f:mesh.faces()) {
        const auto inside=compact[mesh.region(mesh.owner(f)).index()];
        const auto outside=mesh.is_internal(f)?compact[mesh.region(mesh.neighbour(f)).index()]:RegionId{};
        if(inside==outside) continue;
        const auto v=mesh.face_vertices(f);
        for(std::size_t k=1;k+1<v.size();++k)
            triangles.push_back({{v[0].value,v[k].value,v[k+1].value},inside,outside,mesh.patch(f)});
    }
    Partition3D partition(mesh.points(),std::move(triangles),std::move(regions),mesh.media(),std::move(patches));
    for(std::size_t r=0;r<partition.region_count();++r) {
        auto surface=partition.region_surface(RegionId::from_index(r));
        if(!surface) return std::unexpected(surface.error());
    }
    return LayeredCvtDomain{std::move(partition),std::move(original)};
}
} // namespace vmm
