// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iomanip>
#include <istream>
#include <limits>
#include <new>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/io/layered.hpp>
#include <vmm/io/native.hpp>
namespace vmm {
namespace {
bool count_fits(std::istream& in, std::size_t count) {
    if (count >= std::numeric_limits<std::uint32_t>::max()) return false;
    auto* buffer = in.rdbuf();
    const auto position = buffer->pubseekoff(0, std::ios::cur, std::ios::in);
    if (position == std::streampos(-1)) return true;
    const auto end = buffer->pubseekoff(0, std::ios::end, std::ios::in);
    if (buffer->pubseekpos(position, std::ios::in) == std::streampos(-1)) return false;
    return end == std::streampos(-1) ||
           (end >= position && count <= static_cast<std::size_t>(end - position));
}
}  // namespace

Status write_layered(const LayeredMesh& mesh,std::ostream& out) {
    const auto& d=mesh.data();
    out << "VMM_LAYERS " << (d.column_levels.empty()?2:3) << '\n' << std::setprecision(17);
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
    std::vector<Real> generators;
    for(const auto& point:d.base_generator) { generators.push_back(point[0]); generators.push_back(point[1]); }
    row(generators);
    const std::vector<unsigned> flags(d.site_is_centroid.begin(),d.site_is_centroid.end());
    row(flags);
    if(!d.column_levels.empty()) {
        out << d.column_levels.size() << '\n';
        for(const auto& levels:d.column_levels) row(levels);
        row(d.kfloor);
    }
    auto st=write_native(d.mesh,out);
    if(!st) return st;
    return out ? Status{} : fail(ErrorCode::FileOpenFailed,"writing layered mesh");
}
Result<LayeredMesh> read_layered(std::istream& in) try {
    std::string magic; int version=0;
    if(!(in>>magic>>version) || magic!="VMM_LAYERS") return fail(ErrorCode::ParseError,"layered header");
    if(version!=1 && version!=2 && version!=3) return fail(ErrorCode::UnsupportedVersion,"layered format");
    const auto row=[&]<class T>(std::vector<T>& values) {
        std::size_t n=0;
        if(!(in>>n) || !count_fits(in,n)) return false;
        for(std::size_t k=0;k<n;++k) {
            T v{};
            if(!(in>>v)) return false;
            if constexpr (std::is_floating_point_v<T>) {
                if (!std::isfinite(v)) return false;
            }
            values.push_back(v);
        }
        return true;
    };
    std::vector<Real> x,y;
    std::vector<std::string> names;
    std::vector<std::vector<Real>> z;
    std::size_t n=0;
    if(!row(x)||!row(y)||!(in>>n)||n<2||!count_fits(in,n))
        return fail(ErrorCode::ParseError,"horizon grid");
    for(std::size_t h=0;h<n;++h) {
        std::string name; std::vector<Real> values;
        if(!(in>>std::quoted(name))||!row(values)) return fail(ErrorCode::ParseError,"horizon elevations");
        names.push_back(std::move(name)); z.push_back(std::move(values));
    }
    auto grid=HorizonGrid::make(std::move(x),std::move(y),std::move(names),std::move(z));
    if(!grid) return std::unexpected(grid.error());
    LayeredData d; d.horizons=std::move(*grid);
    if(!(in>>d.column_count>>n) || !count_fits(in,d.column_count) || n!=d.horizons.horizon_count()-1)
        return fail(ErrorCode::ParseError,"layer counts");
    for(std::size_t k=0;k<n;++k) {
        std::vector<Real> f;
        if(!row(f)) return fail(ErrorCode::ParseError,"layer fractions");
        d.fractions.push_back(std::move(f));
    }
    if(!row(d.column)||!row(d.layer)||!row(d.face_level))
        return fail(ErrorCode::ParseError,"layer metadata");
    if(version>=2) {
        std::vector<Real> generators;
        std::vector<unsigned> flags;
        if(!row(generators) || generators.size()%2!=0 || !row(flags))
            return fail(ErrorCode::ParseError,"generator metadata");
        for(std::size_t i=0;i<generators.size();i+=2)
            d.base_generator.push_back({generators[i],generators[i+1]});
        for(auto flag:flags) {
            if(flag>1) return fail(ErrorCode::ParseError,"centroid fallback flag");
            d.site_is_centroid.push_back(static_cast<std::uint8_t>(flag));
        }
    }
    if(version==3) {
        std::size_t columns=0;
        if(!(in>>columns) || columns!=d.column_count || !count_fits(in,columns))
            return fail(ErrorCode::ParseError,"flat column count");
        for(std::size_t column=0;column<columns;++column) {
            std::vector<Real> levels;
            if(!row(levels)) return fail(ErrorCode::ParseError,"flat column levels");
            d.column_levels.push_back(std::move(levels));
        }
        if(!row(d.kfloor)) return fail(ErrorCode::ParseError,"kfloor metadata");
    }
    std::string remainder;
    std::getline(in, remainder);
    if (remainder.find_first_not_of(" \t\r") != std::string::npos)
        return fail(ErrorCode::ParseError, "extra layer metadata");
    auto mesh=read_native<3>(in);
    if(!mesh) return std::unexpected(mesh.error());
    d.mesh=std::move(*mesh);
    return LayeredMesh::from_data(std::move(d));
} catch (const std::bad_alloc&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::length_error&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::exception&) {
    return fail(ErrorCode::ParseError);
}
Status write_layered(const LayeredMesh& mesh,const std::filesystem::path& path) {
    std::ofstream out(path);
    if(!out) return fail(ErrorCode::FileOpenFailed,path.string());
    return write_layered(mesh,out);
}
Result<LayeredMesh> read_layered(const std::filesystem::path& path) try {
    std::ifstream in(path);
    if(!in) return fail(ErrorCode::FileOpenFailed,path.string());
    return read_layered(in);
} catch (const std::bad_alloc&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::length_error&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::exception&) {
    return fail(ErrorCode::ParseError);
}
Result<HorizonSpecification> read_horizons(std::istream& in) try {
    std::string magic; int version=0;
    if(!(in>>magic>>version) || magic!="VMM_HORIZONS")
        return fail(ErrorCode::ParseError,"horizon specification header");
    if(version!=1) return fail(ErrorCode::UnsupportedVersion,"horizon specification");
    const auto row=[&]<class T>(std::vector<T>& values) {
        std::size_t n=0;
        if(!(in>>n) || !count_fits(in,n)) return false;
        for(std::size_t k=0;k<n;++k) {
            T v{};
            if(!(in>>v)) return false;
            if constexpr (std::is_floating_point_v<T>) {
                if (!std::isfinite(v)) return false;
            }
            values.push_back(v);
        }
        return true;
    };
    std::vector<Real> x,y;
    std::vector<std::string> names;
    std::vector<std::vector<Real>> z;
    std::size_t n=0;
    if(!row(x)||!row(y)||!(in>>n)||n<2||!count_fits(in,n))
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
    std::string extra;
    if (in >> extra) return fail(ErrorCode::ParseError, "extra horizon data");
    return d;
} catch (const std::bad_alloc&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::length_error&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::exception&) {
    return fail(ErrorCode::ParseError);
}
Result<HorizonSpecification> read_horizons(const std::filesystem::path& path) try {
    std::ifstream in(path);
    if(!in) return fail(ErrorCode::FileOpenFailed,path.string());
    return read_horizons(in);
} catch (const std::bad_alloc&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::length_error&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::exception&) {
    return fail(ErrorCode::ParseError);
}
} // namespace vmm
