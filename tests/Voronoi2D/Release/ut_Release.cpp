// SPDX-License-Identifier: GPL-3.0-or-later
/** @file ut_Release.cpp
 * @brief Known-answer release gates for clipped Voronoi meshes.
 * @ingroup voronoi2d_tests
 */
#include <gtest/gtest.h>
#include <map>
#include <numbers>
#include <iostream>
#include <bit>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <CGAL/Polygon_2.h>
#include <VoronoiMeshMaker/Voronoi2D/CVT/LloydOptimizer2D.hpp>
#include <VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_XML_ClippedVoronoi2D.hpp>
#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>
#include <VoronoiMeshMaker/ClippedVoronoiBuilder2D.hpp>
#include <VoronoiMeshMaker/VoronoiBandwidth2D.hpp>
#include <VoronoiMeshMaker/VoronoiVolumeOrdering2D.hpp>
using namespace vmm::s2d;
using namespace vmm::vd2d;
namespace {
auto boundary() { return vmm::b2d::make_boundary(vmm::b2d::Rectangle(Point2{-15,-5},30,10)); }
template<class Pattern> auto mesh(Pattern pattern) {
    const auto b = boundary();
    ClippedVoronoiBuildOptions2D options;
    options.allow_parallel_cell_build = false;
    return ClippedVoronoiBuilder2D::build(make_sites(b, pattern), b, options);
}
const auto& cartesian() {
    static const auto d = mesh(CartesianGridCount2D{256,85});
    return d;
}
void pairing_and_tpfa(const ClippedVoronoiDiagram2D& d, bool regular) {
    std::size_t directed = 0, outside = 0, unpaired = 0, unequal = 0;
    Real max_length_difference=0;
    for (const auto& cell : d.cells) {
        const auto p = d.sites[static_cast<std::size_t>(cell.site_id.value)].point;
        for (const auto& e : cell.edges) {
            if (e.is_boundary_edge) { EXPECT_EQ(e.neighbour_site_id, kInvalidSiteId); continue; }
            ASSERT_NE(e.neighbour_site_id, kInvalidSiteId);
            const auto& neighbour = d.cell(e.neighbour_site_id);
            const auto q = d.sites[static_cast<std::size_t>(neighbour.site_id.value)].point;
            std::size_t reciprocal = 0;
            for (const auto& f : neighbour.edges) if (f.neighbour_site_id == cell.site_id) {
                ++reciprocal;
                const Real difference=std::abs(e.length-f.length);
                max_length_difference=std::max(max_length_difference,difference);
                unequal+=difference>1e-10;
            }
            unpaired+=reciprocal!=1U;
            const Point2 crossing{(p.x+q.x)/2, (p.y+q.y)/2};
            const Real t = ((crossing.x-e.a.x)*(e.b.x-e.a.x)+(crossing.y-e.a.y)*(e.b.y-e.a.y))/(e.length*e.length);
            EXPECT_NEAR(e.crossing_parameter,t,1e-8);
            EXPECT_EQ(e.representative_inside_local_edge,t>=0 && t<=1);
            EXPECT_EQ(e.representative_valid,e.representative_inside_local_edge);
            EXPECT_NEAR(e.generator_distance,std::hypot(p.x-q.x,p.y-q.y),1e-13);
            EXPECT_NEAR(e.face_distance,e.generator_distance/2,1e-10);
            if (regular) {
                EXPECT_TRUE(e.representative_inside_local_edge);
                EXPECT_NEAR(e.representative_point.x,e.midpoint.x,1e-11);
                EXPECT_NEAR(e.representative_point.y,e.midpoint.y,1e-11);
                EXPECT_NEAR(e.generator_distance,p.x==q.x ? 10.0/85 : 30.0/256,1e-12);
            }
            ++directed; outside += !e.representative_inside_local_edge;
        }
    }
    std::cout << "directed_faces=" << directed << " outside=" << outside
              << " unpaired="<<unpaired<<" unequal_lengths="<<unequal
              << " max_length_difference="<<std::setprecision(17)<<max_length_difference<<'\n';
    EXPECT_EQ(unpaired,0U);EXPECT_EQ(unequal,0U);
}
}
TEST(Release,T4T5Cartesian) { pairing_and_tpfa(cartesian(),true); }
TEST(Release,T4T5Random) { pairing_and_tpfa(mesh(UniformRandom2D{21760,8675309U}),false); }
TEST(Release,T3ReferenceGraphs) {
    const auto& d=cartesian();
    std::map<std::size_t,std::size_t> fv,dt,corners;
    std::size_t boundary_edges=0;
    Real max_dgc=0,area=0,perimeter=0;
    for (const auto& c:d.cells) {
        const auto nf=static_cast<std::size_t>(std::ranges::distance(c.face_neighbours()));
        ++fv[nf]; ++dt[c.delaunay_neighbours().size()];
        if(nf==2) ++corners[c.delaunay_neighbours().size()];
        area+=c.area();
        const auto p=d.sites[static_cast<std::size_t>(c.site_id.value)].point;
        const auto g=c.centroid();
        max_dgc=std::max(max_dgc,std::hypot(g.x-p.x,g.y-p.y));
        EXPECT_NEAR(c.area(),300.0/21760,1e-13);
        EXPECT_NEAR(4*std::numbers::pi*c.area()/(c.perimeter()*c.perimeter()),0.7853951556030,1e-12);
        for(const auto& e:c.edges) if(e.is_boundary_edge) {++boundary_edges;perimeter+=e.length;}
    }
    EXPECT_EQ(fv,(std::map<std::size_t,std::size_t>{{4,21082},{3,674},{2,4}}));
    EXPECT_EQ(dt,(std::map<std::size_t,std::size_t>{{6,21082},{4,674},{3,2},{2,2}}));
    EXPECT_EQ(corners,(std::map<std::size_t,std::size_t>{{3,2},{2,2}}));
    const auto a=compute_voronoi_bandwidth(d);
    const auto b=compute_voronoi_bandwidth(d,AdjacencyGraph2D::Delaunay);
    EXPECT_EQ(a.adjacency_count,43179U); EXPECT_EQ(b.adjacency_count,64599U);
    EXPECT_EQ(b.adjacency_count-a.adjacency_count,21420U);
    EXPECT_EQ(d.cells.size()+2*a.adjacency_count,108118U);
    EXPECT_EQ(d.cells.size()+2*b.adjacency_count,150958U);
    EXPECT_EQ(a.max_volume_id_distance,256U); EXPECT_EQ(b.max_volume_id_distance,256U);
    EXPECT_EQ(boundary_edges,682U); EXPECT_EQ(d.boundary_volume_count(),678U);
    EXPECT_NEAR(area,300,1e-9); EXPECT_NEAR(perimeter,80,1e-11);
    EXPECT_LE(max_dgc,5.03e-13);
    std::cout.precision(17);
    std::cout << "area="<<area<<" perimeter="<<perimeter<<" max_dgc="<<max_dgc
              <<" sizeof_cell="<<sizeof(VoronoiCell2D)<<" sizeof_edge="<<sizeof(VoronoiCellEdge2D)<<'\n';
}

