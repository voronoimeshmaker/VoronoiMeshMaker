// ============================================================================
// File: csr.hpp
// Description: Compressed sparse rows, cell adjacency and the sparse-matrix
//              pattern derived from it without geometry (DEC-015).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>

namespace vmm {

/// Row i owns values[offsets[i], offsets[i + 1]). Always has offsets.size() == rows() + 1.
template <class T>
struct Csr {
    std::vector<std::size_t> offsets{0};
    std::vector<T> values;

    [[nodiscard]] std::size_t rows() const noexcept { return offsets.size() - 1; }
    [[nodiscard]] std::span<const T> row(std::size_t i) const noexcept {
        return {values.data() + offsets[i], offsets[i + 1] - offsets[i]};
    }
    void push_row(std::span<const T> r) {
        values.insert(values.end(), r.begin(), r.end());
        offsets.push_back(values.size());
    }
};

using CellPair = std::pair<CellId, CellId>;

/// Symmetric cell-to-cell adjacency; each row sorted, duplicates removed.
[[nodiscard]] Csr<CellId> build_adjacency(std::size_t cell_count, std::span<const CellPair> pairs);

/// Pattern of a cell-centred operator: the diagonal plus one entry per neighbour.
[[nodiscard]] Csr<CellId> sparse_pattern(const Csr<CellId>& adjacency);

/// True when (i, j) present implies (j, i) present. Rows must be sorted.
[[nodiscard]] bool is_structurally_symmetric(const Csr<CellId>& m);

}  // namespace vmm
