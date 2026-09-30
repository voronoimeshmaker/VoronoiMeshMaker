// ============================================================================
// File: run.cpp
// Description: Facade requests from a configuration and run_config
//              (DEC-040). One template serves 2D and 3D through a traits
//              struct per dimension.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/run.hpp>
#include <vmm/io/native.hpp>
#include <vmm/io/vtu.hpp>

namespace vmm {

namespace {

constexpr std::string_view kSitesPrefix = "sites.";

/// The error `e` with the section and line in front of its context.
std::unexpected<Error> at(const ConfigSection& s, std::size_t line, const Error& e) {
    const std::string where = s.name.empty() ? std::format("line {} [{}]", line, s.kind)
                                             : std::format("line {} [{} {}]", line, s.kind, s.name);
    return std::unexpected(Error(e.code(), e.context().empty() ? where : std::format("{}: {}", where, e.context())));
}

std::unexpected<Error> at(const ConfigSection& s, std::size_t line, ErrorCode code, std::string what) {
    return at(s, line, Error(code, std::move(what)));
}

std::size_t line_of(const ConfigSection& s, std::string_view key) {
    const auto it = std::ranges::find(s.entries, key, &ConfigEntry::key);
    return it == s.entries.end() ? s.line : it->line;
}

/// Every token a number (separated by spaces or commas)? Then the numbers.
std::optional<std::vector<Real>> as_numbers(std::string_view value) {
    std::vector<Real> out;
    std::size_t i = 0;
    while (true) {
        i = value.find_first_not_of(" \t,", i);
        if (i == std::string_view::npos) break;
        const auto end = std::min(value.find_first_of(" \t,", i), value.size());
        Real x = 0;
        const auto [ptr, ec] = std::from_chars(value.data() + i, value.data() + end, x);
        if (ec != std::errc{} || ptr != value.data() + end) return std::nullopt;
        out.push_back(x);
        i = end;
    }
    if (out.empty()) return std::nullopt;
    return out;
}

/// Parameters of the shape (keys without a prefix) or of the site source (keys "sites.<key>").
/// Every value is kept as text; numeric values also as numbers. "file" is resolved against the base directory.
Result<ShapeParameters> parameters(const ConfigSection& s, const MeshConfig& config, bool sites) {
    ShapeParameters p;
    for (const auto& [key, value, line] : s.entries) {
        std::string name = key;
        if (sites) {
            if (!key.starts_with(kSitesPrefix)) continue;
            name = key.substr(kSitesPrefix.size());
        } else {
            if (key == "shape" || key == "sites" || key == "medium" || key.starts_with(kSitesPrefix)) continue;
            if (key.find('.') != std::string::npos) {
                return at(s, line, ErrorCode::ParseError, std::format("unknown key '{}'", key));
            }
        }
        p.texts[name] = name == "file" ? (config.base_directory() / value).string() : value;
        if (auto numbers = as_numbers(value)) p.numbers[name] = std::move(*numbers);
    }
    return p;
}

Result<MediumId> medium_of(MediumRegistry& media, const ConfigSection& s) {
    const std::string name(s.find("medium").value_or(s.name));
    if (const auto found = media.find(name)) return *found;
    auto added = media.add(name);
    if (!added) return at(s, line_of(s, "medium"), added.error());
    return added;
}

struct Traits2 {
    using Request = MeshRequest2D;
    using Declaration = Declaration2D;

    static auto shape(const ConfigRegistries& r, const std::string& name, const ShapeParameters& p,
                      const Declaration& d) {
        return r.shapes_2d.make(name, p, d.polygonize_options());
    }
    static auto sites(const ConfigRegistries& r, const std::string& name, RegionId region,
                      const ShapeParameters& p) {
        return r.sites_2d.make(name, region, p);
    }
    static Result<RegionId> add_region(Declaration& d, std::string name, MediumId medium, ShapeOutline outline) {
        return d.add_region_outline(std::move(name), medium, std::move(outline));
    }
    static Status add_hole(Declaration& d, ShapeOutline outline) { return d.add_hole_outline(std::move(outline)); }
    static Result<RegionId> background(Declaration& d, std::string name, MediumId medium) {
        return d.set_background(std::move(name), medium);
    }
    static void pairs(Request& request, Real spacing) { request.sites.interface_pairs = InterfacePairs(spacing); }
};

struct Traits3 {
    using Request = MeshRequest3D;
    using Declaration = Declaration3D;

