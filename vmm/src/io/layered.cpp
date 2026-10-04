// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <istream>
#include <limits>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/io/layered.hpp>
#include <vmm/io/native.hpp>
namespace vmm {
Status write_layered(const LayeredMesh& mesh,std::ostream& out) {
    const auto& d=mesh.data();
    out << "VMM_LAYERS 1\n" << std::setprecision(17);
    const auto row=[&](const auto& values) {
        out << values.size();
        for(const auto& v:values) out << ' ' << v;
        out << '\n';
    };
    row(d.horizons.x()); row(d.horizons.y());
    out << d.horizons.horizon_count() << '\n';
    for(std::size_t h=0;h<d.horizons.horizon_count();++h) {
        out << std::quoted(d.horizons.names()[h]) << '\n';
        row(d.horizons.elevations()[h]);
    }
    out << d.column_count << ' ' << d.fractions.size() << '\n';
    for(const auto& f:d.fractions) row(f);
    row(d.column); row(d.layer); row(d.face_level);
    auto st=write_native(d.mesh,out);
    if(!st) return st;
    return out ? Status{} : fail(ErrorCode::FileOpenFailed,"writing layered mesh");
}
Result<LayeredMesh> read_layered(std::istream& in) {
    std::string magic; int version=0;
    if(!(in>>magic>>version) || magic!="VMM_LAYERS") return fail(ErrorCode::ParseError,"layered header");
    if(version!=1) return fail(ErrorCode::UnsupportedVersion,"layered format");
    const auto row=[&]<class T>(std::vector<T>& values) {
        std::size_t n=0;
        if(!(in>>n) || n>=std::numeric_limits<std::uint32_t>::max()) return false;
        for(std::size_t k=0;k<n;++k) {
            T v{};
            if(!(in>>v)) return false;
            values.push_back(v);
        }
        return true;
    };
    std::vector<Real> x,y;
    std::vector<std::string> names;
    std::vector<std::vector<Real>> z;
    std::size_t n=0;
    if(!row(x)||!row(y)||!(in>>n)||n<2||n>=std::numeric_limits<std::uint32_t>::max())
        return fail(ErrorCode::ParseError,"horizon grid");
    for(std::size_t h=0;h<n;++h) {
        std::string name; std::vector<Real> values;
        if(!(in>>std::quoted(name))||!row(values)) return fail(ErrorCode::ParseError,"horizon elevations");
        names.push_back(std::move(name)); z.push_back(std::move(values));
    }
    auto grid=HorizonGrid::make(std::move(x),std::move(y),std::move(names),std::move(z));
    if(!grid) return std::unexpected(grid.error());
    LayeredData d; d.horizons=std::move(*grid);
    if(!(in>>d.column_count>>n) || n!=d.horizons.horizon_count()-1)
        return fail(ErrorCode::ParseError,"layer counts");
    for(std::size_t k=0;k<n;++k) {
        std::vector<Real> f;
        if(!row(f)) return fail(ErrorCode::ParseError,"layer fractions");
        d.fractions.push_back(std::move(f));
    }
    if(!row(d.column)||!row(d.layer)||!row(d.face_level))
        return fail(ErrorCode::ParseError,"layer metadata");
    in.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    auto mesh=read_native<3>(in);
    if(!mesh) return std::unexpected(mesh.error());
    d.mesh=std::move(*mesh);
    return LayeredMesh::from_data(std::move(d));
}
Status write_layered(const LayeredMesh& mesh,const std::filesystem::path& path) {
    std::ofstream out(path);
    if(!out) return fail(ErrorCode::FileOpenFailed,path.string());
    return write_layered(mesh,out);
}
Result<LayeredMesh> read_layered(const std::filesystem::path& path) {
    std::ifstream in(path);
    if(!in) return fail(ErrorCode::FileOpenFailed,path.string());
    return read_layered(in);
}
Result<HorizonSpecification> read_horizons(std::istream& in) {
    std::string magic; int version=0;
    if(!(in>>magic>>version) || magic!="VMM_HORIZONS")
        return fail(ErrorCode::ParseError,"horizon specification header");
    if(version!=1) return fail(ErrorCode::UnsupportedVersion,"horizon specification");
    const auto row=[&]<class T>(std::vector<T>& values) {
        std::size_t n=0;
        if(!(in>>n) || n>=std::numeric_limits<std::uint32_t>::max()) return false;
        for(std::size_t k=0;k<n;++k) {
            T v{};
            if(!(in>>v)) return false;
            values.push_back(v);
        }
        return true;
    };
    std::vector<Real> x,y;
    std::vector<std::string> names;
    std::vector<std::vector<Real>> z;
    std::size_t n=0;
    if(!row(x)||!row(y)||!(in>>n)||n<2||n>=std::numeric_limits<std::uint32_t>::max())
        return fail(ErrorCode::ParseError,"horizon grid");
    for(std::size_t h=0;h<n;++h) {
        std::string name; std::vector<Real> values;
        if(!(in>>std::quoted(name))||!row(values)) return fail(ErrorCode::ParseError,"horizon elevations");
        names.push_back(std::move(name)); z.push_back(std::move(values));
    }
    auto grid=HorizonGrid::make(std::move(x),std::move(y),std::move(names),std::move(z));
    if(!grid) return std::unexpected(grid.error());
    HorizonSpecification d; d.first=std::move(*grid);
    if(!(in>>n) || n!=d.first.horizon_count()-1)
        return fail(ErrorCode::ParseError,"layer counts");
    for(std::size_t k=0;k<n;++k) {
        std::vector<Real> f;
        if(!row(f)) return fail(ErrorCode::ParseError,"layer fractions");
        d.second.push_back(std::move(f));
    }
    return d;
}
Result<HorizonSpecification> read_horizons(const std::filesystem::path& path) {
    std::ifstream in(path);
    if(!in) return fail(ErrorCode::FileOpenFailed,path.string());
    return read_horizons(in);
}
} // namespace vmm
