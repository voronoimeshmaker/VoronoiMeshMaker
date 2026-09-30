// ============================================================================
// File: p05a_1_csr.cpp
// Description: P05a.1 - compact adjacency (CSR) from Delaunay neighbour pairs
//              and the sparse-matrix pattern derived from it, in 2D and 3D.
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <format>
#include <print>
#include <set>
#include <span>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "app_support.hpp"
#include <p05a/backend.hpp>
#include <p05a/csr.hpp>

namespace {

using namespace vmm::p05a;

/// Pattern computed straight from the pairs with ordered sets (reference).
std::vector<std::set<CellId>> reference_pattern(std::size_t n, std::span<const CellPair> pairs) {
    std::vector<std::set<CellId>> rows(n);
    for (std::size_t i = 0; i < n; ++i) rows[i].insert(CellId{static_cast<std::uint32_t>(i)});
    for (const auto& [a, b] : pairs) {
        rows[a.index()].insert(b);
        rows[b.index()].insert(a);
    }
    return rows;
}

void check_pattern(std::string_view label, std::size_t n, std::span<const CellPair> pairs, app::Checks& checks) {
    const Csr<CellId> adjacency = build_adjacency(n, pairs);
    const Csr<CellId> pattern = sparse_pattern(adjacency);
    const auto reference = reference_pattern(n, pairs);

    bool equal = pattern.rows() == n;
    std::size_t min_degree = n;
    std::size_t max_degree = 0;
    for (std::size_t i = 0; equal && i < n; ++i) {
        const auto row = pattern.row(i);
        equal = std::ranges::equal(row, reference[i]);
        min_degree = std::min(min_degree, adjacency.row(i).size());
        max_degree = std::max(max_degree, adjacency.row(i).size());
    }
    const std::size_t nnz = pattern.values.size();
    const std::size_t bytes = adjacency.offsets.size() * sizeof(std::size_t) + adjacency.values.size() * sizeof(CellId);

    std::println("{}: cells {} | Delaunay pairs {} | nnz {} | degree min {} max {} mean {:.3f} | CSR bytes/cell {:.1f}",
                 label, n, pairs.size(), nnz, min_degree, max_degree,
                 2.0 * static_cast<double>(pairs.size()) / static_cast<double>(n),
                 static_cast<double>(bytes) / static_cast<double>(n));
    checks.expect(is_structurally_symmetric(pattern), std::format("{} pattern is structurally symmetric", label));
    checks.expect(equal, std::format("{} pattern equals the pattern computed directly from the pairs", label));
    checks.expect(nnz == n + 2 * pairs.size(), std::format("{} nnz == cells + 2 * pairs", label));
    checks.expect(min_degree > 0, std::format("{} every cell has at least one neighbour", label));
}

}  // namespace

int main() {
    std::println("P05a.1 - CSR adjacency and sparse pattern (DEC-015), backend behind DEC-007 firewall");
    app::print_backend();
    app::Checks checks;

    app::Random rng(20260928);
    std::vector<Vec2> sites2(2000);
    for (auto& p : sites2) p = {rng.uniform(), rng.uniform()};
    const auto pairs2 = delaunay_pairs_2d(sites2);
    check_pattern("2D", sites2.size(), pairs2, checks);

    std::vector<Vec3> sites3(1000);
    for (auto& p : sites3) p = {rng.uniform(), rng.uniform(), rng.uniform()};
    const auto pairs3 = delaunay_pairs_3d(sites3);
    check_pattern("3D", sites3.size(), pairs3, checks);

    std::println("P05a.1 result: {}", checks.failures() == 0 ? "PASS" : "FAIL");
    return checks.exit_code();
}
