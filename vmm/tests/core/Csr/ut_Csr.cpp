// ============================================================================
// File: ut_Csr.cpp
// Description: Csr, build_adjacency, sparse_pattern, is_structurally_symmetric.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/csr.hpp>

namespace {

using vmm::CellId;
using vmm::CellPair;

CellId c(unsigned i) { return CellId::from_index(i); }

TEST(Csr, DefaultIsEmptyWithOneOffset) {
    const vmm::Csr<int> m;
    EXPECT_EQ(m.rows(), 0u);
    ASSERT_EQ(m.offsets.size(), 1u);
}

TEST(Csr, PushRowAndRowView) {
    vmm::Csr<int> m;
    const std::vector<int> a{1, 2, 3};
    const std::vector<int> b{};
    m.push_row(a);
    m.push_row(b);
    ASSERT_EQ(m.rows(), 2u);
    EXPECT_EQ(m.row(0).size(), 3u);
    EXPECT_EQ(m.row(0)[2], 3);
    EXPECT_TRUE(m.row(1).empty());
}

TEST(Csr, AdjacencyIsSortedSymmetricAndDeduplicated) {
    const std::vector<CellPair> pairs{{c(0), c(2)}, {c(2), c(0)}, {c(1), c(2)}, {c(0), c(1)}};
    const auto adj = vmm::build_adjacency(4, pairs);
    ASSERT_EQ(adj.rows(), 4u);
    EXPECT_EQ(std::vector<CellId>(adj.row(0).begin(), adj.row(0).end()), (std::vector<CellId>{c(1), c(2)}));
    EXPECT_EQ(std::vector<CellId>(adj.row(2).begin(), adj.row(2).end()), (std::vector<CellId>{c(0), c(1)}));
    EXPECT_TRUE(adj.row(3).empty());
    EXPECT_TRUE(vmm::is_structurally_symmetric(adj));
}

TEST(Csr, SparsePatternInsertsDiagonalInOrder) {
    const std::vector<CellPair> pairs{{c(0), c(1)}, {c(1), c(2)}};
    const auto pattern = vmm::sparse_pattern(vmm::build_adjacency(3, pairs));
    EXPECT_EQ(pattern.values, (std::vector<CellId>{c(0), c(1), c(0), c(1), c(2), c(1), c(2)}));
    EXPECT_EQ(pattern.offsets, (std::vector<std::size_t>{0, 2, 5, 7}));
    EXPECT_TRUE(vmm::is_structurally_symmetric(pattern));
}

TEST(Csr, SparsePatternOfIsolatedCellIsDiagonalOnly) {
    const auto pattern = vmm::sparse_pattern(vmm::build_adjacency(1, {}));
    EXPECT_EQ(pattern.values, (std::vector<CellId>{c(0)}));
}

TEST(Csr, AsymmetricOrOutOfRangeIsDetected) {
    vmm::Csr<CellId> m;
    const std::vector<CellId> r0{c(1)};
    const std::vector<CellId> r1{};
    m.push_row(r0);
    m.push_row(r1);
    EXPECT_FALSE(vmm::is_structurally_symmetric(m));
    vmm::Csr<CellId> bad;
    const std::vector<CellId> r{c(5)};
    bad.push_row(r);
    EXPECT_FALSE(vmm::is_structurally_symmetric(bad));
}

}  // namespace
