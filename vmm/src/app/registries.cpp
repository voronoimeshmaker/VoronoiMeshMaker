// ============================================================================
// File: registries.cpp
// Description: Built-in site sources by name and the registries used by
//              configuration files (DEC-040).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <cstddef>
#include <format>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/registries.hpp>
#include <vmm/io/stl.hpp>

namespace vmm {

namespace {

std::unexpected<Error> bad(const std::string& what) { return fail(ErrorCode::InvalidShapeParameter, what); }

bool has(const ShapeParameters& p, const std::string& key) { return p.numbers.contains(key) || p.texts.contains(key); }

/// Exactly `count` numbers under `key` (any positive count if `count` is 0).
Result<std::vector<Real>> numbers(const ShapeParameters& p, const std::string& key, std::size_t count) {
    const auto it = p.numbers.find(key);
    if (it == p.numbers.end() || it->second.empty() || (count != 0 && it->second.size() != count)) {
        return bad(count == 0 ? std::format("'{}' needs numbers", key)
                              : std::format("'{}' needs {} number(s)", key, count));
    }
    return it->second;
}

Result<Real> number(const ShapeParameters& p, const std::string& key) {
    auto v = numbers(p, key, 1);
    if (!v) return std::unexpected(v.error());
    return (*v)[0];
}

/// Optional single number: the fallback when absent, an error when present but not one number.
Result<Real> number_or(const ShapeParameters& p, const std::string& key, Real fallback) {
    if (!has(p, key)) return fallback;
    return number(p, key);
}

template <std::size_t D>
Result<Vec<D>> vec_or(const ShapeParameters& p, const std::string& key, Vec<D> fallback) {
    if (!has(p, key)) return fallback;
    auto v = numbers(p, key, D);
    if (!v) return std::unexpected(v.error());
    Vec<D> out{};
    for (std::size_t i = 0; i < D; ++i) out[i] = (*v)[i];
    return out;
}

template <std::size_t D>
Result<std::vector<Vec<D>>> points(const ShapeParameters& p, const std::string& key) {
    auto v = numbers(p, key, 0);
    if (!v) return std::unexpected(v.error());
    if (v->size() % D != 0) return bad(std::format("'{}' needs groups of {} numbers", key, D));
    std::vector<Vec<D>> out(v->size() / D);
    for (std::size_t i = 0; i < v->size(); ++i) out[i / D][i % D] = (*v)[i];
    return out;
}

Result<std::size_t> count_of(const ShapeParameters& p) {
    auto n = number(p, "count");
    if (!n) return std::unexpected(n.error());
    if (!(*n >= 1) || *n != std::floor(*n)) return bad("'count' needs a positive integer");
    return static_cast<std::size_t>(*n);
}

std::string text(const ShapeParameters& p, const std::string& key) {
    const auto it = p.texts.find(key);
    return it == p.texts.end() ? std::string{} : it->second;
}

/// Uniform random source of either dimension: spacing; min_distance and margin fractions.
template <class Source>
Result<Source> uniform(const ShapeParameters& p) {
    auto spacing = number(p, "spacing");
    if (!spacing) return std::unexpected(spacing.error());
    Source source(*spacing);
    if (has(p, "min_distance")) {
        auto f = number(p, "min_distance");
        if (!f) return std::unexpected(f.error());
        source.min_distance_fraction(*f);
    }
    if (has(p, "margin")) {
        auto f = number(p, "margin");
        if (!f) return std::unexpected(f.error());
        source.boundary_margin_fraction(*f);
    }
    return source;
}

/// Shared add/make of the two site source registries.
template <class Map, class Factory>
Status add_to(Map& factories, std::string name, Factory factory) {
    if (name.empty() || !factory) return fail(ErrorCode::InvalidArgument, "site source needs a name and a factory");
    if (factories.contains(name)) return fail(ErrorCode::DuplicateName, name);
    factories.emplace(std::move(name), std::move(factory));
    return {};
}

template <class Map>
auto make_from(const Map& factories, const std::string& name, RegionId region, const ShapeParameters& parameters)
    -> decltype(factories.begin()->second(region, parameters)) {
    const auto it = factories.find(name);
    if (it == factories.end()) return fail(ErrorCode::InvalidArgument, std::format("unknown site source '{}'", name));
    return it->second(region, parameters);
}

}  // namespace

SiteSourceRegistry2D SiteSourceRegistry2D::with_builtin_sources() {
    SiteSourceRegistry2D r;
    r.factories_["uniform"] = [](RegionId region, const ShapeParameters& p) -> Result<RegionSites> {
        auto s = uniform<UniformRandomSource>(p);
        if (!s) return std::unexpected(s.error());
        return sites_for(region, std::move(*s));
    };
    r.factories_["count"] = [](RegionId region, const ShapeParameters& p) -> Result<RegionSites> {
        auto n = count_of(p);
        if (!n) return std::unexpected(n.error());
        auto margin = number_or(p, "margin", 0);
        if (!margin) return std::unexpected(margin.error());
        return sites_for(region, RandomCountSource(*n, *margin));
    };
    const auto grid = [](auto make) {
        return [make](RegionId region, const ShapeParameters& p) -> Result<RegionSites> {
            auto spacing = number(p, "spacing");
            if (!spacing) return std::unexpected(spacing.error());
            auto origin = vec_or<2>(p, "origin", Vec2{});
            if (!origin) return std::unexpected(origin.error());
            auto margin = number_or(p, "margin", 0.25);
            if (!margin) return std::unexpected(margin.error());
            return make(region, *spacing, *origin, *margin);
        };
    };
    r.factories_["grid"] = grid([](RegionId region, Real spacing, Vec2 origin, Real margin) {
        return sites_for(region, CartesianGridSource(spacing, origin, margin));
    });
    r.factories_["hexagonal"] = grid([](RegionId region, Real spacing, Vec2 origin, Real margin) {
        return sites_for(region, HexagonalGridSource(spacing, origin, margin));
    });
    r.factories_["explicit"] = [](RegionId region, const ShapeParameters& p) -> Result<RegionSites> {
        auto xy = points<2>(p, "xy");
        if (!xy) return std::unexpected(xy.error());
        return sites_for(region, ExplicitSites(std::move(*xy)));
    };
    return r;
}

Status SiteSourceRegistry2D::add(std::string name, Factory factory) {
    return add_to(factories_, std::move(name), std::move(factory));
}

Result<RegionSites> SiteSourceRegistry2D::make(const std::string& name, RegionId region,
                                               const ShapeParameters& parameters) const {
    return make_from(factories_, name, region, parameters);
}

SiteSourceRegistry3D SiteSourceRegistry3D::with_builtin_sources() {
    SiteSourceRegistry3D r;
    r.factories_["uniform"] = [](RegionId region, const ShapeParameters& p) -> Result<RegionSites3D> {
        auto s = uniform<UniformRandomSource3D>(p);
        if (!s) return std::unexpected(s.error());
        return sites_for_3d(region, std::move(*s));
    };
    r.factories_["count"] = [](RegionId region, const ShapeParameters& p) -> Result<RegionSites3D> {
        auto n = count_of(p);
        if (!n) return std::unexpected(n.error());
        auto margin = number_or(p, "margin", 0);
        if (!margin) return std::unexpected(margin.error());
        return sites_for_3d(region, RandomCountSource3D(*n, *margin));
    };
    r.factories_["grid"] = [](RegionId region, const ShapeParameters& p) -> Result<RegionSites3D> {
        auto spacing = number(p, "spacing");
        if (!spacing) return std::unexpected(spacing.error());
        auto origin = vec_or<3>(p, "origin", Vec3{});
        if (!origin) return std::unexpected(origin.error());
        auto margin = number_or(p, "margin", 0.25);
        if (!margin) return std::unexpected(margin.error());
        return sites_for_3d(region, CartesianGridSource3D(*spacing, *origin, *margin));
    };
    r.factories_["explicit"] = [](RegionId region, const ShapeParameters& p) -> Result<RegionSites3D> {
        auto xyz = points<3>(p, "xyz");
        if (!xyz) return std::unexpected(xyz.error());
        return sites_for_3d(region, ExplicitSites3D(std::move(*xyz)));
    };
    return r;
}

Status SiteSourceRegistry3D::add(std::string name, Factory factory) {
    return add_to(factories_, std::move(name), std::move(factory));
}

Result<RegionSites3D> SiteSourceRegistry3D::make(const std::string& name, RegionId region,
                                                 const ShapeParameters& parameters) const {
    return make_from(factories_, name, region, parameters);
}

ConfigRegistries ConfigRegistries::with_builtins() {
    ConfigRegistries r{ShapeRegistry::with_builtin_shapes(), ShapeRegistry3D::with_builtin_shapes(),
                       SiteSourceRegistry2D::with_builtin_sources(), SiteSourceRegistry3D::with_builtin_sources()};
    (void)r.shapes_3d.add("stl", [](const ShapeParameters& p, const PolygonizeOptions3&) -> Result<TriangleSurface> {
        const std::string file = text(p, "file");
        if (file.empty()) return bad("'stl' needs 'file'");
        StlReadOptions options;
        if (const std::string patch = text(p, "patch"); !patch.empty()) options.patch_name = patch;
        return read_stl_surface(file, options);
    });
    (void)r.shapes_3d.add("extrusion",
                          [](const ShapeParameters& p, const PolygonizeOptions3& o) -> Result<TriangleSurface> {
        auto xy = points<2>(p, "xy");
        if (!xy) return std::unexpected(xy.error());
        auto z = numbers(p, "z", 2);
        if (!z) return std::unexpected(z.error());
        const std::string tag = text(p, "tag");
        std::vector<std::string> tags(tag.empty() ? 0 : xy->size(), tag);
        auto outline = ShapeOutline::make(std::move(*xy), std::move(tags));
        if (!outline) return std::unexpected(outline.error());
        return Extrusion(std::move(*outline), (*z)[0], (*z)[1], text(p, "bottom"), text(p, "top")).surface(o);
    });
    return r;
}

}  // namespace vmm
