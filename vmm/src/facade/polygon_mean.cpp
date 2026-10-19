// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/layered.hpp>
namespace vmm {
Result<Real> PolygonMeanSampler::operator()(const Mesh2D& base,const HorizonGrid& grid,
    CellId column,std::size_t horizon) const {
    if(&base!=base_ || !column.valid() || column.index()>=base.cell_count() || horizon>=grid.horizon_count())
        return fail(ErrorCode::InvalidArgument,"invalid polygon mean input or different base");
    const auto cross=[](Vec2 a,Vec2 b,Vec2 c) {
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
    };
    const auto cell_faces=index_.faces_of(column);
    if(cell_faces.empty()) return fail(ErrorCode::InvalidPolygon,"empty polygon mean boundary");
    const auto origin=base.point(base.face_vertices(cell_faces.front()).front());
    long double integral=0,area=0;
    for(auto face:cell_faces) {
        const auto edge=base.face_vertices(face);
        if(edge.size()!=2) return fail(ErrorCode::InvalidPolygon,"polygon mean requires edges");
        const bool owner=base.owner(face)==column;
        const auto a=base.point(edge[owner?0:1]),b=base.point(edge[owner?1:0]);
        for(auto v:{a,b}) if(v[0]<grid.x().front() || v[0]>grid.x().back() ||
                             v[1]<grid.y().front() || v[1]>grid.y().back())
            return fail(ErrorCode::InvalidArgument,"polygon outside horizon grid");
        const Real signed_area=cross(origin,a,b)/2;
        if(signed_area==0) continue;
        const Real sign=signed_area>0?1:-1;
        const auto start=[](const auto& axis,Real q) {
            return static_cast<std::size_t>(std::upper_bound(axis.begin(),axis.end(),q)-axis.begin()-1);
        };
        const Real xmin=std::min({origin[0],a[0],b[0]}),xmax=std::max({origin[0],a[0],b[0]});
        const Real ymin=std::min({origin[1],a[1],b[1]}),ymax=std::max({origin[1],a[1],b[1]});
        for(std::size_t j=start(grid.y(),ymin);j+1<grid.y().size() && grid.y()[j]<ymax;++j)
        for(std::size_t i=start(grid.x(),xmin);i+1<grid.x().size() && grid.x()[i]<xmax;++i) {
            const Vec2 sw{grid.x()[i],grid.y()[j]},se{grid.x()[i+1],grid.y()[j]},
                       ne{grid.x()[i+1],grid.y()[j+1]},nw{grid.x()[i],grid.y()[j+1]};
            for(const auto& clip: {std::array<Vec2,3>{sw,se,ne},std::array<Vec2,3>{sw,ne,nw}}) {
                std::vector<Vec2> polygon=sign>0?std::vector<Vec2>{origin,a,b}:std::vector<Vec2>{origin,b,a};
                for(std::size_t side=0;side<3 && !polygon.empty();++side) {
                    std::vector<Vec2> output;
                    auto previous=polygon.back(); Real dp=cross(clip[side],clip[(side+1)%3],previous);
                    for(auto current:polygon) {
                        const Real dc=cross(clip[side],clip[(side+1)%3],current);
                        if((dp>=0)!=(dc>=0)) {
                            const Real t=dp/(dp-dc);
                            output.push_back({std::lerp(previous[0],current[0],t),std::lerp(previous[1],current[1],t)});
                        }
                        if(dc>=0) output.push_back(current);
                        previous=current; dp=dc;
                    }
                    polygon=std::move(output);
                }
                if(polygon.size()<3) continue;
                // Each clipped polygon lies in one affine terrain triangle.
                const auto node=j*grid.x().size()+i; const auto& z=grid.elevations()[horizon];
                const auto height=[&](Vec2 p) {
                    const Real u=(p[0]-sw[0])/(se[0]-sw[0]),v=(p[1]-sw[1])/(nw[1]-sw[1]);
                    return clip[1]==se ? z[node]+u*(z[node+1]-z[node])+v*(z[node+grid.x().size()+1]-z[node+1])
                        : z[node]+v*(z[node+grid.x().size()]-z[node])+u*(z[node+grid.x().size()+1]-z[node+grid.x().size()]);
                };
                for(std::size_t k=1;k+1<polygon.size();++k) {
                    const long double weight=sign*cross(polygon[0],polygon[k],polygon[k+1])/2;
                    area+=weight;
                    integral+=weight*(static_cast<long double>(height(polygon[0]))+height(polygon[k])+height(polygon[k+1]))/3;
                }
            }
        }
    }
    if(!(area>0) || !std::isfinite(integral/area))
        return fail(ErrorCode::InvalidPolygon,"nonpositive or nonfinite polygon mean area");
    return static_cast<Real>(integral/area);
}
} // namespace vmm