    static auto shape(const ConfigRegistries& r, const std::string& name, const ShapeParameters& p,
                      const Declaration& d) {
        return r.shapes_3d.make(name, p, d.polygonize_options());
    }
    static auto sites(const ConfigRegistries& r, const std::string& name, RegionId region,
                      const ShapeParameters& p) {
        return r.sites_3d.make(name, region, p);
    }
    static Result<RegionId> add_region(Declaration& d, std::string name, MediumId medium, TriangleSurface surface) {
        return d.add_region_surface(std::move(name), medium, std::move(surface));
    }
    static Status add_hole(Declaration& d, TriangleSurface surface) { return d.add_hole_surface(std::move(surface)); }
    static Result<RegionId> background(Declaration&, std::string, MediumId) {
        return fail(ErrorCode::ParseError, "[background] is 2D only: declare the region with a shape");
    }
    static void pairs(Request& request, Real spacing) { request.sites.interface_pairs = InterfacePairs3D(spacing); }
};

template <class T>
Status add_sites(typename T::Request& request, const ConfigRegistries& registries, const MeshConfig& config,
                 const ConfigSection& s, RegionId region) {
    const auto name = s.find("sites");
    if (!name) return at(s, s.line, ErrorCode::ParseError, "'sites = <source>' is required");
    auto p = parameters(s, config, true);
    if (!p) return std::unexpected(p.error());
    auto source = T::sites(registries, std::string(*name), region, *p);
    if (!source) return at(s, line_of(s, "sites"), source.error());
    request.sources.push_back(std::move(*source));
    return {};
}

template <class T>
Result<typename T::Request> make_request(const MeshConfig& config, const ConfigRegistries& registries,
                                         int dimension) {
    if (config.dimension() != dimension) {
        return fail(ErrorCode::InvalidArgument, std::format("the configuration is {}D", config.dimension()));
    }
    typename T::Request request;
    request.sites.seed = config.seed();
    if (config.interface_pairs()) T::pairs(request, *config.interface_pairs());
    if (config.tolerance()) request.build.relative_tolerance = *config.tolerance();
    auto& declaration = request.declaration;
    for (const auto& s : config.sections()) {
        if (s.kind == "background") {
            auto medium = medium_of(declaration.media(), s);
            if (!medium) return std::unexpected(medium.error());
            auto region = T::background(declaration, s.name, *medium);
            if (!region) return at(s, s.line, region.error());
            if (auto st = add_sites<T>(request, registries, config, s, *region); !st) return std::unexpected(st.error());
            continue;
        }
        const auto shape = s.find("shape");
        if (!shape) return at(s, s.line, ErrorCode::ParseError, "'shape = <name>' is required");
        auto p = parameters(s, config, false);
        if (!p) return std::unexpected(p.error());
        auto geometry = T::shape(registries, std::string(*shape), *p, declaration);
        if (!geometry) return at(s, line_of(s, "shape"), geometry.error());
        if (s.kind == "hole") {
            if (s.find("sites") || s.find("medium")) {
                return at(s, s.line, ErrorCode::ParseError, "a hole takes no medium and no sites");
            }
            if (auto st = T::add_hole(declaration, std::move(*geometry)); !st) return at(s, s.line, st.error());
            continue;
        }
        auto medium = medium_of(declaration.media(), s);
        if (!medium) return std::unexpected(medium.error());
        auto region = T::add_region(declaration, s.name, *medium, std::move(*geometry));
        if (!region) return at(s, s.line, region.error());
        if (auto st = add_sites<T>(request, registries, config, s, *region); !st) return std::unexpected(st.error());
    }
    return request;
}

/// Output formats by name: file extension and writer.
template <class M>
struct Writer {
    std::string_view extension;
    std::function<Status(const M&, const std::filesystem::path&)> write;
};

template <class M>
std::map<std::string, Writer<M>, std::less<>> writers() {
    return {{"vmesh", {".vmesh", [](const M& m, const std::filesystem::path& p) { return write_native(m, p); }}},
            {"vtu", {".vtu", [](const M& m, const std::filesystem::path& p) { return write_vtu(m, p); }}}};
}

template <class MeshResult>
Result<ConfigRunReport> write_all(const MeshConfig& config, const MeshResult& result) {
    ConfigRunReport report;
    report.dimension = config.dimension();
    report.cells = result.mesh.cell_count();
    report.internal_faces = result.mesh.internal_face_count();
    report.boundary_faces = result.mesh.boundary_faces().size();
    report.invariants = result.invariants;
    const auto table = writers<std::decay_t<decltype(result.mesh)>>();
    const auto base = config.output();
    if (base.has_parent_path()) {
        std::error_code ignored;  // a failure shows up as FileOpenFailed of the writer
        std::filesystem::create_directories(base.parent_path(), ignored);
    }
    for (const auto& format : config.formats()) {
        const auto& writer = table.find(format)->second;
        auto path = base;
        path += writer.extension;
        if (auto st = writer.write(result.mesh, path); !st) return std::unexpected(st.error());
        report.written.push_back(std::move(path));
    }
    return report;
}

}  // namespace

Result<MeshRequest2D> make_request_2d(const MeshConfig& config, const ConfigRegistries& registries) {
    return make_request<Traits2>(config, registries, 2);
}

Result<MeshRequest3D> make_request_3d(const MeshConfig& config, const ConfigRegistries& registries) {
    return make_request<Traits3>(config, registries, 3);
}

Result<ConfigRunReport> run_config(const MeshConfig& config, const ConfigRegistries& registries) {
    // Formats are checked before the (possibly long) meshing.
    const auto known = writers<Mesh2D>();
    for (const auto& format : config.formats()) {
        if (!known.contains(format)) {
            return fail(ErrorCode::ParseError, std::format("unknown output format '{}' (vmesh, vtu)", format));
        }
    }
    if (config.dimension() == 2) {
        auto request = make_request_2d(config, registries);
        if (!request) return std::unexpected(request.error());
        auto result = generate_mesh_2d(*request);
        if (!result) return std::unexpected(result.error());
        return write_all(config, *result);
    }
    auto request = make_request_3d(config, registries);
    if (!request) return std::unexpected(request.error());
    auto result = generate_mesh_3d(*request);
    if (!result) return std::unexpected(result.error());
    return write_all(config, *result);
}

}  // namespace vmm
