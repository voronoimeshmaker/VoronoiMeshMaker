// ============================================================================
// File: p05a_core_only.cpp
// Description: P05a.1 - links against p05a_core only (no CGAL, no backend):
//              structured-grid adjacency through the same CSR and mesh types.
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <print>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/csr.hpp>
#include <p05a/mesh.hpp>

int main() {
    using namespace vmm::p05a;
    constexpr std::uint32_t nx = 4;
    constexpr std::uint32_t ny = 3;
    std::vector<CellPair> pairs;
    for (std::uint32_t j = 0; j < ny; ++j) {
        for (std::uint32_t i = 0; i < nx; ++i) {
            const std::uint32_t c = j * nx + i;
            if (i + 1 < nx) pairs.emplace_back(CellId{c}, CellId{c + 1});
            if (j + 1 < ny) pairs.emplace_back(CellId{c}, CellId{c + nx});
        }
    }
    const Csr<CellId> pattern = sparse_pattern(build_adjacency(nx * ny, pairs));
    // 5-point stencil: 5 * interior + 4 * edge + 3 * corner entries.
    const std::size_t expected = 5 * 2 + 4 * 6 + 3 * 4;
    const bool ok = pattern.values.size() == expected && is_structurally_symmetric(pattern);
    std::println("core-only 4x3 grid: nnz {} (expected {}) -> {}", pattern.values.size(), expected, ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
