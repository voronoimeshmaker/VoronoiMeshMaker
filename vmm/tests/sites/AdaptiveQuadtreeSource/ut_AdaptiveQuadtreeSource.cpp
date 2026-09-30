// ============================================================================
// File: ut_AdaptiveQuadtreeSource.cpp
// Description: Variable spacing by adaptive quadtree.
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


TEST(AdaptiveQuadtreeSource, DensityFollowsTheSpacing) {
    vmm::Random r(3);
    const vmm::SpacingField h = [](const Vec2& x) { return x[0] < 0.5 ? 0.02 : 0.1; };
    const auto a = vmm::AdaptiveQuadtreeSource(h, 0.01).generate(unit_square(), r);
    ASSERT_TRUE(a);
    const auto left = std::ranges::count_if(*a, [](const Vec2& p) { return p[0] < 0.5; });
    const auto right = static_cast<long>(a->size()) - left;
    EXPECT_GT(left, 10 * right);
    EXPECT_GT(min_distance(*a), 0.3 * 0.015);
}

TEST(AdaptiveQuadtreeSource, Errors) {
    vmm::Random r(3);
    const vmm::SpacingField bad = [](const Vec2&) { return -1.0; };
    EXPECT_EQ(vmm::AdaptiveQuadtreeSource(bad, 0.01).generate(unit_square(), r).error().code(), vmm::ErrorCode::InvalidSpacing);
    EXPECT_EQ(vmm::AdaptiveQuadtreeSource({}, 0.01).generate(unit_square(), r).error().code(), vmm::ErrorCode::InvalidSpacing);
    EXPECT_EQ(vmm::AdaptiveQuadtreeSource([](const Vec2&) { return 0.1; }, 0).generate(unit_square(), r).error().code(),
              vmm::ErrorCode::InvalidSpacing);
}

}  // namespace
