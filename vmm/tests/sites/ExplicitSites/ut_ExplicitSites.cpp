// ============================================================================
// File: ut_ExplicitSites.cpp
// Description: User-given sites.
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


TEST(ExplicitSites, KeepsOnlyInsidePoints) {
    vmm::Random r(0);
    const auto a = vmm::ExplicitSites({{0.5, 0.5}, {2, 2}}).generate(unit_square(), r);
    ASSERT_TRUE(a);
    ASSERT_EQ(a->size(), 1u);
    EXPECT_EQ((*a)[0], (Vec2{0.5, 0.5}));
}

TEST(ExplicitSites, GenerateSitesEndToEnd) {
    const auto p = *vmm::cgal_backend_2d().build_partition(vmm::test::square_with_hole());
    std::vector<vmm::RegionSites> src{vmm::sites_for(RegionId{0}, vmm::ExplicitSites({{0.9, 0.9}})),
                                      vmm::sites_for(RegionId{1}, vmm::ExplicitSites({{0.1, 0.1}}))};
    const auto s = vmm::generate_sites(p, src);
    ASSERT_TRUE(s);
    EXPECT_EQ(s->size(), 2u);
    std::vector<vmm::RegionSites> bad{vmm::sites_for(RegionId{9}, vmm::ExplicitSites({}))};
    EXPECT_EQ(vmm::generate_sites(p, bad).error().code(), vmm::ErrorCode::InvalidArgument);
    std::vector<vmm::RegionSites> none{vmm::sites_for(RegionId{0}, vmm::ExplicitSites({{0.9, 0.9}}))};
    EXPECT_EQ(vmm::generate_sites(p, none).error().code(), vmm::ErrorCode::RegionWithoutSites);
    std::vector<vmm::RegionSites> failing{vmm::sites_for(RegionId{0}, vmm::UniformRandomSource(-1))};
    EXPECT_FALSE(vmm::generate_sites(p, failing));
}

}  // namespace
