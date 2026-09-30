// ============================================================================
// File: ut_HilbertOrdering.cpp
// Description: HilbertOrdering: valid permutation and locality.
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

TEST(HilbertOrdering, LocalityBeatsRandomOrder) {
    const auto b = vmm::test::random_square_mesh(1000, 4);
    const auto p = vmm::HilbertOrdering{}.permutation(b.mesh);
    expect_valid_renumbering(b.mesh, p);
    const auto r = *vmm::renumber(b.mesh, p);
    double jump = 0;
    for (std::size_t c = 1; c < r.cell_count(); ++c) jump += vmm::norm(r.sites()[c] - r.sites()[c - 1]);
    EXPECT_LT(jump / static_cast<double>(r.cell_count()), 0.1);  // random order gives ~0.52
    const auto coarse = vmm::HilbertOrdering{2}.permutation(b.mesh);
    EXPECT_EQ(coarse.size(), b.mesh.cell_count());
}
}  // namespace
