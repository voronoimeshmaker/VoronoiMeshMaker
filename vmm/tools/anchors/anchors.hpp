// ============================================================================
// File: anchors.hpp
// Description: Anchor problems of P04 (DEC-017) as declarations plus site
//              sources: A1 (river cross-section, optional air layer) and A2
//              (river plan with meander, widening and island); the 3D terrain
//              block of P17; A3 (soil block with river, P18). Shared by the
//              integration tests, the benchmark and the examples.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/declaration.hpp>
#include <vmm/domain/declaration3d.hpp>
#include <vmm/domain/shapes.hpp>
#include <vmm/domain/shapes3d.hpp>
#include <vmm/geometry/surface.hpp>
#include <vmm/sites/sources.hpp>
#include <vmm/sites/sources3d.hpp>

namespace vmm::anchors {

struct Anchor {
    Declaration2D declaration;
    std::vector<RegionSites> sources;
};

/// A1 (P04 §2.1): 200 m x 15 m section, trapezoidal channel (top 40 m, depth
/// 5 m, 1V:2H), soil contact at z = -8 m; with `air`, a 5 m air layer on top.
/// `refinement` scales every spacing (1 = 0.5 m near the bed, up to 5 m).
inline Anchor a1(bool air = false, double refinement = 1) {
    Anchor a;
    auto& d = a.declaration;
    const MediumId water = *d.media().add("water");
    const MediumId solid = *d.media().add("solid");
    const double top = air ? 5 : 0;
    const std::string sky = air ? "" : "terreno";
    (void)d.add_region("solo_inf", solid, Rectangle(Vec2{-100, -15}, Vec2{100, top}, {"base", "lateral_dir", air ? "topo_ar" : "terreno", "lateral_esq"}));
    (void)d.add_region("solo_sup", solid, Rectangle(Vec2{-100, -8}, Vec2{100, 0}, {"", "lateral_dir", sky, "lateral_esq"}));
    (void)d.add_region("canal", water,
                       PolygonShape({{-20, 0}, {-10, -5}, {10, -5}, {20, 0}}, {"", "", "", air ? "" : "superficie_agua"}));
    if (air) {
        const MediumId gas = *d.media().add("air");
        (void)d.add_region("atmosfera", gas, Rectangle(Vec2{-100, 0}, Vec2{100, 5}, {"", "lateral_dir", "topo_ar", "lateral_esq"}));
    }
    const SpacingField h = [refinement](const Vec2& x) {
        // 0.5 m at the channel bed, growing to 5 m away from it.
        const double distance = std::hypot(std::max(0.0, std::abs(x[0]) - 15), x[1] + 3);
        return refinement * std::min(5.0, 0.5 + 0.12 * distance);
    };
    for (std::size_t r = 0; r < d.regions().size(); ++r) {
        a.sources.push_back(sites_for(RegionId::from_index(r), AdaptiveQuadtreeSource(h, 0.5 * refinement)));
    }
    return a;
}

/// Centre line of the A2 channel: sine meander y = 500 + 150 sin(2 pi x / 800).
inline Vec2 a2_axis(double x) { return {x, 500 + 150 * std::sin(2 * std::numbers::pi * x / 800)}; }

/// A2 (P04 §2.2): 2 km x 1 km floodplain, meandering channel 30 m wide that
/// widens to 80 m around an island (60 m x 15 m ellipse) at x = 1000 m.
inline Anchor a2(double refinement = 1) {
    Anchor a;
    auto& d = a.declaration;
    const MediumId water = *d.media().add("water");
    const MediumId solid = *d.media().add("solid");
    (void)d.add_region("planicie", solid,
                       Rectangle(Vec2{0, 0}, Vec2{2000, 1000}, {"limite_planicie", "limite_planicie", "limite_planicie", "limite_planicie"}));
    auto width = [](double x) { return 30 + 50 * std::exp(-std::pow((x - 1000) / 60, 2)); };
    std::vector<Vec2> left;
    std::vector<Vec2> right;
    for (int k = 0; k <= 400; ++k) {
        const double x = 2000.0 * k / 400;
        const Vec2 c = a2_axis(x);
        const double slope = 150 * 2 * std::numbers::pi / 800 * std::cos(2 * std::numbers::pi * x / 800);
        const double n = std::hypot(1.0, slope);
        const Vec2 normal{-slope / n, 1 / n};
        const double half = 0.5 * width(x);
        // Clamp to the plain at the two ends so the channel meets the boundary.
        Vec2 l = c + half * normal;
        Vec2 r = c - half * normal;
        if (k == 0 || k == 400) {
            l = Vec2{x, c[1] + half};
            r = Vec2{x, c[1] - half};
        }
        left.push_back(l);
        right.push_back(r);
    }
    std::vector<Vec2> channel = right;
    channel.insert(channel.end(), left.rbegin(), left.rend());
    std::vector<std::string> tags(channel.size());
    tags[right.size() - 1] = "jusante";   // right end x = 2000
    tags[channel.size() - 1] = "montante";  // closing edge at x = 0
    (void)d.add_region("canal", water, PolygonShape(channel, tags));
    (void)d.add_region("ilha", solid, Ellipse(a2_axis(1000), 30, 7.5, std::atan(150 * 2 * std::numbers::pi / 800 * std::cos(2 * std::numbers::pi * 1000 / 800))));
    const SpacingField h = [refinement](const Vec2& x) {
        const double distance = std::abs(x[1] - a2_axis(x[0])[1]);
        const double near_island = std::hypot(x[0] - 1000, x[1] - a2_axis(1000)[1]);
        return refinement * std::min({50.0, 2 + 0.3 * std::max(0.0, distance - 40), near_island < 80 ? 1.0 : 1e9});
    };
    for (std::size_t r = 0; r < d.regions().size(); ++r) {
        a.sources.push_back(sites_for(RegionId::from_index(r), AdaptiveQuadtreeSource(h, 0.5 * refinement)));
    }
    return a;
}

/// Terrain block (P17): [0,1]^2 x [0, h(x, y)] with a wavy top, every face an
/// n x n grid of squares split in two (4 n^2 + 8 n triangles), patches
/// "terrain" (top) and "rock" (bottom and sides): the kind of closed surface an
/// STL file of a terrain brings.
inline TriangleSurface terrain_block(int n) {
    std::vector<Vec3> pts;
    std::vector<Triangle> tris;
    std::vector<std::uint32_t> patch;
    const auto h = [](double x, double y) {
        return 0.4 + 0.08 * std::sin(2 * std::numbers::pi * x) * std::cos(2 * std::numbers::pi * y) + 0.05 * x;
    };
    std::map<std::array<long long, 3>, std::uint32_t> id;  // points shared by neighbouring faces
    const auto point = [&](double x, double y, double z) {
        const std::array<long long, 3> key{std::llround(x * 1e9), std::llround(y * 1e9), std::llround(z * 1e9)};
        const auto [it, added] = id.emplace(key, static_cast<std::uint32_t>(pts.size()));
        if (added) pts.push_back({x, y, z});
        return it->second;
    };
    const auto quad = [&](std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t p) {
        tris.push_back({a, b, c});
        tris.push_back({a, c, d});
        patch.insert(patch.end(), {p, p});
    };
    const double m = n;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            const double x0 = i / m, x1 = (i + 1) / m, y0 = j / m, y1 = (j + 1) / m;
            quad(point(x0, y0, h(x0, y0)), point(x1, y0, h(x1, y0)), point(x1, y1, h(x1, y1)), point(x0, y1, h(x0, y1)), 0);
            quad(point(x0, y0, 0), point(x0, y1, 0), point(x1, y1, 0), point(x1, y0, 0), 1);
        }
        const double a = i / m, b = (i + 1) / m;
        quad(point(a, 0, 0), point(b, 0, 0), point(b, 0, h(b, 0)), point(a, 0, h(a, 0)), 1);  // y = 0
        quad(point(b, 1, 0), point(a, 1, 0), point(a, 1, h(a, 1)), point(b, 1, h(b, 1)), 1);  // y = 1
        quad(point(0, b, 0), point(0, a, 0), point(0, a, h(0, a)), point(0, b, h(0, b)), 1);  // x = 0
        quad(point(1, a, 0), point(1, b, 0), point(1, b, h(1, b)), point(1, a, h(1, a)), 1);  // x = 1
    }
    return *TriangleSurface::make(std::move(pts), std::move(tris), std::move(patch), {"terrain", "rock"});
}

