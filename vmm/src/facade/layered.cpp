// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/column_overlay.hpp>
#include <vmm/geometry/polygon.hpp>
#include <vmm/layered.hpp>
#include <vmm/mesh/invariants.hpp>
namespace vmm {
Result<LayeredMesh> generate_layered_mesh(const Mesh2D& base, const HorizonGrid& horizons,
    std::vector<std::vector<Real>> fractions) {
    if(base.cell_count()==0 || horizons.horizon_count()<2)
        return fail(ErrorCode::InvalidArgument,"empty base or horizons");
    const std::size_t intervals=horizons.horizon_count()-1;
    if(fractions.empty()) fractions.assign(intervals,{0,1});
    if(fractions.size()!=intervals) return fail(ErrorCode::InvalidArgument,"one fraction list per horizon interval");
    std::vector<std::size_t> interval;
    std::vector<std::pair<Real,Real>> cuts;
    for(std::size_t h=0;h<intervals;++h) {
        const auto& f=fractions[h];
        if(f.size()<2 || f.front()!=0 || f.back()!=1 ||
            !std::ranges::all_of(f,[](Real v){return std::isfinite(v);}) ||
            std::adjacent_find(f.begin(),f.end(),std::greater_equal<Real>{})!=f.end())
            return fail(ErrorCode::InvalidArgument,"fractions must increase strictly from 0 to 1");
        for(std::size_t k=1;k<f.size();++k) { interval.push_back(h); cuts.emplace_back(f[k-1],f[k]); }
    }
    const auto limit=static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
    if(cuts.size()>=limit/base.cell_count() || intervals>=limit/base.regions().size())
        return fail(ErrorCode::InvalidArgument,"layered mesh exceeds ID capacity");
    std::vector<ColumnEdge> edges;
    for(FaceId f:base.faces()) {
        const auto v=base.face_vertices(f);
        if(v.size()!=2) return fail(ErrorCode::InvalidPolygon,"base faces must have two vertices");
        const Vec2 a=base.point(v[0]),b=base.point(v[1]);
        if(a==b || !std::isfinite(a[0]) || !std::isfinite(a[1]) || !std::isfinite(b[0]) || !std::isfinite(b[1]))
            return fail(ErrorCode::InvalidPolygon,"invalid footprint edge");
        edges.push_back({a,b,base.owner(f),base.neighbour(f),base.patch(f)});
    }
    auto overlay=cgal_column_overlay(edges,base.cell_count(),horizons);
    if(!overlay) return std::unexpected(overlay.error());
    const auto& ov=*overlay;
    if(ov.triangles.empty()) return fail(ErrorCode::InvalidPolygon,"empty footprint");
    const auto height=[&](std::size_t p,std::size_t layer,Real t) {
        const auto h=interval[layer];
        const Real a=ov.elevations[h][p],b=ov.elevations[h+1][p];
        return t==0 ? a : t==1 ? b : std::lerp(a,b,t);
    };
    const std::size_t nl=cuts.size();
    std::vector<Real> volume(base.cell_count()*nl,0);
    std::vector<Vec3> moment(volume.size());
    // Analytic integral of affine thickness, independent of face-based invariants.
    for(const auto& tri:ov.triangles) {
        const auto v=tri.vertices;
        const Real area=cross(ov.points[v[1]]-ov.points[v[0]],ov.points[v[2]]-ov.points[v[0]])/2;
        if(!(area>0)) {
            std::ostringstream detail; detail.precision(17);
            detail << "rounded overlay triangle is degenerate:";
            for(auto p:v) detail << " (" << ov.points[p][0] << "," << ov.points[p][1] << ")";
            return fail(ErrorCode::InvalidPolygon,detail.str());
        }
        for(std::size_t l=0;l<nl;++l) {
            std::array<Real,3> lo{},hi{},d{};
            Real sum=0;
            for(std::size_t k=0;k<3;++k) {
                lo[k]=height(v[k],l,cuts[l].first); hi[k]=height(v[k],l,cuts[l].second);
                d[k]=hi[k]-lo[k]; sum+=d[k];
            }
            if(sum==0) continue;
            const auto c=tri.column.index()*nl+l;
            const Real vol=area*sum/3;
            volume[c]+=vol;
            // Integrals of barycentric products: area/6 diagonal, area/12 off-diagonal.
            for(std::size_t i=0;i<3;++i) for(std::size_t j=0;j<3;++j) {
                const Real w=area*(i==j ? 1.0/6 : 1.0/12);
                moment[c][0]+=w*ov.points[v[i]][0]*d[j];
                moment[c][1]+=w*ov.points[v[i]][1]*d[j];
                moment[c][2]+=0.5*w*(d[i]*hi[j]+lo[i]*d[j]);
            }
        }
    }
    LayeredData result;
    result.horizons=horizons; result.column_count=base.cell_count(); result.fractions=fractions;
    MeshData<3> md;
    md.media=base.media();
    for(const auto& reg:base.regions()) for(std::size_t h=0;h<intervals;++h)
        md.regions.push_back({reg.name+"/h"+std::to_string(h),reg.medium});
    std::vector<CellId> cell(volume.size());
    std::vector<Real> expected;
    for(std::size_t c=0;c<volume.size();++c) if(volume[c]>0) {
        cell[c]=CellId::from_index(md.sites.size());
        md.sites.push_back((1/volume[c])*moment[c]);
        const auto col=c/nl,l=c%nl;
        result.column.push_back(col); result.layer.push_back(l);
        md.cell_region.push_back(RegionId::from_index(base.region(CellId::from_index(col)).index()*intervals+interval[l]));
        md.cell_input_site.push_back(SiteId::from_index(col));
        expected.push_back(volume[c]);
    }
    for(Real v:volume) if(!std::isfinite(v) || v<0)
        return fail(ErrorCode::InvalidArgument,"non-finite or negative integrated volume");
    if(md.sites.empty()) return fail(ErrorCode::InvalidArgument,"all intervals have zero volume");
    struct Face {
        std::vector<VertexId> vertices;
        CellId owner,neighbour;
        std::size_t level=lateral_face,patch=0,uses=1;
        bool cancelled=false;
    };
    std::map<std::tuple<std::size_t,Real>,VertexId> vertex;
    const auto point_id=[&](std::size_t p,Real z) {
        const auto [it,added]=vertex.emplace(std::tuple(p,z),VertexId::from_index(md.points.size()));
        if(added) md.points.push_back({ov.points[p][0],ov.points[p][1],z});
        return it->second;
    };
    std::map<std::vector<VertexId>,Face> faces;
    const auto add=[&](std::vector<VertexId> v,CellId c,std::size_t level,std::size_t patch)->Status {
        v.erase(std::unique(v.begin(),v.end()),v.end());
        if(v.size()>1 && v.front()==v.back()) v.pop_back();
        if(v.size()<3) return {};
        auto key=v; std::ranges::sort(key);
        const auto [it,added]=faces.emplace(key,Face{v,c,{},level,patch,1,false});
        if(!added) {
            auto& f=it->second;
            if(++f.uses>2) return fail(ErrorCode::InterfaceNotConforming,"non-manifold layered face");
            if(f.owner==c) f.cancelled=true;
            else {
                f.neighbour=c; f.level=std::min(f.level,level);
                if(c<f.owner) { std::swap(f.owner,f.neighbour); std::ranges::reverse(f.vertices); }
            }
        }
        return {};
    };
    for(const auto& tri:ov.triangles) for(std::size_t l=0;l<nl;++l) {
        const auto v=tri.vertices;
        std::array<VertexId,3> a{},b{};
        bool positive=false;
        for(std::size_t k=0;k<3;++k)
            positive=positive || height(v[k],l,cuts[l].second)>height(v[k],l,cuts[l].first);
        if(!positive) continue;
        const CellId c=cell[tri.column.index()*nl+l];
        for(std::size_t k=0;k<3;++k) {
            a[k]=point_id(v[k],height(v[k],l,cuts[l].first));
            b[k]=point_id(v[k],height(v[k],l,cuts[l].second));
        }
        auto st=add({a[2],a[1],a[0]},c,l,base.patches().size());
        if(!st) return std::unexpected(st.error());
        st=add({b[0],b[1],b[2]},c,l+1,base.patches().size()+1);
        if(!st) return std::unexpected(st.error());
        for(std::size_t k=0;k<3;++k) {
            const auto j=(k+1)%3;
            st=add({a[k],a[j],b[j],b[k]},c,lateral_face,tri.patches[k].valid()?tri.patches[k].index():base.patches().size()+2);
            if(!st) return std::unexpected(st.error());
        }
    }
    if(md.points.size()>=limit || faces.size()>=limit)
        return fail(ErrorCode::InvalidArgument,"layered mesh exceeds ID capacity");
    std::vector<Face> sorted;
    for(auto& [key,f]:faces) if(!f.cancelled) {
        if(!f.neighbour.valid() && f.patch>=base.patches().size()+2)
            return fail(ErrorCode::InterfaceNotConforming,"unpaired internal lateral face");
        sorted.push_back(std::move(f));
    }
    std::ranges::sort(sorted,[](const Face& a,const Face& b) {
        return std::tuple(!a.neighbour.valid(),a.neighbour.valid()?0:a.patch,a.owner,a.neighbour,a.vertices) <
               std::tuple(!b.neighbour.valid(),b.neighbour.valid()?0:b.patch,b.owner,b.neighbour,b.vertices);
    });
    for(const auto& f:sorted) {
        md.face_vertices.push_row(f.vertices); md.owner.push_back(f.owner);
        if(f.neighbour.valid()) md.neighbour.push_back(f.neighbour);
        result.face_level.push_back(f.level);
    }
    std::size_t next=md.neighbour.size();
    for(std::size_t p=0;p<base.patches().size()+2;++p) {
        const auto begin=next;
        while(next<sorted.size() && sorted[next].patch==p) ++next;
        md.patches.push_back({p<base.patches().size() ? base.patches()[p].name :
            p==base.patches().size() ? "column_bottom" : "column_top",begin,next-begin});
    }
    auto mesh=Mesh3D::from_data(std::move(md));
    if(!mesh) return std::unexpected(mesh.error());
    InvariantReference ref;
    Vec3 low=mesh->points().front(),high=low;
    for(const Vec3& p:mesh->points()) for(std::size_t k=0;k<3;++k) {
        low[k]=std::min(low[k],p[k]); high[k]=std::max(high[k],p[k]);
    }
    ref.length_scale=norm(high-low); ref.cell_measure=expected;
    ref.total_measure=std::accumulate(expected.begin(),expected.end(),Real{0});
    ref.region_measure.resize(mesh->regions().size(),0);
    for(CellId c:mesh->cells()) ref.region_measure[mesh->region(c).index()]+=expected[c.index()];
    const auto report=check_invariants(*mesh,ref);
    if(!report.passed(ref)) return fail(ErrorCode::InvariantViolated,"layered mesh: "+report.first_problem);
    result.mesh=std::move(*mesh);
    return LayeredMesh::from_data(std::move(result));
}
} // namespace vmm
