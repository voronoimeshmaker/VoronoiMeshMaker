// SPDX-License-Identifier: GPL-3.0-or-later
/** @file VTK_XML_ClippedVoronoi2D.cpp
 * @brief Deterministic XML mesh export with shared vertices and lossless encoding.
 * @ingroup io_voronoi2d
 */
#include <VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_XML_ClippedVoronoi2D.hpp>
#include <zlib.h>
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <numbers>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace vmm::io {
namespace {
using Real = s2d::Real;
using Point = s2d::Point2;
struct Array {
    std::string name, type;
    int components{1};
    std::vector<unsigned char> bytes;
    std::string ascii;
};
template<class T> void append_bytes(std::vector<unsigned char>& output, T value) {
    auto bytes = std::bit_cast<std::array<unsigned char,sizeof(T)>>(value);
    if constexpr (std::endian::native == std::endian::big) std::reverse(bytes.begin(),bytes.end());
    output.insert(output.end(),bytes.begin(),bytes.end());
}
template<class Range> Array array(std::string name, const Range& values, bool ascii, int components=1) {
    using T = typename Range::value_type;
    Array a;
    a.name=std::move(name); a.components=components;
    if constexpr(std::is_floating_point_v<T>) a.type=sizeof(T)==8 ? "Float64":"Float32";
    else a.type=sizeof(T)==1 ? "UInt8":"Int32";
    if(ascii) {
        std::ostringstream text;
        text.imbue(std::locale::classic());
        text << std::setprecision(std::numeric_limits<T>::max_digits10);
        for(const auto value:values) text << +value << ' ';
        a.ascii=text.str();
    } else {
        std::vector<unsigned char> raw;
        raw.reserve(values.size()*sizeof(T));
        for(const auto value:values) append_bytes(raw,value);
        constexpr std::uint32_t block_size=32768;
        const auto n=(raw.size()+block_size-1)/block_size;
        if(n>std::numeric_limits<std::uint32_t>::max()) throw std::length_error("VTK block count overflow");
        std::vector<std::vector<unsigned char>> blocks(n);
        append_bytes(a.bytes,static_cast<std::uint32_t>(n));
        append_bytes(a.bytes,block_size);
        append_bytes(a.bytes,static_cast<std::uint32_t>(raw.size()%block_size));
        for(std::size_t i=0;i<n;++i) {
            const auto count=std::min<std::size_t>(block_size,raw.size()-i*block_size);
            uLongf compressed_size=compressBound(static_cast<uLong>(count));
            blocks[i].resize(compressed_size);
            if(compress2(blocks[i].data(),&compressed_size,raw.data()+i*block_size,
                         static_cast<uLong>(count),Z_BEST_SPEED)!=Z_OK) throw std::runtime_error("VTK compression failed");
            blocks[i].resize(compressed_size);
            append_bytes(a.bytes,static_cast<std::uint32_t>(compressed_size));
        }
        for(const auto& block:blocks) a.bytes.insert(a.bytes.end(),block.begin(),block.end());
    }
    return a;
}
std::string xml_attribute(std::string_view name) {
    std::string escaped;
    for(const char c:name) {
        switch(c) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped += c; break;
        }
    }
    return escaped;
}
void validate_extra_arrays(std::span<const VtkXmlCellDataArray2D> extras, std::size_t cells) {
    std::set<std::string_view> names{
        "volume_id", "site_id", "is_boundary_cell", "cell_class", "V_P", "n_faces",
        "n_neighbours_fv", "n_neighbours_delaunay", "d_gc", "q_compactness",
        "tpfa_offset", "tpfa_crossing_ok"};
    for(const auto& extra:extras) {
        if(extra.name.empty() || std::any_of(extra.name.begin(),extra.name.end(),[](char c) {
            const auto value=static_cast<unsigned char>(c);
            return value<32U || value>126U;
        })) throw std::invalid_argument("VTK requires a non-empty printable ASCII array name");
        if(!names.insert(extra.name).second)
            throw std::invalid_argument("VTK duplicate cell array: "+std::string(extra.name));
        if(extra.components<=0) throw std::invalid_argument("VTK requires a positive component count");
        const auto components=static_cast<std::size_t>(extra.components);
        if(cells>std::numeric_limits<std::size_t>::max()/components ||
           extra.values.size()!=cells*components)
            throw std::invalid_argument("VTK incorrect tuple count: "+std::string(extra.name));
    }
}
}
VtkXmlClippedSummary2D write_clipped_voronoi_vtu(const vd2d::ClippedVoronoiDiagram2D& d,
    const std::filesystem::path& path,VtkXmlClippedOptions2D options) {
    return write_clipped_voronoi_vtu(d,path,options,{});
}
VtkXmlClippedSummary2D write_clipped_voronoi_vtu(const vd2d::ClippedVoronoiDiagram2D& d,
    const std::filesystem::path& path,VtkXmlClippedOptions2D options,
    std::span<const VtkXmlCellDataArray2D> extra_cell_arrays) {
    if(d.empty()) throw std::invalid_argument("VTK requires a non-empty diagram");
    validate_extra_arrays(extra_cell_arrays,d.cells.size());
    Real total_area=0;
    for(const auto& c:d.cells) total_area+=c.area();
    const Real tolerance=Real{1e-12}*std::sqrt(total_area/static_cast<Real>(d.cells.size()));
    if(!(tolerance>0) || !std::isfinite(tolerance)) throw std::invalid_argument("VTK invalid mean area");
    using Key=std::pair<std::int64_t,std::int64_t>;
    std::map<Key,std::vector<std::int32_t>> buckets;
    std::vector<Point> points;
    const auto vertex_id=[&](Point p) {
        const auto quantise=[&](Real value) {
            const long double q=std::floor(static_cast<long double>(value)/tolerance);
            if(!std::isfinite(q) || std::abs(q)>=static_cast<long double>(std::numeric_limits<std::int64_t>::max()-2))
                throw std::length_error("VTK coordinate quantisation overflow");
            return static_cast<std::int64_t>(q);
        };
        const Key key{quantise(p.x),quantise(p.y)};
        std::int32_t found=-1;
        for(int dx=-1;dx<=1;++dx) for(int dy=-1;dy<=1;++dy) {
            const auto it=buckets.find({key.first+dx,key.second+dy});
            if(it==buckets.end()) continue;
            for(const auto id:it->second) {
                const auto q=points[static_cast<std::size_t>(id)];
                if(std::hypot(p.x-q.x,p.y-q.y)<=tolerance && (found<0 || id<found)) found=id;
            }
        }
        if(found>=0) return found;
        if(points.size()>=static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::length_error("VTK point limit");
        const auto id=static_cast<std::int32_t>(points.size());
        points.push_back(p); buckets[key].push_back(id); return id;
    };
    std::vector<std::int32_t> connectivity,offsets,volume_ids,site_ids,n_faces,n_fv,n_dt;
    std::vector<std::uint8_t> types,boundary,classes,crossing;
    std::vector<Real> areas,dgc,compactness,tpfa_offset;
    for(const auto& c:d.cells) {
        if(c.polygon.size()<3 || c.edges.size()!=c.polygon.size()) throw std::invalid_argument("VTK requires complete cell geometry");
        for(const auto p:c.polygon) connectivity.push_back(vertex_id(p));
        if(connectivity.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::length_error("VTK connectivity limit");
        offsets.push_back(static_cast<std::int32_t>(connectivity.size())); types.push_back(7);
        volume_ids.push_back(static_cast<std::int32_t>(c.volume_id));
        site_ids.push_back(static_cast<std::int32_t>(c.site_id.value));
        boundary.push_back(static_cast<std::uint8_t>(c.is_boundary_cell));
        std::size_t nb=0; bool ok=true; Real offset=0;
        for(const auto& e:c.edges) {
            if(e.is_boundary_edge) ++nb;
            else { ok=ok && e.representative_valid; offset=std::max(offset,std::abs(e.crossing_parameter-Real{0.5})); }
        }
        classes.push_back(static_cast<std::uint8_t>(std::min<std::size_t>(nb,2)));
        crossing.push_back(static_cast<std::uint8_t>(ok)); tpfa_offset.push_back(offset);
        n_faces.push_back(static_cast<std::int32_t>(c.edges.size()));
        n_fv.push_back(static_cast<std::int32_t>(c.edges.size()-nb));
        n_dt.push_back(static_cast<std::int32_t>(c.delaunay_neighbours().size()));
        areas.push_back(c.area());
        const auto p=d.sites[static_cast<std::size_t>(c.site_id.value)].point, g=c.centroid();
        dgc.push_back(std::hypot(p.x-g.x,p.y-g.y));
        compactness.push_back(Real{4}*std::numbers::pi_v<Real>*c.area()/(c.perimeter()*c.perimeter()));
    }
    std::vector<Real> xyz;
    for(const auto p:points) {xyz.push_back(p.x);xyz.push_back(p.y);xyz.push_back(0);}
    const bool ascii=options.ascii;
    std::vector<Array> arrays;
    arrays.push_back(array("Points",xyz,ascii,3));
    arrays.push_back(array("connectivity",connectivity,ascii));
    arrays.push_back(array("offsets",offsets,ascii)); arrays.push_back(array("types",types,ascii));
    arrays.push_back(array("volume_id",volume_ids,ascii)); arrays.push_back(array("site_id",site_ids,ascii));
    arrays.push_back(array("is_boundary_cell",boundary,ascii)); arrays.push_back(array("cell_class",classes,ascii));
    arrays.push_back(array("V_P",areas,ascii)); arrays.push_back(array("n_faces",n_faces,ascii));
    arrays.push_back(array("n_neighbours_fv",n_fv,ascii)); arrays.push_back(array("n_neighbours_delaunay",n_dt,ascii));
    arrays.push_back(array("d_gc",dgc,ascii)); arrays.push_back(array("q_compactness",compactness,ascii));
    arrays.push_back(array("tpfa_offset",tpfa_offset,ascii)); arrays.push_back(array("tpfa_crossing_ok",crossing,ascii));
    for(const auto& extra:extra_cell_arrays)
        arrays.push_back(array(std::string(extra.name),extra.values,ascii,extra.components));
    std::ofstream out(path,std::ios::binary);
    out.exceptions(std::ios::badbit|std::ios::failbit);
    out.imbue(std::locale::classic());
    out << "<?xml version=\"1.0\"?>\n<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\"";
    if(!ascii) out << " compressor=\"vtkZLibDataCompressor\"";
    out << ">\n<UnstructuredGrid><Piece NumberOfPoints=\""<<points.size()<<"\" NumberOfCells=\""<<d.cells.size()<<"\">\n<Points>\n";
    std::size_t offset=0;
    for(std::size_t i=0;i<arrays.size();++i) {
        if(i==1) out << "</Points><Cells>\n";
        if(i==4) out << "</Cells><CellData>\n";
        const auto& a=arrays[i];
        out << "<DataArray type=\""<<a.type<<"\" Name=\""<<xml_attribute(a.name)<<"\" NumberOfComponents=\""<<a.components<<"\"";
        if(ascii) out << " format=\"ascii\">"<<a.ascii<<"</DataArray>\n";
        else {out << " format=\"appended\" offset=\""<<offset<<"\"/>\n"; offset+=a.bytes.size();}
    }
    out << "</CellData></Piece></UnstructuredGrid>\n";
    if(!ascii) {
        out << "<AppendedData encoding=\"raw\">_";
        for(const auto& a:arrays) out.write(reinterpret_cast<const char*>(a.bytes.data()),static_cast<std::streamsize>(a.bytes.size()));
        out << "</AppendedData>\n";
    }
    out << "</VTKFile>\n"; out.close();
    return {points.size(),d.cells.size(),std::filesystem::file_size(path)};
}
}
