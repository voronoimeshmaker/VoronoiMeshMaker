// ============================================================================
// File: ut_RcmOrdering.cpp
// Description: RcmOrdering: bandwidth reduction and determinism.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "random_mesh.hpp"
#include <vmm/core/random.hpp>
#include <vmm/mesh/invariants.hpp>
#include <vmm/reorder/reorder.hpp>

namespace {

using vmm::CellId;

/// Renumbering keeps every invariant and the multiset of geometry.
[[maybe_unused]] void expect_valid_renumbering(const vmm::Mesh2D& m, const vmm::Permutation& p) {
    const auto r = vmm::renumber(m, p);
    ASSERT_TRUE(r) << r.error().message();
    vmm::InvariantReference ref;
    ref.length_scale = 1.4142135623730951;
    ref.total_measure = 1;
    ref.region_measure = {1};
    ref.boundary_measure = 4;
    const auto rep = vmm::check_invariants(*r, ref);
    EXPECT_TRUE(rep.passed(ref)) << rep.first_problem;
    EXPECT_EQ(r->face_count(), m.face_count());
    EXPECT_EQ(r->internal_face_count(), m.internal_face_count());
    for (const CellId c : m.cells()) ASSERT_EQ(r->site(p.new_of(c)), m.site(c));
    for (std::size_t k = 0; k < m.patches().size(); ++k) EXPECT_EQ(r->patches()[k].count, m.patches()[k].count);
}

TEST(RcmOrdering, ReducesBandwidth) {
    const auto b = vmm::test::random_square_mesh(2000, 5);
    // Start from a shuffled numbering: the canonical order is already banded.
    std::vector<CellId> order;
    for (const CellId c : b.mesh.cells()) order.push_back(c);
    vmm::Random rng(11);
    rng.shuffle(std::span<CellId>(order));
    const auto shuffled = *vmm::renumber(b.mesh, *vmm::Permutation::from_order(order));
    const auto p = vmm::RcmOrdering{}.permutation(shuffled);
    expect_valid_renumbering(shuffled, p);
    const auto r = *vmm::renumber(shuffled, p);
    const auto before = vmm::bandwidth(vmm::cell_adjacency(shuffled));
    const auto after = vmm::bandwidth(vmm::cell_adjacency(r));
    EXPECT_LT(after, before / 10);
    EXPECT_LT(vmm::profile(vmm::cell_adjacency(r)), vmm::profile(vmm::cell_adjacency(shuffled)) / 10);
    // Comparable to the canonical (lexicographic) numbering.
    EXPECT_LT(after, 2 * vmm::bandwidth(vmm::cell_adjacency(b.mesh)));
    // Deterministic.
    EXPECT_EQ(vmm::RcmOrdering{}.permutation(shuffled).old_of_new()[0], p.old_of_new()[0]);
}

TEST(RcmOrdering, DisconnectedComponents) {
    vmm::Csr<CellId> adj;
    const CellId r0[] = {CellId{1}};
    const CellId r1[] = {CellId{0}};
    adj.push_row(r0);
    adj.push_row(r1);
    adj.push_row(std::span<const CellId>{});
    const auto p = vmm::reverse_cuthill_mckee(adj);
    EXPECT_EQ(p.size(), 3u);
}
}  // namespace
