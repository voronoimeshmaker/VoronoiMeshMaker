// ============================================================================
// File: ut_Permutation.cpp
// Description: Permutation: construction, inverse, errors; renumber() checks.
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

TEST(Permutation, FromOrderAndInverse) {
    const auto p = vmm::Permutation::from_order({CellId{2}, CellId{0}, CellId{1}});
    ASSERT_TRUE(p);
    EXPECT_EQ(p->size(), 3u);
    EXPECT_EQ(p->new_of(CellId{2}), CellId{0});
    EXPECT_EQ(p->old_of(CellId{0}), CellId{2});
    const auto q = p->inverse();
    EXPECT_EQ(q.new_of(CellId{0}), CellId{2});
    EXPECT_EQ(q.old_of_new().size(), 3u);
    EXPECT_EQ(p->new_of_old().size(), 3u);
    EXPECT_EQ(vmm::Permutation::identity(4).new_of(CellId{3}), CellId{3});
}

TEST(Permutation, RejectsNonBijections) {
    EXPECT_FALSE(vmm::Permutation::from_order({CellId{0}, CellId{0}}));
    EXPECT_FALSE(vmm::Permutation::from_order({CellId{0}, CellId{5}}));
    EXPECT_FALSE(vmm::Permutation::from_order({CellId{0}, CellId::invalid()}));
}

TEST(Permutation, RenumberRoundTrip) {
    const auto b = vmm::test::random_square_mesh(300, 1);
    const auto p = *vmm::Permutation::from_order([&] {
        std::vector<CellId> o;
        for (const CellId c : b.mesh.cells()) o.push_back(c);
        std::ranges::reverse(o);
        return o;
    }());
    expect_valid_renumbering(b.mesh, p);
    const auto there = *vmm::renumber(b.mesh, p);
    const auto back = *vmm::renumber(there, p.inverse());
    EXPECT_EQ(std::vector<CellId>(back.owners().begin(), back.owners().end()),
              std::vector<CellId>(b.mesh.owners().begin(), b.mesh.owners().end()));
    EXPECT_EQ(vmm::renumber(b.mesh, vmm::Permutation::identity(3)).error().code(), vmm::ErrorCode::InvalidArgument);
}

TEST(Permutation, BandwidthAndProfile) {
    vmm::Csr<CellId> adj;
    const CellId r0[] = {CellId{2}};
    const CellId r1[] = {CellId{2}};
    const CellId r2[] = {CellId{0}, CellId{1}};
    adj.push_row(r0);
    adj.push_row(r1);
    adj.push_row(r2);
    EXPECT_EQ(vmm::bandwidth(adj), 2u);
    EXPECT_EQ(vmm::profile(adj), 2u);
}
}  // namespace