/// Closed block between two heightfields over [x0, x1] x [y0, y1]: bottom z =
/// lo(x, y), top z = hi(x, y), an nx x ny grid on every face. Tags: x-, x+,
/// y-, y+, z-, z+ ("" = boundary).
inline TriangleSurface heightfield_block(double x0, double x1, double y0, double y1,
                                         const std::function<double(double, double)>& lo,
                                         const std::function<double(double, double)>& hi, int nx, int ny,
                                         const std::array<std::string, 6>& tags) {
    std::vector<Vec3> pts;
    std::vector<Triangle> tris;
    std::vector<std::uint32_t> patch;
    std::map<std::array<long long, 3>, std::uint32_t> id;
    const auto point = [&](double x, double y, double z) {
        const std::array<long long, 3> key{std::llround(x * 1e6), std::llround(y * 1e6), std::llround(z * 1e6)};
        const auto [it, added] = id.emplace(key, static_cast<std::uint32_t>(pts.size()));
        if (added) pts.push_back({x, y, z});
        return it->second;
    };
    const auto quad = [&](std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t p) {
        tris.push_back({a, b, c});
        tris.push_back({a, c, d});
        patch.insert(patch.end(), {p, p});
    };
    const auto X = [&](int i) { return x0 + (x1 - x0) * i / nx; };
    const auto Y = [&](int j) { return y0 + (y1 - y0) * j / ny; };
    for (int i = 0; i < nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            const double a = X(i), b = X(i + 1), c = Y(j), d = Y(j + 1);
            quad(point(a, c, hi(a, c)), point(b, c, hi(b, c)), point(b, d, hi(b, d)), point(a, d, hi(a, d)), 5);
            quad(point(a, c, lo(a, c)), point(a, d, lo(a, d)), point(b, d, lo(b, d)), point(b, c, lo(b, c)), 4);
        }
    }
    for (int i = 0; i < nx; ++i) {
        const double a = X(i), b = X(i + 1);
        quad(point(a, y0, lo(a, y0)), point(b, y0, lo(b, y0)), point(b, y0, hi(b, y0)), point(a, y0, hi(a, y0)), 2);
        quad(point(b, y1, lo(b, y1)), point(a, y1, lo(a, y1)), point(a, y1, hi(a, y1)), point(b, y1, hi(b, y1)), 3);
    }
    for (int j = 0; j < ny; ++j) {
        const double c = Y(j), d = Y(j + 1);
        quad(point(x0, d, lo(x0, d)), point(x0, c, lo(x0, c)), point(x0, c, hi(x0, c)), point(x0, d, hi(x0, d)), 0);
        quad(point(x1, c, lo(x1, c)), point(x1, d, lo(x1, d)), point(x1, d, hi(x1, d)), point(x1, c, hi(x1, c)), 1);
    }
    std::vector<std::string> names;
    for (const auto& t : tags) names.push_back(t.empty() ? "boundary" : t);
    return *TriangleSurface::make(std::move(pts), std::move(tris), std::move(patch), std::move(names));
}

