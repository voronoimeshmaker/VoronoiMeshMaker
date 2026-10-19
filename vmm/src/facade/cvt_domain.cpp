// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <numeric>
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
        // Ear clipping preserves collinear boundary vertices and handles concave
        // merged faces; a fan can emit zero-area or exterior triangles.
        const Vec3 origin=mesh.point(v[0]);
        Vec3 normal{};
        for(std::size_t k=1;k+1<v.size();++k)
            normal=normal+cross(mesh.point(v[k])-origin,mesh.point(v[k+1])-origin);
        std::size_t drop=0;
        for(std::size_t k=1;k<3;++k) if(std::abs(normal[k])>std::abs(normal[drop])) drop=k;
        const auto project=[&](VertexId vertex) {
            const auto point=mesh.point(vertex)-origin;
            return Vec2{point[(drop+1)%3],point[(drop+2)%3]};
        };
        const Real sign=normal[drop]>0 ? 1 : -1;
        const auto turn=[&](const Vec2& a,const Vec2& b,const Vec2& c) {
            return sign*((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]));
        };
        std::vector<std::size_t> ring(v.size());
        std::iota(ring.begin(),ring.end(),0);
        while(ring.size()>3) {
            bool clipped=false;
            for(std::size_t k=0;k<ring.size();++k) {
                const auto ia=ring[(k+ring.size()-1)%ring.size()],ib=ring[k],ic=ring[(k+1)%ring.size()];
                const auto a=project(v[ia]),b=project(v[ib]),c=project(v[ic]);
                if(!(turn(a,b,c)>0)) continue;
                bool occupied=false;
                for(auto index:ring) if(index!=ia && index!=ib && index!=ic) {
                    const auto p=project(v[index]);
                    if(turn(a,b,p)>=0 && turn(b,c,p)>=0 && turn(c,a,p)>=0) { occupied=true; break; }
                }
                if(occupied) continue;
                triangles.push_back({{v[ia].value,v[ib].value,v[ic].value},inside,outside,mesh.patch(f)});
                ring.erase(ring.begin()+static_cast<std::ptrdiff_t>(k));
                clipped=true; break;
            }
            if(!clipped) return fail(ErrorCode::InvalidPolygon,"cannot triangulate layered interface without degeneracy");
        }
        if(!(turn(project(v[ring[0]]),project(v[ring[1]]),project(v[ring[2]]))>0))
            return fail(ErrorCode::InvalidPolygon,"degenerate layered interface triangle");
        triangles.push_back({{v[ring[0]].value,v[ring[1]].value,v[ring[2]].value},inside,outside,mesh.patch(f)});
    }
    Partition3D partition(mesh.points(),std::move(triangles),std::move(regions),mesh.media(),std::move(patches));
    for(std::size_t r=0;r<partition.region_count();++r) {
        auto surface=partition.region_surface(RegionId::from_index(r));
        if(!surface) return std::unexpected(surface.error());
    }
    return LayeredCvtDomain{std::move(partition),std::move(original)};
}
} // namespace vmm
