// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/cgal.hpp>
#include <vmm/cvt.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/mesh/metrics.hpp>
#include <vmm/voronoi/builder2d.hpp>
#include <vmm/voronoi/builder3d.hpp>
namespace vmm {
template<std::size_t D>
Real cvt_energy(const Mesh<D>& mesh) {
    Real energy=0;
    for(FaceId f:mesh.faces()) for(CellId cell:{mesh.owner(f),mesh.neighbour(f)}) {
        if(!cell.valid()) continue;
        const Real sign=cell==mesh.owner(f)?1:-1;
        const auto vertices=mesh.face_vertices(f);
        const auto origin=mesh.site(cell);
        if constexpr(D==2) {
            const auto a=mesh.point(vertices[0])-origin,b=mesh.point(vertices[1])-origin;
            energy+=sign*cross(a,b)*(dot(a,a)+dot(a,b)+dot(b,b))/12;
        } else {
            const auto a=mesh.point(vertices[0])-origin;
            for(std::size_t k=1;k+1<vertices.size();++k) {
                const auto b=mesh.point(vertices[k])-origin,c=mesh.point(vertices[k+1])-origin;
                energy+=sign*dot(a,cross(b,c))*
                    (dot(a,a)+dot(b,b)+dot(c,c)+dot(a,b)+dot(a,c)+dot(b,c))/60;
            }
        }
    }
    return energy;
}
namespace {

bool same_sides(RegionId a,RegionId b,RegionId x,RegionId y) {
    return (a==x && b==y)||(a==y && b==x);
}
bool supported(const Partition2D& p,Vec2 point,RegionId a,RegionId b,PatchId patch,Real tol) {
    for(const auto& s:p.segments()) {
        if(!same_sides(a,b,s.left,s.right) || (!b.valid() && s.patch!=patch)) continue;
        const auto u=p.vertices()[s.v0],v=p.vertices()[s.v1],edge=v-u;
        const Real length=norm(edge);
        if(length>0 && std::abs(cross(edge,point-u))<=tol*length &&
           dot(point-u,edge)>=-tol*length && dot(point-v,edge)<=tol*length) return true;
    }
    return false;
}
bool supported(const Partition3D& p,Vec3 point,RegionId a,RegionId b,PatchId patch,Real tol) {
    for(const auto& t:p.triangles()) {
        if(!same_sides(a,b,t.inside,t.outside) || (!b.valid() && t.patch!=patch)) continue;
        const auto u=p.vertices()[t.v[0]],v=p.vertices()[t.v[1]],w=p.vertices()[t.v[2]];
        const auto normal=cross(v-u,w-u);
        const Real area=norm(normal);
        if(area==0 || std::abs(dot(point-u,normal))>tol*area) continue;
        const auto n=(1/area)*normal;
        if(dot(cross(v-u,point-u),n)>=-tol*norm(v-u) &&
           dot(cross(w-v,point-v),n)>=-tol*norm(w-v) &&
           dot(cross(u-w,point-w),n)>=-tol*norm(u-w)) return true;
    }
    return false;
}
template<class Partition,std::size_t D>
bool fixed_support(const Partition& p,const Mesh<D>& m) {
    const Real tol=1e-10*p.length_scale();
    for(FaceId f:m.faces()) {
        const auto a=m.region(m.owner(f));
        const auto b=m.is_internal(f)?m.region(m.neighbour(f)):RegionId{};
        if(a==b) continue;
        const auto verts=m.face_vertices(f);
        Vec<D> centre{};
        for(std::size_t k=0;k<verts.size();++k) {
            const auto point=m.point(verts[k]),next=m.point(verts[(k+1)%verts.size()]);
            if(!supported(p,point,a,b,m.patch(f),tol) ||
               !supported(p,0.5*(point+next),a,b,m.patch(f),tol)) return false;
            centre=centre+point;
        }
        if(!supported(p,(1/static_cast<Real>(verts.size()))*centre,a,b,m.patch(f),tol)) return false;
    }
    return true;
}

struct PlanarCvt {
    using Partition=Partition2D;
    using Sites=SiteSet;
    static constexpr std::size_t dimension=2;
    static Status validate(const Partition& p,const Sites& s) { return validate_sites(p,s); }
    static Result<Mesh2D> build(const Partition& p,const Sites& s) {
        auto result=build_mesh_2d(p,s,cgal_backend_2d());
        if(!result) return std::unexpected(result.error());
        auto ref=invariant_reference(p); ref.cell_measure=result->cell_polygon_area;
        if(!check_invariants(result->mesh,ref).passed(ref) || !fixed_support(p,result->mesh))
            return fail(ErrorCode::InvariantViolated,"CVT reconstructed 2D mesh");
        return std::move(result->mesh);
    }
};
struct SpatialCvt {
    using Partition=Partition3D;
    using Sites=SiteSet3D;
    static constexpr std::size_t dimension=3;
    static Status validate(const Partition& p,const Sites& s) { return validate_sites_3d(p,s); }
    static Result<Mesh3D> build(const Partition& p,const Sites& s) {
        auto result=build_mesh_3d(p,s,cgal_backend_3d());
        if(!result) return std::unexpected(result.error());
        auto ref=invariant_reference(p); ref.cell_measure=result->cell_volume;
        if(!check_invariants(result->mesh,ref).passed(ref) || !fixed_support(p,result->mesh))
            return fail(ErrorCode::InvariantViolated,"CVT reconstructed 3D mesh");
        return std::move(result->mesh);
    }
};
template<class Traits>
Result<CvtResult<Traits::dimension>> run(const typename Traits::Partition& partition,
    const typename Traits::Sites& input,const CvtOptions& options) {
    constexpr auto D=Traits::dimension;
    if(options.max_backtracks==0 || !std::isfinite(options.relaxation) ||
       options.relaxation<=0 || options.relaxation>1 ||
       !std::isfinite(options.relative_tolerance) || options.relative_tolerance<=0 ||
       !std::isfinite(partition.length_scale()) || partition.length_scale()<=0)
        return fail(ErrorCode::InvalidArgument,"invalid CVT controls or partition scale");
    if constexpr(requires { input.weights(); }) {
        if(!input.weights().empty())
            return fail(ErrorCode::InvalidArgument,"CVT currently requires unweighted sites");
    }
    auto valid=Traits::validate(partition,input);
    if(!valid) return std::unexpected(valid.error());
    auto initial=Traits::build(partition,input);
    if(!initial) return std::unexpected(initial.error());
    CvtResult<D> out{std::move(*initial),{}};
    auto sites=input;
    Real energy=cvt_energy(out.mesh);
    if(!std::isfinite(energy) || energy<=0)
        return fail(ErrorCode::InvariantViolated,"invalid initial CVT energy");
    out.report.energy.push_back(energy);
    for(std::size_t iteration=0;iteration<options.max_iterations;++iteration) {
        const auto metrics=compute_metrics(out.mesh);
        std::vector<Vec<D>> targets(sites.positions().begin(),sites.positions().end());
        Real residual=0;
        for(CellId c:out.mesh.cells()) {
            const auto id=out.mesh.cell_input_sites()[c.index()].index();
            if(id>=targets.size()) return fail(ErrorCode::InconsistentData,"CVT generator identity");
            targets[id]=metrics.cell_centroid[c.index()];
            residual=std::max(residual,norm(targets[id]-sites.positions()[id])/partition.length_scale());
        }
        out.report.relative_residual=residual;
        if(residual<=options.relative_tolerance) { out.report.converged=true; break; }
        Real step=options.relaxation;
        bool accepted=false;
        for(std::size_t trial=0;trial<options.max_backtracks;++trial,step*=0.5) {
            typename Traits::Sites candidate;
            for(std::size_t i=0;i<targets.size();++i)
                candidate.add(sites.positions()[i]+step*(targets[i]-sites.positions()[i]),sites.regions()[i]);
            auto admissible=Traits::validate(partition,candidate);
            if(!admissible) {
                out.report.last_rejection=admissible.error().message();
                ++out.report.rejected_steps; continue;
            }
            auto rebuilt=Traits::build(partition,candidate);
            if(!rebuilt) {
                out.report.last_rejection=rebuilt.error().message();
                ++out.report.rejected_steps; continue;
            }
            const Real next=cvt_energy(*rebuilt);
            if(!std::isfinite(next) || next<=0 || next>energy ||
               energy-next<=32*std::numeric_limits<Real>::epsilon()*energy) {
                out.report.last_rejection="no numerically resolved decrease in CVT energy";
                ++out.report.rejected_steps; continue;
            }
            out.mesh=std::move(*rebuilt); sites=std::move(candidate); energy=next;
            out.report.energy.push_back(energy);
            out.report.relative_displacement.push_back(step*residual);
            accepted=true; break;
        }
        if(!accepted) { out.report.stalled=true; break; }
    }
    // The last accepted move may itself reach the requested tolerance.
    if(!out.report.converged && !out.report.stalled) {
        const auto metrics=compute_metrics(out.mesh);
        Real residual=0;
        for(CellId c:out.mesh.cells())
            residual=std::max(residual,norm(metrics.cell_centroid[c.index()]-out.mesh.site(c))/partition.length_scale());
        out.report.relative_residual=residual;
        out.report.converged=residual<=options.relative_tolerance;
    }
    return out;
}
} // namespace
Result<CvtResult<2>> optimize_cvt(const Partition2D& p,const SiteSet& s,const CvtOptions& o) {
    return run<PlanarCvt>(p,s,o);
}
Result<CvtResult<3>> optimize_cvt(const Partition3D& p,const SiteSet3D& s,const CvtOptions& o) {
    return run<SpatialCvt>(p,s,o);
}
template Real cvt_energy(const Mesh<2>&);
template Real cvt_energy(const Mesh<3>&);
} // namespace vmm
