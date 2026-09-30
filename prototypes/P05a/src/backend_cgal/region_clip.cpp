// ============================================================================
// File: region_clip.cpp
// Description: P05a prototype - clipping of a convex cell by a region polygon
//              with CGAL Boolean_set_operations_2, keeping edge labels.
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <iterator>
#include <optional>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/backend.hpp>

namespace vmm::p05a {
namespace {

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

bool supported_by(const Support& s, const Point& p, const Point& q) {
    // collinear_are_ordered_along_line() assumes collinearity; test it first.
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
        const Point& p = poly.vertex(static_cast<int>(k));
        const Point& q = poly.vertex(static_cast<int>((k + 1) % n));
        const auto from_region = find_label(region, p, q);
        const auto from_cell = find_label(cell, p, q);
        if (from_region && from_cell) ++report.ambiguous_edges;
        if (!from_region && !from_cell) ++report.unlabelled_edges;
        loop.points.push_back(Vec2{CGAL::to_double(p.x()), CGAL::to_double(p.y())});
        loop.labels.push_back(from_region ? *from_region : (from_cell ? *from_cell : EdgeLabel{0}));
    }
    return loop;
}

}  // namespace

RegionClip2 clip_by_region_2d(const LabelledLoop2& convex_cell, const LabelledPolygon2& region) {
    std::vector<Support> cell_supports;
    std::vector<Support> region_supports;
    Polygon cell = to_polygon(convex_cell, cell_supports);
    Polygon outer = to_polygon(region.outer, region_supports);
    std::vector<Polygon> holes;
    for (const auto& h : region.holes) holes.push_back(to_polygon(h, region_supports));
    const PolygonWithHoles domain(outer, holes.begin(), holes.end());

    std::vector<PolygonWithHoles> result;
    CGAL::intersection(cell, domain, std::back_inserter(result));

    RegionClip2 report;
    for (const auto& piece : result) {
        LabelledPolygon2 out;
        out.outer = to_loop(piece.outer_boundary(), region_supports, cell_supports, report);
        for (const auto& h : piece.holes()) out.holes.push_back(to_loop(h, region_supports, cell_supports, report));
        report.pieces.push_back(std::move(out));
    }
    return report;
}

}  // namespace vmm::p05a
