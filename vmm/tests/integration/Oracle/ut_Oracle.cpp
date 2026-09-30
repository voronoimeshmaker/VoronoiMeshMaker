// ============================================================================
// File: ut_Oracle.cpp
// Description: Oracle check against the retired VMMLib (DEC-011, DEC-033),
//              whose meshes are frozen in tests/data/golden: cases O1-O4
//              rebuilt from the same sites; per cell (by site identity) the
//              area must agree to 1e-12 (relative) and the neighbour pairs
//              must be identical, zero-length VMMLib edges excepted.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "mesh_checks.hpp"
#include <vmm/backend/cgal.hpp>
#include <vmm/sites/sources.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace {

using vmm::Vec2;

struct Golden {
    Vec2 lo;
    Vec2 hi;
    std::vector<Vec2> sites;
    std::vector<double> area;                                   // by site
    std::set<std::pair<std::size_t, std::size_t>> pairs;        // positive-length edges
    std::set<std::pair<std::size_t, std::size_t>> zero_pairs;   // zero-length edges
};

bool read_golden(const std::filesystem::path& path, Golden& g) {
    std::ifstream in(path);
    std::string line;
    auto next = [&]() {
        while (std::getline(in, line)) {
            if (!line.empty() && line[0] != '#') return true;
        }
        return false;
    };
    std::string key;
    std::size_t n = 0;
    if (!next()) return false;
    std::istringstream(line) >> key >> g.lo[0] >> g.lo[1] >> g.hi[0] >> g.hi[1];
    if (!next()) return false;
    std::istringstream(line) >> key >> n;
    g.sites.resize(n);
    g.area.assign(n, 0);
    for (auto& s : g.sites) {
        if (!next()) return false;
        std::istringstream(line) >> s[0] >> s[1];
    }
    if (!next()) return false;
    std::istringstream(line) >> key >> n;
    for (std::size_t c = 0; c < n; ++c) {
        if (!next()) return false;
        std::istringstream row(line);
        std::size_t site = 0;
        std::size_t k = 0;
        double area = 0;
        row >> site >> area >> k;
        g.area[site] = area;
        for (std::size_t j = 0; j < k; ++j) {
            std::size_t nb = 0;
            double length = 0;
            row >> nb >> length;
            const auto p = std::pair(std::min(site, nb), std::max(site, nb));
            (length > 1e-9 * std::hypot(g.hi[0] - g.lo[0], g.hi[1] - g.lo[1]) ? g.pairs : g.zero_pairs).insert(p);
        }
    }
    return next() && line == "end";
}

void check_case(const std::string& name) {
    const auto path = std::filesystem::path(VMM_TEST_DATA_DIR) / "golden" / (name + ".golden");
    // Frozen since the VMMLib retirement (DEC-033): a missing file cannot be regenerated.
    ASSERT_TRUE(std::filesystem::exists(path)) << "golden file missing: " << path;
    Golden g;
    ASSERT_TRUE(read_golden(path, g)) << path;
    const auto backend = vmm::cgal_backend_2d();
    vmm::Declaration2D d;
    (void)d.add_region("box", *d.media().add("m"), vmm::Rectangle(g.lo, g.hi));
    const auto p = *backend.build_partition(d);
    vmm::SiteSet sites;
    sites.append(g.sites, vmm::RegionId{0});
    const auto b = vmm::build_mesh_2d(p, sites, backend);
    ASSERT_TRUE(b) << b.error().message();
    vmm::test::expect_invariants(p, *b, name);
    const auto& m = b->mesh;
    double max_area_error = 0;
    for (const vmm::CellId c : m.cells()) {
        const std::size_t s = m.cell_input_sites()[c.index()].index();
        max_area_error = std::max(max_area_error, std::abs(b->cell_polygon_area[c.index()] - g.area[s]) / g.area[s]);
    }
    std::set<std::pair<std::size_t, std::size_t>> mine;
    for (const vmm::FaceId f : m.internal_faces()) {
        const std::size_t a = m.cell_input_sites()[m.owner(f).index()].index();
        const std::size_t c = m.cell_input_sites()[m.neighbour(f).index()].index();
        mine.emplace(std::min(a, c), std::max(a, c));
    }
    std::size_t missing = 0;
    std::size_t extra = 0;
    for (const auto& pr : g.pairs) missing += mine.contains(pr) ? 0u : 1u;
    for (const auto& pr : mine) extra += (g.pairs.contains(pr) || g.zero_pairs.contains(pr)) ? 0u : 1u;
    std::cout << "[ ORACLE   ] " << name << ": cells " << m.cell_count() << ", pairs " << mine.size() << " (VMMLib "
              << g.pairs.size() << " + " << g.zero_pairs.size() << " zero-length), max relative area difference "
              << max_area_error << ", missing " << missing << ", extra " << extra << "\n";
    EXPECT_EQ(m.cell_count(), g.sites.size());
    EXPECT_LE(max_area_error, 1e-12);
    EXPECT_EQ(missing, 0u);
    EXPECT_EQ(extra, 0u);
}

TEST(Oracle, O1Random) { check_case("O1_random"); }
TEST(Oracle, O2Cartesian) { check_case("O2_cartesian"); }
TEST(Oracle, O3Hexagonal) { check_case("O3_hexagonal"); }
TEST(Oracle, O4UnitRandom) { check_case("O4_unit_random"); }

}  // namespace
