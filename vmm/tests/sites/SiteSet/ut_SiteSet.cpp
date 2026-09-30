// ============================================================================
// File: ut_SiteSet.cpp
// Description: SiteSet and validate_sites (pathological sites).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "test_domains.hpp"
#include <vmm/backend/cgal.hpp>
#include <vmm/core/random.hpp>
#include <vmm/sites/sources.hpp>

namespace {

using vmm::PolygonWithHoles2;
using vmm::RegionId;
using vmm::Vec2;

[[maybe_unused]] PolygonWithHoles2 unit_square() { return PolygonWithHoles2({{0, 0}, {1, 0}, {1, 1}, {0, 1}}); }

[[maybe_unused]] double min_distance(const std::vector<Vec2>& p) {
    double d = 1e300;
    for (std::size_t i = 0; i < p.size(); ++i)
        for (std::size_t j = i + 1; j < p.size(); ++j) d = std::min(d, vmm::norm(p[i] - p[j]));
    return d;
}


TEST(SiteSet, AddAppendCount) {
    vmm::SiteSet s;
    EXPECT_TRUE(s.empty());
    const auto id = s.add(Vec2{0.1, 0.1}, RegionId{0});
    EXPECT_EQ(id.value, 0u);
    const std::vector<Vec2> more{{0.2, 0.2}, {0.3, 0.3}};
    s.append(more, RegionId{1});
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.count(RegionId{1}), 2u);
    EXPECT_EQ(s.positions()[2], (Vec2{0.3, 0.3}));
    EXPECT_EQ(s.regions()[0], RegionId{0});
    EXPECT_TRUE(s.weights().empty());
    s.set_weights({1, 2, 3});
    EXPECT_EQ(s.weights().size(), 3u);
}

TEST(SiteSet, ValidationCatchesBadSites) {
    const auto p = *vmm::cgal_backend_2d().build_partition(vmm::test::square_with_hole());
    const RegionId b{0};
    const RegionId a{1};
    auto code = [&](const vmm::SiteSet& s) { const auto r = vmm::validate_sites(p, s); return r ? vmm::ErrorCode{} : r.error().code(); };
    vmm::SiteSet ok;
    ok.add(Vec2{0.2, 0.2}, a);
    ok.add(Vec2{0.9, 0.9}, b);
    EXPECT_TRUE(vmm::validate_sites(p, ok));
    vmm::SiteSet in_hole = ok;
    in_hole.add(Vec2{0.5, 0.5}, a);
    EXPECT_EQ(code(in_hole), vmm::ErrorCode::SiteOutsideRegion);
    vmm::SiteSet wrong_region = ok;
    wrong_region.add(Vec2{0.1, 0.1}, b);
    EXPECT_EQ(code(wrong_region), vmm::ErrorCode::SiteOutsideRegion);
    vmm::SiteSet on_interface = ok;
    on_interface.add(Vec2{0.7, 0.2}, a);  // exactly on the interface
    EXPECT_EQ(code(on_interface), vmm::ErrorCode::SiteOutsideRegion);
    vmm::SiteSet glued = ok;
    glued.add(Vec2{0.7 - 1e-9, 0.2}, a);  // glued to the interface but inside: accepted
    EXPECT_TRUE(vmm::validate_sites(p, glued));
    vmm::SiteSet dup = ok;
    dup.add(Vec2{0.2, 0.2}, a);
    EXPECT_EQ(code(dup), vmm::ErrorCode::DuplicateSite);
    vmm::SiteSet nan = ok;
    nan.add(Vec2{std::nan(""), 0.2}, a);
    EXPECT_EQ(code(nan), vmm::ErrorCode::SiteOutsideRegion);
    vmm::SiteSet bad_region = ok;
    bad_region.add(Vec2{0.2, 0.3}, RegionId{7});
    EXPECT_EQ(code(bad_region), vmm::ErrorCode::SiteOutsideRegion);
    vmm::SiteSet missing;
    missing.add(Vec2{0.2, 0.2}, a);
    EXPECT_EQ(code(missing), vmm::ErrorCode::RegionWithoutSites);
    EXPECT_EQ(vmm::validate_sites(vmm::Partition2D{}, ok).error().code(), vmm::ErrorCode::InvalidLengthScale);
}

}  // namespace
