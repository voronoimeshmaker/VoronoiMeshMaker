// ============================================================================
// File: generate_golden.cpp
// Description: Oracle golden files from the legacy VMMLib (DEC-011): cases
//              O1-O4 of P06 §11. Writes the sites and, per cell, the area and
//              the sorted neighbour sites, with compiler, flags and versions
//              in the header. Linked with the legacy library (GPL).
// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <boost/version.hpp>
#include <CGAL/version.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/ClippedVoronoiBuilder2D.hpp>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>

#ifndef VMM_GOLDEN_FLAGS
#define VMM_GOLDEN_FLAGS "unknown"
#endif

namespace {

using namespace vmm::s2d;
using namespace vmm::vd2d;

template <class Pattern>
int write_case(const std::filesystem::path& dir, const std::string& name, double w, double h, Pattern pattern) {
    const auto boundary = vmm::b2d::make_boundary(vmm::b2d::Rectangle(Point2{0, 0}, w, h));
    const auto sites = make_sites(boundary, pattern);
    ClippedVoronoiBuildOptions2D options;
    options.allow_parallel_cell_build = false;
    const auto d = ClippedVoronoiBuilder2D::build(sites, boundary, options);
    double lo[2] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()};
    double hi[2] = {-lo[0], -lo[1]};
    for (const auto& c : d.cells) {
        for (const auto& p : c.polygon) {
            lo[0] = std::min(lo[0], static_cast<double>(p.x));
            lo[1] = std::min(lo[1], static_cast<double>(p.y));
            hi[0] = std::max(hi[0], static_cast<double>(p.x));
            hi[1] = std::max(hi[1], static_cast<double>(p.y));
        }
    }
    std::ofstream out(dir / (name + ".golden"));
    out << "# vmm golden file (VMMLib oracle, DEC-011) case " << name << "\n";
    out << "# compiler " << __VERSION__ << "; flags " << VMM_GOLDEN_FLAGS << "\n";
    out << "# CGAL " << CGAL_VERSION_STR << "; Boost " << BOOST_LIB_VERSION << "\n";
    out << std::format("domain {:.17g} {:.17g} {:.17g} {:.17g}\n", lo[0], lo[1], hi[0], hi[1]);
    out << "sites " << d.sites.size() << "\n";
    for (const auto& s : d.sites) out << std::format("{:.17g} {:.17g}\n", static_cast<double>(s.point.x), static_cast<double>(s.point.y));
    out << "cells " << d.cells.size() << "\n";
    for (const auto& c : d.cells) {
        // Neighbour site and the total length of the shared edges (zero-length
        // edges between cocircular sites are kept by VMMLib).
        std::vector<std::pair<std::size_t, double>> nb;
        for (const auto& e : c.edges) {
            if (e.is_boundary_edge) continue;
            const auto id = static_cast<std::size_t>(e.neighbour_site_id.value);
            const auto it = std::ranges::find(nb, id, &std::pair<std::size_t, double>::first);
            if (it == nb.end()) {
                nb.emplace_back(id, static_cast<double>(e.length));
            } else {
                it->second += static_cast<double>(e.length);
            }
        }
        std::ranges::sort(nb);
        out << std::format("{} {:.17g} {}", c.site_id.value, static_cast<double>(c.area()), nb.size());
        for (const auto& [n, length] : nb) out << std::format(" {} {:.17g}", n, length);
        out << "\n";
    }
    out << "end\n";
    std::cout << name << ": " << d.cells.size() << " cells -> " << (dir / (name + ".golden")).string() << "\n";
    return out ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    const std::filesystem::path dir = argc > 1 ? argv[1] : ".";
    std::filesystem::create_directories(dir);
    int status = 0;
    status |= write_case(dir, "O1_random", 4, 3, UniformRandom2D{2000, 20260929U});
    status |= write_case(dir, "O2_cartesian", 4, 3, CartesianGridCount2D{40, 30});
    status |= write_case(dir, "O3_hexagonal", 4, 3, HexagonalGrid2D{40});
    status |= write_case(dir, "O4_unit_random", 1, 1, UniformRandom2D{10000, 8675309U});
    return status;
}
