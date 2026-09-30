// ============================================================================
// File: ut_RandomCountSource.cpp
// Description: Exactly N uniform points.
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


TEST(RandomCountSource, CountAndMargin) {
    vmm::Random r(8);
    const auto a = vmm::RandomCountSource(500, 0.01).generate(unit_square(), r);
    ASSERT_TRUE(a);
    EXPECT_EQ(a->size(), 500u);
    for (const auto& p : *a) EXPECT_GE(unit_square().distance_to_boundary(p), 0.01);
}

TEST(RandomCountSource, ImpossibleMarginFails) {
    vmm::Random r(8);
    EXPECT_EQ(vmm::RandomCountSource(3, 10).generate(unit_square(), r).error().code(), vmm::ErrorCode::SiteGenerationFailed);
}

}  // namespace
