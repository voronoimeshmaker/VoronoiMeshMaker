// ============================================================================
// File: ut_InterfacePairs.cpp
// Description: Mirrored pairs across interfaces.
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


TEST(InterfacePairs, PairsAreMirroredAndInsideTheirRegions) {
    const auto p = *vmm::cgal_backend_2d().build_partition(vmm::test::square_with_hole());
    const auto pairs = vmm::InterfacePairs(0.05).generate(p);
    ASSERT_TRUE(pairs);
    EXPECT_GT(pairs->size(), 10u);
    for (const auto& pr : *pairs) {
        const Vec2 mid = 0.5 * (pr.left + pr.right);
        const bool on_vertical = std::abs(mid[0] - 0.7) < 1e-12;
        const bool on_horizontal = std::abs(mid[1] - 0.7) < 1e-12;
        EXPECT_TRUE(on_vertical || on_horizontal);
        EXPECT_NEAR(vmm::norm(pr.left - pr.right), 2 * 0.3 * 0.05, 1e-12);
        EXPECT_NE(pr.left_region, pr.right_region);
    }
    EXPECT_EQ(vmm::InterfacePairs(0).generate(p).error().code(), vmm::ErrorCode::InvalidSpacing);
    EXPECT_TRUE(vmm::InterfacePairs(10).generate(p)->empty());  // clearance longer than every segment
}

TEST(InterfacePairs, GenerateSitesWithPairsExcludesNearbySites) {
    const auto p = *vmm::cgal_backend_2d().build_partition(vmm::test::square_with_hole());
    std::vector<vmm::RegionSites> src{vmm::sites_for(RegionId{0}, vmm::UniformRandomSource(0.05)),
                                      vmm::sites_for(RegionId{1}, vmm::UniformRandomSource(0.05))};
    vmm::SiteGenerationOptions o;
    o.seed = 3;
    o.interface_pairs = vmm::InterfacePairs(0.05);
    const auto s = vmm::generate_sites(p, src, o);
    ASSERT_TRUE(s) << s.error().message();
    const auto pairs = *o.interface_pairs->generate(p);
    for (std::size_t i = 2 * pairs.size(); i < s->size(); ++i) {
        for (const auto& pr : pairs) {
            ASSERT_GE(vmm::norm(s->positions()[i] - pr.left), 0.75 * 0.05);
            ASSERT_GE(vmm::norm(s->positions()[i] - pr.right), 0.75 * 0.05);
        }
    }
}

TEST(InterfacePairs, ThinRegionStillGetsSites) {
    vmm::Declaration2D d;
    const auto m = *d.media().add("m");
    (void)d.add_region("wide", m, vmm::Rectangle(Vec2{0, 0}, Vec2{1, 1}));
    (void)d.add_region("thin", m, vmm::Rectangle(Vec2{0, 0.49}, Vec2{1, 0.51}));
    const auto p = *vmm::cgal_backend_2d().build_partition(d);
    std::vector<vmm::RegionSites> src{vmm::sites_for(RegionId{0}, vmm::UniformRandomSource(0.05)),
                                      vmm::sites_for(RegionId{1}, vmm::UniformRandomSource(0.005))};
    const auto s = vmm::generate_sites(p, src);
    ASSERT_TRUE(s) << s.error().message();
    EXPECT_GT(s->count(RegionId{1}), 50u);
}

}  // namespace
