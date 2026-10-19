// ============================================================================
// File: flat_columns.cpp
// Description: Direct prism construction from constant column elevations.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/layered.hpp>
namespace vmm::detail {
Result<LayeredMesh> generate_flat_columns(const Mesh2D& base,const HorizonGrid& horizons,
    std::vector<std::vector<Real>> fractions,std::vector<std::vector<Real>> elevations,FlatColumnOptions options) {
    if(!std::isfinite(options.min_thickness_absolute) || options.min_thickness_absolute<0 ||
       !std::isfinite(options.min_thickness_relative) || options.min_thickness_relative<0 ||
       options.min_thickness_relative>1 || !std::isfinite(options.min_bottom_thickness) ||
       options.min_bottom_thickness<0)
        return fail(ErrorCode::InvalidArgument,"invalid flat minimum thickness options");
    if(base.cell_count()==0 || horizons.horizon_count()<2)
        return fail(ErrorCode::InvalidArgument,"empty flat column input");
    const auto intervals=horizons.horizon_count()-1;
    if(fractions.empty()) fractions.assign(intervals,{0,1});
    if(fractions.size()!=intervals || elevations.size()!=base.cell_count())
        return fail(ErrorCode::InvalidArgument,"flat column dimensions");
    std::vector<std::size_t> interval;
    for(std::size_t h=0;h<intervals;++h) {
        const auto& f=fractions[h];
        if(f.size()<2 || f.front()!=0 || f.back()!=1 ||
           !std::ranges::all_of(f,[](Real x){return std::isfinite(x);}) ||
           std::adjacent_find(f.begin(),f.end(),std::greater_equal<Real>{})!=f.end())
            return fail(ErrorCode::InvalidArgument,"invalid flat layer fractions");
        for(std::size_t k=1;k<f.size();++k) interval.push_back(h);
    }
    const auto layers=interval.size(),columns=base.cell_count();
    if(layers==0 || columns>=std::numeric_limits<std::uint32_t>::max()/layers)
        return fail(ErrorCode::InvalidArgument,"flat column ID capacity exceeded");
    LayeredData data;
    data.horizons=horizons; data.fractions=fractions; data.column_count=columns;
    data.column_levels.resize(columns); data.kfloor.resize(columns,layers);
    MeshData<3> mesh; mesh.media=base.media();
    for(const auto& region:base.regions()) for(std::size_t h=0;h<intervals;++h)
        mesh.regions.push_back({region.name+"/h"+std::to_string(h),region.medium});
    std::vector<std::vector<CellId>> cells(columns,std::vector<CellId>(layers));
    for(CellId column:base.cells()) {
        const auto c=column.index(); const auto& z=elevations[c];
        if(z.size()!=intervals+1 || !std::is_sorted(z.begin(),z.end()) ||
           !std::ranges::all_of(z,[](Real x){return std::isfinite(x);}))
            return fail(ErrorCode::InvalidArgument,"sampled horizons must be finite and nondecreasing");
        auto& levels=data.column_levels[c]; levels.push_back(z.front());
        for(std::size_t h=0;h<intervals;++h) for(std::size_t k=1;k<fractions[h].size();++k)
            levels.push_back(std::lerp(z[h],z[h+1],fractions[h][k]));
        if(options.min_bottom_thickness>0) {
            std::size_t first=0;
            while(first<layers && levels[first]==levels[first+1]) ++first;
            if(first<layers && levels[first+1]-levels[first]<options.min_bottom_thickness) {
                std::size_t end=first+1;
                while(end<layers && interval[end]==interval[first]) ++end;
                const Real target=levels[first]+options.min_bottom_thickness;
                if(!std::isfinite(target) || target>levels[end])
                    return fail(ErrorCode::InvalidArgument,"bottom thickness exceeds its horizon interval: column="+std::to_string(c));
                for(std::size_t k=first+1;k<end && levels[k]<target;++k) levels[k]=target;
            }
        }
        const Real minimum=std::max(options.min_thickness_absolute,
                                    options.min_thickness_relative*(levels.back()-levels.front()));
        if(options.thickness_policy) {
            const auto original=levels;
            auto adjusted=options.thickness_policy(levels,interval,minimum);
            if(!adjusted) return std::unexpected(adjusted.error());
            if(levels.size()!=original.size() || levels.front()!=original.front() || levels.back()!=original.back() ||
               !std::ranges::all_of(levels,[](Real value){return std::isfinite(value);}) || !std::is_sorted(levels.begin(),levels.end()))
                return fail(ErrorCode::InvalidArgument,"thickness policy changed column bounds or invalidated levels");
            for(std::size_t l=1;l<layers;++l) if(interval[l]!=interval[l-1] && levels[l]!=original[l])
                return fail(ErrorCode::InvalidArgument,"thickness policy moved a regional interface");
        }
        const auto generator=base.site(column); data.base_generator.push_back(generator);
        for(std::size_t l=0;l<layers;++l) if(levels[l+1]>levels[l]) {
            if(levels[l+1]-levels[l]<minimum)
                return fail(ErrorCode::InvalidArgument,"flat layer below minimum thickness: column="+
                    std::to_string(c)+" layer="+std::to_string(l));
            data.kfloor[c]=std::min(data.kfloor[c],l);
            cells[c][l]=CellId::from_index(mesh.sites.size());
            mesh.sites.push_back({generator[0],generator[1],std::midpoint(levels[l],levels[l+1])});
            mesh.cell_region.push_back(RegionId::from_index(base.region(column).index()*intervals+interval[l]));
            mesh.cell_input_site.push_back(SiteId::from_index(c));
            data.column.push_back(c); data.layer.push_back(l); data.site_is_centroid.push_back(0);
        }
    }
    if(mesh.sites.empty()) return fail(ErrorCode::InvalidArgument,"all flat layers absent");
    std::vector<std::map<VertexId,VertexId>> next(columns);
    std::vector<std::vector<Real>> vertex_levels(base.point_count());
    for(FaceId face:base.faces()) {
        const auto v=base.face_vertices(face);
        if(v.size()!=2) return fail(ErrorCode::InvalidPolygon,"flat base edges require two vertices");
        for(CellId column:{base.owner(face),base.neighbour(face)}) if(column.valid()) {
            const bool owner=column==base.owner(face);
            if(!next[column.index()].emplace(v[owner?0:1],v[owner?1:0]).second)
                return fail(ErrorCode::InvalidPolygon,"branched flat base boundary");
            for(auto vertex:v) {
                auto& levels=vertex_levels[vertex.index()];
                const auto& source=data.column_levels[column.index()];
                levels.insert(levels.end(),source.begin(),source.end());
            }
        }
    }
    for(auto& levels:vertex_levels) {
        std::ranges::sort(levels); levels.erase(std::unique(levels.begin(),levels.end()),levels.end());
    }
    std::map<std::pair<VertexId,Real>,VertexId> vertices;
    const auto vertex_id=[&](VertexId v,Real z) {
        const auto [it,added]=vertices.emplace(std::pair(v,z),VertexId::from_index(mesh.points.size()));
        if(added) { const auto xy=base.point(v); mesh.points.push_back({xy[0],xy[1],z}); }
        return it->second;
    };
    struct Face { std::vector<VertexId> vertices; CellId owner,neighbour; std::size_t level,patch; };
    std::vector<Face> faces;
    const auto append=[&](std::vector<VertexId> v,CellId owner,CellId neighbour,std::size_t level,std::size_t patch) {
        if(neighbour.valid() && neighbour<owner) { std::swap(owner,neighbour); std::ranges::reverse(v); }
        faces.push_back({std::move(v),owner,neighbour,level,patch});
    };
    const auto bottom_patch=base.patches().size(),top_patch=bottom_patch+1,step_patch=bottom_patch+2;
    for(std::size_t c=0;c<columns;++c) {
        if(next[c].empty()) return fail(ErrorCode::InvalidPolygon,"empty flat base boundary");
        std::vector<VertexId> ring;
        const auto first=next[c].begin()->first; auto current=first;
        do {
            ring.push_back(current);
            const auto it=next[c].find(current);
            if(it==next[c].end()) return fail(ErrorCode::InvalidPolygon,"open flat base boundary");
            current=it->second;
        } while(current!=first && ring.size()<=next[c].size());
        if(current!=first || ring.size()!=next[c].size())
            return fail(ErrorCode::InvalidPolygon,"flat column multiple boundary cycles not yet supported");
        std::size_t last=layers;
        while(last>0 && !cells[c][last-1].valid()) --last;
        CellId below;
        for(std::size_t l=0;l<layers;++l) if(cells[c][l].valid()) {
            const auto cell=cells[c][l]; std::vector<VertexId> polygon;
            for(auto v:ring) polygon.push_back(vertex_id(v,data.column_levels[c][l]));
            if(below.valid()) append(polygon,below,cell,l,0);
            else { std::ranges::reverse(polygon); append(polygon,cell,{},l,bottom_patch); }
            below=cell;
            if(l+1==last) {
                polygon.clear(); for(auto v:ring) polygon.push_back(vertex_id(v,data.column_levels[c][l+1]));
                append(polygon,cell,{},l+1,top_patch);
            }
        }
    }
    for(FaceId face:base.faces()) {
        const auto edge=base.face_vertices(face); const auto owner=base.owner(face),neighbour=base.neighbour(face);
        const auto& a=data.column_levels[owner.index()];
        std::vector<Real> cuts=a;
        if(neighbour.valid()) {
            const auto& b=data.column_levels[neighbour.index()];
            cuts.clear(); cuts.reserve(a.size()+b.size());
            std::set_union(a.begin(),a.end(),b.begin(),b.end(),std::back_inserter(cuts));
        }
        cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
        std::size_t ia=0,ib=0;
        for(std::size_t k=1;k<cuts.size();++k) {
            const auto low=cuts[k-1],high=cuts[k];
            while(ia<layers && a[ia+1]<=low) ++ia;
            CellId ca=ia<layers && a[ia]<=low && a[ia+1]>=high ? cells[owner.index()][ia] : CellId{};
            CellId cb;
            if(neighbour.valid()) {
                const auto& b=data.column_levels[neighbour.index()];
                while(ib<layers && b[ib+1]<=low) ++ib;
                if(ib<layers && b[ib]<=low && b[ib+1]>=high) cb=cells[neighbour.index()][ib];
            }
            if(!ca.valid() && !cb.valid()) continue;
            std::vector<VertexId> polygon{vertex_id(edge[0],low),vertex_id(edge[1],low)};
            const auto& right=vertex_levels[edge[1].index()];
            const auto right_end=std::lower_bound(right.begin(),right.end(),high);
            for(auto it=std::upper_bound(right.begin(),right.end(),low);it!=right_end;++it)
                polygon.push_back(vertex_id(edge[1],*it));
            polygon.push_back(vertex_id(edge[1],high)); polygon.push_back(vertex_id(edge[0],high));
            const auto& left=vertex_levels[edge[0].index()];
            const auto left_begin=std::upper_bound(left.begin(),left.end(),low);
            for(auto it=std::lower_bound(left.begin(),left.end(),high);it!=left_begin;)
                polygon.push_back(vertex_id(edge[0],*--it));
            if(!ca.valid()) { ca=cb; cb={}; std::ranges::reverse(polygon); }
            append(polygon,ca,cb,lateral_face,neighbour.valid()?step_patch:base.patch(face).index());
        }
    }
    std::ranges::sort(faces,[](const Face& a,const Face& b) {
        return std::tuple(!a.neighbour.valid(),a.neighbour.valid()?0:a.patch,a.owner,a.neighbour)<
               std::tuple(!b.neighbour.valid(),b.neighbour.valid()?0:b.patch,b.owner,b.neighbour);
    });
    for(const auto& face:faces) {
        mesh.face_vertices.push_row(face.vertices); mesh.owner.push_back(face.owner);
        if(face.neighbour.valid()) mesh.neighbour.push_back(face.neighbour);
        data.face_level.push_back(face.level);
    }
    std::size_t cursor=mesh.neighbour.size();
    for(std::size_t p=0;p<=step_patch;++p) {
        const auto begin=cursor;
        while(cursor<faces.size() && faces[cursor].patch==p) ++cursor;
        mesh.patches.push_back({p<bottom_patch?base.patches()[p].name:p==bottom_patch?"column_bottom":p==top_patch?"column_top":"column_step",begin,cursor-begin});
    }
    auto result=Mesh3D::from_data(std::move(mesh));
    if(!result) return std::unexpected(result.error());
    data.mesh=std::move(*result);
    return LayeredMesh::from_data(std::move(data));
}
} // namespace vmm::detail
