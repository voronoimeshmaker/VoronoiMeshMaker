// ============================================================================
// File: ut_ValidationReport.cpp
// Description: validate_partition() on pathological declarations: void,
//              emptied region, fragmented region, sliver; and a clean case.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "test_domains.hpp"
#include <vmm/backend/cgal.hpp>
#include <vmm/domain/validator.hpp>

namespace {

using vmm::ErrorCode;
using vmm::Rectangle;
using vmm::Vec2;

bool has(const std::vector<vmm::Error>& list, ErrorCode code) {
    return std::ranges::any_of(list, [&](const vmm::Error& e) { return e.code() == code; });
}

vmm::Partition2D build(const vmm::Declaration2D& d) {
    auto p = vmm::cgal_backend_2d().build_partition(d);
    EXPECT_TRUE(p) << (p ? "" : p.error().message());
    return p ? *p : vmm::Partition2D{};
}

TEST(ValidationReport, AddSortsBySeverity) {
    vmm::ValidationReport r;
    EXPECT_TRUE(r.ok());
    r.add(vmm::Error(ErrorCode::RegionFragmented, "", {}, vmm::Severity::Warning));
    EXPECT_TRUE(r.ok());
    r.add(vmm::Error(ErrorCode::DomainVoid));
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.errors().size(), 1u);
    EXPECT_EQ(r.warnings().size(), 1u);
}

TEST(ValidationReport, CleanDomainPasses) {
    const auto r = vmm::validate_partition(build(vmm::test::square_with_hole()));
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(r.warnings().empty());
    EXPECT_TRUE(vmm::validate_partition(build(vmm::test::anchor_a1())).ok());
}

/// Four bars enclosing an uncovered square: a void, unless a background exists.
vmm::Declaration2D frame(bool background) {
    vmm::Declaration2D d;
    const auto m = *d.media().add("m");
    (void)d.add_region("s", m, Rectangle(Vec2{0, 0}, Vec2{3, 1}));
    (void)d.add_region("w", m, Rectangle(Vec2{0, 0}, Vec2{1, 3}));
    (void)d.add_region("e", m, Rectangle(Vec2{2, 0}, Vec2{3, 3}));
    (void)d.add_region("n", m, Rectangle(Vec2{0, 2}, Vec2{3, 3}));
    if (background) (void)d.set_background("fill", m);
    return d;
}

TEST(ValidationReport, VoidAndBackground) {
    const auto p = build(frame(false));
    ASSERT_EQ(p.voids().size(), 1u);
    EXPECT_DOUBLE_EQ(p.voids()[0].area(), 1.0);
    EXPECT_TRUE(has(vmm::validate_partition(p).errors(), ErrorCode::DomainVoid));
    const auto q = build(frame(true));
    EXPECT_TRUE(q.voids().empty());
    EXPECT_DOUBLE_EQ(q.region_area(vmm::RegionId::from_index(4)), 1.0);
    EXPECT_TRUE(vmm::validate_partition(q).ok());
}

TEST(ValidationReport, EmptiedRegion) {
    vmm::Declaration2D d;
    const auto m = *d.media().add("m");
    (void)d.add_region("small", m, Rectangle(Vec2{0.2, 0.2}, Vec2{0.4, 0.4}));
    (void)d.add_region("big", m, Rectangle(Vec2{0, 0}, Vec2{1, 1}));
    const auto r = vmm::validate_partition(build(d));
    EXPECT_TRUE(has(r.errors(), ErrorCode::RegionEmptied));
}

TEST(ValidationReport, FragmentedRegionIsAWarning) {
    vmm::Declaration2D d;
    const auto m = *d.media().add("m");
    (void)d.add_region("plain", m, Rectangle(Vec2{0, 0}, Vec2{2, 1}));
    (void)d.add_region("river", m, Rectangle(Vec2{0.9, 0}, Vec2{1.1, 1}));
    const auto p = build(d);
    EXPECT_EQ(p.components(vmm::RegionId::from_index(0)).size(), 2u);
    const auto r = vmm::validate_partition(p);
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(has(r.warnings(), ErrorCode::RegionFragmented));
}

TEST(ValidationReport, SliverWarningOrError) {
    vmm::Declaration2D d;
    const auto m = *d.media().add("m");
    (void)d.add_region("under", m, Rectangle(Vec2{0, 0}, Vec2{1, 1}));
    (void)d.add_region("over", m, Rectangle(Vec2{0, 0}, Vec2{1, 1 - 1e-7}));
    const auto p = build(d);
    EXPECT_TRUE(has(vmm::validate_partition(p).warnings(), ErrorCode::Sliver));
    vmm::ValidationOptions strict;
    strict.slivers_are_errors = true;
    strict.local_spacing = 0.01;
    EXPECT_TRUE(has(vmm::validate_partition(p, strict).errors(), ErrorCode::Sliver));
}

}  // namespace
