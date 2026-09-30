// ============================================================================
// File: delaunay_pairs.cpp
// Description: P05a prototype - Delaunay neighbour pairs through CGAL. One of
//              the only two translation units that include CGAL headers.
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <typeinfo>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <boost/core/demangle.hpp>
#include <boost/version.hpp>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Triangulation_data_structure_2.h>
#include <CGAL/Triangulation_data_structure_3.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_3.h>
#include <CGAL/version.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/backend.hpp>

#if defined(P05A_EXPECTED_CGAL_VERSION_NR)
static_assert(CGAL_VERSION_NR == P05A_EXPECTED_CGAL_VERSION_NR,
              "CGAL headers differ from the CGAL package selected by CMake");
#endif

namespace vmm::p05a {
namespace {

using Epick = CGAL::Exact_predicates_inexact_constructions_kernel;

using Vb2 = CGAL::Triangulation_vertex_base_with_info_2<std::uint32_t, Epick>;
using Tds2 = CGAL::Triangulation_data_structure_2<Vb2>;
using Delaunay2 = CGAL::Delaunay_triangulation_2<Epick, Tds2>;

using Vb3 = CGAL::Triangulation_vertex_base_with_info_3<std::uint32_t, Epick>;
using Tds3 = CGAL::Triangulation_data_structure_3<Vb3>;
using Delaunay3 = CGAL::Delaunay_triangulation_3<Epick, Tds3>;

CellPair make_pair_sorted(std::uint32_t a, std::uint32_t b) {
    if (b < a) std::swap(a, b);
    return {CellId{a}, CellId{b}};
}

void sort_unique(std::vector<CellPair>& pairs) {
    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
}

}  // namespace

BackendInfo backend_info() {
    return BackendInfo{CGAL_VERSION_STR, BOOST_LIB_VERSION,
                       boost::core::demangle(typeid(CGAL::Exact_predicates_exact_constructions_kernel::FT::Exact_type).name())};
}

std::vector<CellPair> delaunay_pairs_2d(std::span<const Vec2> sites) {
    std::vector<std::pair<Epick::Point_2, std::uint32_t>> points;
    points.reserve(sites.size());
    for (std::size_t i = 0; i < sites.size(); ++i) {
        points.emplace_back(Epick::Point_2(sites[i][0], sites[i][1]), static_cast<std::uint32_t>(i));
    }
    const Delaunay2 dt(points.begin(), points.end());
    std::vector<CellPair> pairs;
    for (auto e = dt.finite_edges_begin(); e != dt.finite_edges_end(); ++e) {
        const auto face = e->first;
        const int i = e->second;
        pairs.push_back(make_pair_sorted(face->vertex(Delaunay2::cw(i))->info(), face->vertex(Delaunay2::ccw(i))->info()));
    }
    sort_unique(pairs);
    return pairs;
}

std::vector<CellPair> delaunay_pairs_3d(std::span<const Vec3> sites) {
    std::vector<std::pair<Epick::Point_3, std::uint32_t>> points;
    points.reserve(sites.size());
    for (std::size_t i = 0; i < sites.size(); ++i) {
        points.emplace_back(Epick::Point_3(sites[i][0], sites[i][1], sites[i][2]), static_cast<std::uint32_t>(i));
    }
    const Delaunay3 dt(points.begin(), points.end());
    std::vector<CellPair> pairs;
    for (auto e = dt.finite_edges_begin(); e != dt.finite_edges_end(); ++e) {
        const auto cell = e->first;
        pairs.push_back(make_pair_sorted(cell->vertex(e->second)->info(), cell->vertex(e->third)->info()));
    }
    sort_unique(pairs);
    return pairs;
}

}  // namespace vmm::p05a
