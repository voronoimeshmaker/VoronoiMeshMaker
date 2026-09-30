// ============================================================================
// File: csr.cpp
// Description: Adjacency and sparse pattern construction.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/csr.hpp>

namespace vmm {

Csr<CellId> build_adjacency(std::size_t cell_count, std::span<const CellPair> pairs) {
    std::vector<std::size_t> start(cell_count + 1, 0);
    for (const auto& [a, b] : pairs) {
        ++start[a.index() + 1];
        ++start[b.index() + 1];
    }
    for (std::size_t i = 0; i < cell_count; ++i) start[i + 1] += start[i];
    std::vector<CellId> raw(start.back());
    std::vector<std::size_t> cursor(start.begin(), start.end() - 1);
    for (const auto& [a, b] : pairs) {
        raw[cursor[a.index()]++] = b;
        raw[cursor[b.index()]++] = a;
    }
    Csr<CellId> csr;
    csr.offsets.reserve(cell_count + 1);
    csr.values.reserve(raw.size());
    for (std::size_t i = 0; i < cell_count; ++i) {
        const auto first = raw.begin() + static_cast<std::ptrdiff_t>(start[i]);
        auto last = raw.begin() + static_cast<std::ptrdiff_t>(start[i + 1]);
        std::sort(first, last);
        last = std::unique(first, last);
        csr.values.insert(csr.values.end(), first, last);
        csr.offsets.push_back(csr.values.size());
    }
    return csr;
}

Csr<CellId> sparse_pattern(const Csr<CellId>& adjacency) {
    Csr<CellId> pattern;
    pattern.offsets.reserve(adjacency.rows() + 1);
    pattern.values.reserve(adjacency.values.size() + adjacency.rows());
    for (std::size_t i = 0; i < adjacency.rows(); ++i) {
        const CellId diagonal = CellId::from_index(i);
        bool placed = false;
        for (const CellId j : adjacency.row(i)) {
            if (!placed && diagonal < j) {
                pattern.values.push_back(diagonal);
                placed = true;
            }
            pattern.values.push_back(j);
        }
        if (!placed) pattern.values.push_back(diagonal);
        pattern.offsets.push_back(pattern.values.size());
    }
    return pattern;
}

bool is_structurally_symmetric(const Csr<CellId>& m) {
    for (std::size_t i = 0; i < m.rows(); ++i) {
        const CellId ci = CellId::from_index(i);
        for (const CellId j : m.row(i)) {
            if (j.index() >= m.rows()) return false;
            const auto r = m.row(j.index());
            if (!std::binary_search(r.begin(), r.end(), ci)) return false;
        }
    }
    return true;
}

}  // namespace vmm
