// ============================================================================
// File: ut_IdentityOrdering.cpp
// Description: IdentityOrdering.
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

static_assert(vmm::Ordering<vmm::IdentityOrdering, 2>);

TEST(IdentityOrdering, IsIdentity) {
    const auto b = vmm::test::random_square_mesh(50, 2);
    const auto p = vmm::IdentityOrdering{}.permutation(b.mesh);
    for (const CellId c : b.mesh.cells()) EXPECT_EQ(p.new_of(c), c);
}
}  // namespace
