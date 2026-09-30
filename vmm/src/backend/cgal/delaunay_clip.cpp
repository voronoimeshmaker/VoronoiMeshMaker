// ============================================================================
// File: delaunay_clip.cpp
// Description: Delaunay neighbour pairs (Epick) and exact labelled clipping
//              of a convex cell by region components (Epeck, Boolean ops).
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <optional>
#include <span>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Triangulation_data_structure_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "backend_cgal.hpp"

namespace vmm::cgal_detail {
namespace {

using Epick = CGAL::Exact_predicates_inexact_constructions_kernel;
using Vb = CGAL::Triangulation_vertex_base_with_info_2<std::uint32_t, Epick>;
using Tds = CGAL::Triangulation_data_structure_2<Vb>;
using Delaunay = CGAL::Delaunay_triangulation_2<Epick, Tds>;

using Epeck = CGAL::Exact_predicates_exact_constructions_kernel;
using Point = Epeck::Point_2;
using Polygon = CGAL::Polygon_2<Epeck>;
using PolygonWithHoles = CGAL::Polygon_with_holes_2<Epeck>;

struct Support {
    Point a;
    Point b;
    EdgeLabel label;
};

Polygon to_polygon(const LabelledLoop2& loop, std::vector<Support>& supports) {
    Polygon poly;
    const std::size_t n = loop.points.size();
    for (std::size_t k = 0; k < n; ++k) {
        const Vec2& p = loop.points[k];
        const Vec2& q = loop.points[(k + 1) % n];
        poly.push_back(Point(p[0], p[1]));
        supports.push_back({Point(p[0], p[1]), Point(q[0], q[1]), loop.labels[k]});
    }
    return poly;
}

// collinear_are_ordered_along_line() assumes collinearity (CGAL precondition): test it first.
bool supported_by(const Support& s, const Point& p, const Point& q) {
    return CGAL::collinear(s.a, s.b, p) && CGAL::collinear(s.a, s.b, q) &&
           CGAL::collinear_are_ordered_along_line(s.a, p, s.b) && CGAL::collinear_are_ordered_along_line(s.a, q, s.b);
}

std::optional<EdgeLabel> find_label(const std::vector<Support>& supports, const Point& p, const Point& q) {
    for (const Support& s : supports) {
        if (supported_by(s, p, q)) return s.label;
    }
    return std::nullopt;
}

LabelledLoop2 to_loop(const Polygon& poly, const std::vector<Support>& region, const std::vector<Support>& cell,
                      RegionClip2& report) {
    LabelledLoop2 loop;
    const std::size_t n = poly.size();
    for (std::size_t k = 0; k < n; ++k) {
        const Point& p = poly.vertex(k);
        const Point& q = poly.vertex((k + 1) % n);
        auto label = find_label(region, p, q);
        if (!label) label = find_label(cell, p, q);
        if (!label) ++report.unlabelled_edges;
        loop.points.push_back(Vec2{CGAL::to_double(p.x()), CGAL::to_double(p.y())});
        loop.labels.push_back(label.value_or(EdgeLabel{0}));
    }
    return loop;
}

}  // namespace

std::vector<CellPair> delaunay_pairs(std::span<const Vec2> sites) {
    std::vector<std::pair<Epick::Point_2, std::uint32_t>> points;
    points.reserve(sites.size());
    for (std::size_t i = 0; i < sites.size(); ++i) {
        points.emplace_back(Epick::Point_2(sites[i][0], sites[i][1]), static_cast<std::uint32_t>(i));
    }
    const Delaunay dt(points.begin(), points.end());
    std::vector<CellPair> pairs;
    pairs.reserve(3 * sites.size());
    for (auto e = dt.finite_edges_begin(); e != dt.finite_edges_end(); ++e) {
        std::uint32_t a = e->first->vertex(Delaunay::cw(e->second))->info();
        std::uint32_t b = e->first->vertex(Delaunay::ccw(e->second))->info();
        if (b < a) std::swap(a, b);
        pairs.emplace_back(CellId{a}, CellId{b});
    }
    std::ranges::sort(pairs);
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
    return pairs;
}

namespace {

RegionClip2 clip_unchecked(const LabelledLoop2& cell_loop, std::span<const LabelledPolygon2> components) {
    std::vector<Support> cell_supports;
    std::vector<Support> region_supports;
    const Polygon cell = to_polygon(cell_loop, cell_supports);
    std::vector<PolygonWithHoles> regions;
    for (const auto& comp : components) {
        Polygon outer = to_polygon(comp.outer, region_supports);
        std::vector<Polygon> holes;
        for (const auto& h : comp.holes) holes.push_back(to_polygon(h, region_supports));
        regions.emplace_back(outer, holes.begin(), holes.end());
    }
    std::vector<PolygonWithHoles> result;
    for (const auto& region : regions) CGAL::intersection(cell, region, std::back_inserter(result));
    RegionClip2 report;
    for (const auto& piece : result) {
        LabelledPolygon2 out;
        out.outer = to_loop(piece.outer_boundary(), region_supports, cell_supports, report);
        for (const auto& h : piece.holes()) out.holes.push_back(to_loop(h, region_supports, cell_supports, report));
        report.pieces.push_back(std::move(out));
    }
    return report;
}

}  // namespace

RegionClip2 clip_by_region(const LabelledLoop2& cell_loop, std::span<const LabelledPolygon2> components) {
    try {
        return clip_unchecked(cell_loop, components);
    } catch (const std::exception& e) {
        RegionClip2 failed;
        failed.error = e.what();
        return failed;
    }
}

}  // namespace vmm::cgal_detail
