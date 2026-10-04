// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <utility>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/invariants.hpp>
#include <vmm/mesh/layered.hpp>
#include <vmm/mesh/metrics.hpp>
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
    if(d.mesh.cell_count()==0 || d.mesh.point_count()==0)
        return fail(ErrorCode::InconsistentData,"empty layered geometry");
    Vec3 lo=d.mesh.points().front(),hi=lo;
    for(const Vec3& p:d.mesh.points()) for(std::size_t k=0;k<3;++k) {
        if(!std::isfinite(p[k])) return fail(ErrorCode::InconsistentData,"non-finite vertex");
        lo[k]=std::min(lo[k],p[k]); hi[k]=std::max(hi[k],p[k]);
    }
    const Real tol=1e-12*norm(hi-lo);
    std::vector<std::size_t> intervals;
    std::vector<Real> lower,upper;
    for(std::size_t h=0;h<d.fractions.size();++h)
        for(std::size_t k=1;k<d.fractions[h].size();++k) {
            intervals.push_back(h); lower.push_back(d.fractions[h][k-1]); upper.push_back(d.fractions[h][k]);
        }
    std::map<std::size_t,std::size_t> column_region;
    for(CellId c:d.mesh.cells()) {
        for(Real x:d.mesh.site(c)) if(!std::isfinite(x))
            return fail(ErrorCode::InconsistentData,"non-finite cell reference");
        const auto region=d.mesh.region(c).index(),h=intervals[d.layer[c.index()]];
        if(region%d.fractions.size()!=h)
            return fail(ErrorCode::InconsistentData,"region does not match horizon interval");
        const auto [it,added]=column_region.emplace(d.column[c.index()],region/d.fractions.size());
        if(!added && it->second!=region/d.fractions.size())
            return fail(ErrorCode::InconsistentData,"column has inconsistent horizontal regions");
    }
    std::set<std::vector<VertexId>> face_keys;
    for(FaceId f:d.mesh.faces()) {
        const auto level=d.face_level[f.index()];
        std::vector<Vec3> samples;
        Vec3 centre{};
        const auto verts=d.mesh.face_vertices(f);
        std::vector<VertexId> key(verts.begin(),verts.end());
        std::ranges::sort(key);
        if(!face_keys.insert(std::move(key)).second)
            return fail(ErrorCode::InconsistentData,"duplicate layered face");
        for(VertexId v:verts) { samples.push_back(d.mesh.point(v)); centre=centre+d.mesh.point(v); }
        samples.push_back((1/static_cast<Real>(verts.size()))*centre);
        for(std::size_t k=0;k<verts.size();++k)
            samples.push_back(0.5*(d.mesh.point(verts[k])+d.mesh.point(verts[(k+1)%verts.size()])));
        for(const auto& p:samples) {
            if(level!=lateral_face) {
                const auto l=level==layers ? layers-1 : level;
                auto a=d.horizons.elevation(intervals[l],{p[0],p[1]});
                auto b=d.horizons.elevation(intervals[l]+1,{p[0],p[1]});
                if(!a || !b || std::abs(p[2]-std::lerp(*a,*b,level==layers ? upper[l] : lower[l]))>tol)
                    return fail(ErrorCode::InconsistentData,"face left its fixed horizon support");
            }
            for(CellId c:{d.mesh.owner(f),d.mesh.neighbour(f)}) if(c.valid()) {
                const auto l=d.layer[c.index()];
                auto a=d.horizons.elevation(intervals[l],{p[0],p[1]});
                auto b=d.horizons.elevation(intervals[l]+1,{p[0],p[1]});
                if(!a || !b || p[2]<std::lerp(*a,*b,lower[l])-tol || p[2]>std::lerp(*a,*b,upper[l])+tol)
                    return fail(ErrorCode::InconsistentData,"cell crosses a horizon");
            }
        }
        if(d.mesh.is_internal(f)) {
            const auto a=d.mesh.owner(f).index(),b=d.mesh.neighbour(f).index();
            if(level!=lateral_face && d.column[a]!=d.column[b])
                return fail(ErrorCode::InconsistentData,"horizontal face connects different columns");
            if(level==lateral_face && d.layer[a]!=d.layer[b])
                return fail(ErrorCode::InconsistentData,"lateral face connects different layers");
        }
    }
    // Imported metadata also needs closed, oriented cells. These measures are
    // not an independent volume oracle; the generator supplies that separately.
    const auto metrics=compute_metrics(d.mesh);
    InvariantReference reference;
    reference.length_scale=norm(hi-lo);
    reference.region_measure.assign(d.mesh.regions().size(),0);
    for(CellId c:d.mesh.cells()) {
        const Real volume=metrics.cell_measure[c.index()];
        if(!std::isfinite(volume) || !(volume>0))
            return fail(ErrorCode::InconsistentData,"nonpositive layered cell volume");
        reference.total_measure+=volume;
        reference.region_measure[d.mesh.region(c).index()]+=volume;
    }
    if(!check_invariants(d.mesh,reference).passed(reference))
        return fail(ErrorCode::InconsistentData,"layered mesh closure or orientation failure");
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
