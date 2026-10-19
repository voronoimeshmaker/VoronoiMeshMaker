// SPDX-License-Identifier: GPL-3.0-or-later
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <format>
#include <map>
#include <queue>
#include <set>
#include <span>
#include <vector>
//==============================================================================
//  External libraries
//==============================================================================
#include <CGAL/Cartesian.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Constrained_triangulation_plus_2.h>
#include <CGAL/Gmpq.h>
#include <CGAL/Snap_rounding_2.h>
#include <CGAL/Snap_rounding_traits_2.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/column_overlay.hpp>
namespace vmm {
Result<ColumnOverlay> cgal_column_overlay(std::span<const ColumnEdge> edges,
    std::size_t columns, const HorizonGrid& grid) {
    using K=CGAL::Cartesian<CGAL::Gmpq>;
    using P=K::Point_2;
    using FT=K::FT;
    using BaseCDT=CGAL::Constrained_Delaunay_triangulation_2<K,CGAL::Default,CGAL::Exact_intersections_tag>;
    using CDT=CGAL::Constrained_triangulation_plus_2<BaseCDT>;
    if(grid.horizon_count()<2 || columns==0 || edges.empty())
        return fail(ErrorCode::InvalidArgument,"empty overlay input");
    for(const auto& e:edges) {
        if(e.a==e.b || !std::isfinite(e.a[0]) || !std::isfinite(e.a[1]) ||
           !std::isfinite(e.b[0]) || !std::isfinite(e.b[1]) ||
           !e.owner.valid() || e.owner.index()>=columns ||
           (e.neighbour.valid() && e.neighbour.index()>=columns))
            return fail(ErrorCode::InvalidArgument,"overlay cell id");
    }
    try {
        CDT cdt;
        const auto point=[](Vec2 p){return P(p[0],p[1]);};
        const auto& x=grid.x(); const auto& y=grid.y();
        std::vector<CDT::Constraint_id> base_constraints;
        std::vector<K::Segment_2> segments;
        Vec2 lo=edges.front().a,hi=lo;
        for(const auto& e:edges) {
            segments.emplace_back(point(e.a),point(e.b));
            for(const auto& p:{e.a,e.b}) for(std::size_t k=0;k<2;++k) {
                lo[k]=std::min(lo[k],p[k]); hi[k]=std::max(hi[k],p[k]);
            }
        }
        if(lo[0]<x.front() || hi[0]>x.back() || lo[1]<y.front() || hi[1]>y.back())
            return fail(ErrorCode::InvalidArgument,"horizon grid does not cover footprint");
        const auto first_index=[](const auto& axis,Real value) {
            const auto it=std::lower_bound(axis.begin(),axis.end(),value);
            const auto index=static_cast<std::size_t>(it-axis.begin());
            return index==0 ? 0 : index-1;
        };
        const auto last_index=[](const auto& axis,Real value) {
            return std::min(axis.size()-1,static_cast<std::size_t>(std::upper_bound(axis.begin(),axis.end(),value)-axis.begin()));
        };
        for(std::size_t j=first_index(y,lo[1]);j<last_index(y,hi[1]);++j)
            for(std::size_t i=first_index(x,lo[0]);i<last_index(x,hi[0]);++i) {
                const P a(x[i],y[j]),b(x[i+1],y[j]),c(x[i+1],y[j+1]),d(x[i],y[j+1]);
                segments.emplace_back(a,b); segments.emplace_back(b,c);
                segments.emplace_back(c,d); segments.emplace_back(d,a); segments.emplace_back(a,c);
            }
        // Work near the footprint origin to avoid losing pixel precision in UTM.
        const Real length=norm(hi-lo);
        const Real pixel=std::exp2(std::floor(std::log2(1e-12*length)));
        if(!(pixel>0) || !std::isfinite(pixel))
            return fail(ErrorCode::InvalidPolygon,"unrepresentable snap-rounding scale");
        const FT shift_x=FT(lo[0])-FT(pixel)/2,shift_y=FT(lo[1])-FT(pixel)/2;
        for(auto& segment:segments) segment=K::Segment_2(
            P(segment.source().x()-shift_x,segment.source().y()-shift_y),
            P(segment.target().x()-shift_x,segment.target().y()-shift_y));
        std::vector<std::vector<P>> polylines;
        CGAL::snap_rounding_2<CGAL::Snap_rounding_traits_2<K>>(
            segments.begin(),segments.end(),polylines,FT(pixel),true,false);
        for(std::size_t line=0;line<polylines.size();++line) {
            auto& polyline=polylines[line];
            for(auto& p:polyline) p=P(
                std::clamp(p.x()+shift_x,FT(x.front()),FT(x.back())),
                std::clamp(p.y()+shift_y,FT(y.front()),FT(y.back())));
            polyline.erase(std::unique(polyline.begin(),polyline.end()),polyline.end());
            if(polyline.size()<2) {
                if(line<edges.size()) return fail(ErrorCode::InvalidPolygon,"base edge collapses at snap-rounding precision");
                continue;
            }
            const auto id=cdt.insert_constraint(polyline.begin(),polyline.end());
            if(line<edges.size()) base_constraints.push_back(id);
        }
        using Face=CDT::Face_handle;
        using Side=std::pair<Face,int>;
        std::set<Side> barriers;
        std::map<Face,CellId> labels;
        std::queue<Face> pending;
        std::map<std::pair<P,P>,PatchId> boundary_patches;
        const auto key=[](P a,P b){return b<a ? std::pair(b,a) : std::pair(a,b);};
        const auto seed=[&](Face face,CellId id)->Status {
            if(cdt.is_infinite(face)) {
                if(id.valid()) return fail(ErrorCode::InvalidPolygon,"inverted base boundary");
                return {};
            }
            const auto [it,added]=labels.emplace(face,id);
            if(!added && it->second!=id)
                return fail(ErrorCode::InvalidPolygon,std::format("overlay columns {} and {} conflict",it->second.index(),id.index()));
            if(added) pending.push(face);
            return {};
        };
        for(std::size_t edge=0;edge<edges.size();++edge) {
            const auto& e=edges[edge];
            auto vertices=cdt.vertices_in_constraint(base_constraints[edge]);
            auto it=vertices.begin();
            auto previous=*it++;
            for(;it!=vertices.end();++it) {
                const auto current=*it;
                Face face; int opposite=0;
                if(!cdt.is_edge(previous,current,face,opposite))
                    return fail(ErrorCode::InvalidPolygon,"missing base subconstraint");
                const Face other=face->neighbor(opposite);
                barriers.emplace(face,opposite);
                barriers.emplace(other,other->index(face));
                for(const Face side:{face,other}) {
                    CellId id;
                    if(!cdt.is_infinite(side)) {
                        const int index=side==face ? opposite : other->index(face);
                        const auto turn=CGAL::orientation(previous->point(),current->point(),side->vertex(index)->point());
                        id=turn==CGAL::LEFT_TURN ? e.owner : e.neighbour;
                    } else {
                        const Face finite=side==face ? other : face;
                        const int index=side==face ? other->index(face) : opposite;
                        const auto turn=CGAL::orientation(previous->point(),current->point(),finite->vertex(index)->point());
                        id=turn==CGAL::LEFT_TURN ? e.neighbour : e.owner;
                    }
                    auto status=seed(side,id);
                    if(!status) return std::unexpected(status.error());
                }
                if(!e.neighbour.valid()) boundary_patches.emplace(key(previous->point(),current->point()),e.patch);
                previous=current;
            }
        }
        while(!pending.empty()) {
            const Face face=pending.front(); pending.pop();
            for(int i=0;i<3;++i) if(!barriers.contains({face,i})) {
                auto status=seed(face->neighbor(i),labels.at(face));
                if(!status) return std::unexpected(status.error());
            }
        }
        std::map<P,std::size_t> ids;
        struct Tri { std::array<P,3> p; CellId column; };
        std::vector<Tri> tris;
        for(auto f=cdt.finite_faces_begin();f!=cdt.finite_faces_end();++f) {
            const P a=f->vertex(0)->point(),b=f->vertex(1)->point(),c=f->vertex(2)->point();
            const auto label=labels.find(f);
            if(label==labels.end() || !label->second.valid()) continue;
            tris.push_back({{a,b,c},label->second});
            ids.emplace(a,0); ids.emplace(b,0); ids.emplace(c,0);
        }
        ColumnOverlay out;
        out.elevations.resize(grid.horizon_count());
        std::map<std::pair<Real,Real>,std::size_t> rounded_ids;
        for(auto& [p,id]:ids) {
            const Vec2 rounded{CGAL::to_double(p.x()),CGAL::to_double(p.y())};
            const auto [entry,inserted]=rounded_ids.emplace(std::pair(rounded[0],rounded[1]),out.points.size());
            id=entry->second;
            if(!inserted) continue;
            out.points.push_back(rounded);
            const auto interval=[](const auto& axis,const FT& q) {
                auto it=std::upper_bound(axis.begin(),axis.end(),q,[](const FT& a,Real b){return a<FT(b);});
                return std::min(axis.size()-2,static_cast<std::size_t>(it-axis.begin()-1));
            };
            if(p.x()<x.front() || p.x()>x.back() || p.y()<y.front() || p.y()>y.back())
                return fail(ErrorCode::InvalidArgument,"horizon grid does not cover footprint");
            const auto i=interval(x,p.x()),j=interval(y,p.y());
            const FT u=(p.x()-FT(x[i]))/(FT(x[i+1])-FT(x[i]));
            const FT v=(p.y()-FT(y[j]))/(FT(y[j+1])-FT(y[j]));
            const std::size_t a=j*x.size()+i,b=a+1,d=a+x.size(),c=d+1;
            for(std::size_t h=0;h<grid.horizon_count();++h) {
                const auto& z=grid.elevations()[h];
                const FT value=u>=v ? (1-u)*FT(z[a])+(u-v)*FT(z[b])+v*FT(z[c])
                                   : (1-v)*FT(z[a])+u*FT(z[c])+(v-u)*FT(z[d]);
                out.elevations[h].push_back(CGAL::to_double(value));
            }
        }
        std::map<std::pair<std::size_t,std::size_t>,PatchId> rounded_patches;
        for(const auto& [edge,patch]:boundary_patches) {
            const auto a=ids.find(edge.first),b=ids.find(edge.second);
            if(a!=ids.end() && b!=ids.end() && a->second!=b->second)
                rounded_patches.emplace(std::minmax(a->second,b->second),patch);
        }
        for(const auto& t:tris) {
            std::array<std::size_t,3> v{ids.at(t.p[0]),ids.at(t.p[1]),ids.at(t.p[2])};
            // Exact intersections can map to the same representable point. Apply
            // the same quotient to every triangle to preserve shared topology.
            if(v[0]==v[1] || v[1]==v[2] || v[2]==v[0]) continue;
            std::rotate(v.begin(),std::min_element(v.begin(),v.end()),v.end());
            std::array<PatchId,3> patches{};
            for(std::size_t k=0;k<3;++k) {
                const auto patch=rounded_patches.find(std::minmax(v[k],v[(k+1)%3]));
                if(patch!=rounded_patches.end()) patches[k]=patch->second;
            }
            out.triangles.push_back({v,t.column,patches});
        }
        std::vector<bool> represented(columns,false),exact_present(columns,false);
        for(const auto& t:tris) exact_present[t.column.index()]=true;
        for(const auto& t:out.triangles) represented[t.column.index()]=true;
        for(std::size_t c=0;c<columns;++c) if(exact_present[c] && !represented[c])
            return fail(ErrorCode::InvalidPolygon,"base cell disappears at output coordinate precision");
        std::ranges::sort(out.triangles,[](const auto& a,const auto& b){
            return std::pair(a.column,a.vertices)<std::pair(b.column,b.vertices);
        });
        return out;
    } catch(const std::exception& e) {
        return fail(ErrorCode::BackendFailure,e.what());
    }
}
} // namespace vmm
