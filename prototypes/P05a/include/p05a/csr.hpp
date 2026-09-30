// ============================================================================
// File: csr.hpp
// Description: P05a prototype - compact adjacency (CSR) and the sparse-matrix
//              pattern derived from it without any geometry (DEC-015).
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/types.hpp>

namespace vmm::p05a {

/// Unordered neighbour pair between two cells (first < second after normalisation).
using CellPair = std::pair<CellId, CellId>;

/// Compressed sparse rows: row i owns values[offsets[i], offsets[i + 1]).
template <class T>
struct Csr {
    std::vector<std::size_t> offsets{0};
    std::vector<T> values;

    [[nodiscard]] std::size_t rows() const noexcept { return offsets.size() - 1; }
    [[nodiscard]] std::span<const T> row(std::size_t i) const noexcept {
        return {values.data() + offsets[i], offsets[i + 1] - offsets[i]};
    }
};

/// Builds a symmetric cell-to-cell adjacency; every row is sorted, duplicates are dropped.
[[nodiscard]] inline Csr<CellId> build_adjacency(std::size_t cell_count, std::span<const CellPair> pairs) {
    std::vector<std::size_t> degree(cell_count, 0);
    for (const auto& [a, b] : pairs) {
        ++degree[a.index()];
        ++degree[b.index()];
    }
    Csr<CellId> csr;
    csr.offsets.assign(cell_count + 1, 0);
    for (std::size_t i = 0; i < cell_count; ++i) csr.offsets[i + 1] = csr.offsets[i] + degree[i];
    csr.values.resize(csr.offsets.back());
    std::vector<std::size_t> cursor(csr.offsets.begin(), csr.offsets.end() - 1);
    for (const auto& [a, b] : pairs) {
        csr.values[cursor[a.index()]++] = b;
        csr.values[cursor[b.index()]++] = a;
    }
    // Sort and deduplicate each row, then compact.
    std::vector<std::size_t> offsets{0};
    std::vector<CellId> values;
    values.reserve(csr.values.size());
    for (std::size_t i = 0; i < cell_count; ++i) {
        auto first = csr.values.begin() + static_cast<std::ptrdiff_t>(csr.offsets[i]);
        auto last = csr.values.begin() + static_cast<std::ptrdiff_t>(csr.offsets[i + 1]);
        std::sort(first, last);
        last = std::unique(first, last);
        values.insert(values.end(), first, last);
        offsets.push_back(values.size());
    }
    return Csr<CellId>{std::move(offsets), std::move(values)};
}

/// Pattern of a cell-centred operator: the diagonal plus one entry per neighbour.
/// Only the adjacency is read; no coordinates are needed.
[[nodiscard]] inline Csr<CellId> sparse_pattern(const Csr<CellId>& adjacency) {
    Csr<CellId> pattern;
    pattern.offsets.assign(adjacency.rows() + 1, 0);
    pattern.values.reserve(adjacency.values.size() + adjacency.rows());
    for (std::size_t i = 0; i < adjacency.rows(); ++i) {
        const CellId diagonal{static_cast<std::uint32_t>(i)};
        bool diagonal_done = false;
        for (const CellId j : adjacency.row(i)) {
            if (!diagonal_done && diagonal < j) {
                pattern.values.push_back(diagonal);
                diagonal_done = true;
            }
            pattern.values.push_back(j);
        }
        if (!diagonal_done) pattern.values.push_back(diagonal);
        pattern.offsets[i + 1] = pattern.values.size();
    }
    return pattern;
}

/// True when (i, j) present implies (j, i) present.
[[nodiscard]] inline bool is_structurally_symmetric(const Csr<CellId>& m) {
    for (std::size_t i = 0; i < m.rows(); ++i) {
        const CellId ci{static_cast<std::uint32_t>(i)};
        for (const CellId j : m.row(i)) {
            const auto r = m.row(j.index());
            if (!std::binary_search(r.begin(), r.end(), ci)) return false;
        }
    }
    return true;
}

}  // namespace vmm::p05a
