// ============================================================================
// File: flat_columns.cpp
// Description: Direct prism construction from constant column elevations.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/layered.hpp>
namespace vmm::detail {
namespace {
using Ring=std::vector<VertexId>;
Real turn(Vec2 a,Vec2 b,Vec2 c) {
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
}
bool inside(const Mesh2D& base,const Ring& ring,Vec2 p) {
    bool result=false;
    for(std::size_t i=0,j=ring.size()-1;i<ring.size();j=i++) {
        const auto a=base.point(ring[i]),b=base.point(ring[j]);
        if((a[1]>p[1])!=(b[1]>p[1]) && p[0]<(b[0]-a[0])*(p[1]-a[1])/(b[1]-a[1])+a[0]) result=!result;
    }
    return result;
}
Result<std::vector<Ring>> horizontal_parts(const Mesh2D& base,std::map<VertexId,VertexId> edges) {
    std::vector<Ring> outer,holes;
    while(!edges.empty()) {
        Ring ring;auto current=edges.begin()->first;const auto first=current;
        do {
            ring.push_back(current);const auto it=edges.find(current);
            if(it==edges.end()) return fail(ErrorCode::InvalidPolygon,"open flat base boundary");
            current=it->second;edges.erase(it);
        } while(current!=first);
        Real area=0;const auto origin=base.point(ring.front());
        for(std::size_t i=1;i+1<ring.size();++i) area+=turn(origin,base.point(ring[i]),base.point(ring[i+1]));
        if(area==0) return fail(ErrorCode::InvalidPolygon,"zero-area flat base cycle");
        (area>0?outer:holes).push_back(std::move(ring));
    }
    if(outer.empty()) return fail(ErrorCode::InvalidPolygon,"flat base lacks outer cycle");
    std::vector<Ring> result;std::vector<bool> assigned(holes.size());
    for(const auto& boundary:outer) {
        Ring polygon=boundary;bool joined=false;
        for(std::size_t h=0;h<holes.size();++h) {
            const auto& hole=holes[h];
            if(!inside(base,boundary,base.point(hole.front()))) continue;
            if(assigned[h]) return fail(ErrorCode::InvalidPolygon,"overlapping flat components");
            assigned[h]=true;
            Real best=std::numeric_limits<Real>::infinity();std::size_t oi=0,hi=0;
            for(std::size_t i=0;i<polygon.size();++i) for(std::size_t j=0;j<hole.size();++j) {
                const auto a=base.point(polygon[i]),b=base.point(hole[j]);
                const Vec2 midpoint{std::midpoint(a[0],b[0]),std::midpoint(a[1],b[1])};
                if(!inside(base,boundary,midpoint)) continue;
                bool visible=true;
                for(const auto& other:holes) if(inside(base,other,midpoint)) visible=false;
                const auto blocked=[&](const Ring& ring) {
                    for(std::size_t k=0;k<ring.size();++k) {
                        const auto c=base.point(ring[k]),d=base.point(ring[(k+1)%ring.size()]);
                        if(c==a || c==b || d==a || d==b) continue;
                        const Real ac=turn(a,b,c),ad=turn(a,b,d),ca=turn(c,d,a),cb=turn(c,d,b);
                        if(((ac<=0 && ad>=0)||(ac>=0 && ad<=0)) &&
                           ((ca<=0 && cb>=0)||(ca>=0 && cb<=0))) return true;
                    }
                    return false;
                };
                if(blocked(polygon)) visible=false;
                for(const auto& other:holes) if(blocked(other)) visible=false;
                const Real distance=dot(b-a,b-a);
                if(visible && distance<best) { best=distance;oi=i;hi=j; }
            }
            if(!std::isfinite(best)) return fail(ErrorCode::InvalidPolygon,"no visible bridge for flat hole");
            Ring merged;
            merged.insert(merged.end(),polygon.begin(),polygon.begin()+static_cast<std::ptrdiff_t>(oi+1));
            for(std::size_t k=0;k<=hole.size();++k) merged.push_back(hole[(hi+k)%hole.size()]);
            merged.push_back(polygon[oi]);
            merged.insert(merged.end(),polygon.begin()+static_cast<std::ptrdiff_t>(oi+1),polygon.end());
            polygon=std::move(merged);joined=true;
        }
        if(!joined) { result.push_back(std::move(polygon));continue; }
        while(polygon.size()>3) {
            bool found=false;
            for(std::size_t i=0;i<polygon.size();++i) {
                const auto prev=(i+polygon.size()-1)%polygon.size(),next=(i+1)%polygon.size();
                const auto a=base.point(polygon[prev]),b=base.point(polygon[i]),c=base.point(polygon[next]);
                if(turn(a,b,c)<=0) continue;
                bool occupied=false;
                for(auto vertex:polygon) {
                    const auto p=base.point(vertex);
                    if(p==a || p==b || p==c) continue;
                    if(turn(a,b,p)>=0 && turn(b,c,p)>=0 && turn(c,a,p)>=0) { occupied=true;break; }
                }
                if(occupied) continue;
                result.push_back({polygon[prev],polygon[i],polygon[next]});
                polygon.erase(polygon.begin()+static_cast<std::ptrdiff_t>(i));found=true;break;
            }
            if(!found) return fail(ErrorCode::InvalidPolygon,"flat hole triangulation stalled");
        }
        if(turn(base.point(polygon[0]),base.point(polygon[1]),base.point(polygon[2]))<=0)
            return fail(ErrorCode::InvalidPolygon,"degenerate flat hole triangle");
        result.push_back(std::move(polygon));
    }
    if(std::ranges::find(assigned,false)!=assigned.end()) return fail(ErrorCode::InvalidPolygon,"uncontained flat hole");
    return result;
}
} // namespace
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
    data.omitted_count.resize(columns);data.merged_count.resize(columns);data.bottom_adjusted_count.resize(columns);
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
                data.bottom_adjusted_count[c]=1;
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
            for(std::size_t l=0;l<layers;++l)
                if(original[l+1]>original[l] && levels[l+1]==levels[l]) ++data.merged_count[c];
            for(std::size_t l=1;l<layers;++l) if(interval[l]!=interval[l-1] && levels[l]!=original[l])
                return fail(ErrorCode::InvalidArgument,"thickness policy moved a regional interface");
        }
        const auto generator=base.site(column); data.base_generator.push_back(generator);
        for(std::size_t l=0;l<layers;++l) if(levels[l+1]>levels[l]) {
            if(options.omit_layer && options.omit_layer(levels[l+1]-levels[l],minimum)) {
                ++data.omitted_count[c];continue;
            }
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
    const auto vertex_hash=[](const std::pair<VertexId,Real>& key) {
        return std::hash<std::size_t>{}(key.first.index()) ^ (std::hash<Real>{}(key.second)<<1);
    };
    std::unordered_map<std::pair<VertexId,Real>,VertexId,decltype(vertex_hash)> vertices;
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
        const auto parts=horizontal_parts(base,std::move(next[c]));
        if(!parts) return std::unexpected(parts.error());
        std::vector<std::size_t> active;
        for(std::size_t l=0;l<layers;++l) if(cells[c][l].valid()) active.push_back(l);
        for(const auto& ring:*parts) for(std::size_t index=0;index<active.size();++index) {
            const auto l=active[index];const auto cell=cells[c][l];std::vector<VertexId> polygon;
            for(auto v:ring) polygon.push_back(vertex_id(v,data.column_levels[c][l]));
            const bool connected_below=index>0 && data.column_levels[c][active[index-1]+1]==data.column_levels[c][l];
            if(connected_below) append(polygon,cells[c][active[index-1]],cell,l,0);
            else { std::ranges::reverse(polygon);append(polygon,cell,{},l,bottom_patch); }
            const bool connected_above=index+1<active.size() && data.column_levels[c][l+1]==data.column_levels[c][active[index+1]];
            if(!connected_above) {
                polygon.clear();for(auto v:ring) polygon.push_back(vertex_id(v,data.column_levels[c][l+1]));
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
    std::vector<std::size_t> counts(step_patch+2);
    for(const auto& face:faces) ++counts[face.neighbour.valid()?0:face.patch+1];
    std::vector<std::size_t> offsets(counts.size()+1);
    std::partial_sum(counts.begin(),counts.end(),offsets.begin()+1);
    auto cursor=offsets;std::vector<std::size_t> order(faces.size());
    for(std::size_t i=0;i<faces.size();++i)
        order[cursor[faces[i].neighbour.valid()?0:faces[i].patch+1]++]=i;
    // Internal CSR faces require lexicographic (owner, neighbour) ordering.
    // IDs are 32-bit; eight stable byte passes avoid a global comparison sort.
    std::vector<std::size_t> scratch(counts[0]);
    for(unsigned shift=0;shift<64;shift+=8) {
        std::array<std::size_t,256> bins{};
        const auto digit=[&](std::size_t index) {
            const auto& face=faces[index];
            const auto key=(static_cast<std::uint64_t>(face.owner.index())<<32)|face.neighbour.index();
            return static_cast<std::size_t>((key>>shift)&255U);
        };
        for(std::size_t i=0;i<counts[0];++i) ++bins[digit(order[i])];
        std::size_t begin=0;
        for(auto& size:bins) { const auto count=size;size=begin;begin+=count; }
        for(std::size_t i=0;i<counts[0];++i) scratch[bins[digit(order[i])]++]=order[i];
        std::copy(scratch.begin(),scratch.end(),order.begin());
    }
    decltype(scratch){}.swap(scratch);
    for(auto index:order) {
        auto& face=faces[index];mesh.face_vertices.push_row(face.vertices);
        std::vector<VertexId>{}.swap(face.vertices);mesh.owner.push_back(face.owner);
        if(face.neighbour.valid()) mesh.neighbour.push_back(face.neighbour);
        data.face_level.push_back(face.level);
    }
    for(std::size_t p=0;p<=step_patch;++p)
        mesh.patches.push_back({p<bottom_patch?base.patches()[p].name:p==bottom_patch?"column_bottom":p==top_patch?"column_top":"column_step",offsets[p+1],counts[p+1]});
    decltype(order){}.swap(order);
    decltype(faces){}.swap(faces);vertices.clear();
    decltype(next){}.swap(next);decltype(cells){}.swap(cells);decltype(vertex_levels){}.swap(vertex_levels);
    auto result=Mesh3D::from_data(std::move(mesh));
    if(!result) return std::unexpected(result.error());
    data.mesh=std::move(*result);
    return LayeredMesh::from_data(std::move(data));
}
} // namespace vmm::detail
