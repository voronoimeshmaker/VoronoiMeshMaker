// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/layered.hpp>
namespace vmm {
Result<LayeredMesh> LayeredMesh::from_data(LayeredData d) {
    if(d.horizons.horizon_count()<2 || d.column_count==0 ||
       d.fractions.size()!=d.horizons.horizon_count()-1)
        return fail(ErrorCode::InvalidArgument,"invalid column/horizon counts");
    std::size_t layers=0;
    for(const auto& f:d.fractions) {
        if(f.size()<2 || f.front()!=0 || f.back()!=1 ||
           !std::ranges::all_of(f,[](Real x){return std::isfinite(x);}) ||
           std::adjacent_find(f.begin(),f.end(),std::greater_equal<Real>{})!=f.end())
            return fail(ErrorCode::InvalidArgument,"fractions must strictly increase from 0 to 1");
        if(layers>std::numeric_limits<std::size_t>::max()-(f.size()-1))
            return fail(ErrorCode::InvalidArgument,"layer count overflow");
        layers+=f.size()-1;
    }
    if(d.column.size()!=d.mesh.cell_count() || d.layer.size()!=d.mesh.cell_count() ||
       d.face_level.size()!=d.mesh.face_count())
        return fail(ErrorCode::InconsistentData,"layered metadata sizes");
    std::set<std::pair<std::size_t,std::size_t>> used;
    for(std::size_t c=0;c<d.column.size();++c) {
        if(d.column[c]>=d.column_count || d.layer[c]>=layers ||
           !used.emplace(d.column[c],d.layer[c]).second)
            return fail(ErrorCode::InconsistentData,"invalid or repeated column/layer pair");
    }
    for(auto level:d.face_level) if(level!=lateral_face && level>layers)
        return fail(ErrorCode::InconsistentData,"face level out of range");
    LayeredMesh out; out.data_=std::move(d); return out;
}
std::vector<CellId> LayeredMesh::cells_in_column(std::size_t column) const {
    std::vector<CellId> out;
    for(std::size_t c=0;c<data_.column.size();++c)
        if(data_.column[c]==column) out.push_back(CellId::from_index(c));
    std::ranges::sort(out,[&](CellId a,CellId b){return data_.layer[a.index()]<data_.layer[b.index()];});
    return out;
}
std::vector<CellId> LayeredMesh::vertical_neighbours(CellId cell) const {
    std::vector<CellId> out;
    for(FaceId f:data_.mesh.internal_faces()) if(data_.face_level[f.index()]!=lateral_face) {
        if(data_.mesh.owner(f)==cell) out.push_back(data_.mesh.neighbour(f));
        if(data_.mesh.neighbour(f)==cell) out.push_back(data_.mesh.owner(f));
    }
    std::ranges::sort(out); out.erase(std::unique(out.begin(),out.end()),out.end());
    return out;
}
} // namespace vmm