namespace {
std::filesystem::path audit_dir() {
    const auto env=std::getenv("VMM_AUDIT_DIR");
    std::filesystem::path path=env ? env : "release-results";
    std::filesystem::create_directories(path);
    return path;
}
std::uint64_t fingerprint(const ClippedVoronoiDiagram2D& d) {
    std::uint64_t hash=14695981039346656037ULL;
    const auto add=[&](const auto value) {
        for(const auto b:std::bit_cast<std::array<unsigned char,sizeof(value)>>(value)) {hash^=b;hash*=1099511628211ULL;}
    };
    for(const auto& c:d.cells) {
        add(c.site_id.value);add(c.area());add(c.polygon.size());
        for(const auto p:c.polygon) {add(p.x);add(p.y);}
        for(const auto id:c.delaunay_neighbours()) add(id.value);
        for(const auto& e:c.edges) {add(e.length);add(e.neighbour_site_id.value);add(e.crossing_parameter);add(e.face_distance);}
    }
    return hash;
}
void invariant(const ClippedVoronoiDiagram2D& d) {
    long double area=0,perimeter=0;
    for(const auto& c:d.cells) {
        ASSERT_GE(c.polygon.size(),3U);
        CGAL::Polygon_2<CgalKernelTraits2D::Kernel> polygon;
        long double shoelace=0,nx=0,ny=0;
        const auto origin=c.polygon.front();
        for(std::size_t i=0;i<c.polygon.size();++i) {
            const auto p=c.polygon[i],q=c.polygon[(i+1)%c.polygon.size()];
            polygon.push_back(CgalKernelTraits2D::to_cgal(p));
            shoelace+=(static_cast<long double>(p.x)-origin.x)*(static_cast<long double>(q.y)-origin.y)
                      -(static_cast<long double>(q.x)-origin.x)*(static_cast<long double>(p.y)-origin.y);
            nx+=q.y-p.y;ny+=p.x-q.x;
        }
        EXPECT_TRUE(polygon.is_simple()); EXPECT_GT(shoelace,0);
        EXPECT_NEAR(c.area(),static_cast<Real>(shoelace/2),1e-13*c.area());
        EXPECT_NEAR(static_cast<Real>(nx),0,1e-13); EXPECT_NEAR(static_cast<Real>(ny),0,1e-13);
        area+=c.area();
        for(const auto& e:c.edges) if(e.is_boundary_edge) perimeter+=e.length;
    }
    EXPECT_NEAR(static_cast<Real>(area),300,1e-10);
    EXPECT_NEAR(static_cast<Real>(perimeter),80,1e-11);
    std::cout<<"invariants area="<<std::setprecision(17)<<area<<" perimeter="<<perimeter<<'\n';
}
void nearest_owner(const ClippedVoronoiDiagram2D& d,const std::string& family) {
    std::ofstream csv(audit_dir()/(family+"_nearest.csv"));
    csv<<"site_id,x,y,distance_advantage,tolerance,distance_to_face,distance_to_vertex\n"<<std::setprecision(17);
    std::size_t tested=0,flagged=0,interior=0;
    for(const auto& c:d.cells) {
        const auto owner=d.sites[static_cast<std::size_t>(c.site_id.value)].point;
        const auto g=c.centroid();
        const Real tol=1e-9*std::sqrt(c.area());
        const auto hint=d.delaunay.locate(CgalKernelTraits2D::to_cgal(g));
        std::vector<Point2> samples{g};
        for(std::size_t i=0;i<c.polygon.size();++i) {
            const auto a=c.polygon[i],b=c.polygon[(i+1)%c.polygon.size()];
            for(const Real weight:{Real{0.5},Real{0.99999999}}) {
                samples.push_back({g.x+(a.x-g.x)*weight,g.y+(a.y-g.y)*weight});
                samples.push_back({g.x+((a.x+b.x)/2-g.x)*weight,g.y+((a.y+b.y)/2-g.y)*weight});
            }
        }
        for(const auto p:samples) {
            ++tested;
            const auto vertex=d.delaunay.nearest_vertex(CgalKernelTraits2D::to_cgal(p),hint);
            const auto nearest=d.sites[static_cast<std::size_t>(vertex->info().value)].point;
            const Real advantage=std::hypot(p.x-owner.x,p.y-owner.y)-std::hypot(p.x-nearest.x,p.y-nearest.y);
            if(advantage<=tol) continue;
            ++flagged;
            Real dv=std::numeric_limits<Real>::infinity(),df=dv;
            for(std::size_t i=0;i<c.polygon.size();++i) {
                const auto a=c.polygon[i],b=c.polygon[(i+1)%c.polygon.size()];
                dv=std::min(dv,std::hypot(p.x-a.x,p.y-a.y));
                const auto dx=b.x-a.x,dy=b.y-a.y;
                const auto t=std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/(dx*dx+dy*dy),Real{0},Real{1});
                df=std::min(df,std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy));
            }
            interior+=df>1e-6 && dv>1e-6;
            csv<<c.site_id.value<<','<<p.x<<','<<p.y<<','<<advantage<<','<<tol<<','<<df<<','<<dv<<'\n';
        }
    }
    std::cout<<family<<" T2 tested="<<tested<<" flagged="<<flagged<<" deep_interior="<<interior<<'\n';
    EXPECT_EQ(flagged,0U)<<family<<"; detailed evidence in "<<csv.tellp()<<" bytes";
}
template<class Policy> void check_order(Policy policy,bool partition) {
    auto d=cartesian(); const auto original=d;
    const auto mapping=renumber_volumes(d,policy,VolumeRenumberingOptions2D{partition});
    std::vector<bool> seen(d.cells.size(),false);
    for(std::size_t i=0;i<d.cells.size();++i) {
        const auto old=mapping.new_to_old[i];
        ASSERT_LT(old,d.cells.size()); EXPECT_FALSE(seen[old]); seen[old]=true;
        EXPECT_EQ(mapping.old_to_new[old],i); EXPECT_EQ(d.cells[i].volume_id,i);
        const auto& c=d.cells[i];const auto& o=original.cell(c.site_id);
        EXPECT_EQ(&d.cell(c.site_id),&c);
        ASSERT_EQ(c.polygon.size(),o.polygon.size());
        EXPECT_EQ(std::bit_cast<std::uint64_t>(c.area()),std::bit_cast<std::uint64_t>(o.area()));
        for(std::size_t j=0;j<c.polygon.size();++j) {
            EXPECT_EQ(std::bit_cast<std::uint64_t>(c.polygon[j].x),std::bit_cast<std::uint64_t>(o.polygon[j].x));
            EXPECT_EQ(std::bit_cast<std::uint64_t>(c.polygon[j].y),std::bit_cast<std::uint64_t>(o.polygon[j].y));
            EXPECT_EQ(c.edges[j].neighbour_site_id,o.edges[j].neighbour_site_id);
            EXPECT_EQ(std::bit_cast<std::uint64_t>(c.edges[j].length),std::bit_cast<std::uint64_t>(o.edges[j].length));
        }
    }
    for(const auto i:d.internal_indices) EXPECT_FALSE(d.cells[i].is_boundary_cell);
    for(const auto i:d.boundary_indices) EXPECT_TRUE(d.cells[i].is_boundary_cell);
    std::size_t ni=0,nb=0;
    static_assert(std::ranges::random_access_range<decltype(d.boundary_volumes())>);
    for(const auto& c:d.internal_volumes()) {EXPECT_FALSE(c.is_boundary_cell);++ni;}
    for(const auto& c:d.boundary_volumes()) {EXPECT_TRUE(c.is_boundary_cell);++nb;}
    EXPECT_EQ(ni,21082U); EXPECT_EQ(nb,678U);
    if(partition) {
        EXPECT_TRUE(d.volumes_are_renumbered());
        EXPECT_EQ(d.internal_span().size(),21082U); EXPECT_EQ(d.boundary_span().size(),678U);
        EXPECT_EQ(d.internal_span().data(),d.cells.data());
        EXPECT_EQ(d.boundary_span().data(),d.cells.data()+21082);
    }
    static_cast<void>(renumber_volumes(d,[&](const auto&){return mapping.old_to_new;}));
    EXPECT_EQ(fingerprint(d),fingerprint(original));
}
}
TEST(Release,T6T7T8Renumbering) {
    for(const bool partition:{false,true}) {
        check_order(InputVolumeOrdering2D{},partition);
        check_order(LexicographicVolumeOrdering2D{},partition);
        check_order(HilbertVolumeOrdering2D{16},partition);
        check_order([](const auto& d){auto p=InputVolumeOrdering2D{}(d);std::reverse(p.begin(),p.end());return p;},partition);
    }
}
TEST(Release,T1T2Cartesian) { invariant(cartesian());nearest_owner(cartesian(),"cartesian"); }
TEST(Release,T1T2Hexagonal) {
    const auto d=mesh(HexagonalGrid2D{256});invariant(d);nearest_owner(d,"hexagonal");
    std::ofstream exemptions(audit_dir()/"hexagonal_exemptions.csv");
    exemptions<<"site_id,x,y,n_faces\n";
    for(const auto& c:d.internal_volumes()) {
        bool adjacent_boundary=false;
        for(const auto id:c.face_neighbours()) adjacent_boundary|=d.cell(id).is_boundary_cell;
        if(adjacent_boundary) {
            const auto p=d.sites[static_cast<std::size_t>(c.site_id.value)].point;
            exemptions<<c.site_id.value<<','<<p.x<<','<<p.y<<','<<c.edges.size()<<'\n';
        } else {
            EXPECT_EQ(c.edges.size(),6U);
            const auto p=d.sites[static_cast<std::size_t>(c.site_id.value)].point,g=c.centroid();
            EXPECT_NEAR(std::hypot(p.x-g.x,p.y-g.y),0,1e-10);
            for(const auto& e:c.edges) {
                EXPECT_TRUE(e.representative_inside_local_edge);
                EXPECT_NEAR(e.representative_point.x,e.midpoint.x,1e-10);
                EXPECT_NEAR(e.representative_point.y,e.midpoint.y,1e-10);
            }
        }
    }
}
TEST(Release,T1T2Random) {
    const auto d=mesh(UniformRandom2D{21760,8675309U});invariant(d);nearest_owner(d,"random");
}
TEST(Release,T9Lloyd) {
    auto sites=make_sites(boundary(),UniformRandom2D{21760,8675309U});
    Real previous=std::numeric_limits<Real>::infinity();
    ClippedVoronoiBuildOptions2D options;options.allow_parallel_cell_build=false;
    for(std::size_t i=0;i<=5;++i) {
        const auto d=ClippedVoronoiBuilder2D::build(sites,boundary(),options);
        long double sum=0;
        for(const auto& c:d.cells) {
            const auto p=sites[static_cast<std::size_t>(c.site_id.value)].point,g=c.centroid();
            sum+=std::hypot(p.x-g.x,p.y-g.y);
        }
        const Real mean=static_cast<Real>(sum/d.cells.size());
        std::cout<<"Lloyd "<<i<<" mean_dgc="<<mean<<'\n';
        EXPECT_LE(mean,previous);previous=mean;
        invariant(d);pairing_and_tpfa(d,false);
        LloydOptions2D step;step.max_iterations=1;step.stop_on_tolerance=false;
        if(i<5) sites=lloyd_relax(sites,boundary(),step,options).sites;
    }
}
TEST(Release,T10XmlOutput) {
    const auto a=vmm::io::write_clipped_voronoi_vtu(cartesian(),audit_dir()/"cartesian_binary.vtu");
    const auto b=vmm::io::write_clipped_voronoi_vtu(cartesian(),audit_dir()/"cartesian_ascii.vtu",{true});
    // Euler: V = E - F + 1 = 43861 - 21760 + 1 = 22102.
    EXPECT_EQ(a.point_count,257U*86U);EXPECT_EQ(a.point_count,b.point_count);
    EXPECT_EQ(a.cell_count,21760U);EXPECT_LT(a.file_bytes,1500000U);
    std::cout<<"VTU points="<<a.point_count<<" bytes="<<a.file_bytes<<'\n';
}
TEST(Release,T10ExtraCellArrays) {
    const auto& d=cartesian();
    std::vector<Real> field,gradient;
    for(const auto& cell:d.cells) {
        const auto p=cell.centroid();
        field.push_back(Real{2}+Real{3}*p.x-Real{5}*p.y);
        gradient.insert(gradient.end(),{Real{3},Real{-5},Real{0}});
    }
    const std::vector<Real> unavailable(d.cells.size(),std::numeric_limits<Real>::quiet_NaN());
    const std::array<vmm::io::VtkXmlCellDataArray2D,4> extras{{
        {"field_ref",field,1}, {"grad_ref",gradient,3},
        {"field_num",unavailable,1}, {"a&b<\"c'>",field,1}}};
    const auto before=fingerprint(d);
    const auto binary=vmm::io::write_clipped_voronoi_vtu(d,audit_dir()/"cartesian_extra_binary.vtu",{},extras);
    const auto ascii=vmm::io::write_clipped_voronoi_vtu(d,audit_dir()/"cartesian_extra_ascii.vtu",{true},extras);
    EXPECT_EQ(binary.point_count,257U*86U);
    EXPECT_EQ(binary.point_count,ascii.point_count);
    EXPECT_EQ(binary.cell_count,21760U);
    EXPECT_LT(binary.file_bytes,1500000U);
    EXPECT_EQ(fingerprint(d),before);
}
TEST(Release,T10InvalidExtraCellArrays) {
    const auto& d=cartesian();
    const auto path=audit_dir()/"invalid_arrays.txt";
    { std::ofstream out(path); out<<"preserve existing output"; }
    const std::vector<Real> values(d.cells.size(),Real{1});
    const std::array<vmm::io::VtkXmlCellDataArray2D,7> invalid{{
        {"",values,1}, {"bad\nname",values,1}, {"V_P",values,1},
        {"zero",values,0}, {"negative",values,-1},
        {"short_vector",values,3}, {"empty",{},1}}};
    for(const auto& extra:invalid) {
        SCOPED_TRACE(extra.name);
        EXPECT_THROW(static_cast<void>(vmm::io::write_clipped_voronoi_vtu(
            d,path,{},std::span{&extra,1U})),std::invalid_argument);
    }
    const std::array<vmm::io::VtkXmlCellDataArray2D,2> duplicate{{
        {"same",values,1},{"same",values,1}}};
    EXPECT_THROW(static_cast<void>(vmm::io::write_clipped_voronoi_vtu(d,path,{},duplicate)),std::invalid_argument);
    std::ifstream in(path);
    const std::string content((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    EXPECT_EQ(content,"preserve existing output");
}
TEST(Release,T11Determinism) {
    const auto a=mesh(UniformRandom2D{21760,8675309U});
    const auto b=mesh(UniformRandom2D{21760,8675309U});
    EXPECT_EQ(fingerprint(a),fingerprint(b));
    std::ofstream(audit_dir()/"fingerprint.txt")<<fingerprint(a)<<'\n';
}

TEST(Release,T9CartesianFixedPoint) {
    const auto& d=cartesian();
    LloydOptions2D options; options.max_iterations=1;options.stop_on_tolerance=false;
    ClippedVoronoiBuildOptions2D build; build.allow_parallel_cell_build=false;
    const auto relaxed=lloyd_relax(d.sites,d.boundary,options,build);
    EXPECT_LE(relaxed.history.front().max_displacement,5.03e-13);
    Real max_dgc=0;
    for(const auto& c:relaxed.diagram.cells) {
        const auto g=c.centroid(),p=relaxed.sites[static_cast<std::size_t>(c.site_id.value)].point;
        max_dgc=std::max(max_dgc,std::hypot(g.x-p.x,g.y-p.y));
    }
    EXPECT_LE(max_dgc,5.03e-13);
}

TEST(Release,MetadataDoesNotMoveVertices) {
    auto d=cartesian();
    const auto index=DelaunaySiteIndex::from(d.delaunay);
    ClippingWorkspace2D workspace;
    for(const auto& cell:d.cells) {
        const auto bare=VoronoiCellBuilder2D::build_unchecked(d.sites,d.boundary,d.delaunay,index,cell.site_id,workspace);
        ASSERT_EQ(bare.polygon.size(),cell.polygon.size());
        for(std::size_t i=0;i<bare.polygon.size();++i) {
            EXPECT_EQ(std::bit_cast<std::uint64_t>(bare.polygon[i].x),std::bit_cast<std::uint64_t>(cell.polygon[i].x));
            EXPECT_EQ(std::bit_cast<std::uint64_t>(bare.polygon[i].y),std::bit_cast<std::uint64_t>(cell.polygon[i].y));
        }
    }
}