struct Anchor3 {
    Declaration3D declaration;
    std::vector<RegionSites3D> sources;
};

/// Axis of the A3 channel in plan and its depth (P04 §2.3).
inline double a3_axis(double x) { return 100 + 30 * std::sin(2 * std::numbers::pi * x / 500); }
inline double a3_depth(double x) { return 3 + 3 * x / 500; }

/// A3 (P04 §2.3): soil block 500 m x 200 m x 30 m; lower and upper soil with an
/// inclined contact z = 15 + 0.01 x; air above the terrain level z = 25; a
/// trapezoidal channel (top 40 m, banks 1V:2H, depth 3 m to 6 m along x) whose
/// axis curves in plan, filled with water up to the terrain level. Regions by
/// precedence: solo_inf, solo_sup, atmosfera, canal. `n` = slices of the channel.
inline Anchor3 a3(double refinement = 1, int n = 40) {
    Anchor3 a;
    auto& d = a.declaration;
    const auto solid = *d.media().add("solid");
    const auto gas = *d.media().add("air");
    const auto water = *d.media().add("water");
    const std::array<std::string, 6> box_tags{"montante", "jusante", "margem_dir", "margem_esq", "base", "topo_ar"};
    const auto inf = *d.add_region("solo_inf", solid, Cuboid({0, 0, 0}, {500, 200, 30}, box_tags));
    const auto sup = *d.add_region_surface(
        "solo_sup", solid,
        heightfield_block(0, 500, 0, 200, [](double x, double) { return 15 + 0.01 * x; }, [](double, double) { return 30.0; }, 10, 4,
                          box_tags));
    const auto air = *d.add_region("atmosfera", gas, Cuboid({0, 0, 25}, {500, 200, 30}, box_tags));
    // Channel: cross-sections in the planes x = const, swept along x.
    std::vector<Vec3> pts;
    std::vector<Triangle> tris;
    std::vector<std::uint32_t> patch;  // 0 leito, 1 montante, 2 jusante, 3 superficie
    for (int i = 0; i <= n; ++i) {
        const double x = 500.0 * i / n;
        const double yc = a3_axis(x);
        const double h = a3_depth(x);
        pts.push_back({x, yc - 20, 25});
        pts.push_back({x, yc - 20 + 2 * h, 25 - h});
        pts.push_back({x, yc + 20 - 2 * h, 25 - h});
        pts.push_back({x, yc + 20, 25});
    }
    const auto at = [](int i, int k) { return static_cast<std::uint32_t>(4 * i + k); };
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < 4; ++k) {
            const int q = (k + 1) % 4;
            tris.push_back({at(i, k), at(i, q), at(i + 1, q)});
            tris.push_back({at(i, k), at(i + 1, q), at(i + 1, k)});
            const std::uint32_t tag = k == 3 ? 3 : 0;
            patch.insert(patch.end(), {tag, tag});
        }
    }
    tris.push_back({at(0, 3), at(0, 2), at(0, 1)});
    tris.push_back({at(0, 3), at(0, 1), at(0, 0)});
    tris.push_back({at(n, 0), at(n, 1), at(n, 2)});
    tris.push_back({at(n, 0), at(n, 2), at(n, 3)});
    patch.insert(patch.end(), {1, 1, 2, 2});
    const auto canal = *d.add_region_surface(
        "canal", water,
        *TriangleSurface::make(std::move(pts), std::move(tris), std::move(patch), {"leito", "montante", "jusante", "superficie"}));
    const double h = 1 / refinement;
    a.sources.push_back(sites_for_3d(inf, UniformRandomSource3D(12 * h)));
    a.sources.push_back(sites_for_3d(sup, UniformRandomSource3D(9 * h)));
    // The air layer is 5 m thick: a small margin to its surface leaves room for sites.
    a.sources.push_back(sites_for_3d(air, UniformRandomSource3D(8 * h).boundary_margin_fraction(0.1)));
    a.sources.push_back(sites_for_3d(canal, UniformRandomSource3D(3 * h)));
    return a;
}

}  // namespace vmm::anchors
