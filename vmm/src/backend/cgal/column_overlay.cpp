// SPDX-License-Identifier: GPL-3.0-or-later
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <map>
#include <span>
#include <vector>
//==============================================================================
//  External libraries
//==============================================================================
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/column_overlay.hpp>
namespace vmm {
Result<ColumnOverlay> cgal_column_overlay(std::span<const ColumnEdge> edges,
    std::size_t columns, const HorizonGrid& grid) {
    using K=CGAL::Exact_predicates_exact_constructions_kernel;
    using P=K::Point_2;
    using FT=K::FT;
    using CDT=CGAL::Constrained_Delaunay_triangulation_2<K,CGAL::Default,CGAL::Exact_intersections_tag>;
    if(grid.horizon_count()<2 || columns==0)
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
        for (const auto& e:edges) cdt.insert_constraint(point(e.a),point(e.b));
        for (std::size_t j=0;j+1<y.size();++j) for(std::size_t i=0;i+1<x.size();++i) {
            const P a(x[i],y[j]),b(x[i+1],y[j]),c(x[i+1],y[j+1]),d(x[i],y[j+1]);
            cdt.insert_constraint(a,b); cdt.insert_constraint(b,c);
            cdt.insert_constraint(c,d); cdt.insert_constraint(d,a); cdt.insert_constraint(a,c);
        }
        // Winding numbers classify holes and disconnected components without assuming convexity.
        const auto locate=[&](const P& p)->Result<CellId>{
            std::vector<int> winding(columns,0);
            for(const auto& e:edges) {
                P a=point(e.a), b=point(e.b);
                int w=0;
                if(a.y()<=p.y() && b.y()>p.y() && CGAL::orientation(a,b,p)==CGAL::LEFT_TURN) w=1;
                if(b.y()<=p.y() && a.y()>p.y() && CGAL::orientation(a,b,p)==CGAL::RIGHT_TURN) w=-1;
                winding[e.owner.index()]+=w;
                if(e.neighbour.valid()) winding[e.neighbour.index()]-=w;
            }
            CellId id;
            for(std::size_t c=0;c<columns;++c) if(winding[c]!=0) {
                if(winding[c]!=1 || id.valid()) return fail(ErrorCode::InvalidPolygon,"overlapping or inverted base cells");
                id=CellId::from_index(c);
            }
            return id;
        };
        std::map<P,std::size_t> ids;
        struct Tri { std::array<P,3> p; CellId column; };
        std::vector<Tri> tris;
        for(auto f=cdt.finite_faces_begin();f!=cdt.finite_faces_end();++f) {
            const P a=f->vertex(0)->point(),b=f->vertex(1)->point(),c=f->vertex(2)->point();
            const P centre((a.x()+b.x()+c.x())/3,(a.y()+b.y()+c.y())/3);
            auto column=locate(centre);
            if(!column) return std::unexpected(column.error());
            if(!column->valid()) continue;
            tris.push_back({{a,b,c},*column});
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
        for(const auto& t:tris) {
            std::array<std::size_t,3> v{ids.at(t.p[0]),ids.at(t.p[1]),ids.at(t.p[2])};
            // Exact intersections can map to the same representable point. Apply
            // the same quotient to every triangle to preserve shared topology.
            if(v[0]==v[1] || v[1]==v[2] || v[2]==v[0]) continue;
            std::rotate(v.begin(),std::min_element(v.begin(),v.end()),v.end());
            std::array<PatchId,3> patches{};
            std::array<P,3> ordered=t.p;
            for(std::size_t k=0;k<3;++k) for(std::size_t j=0;j<3;++j)
                if(ids.at(t.p[j])==v[k]) ordered[k]=t.p[j];
            for(std::size_t k=0;k<3;++k) for(const auto& e:edges) if(!e.neighbour.valid()) {
                const K::Segment_2 segment(point(e.a),point(e.b));
                if(segment.has_on(ordered[k]) && segment.has_on(ordered[(k+1)%3]))
                    patches[k]=e.patch;
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
