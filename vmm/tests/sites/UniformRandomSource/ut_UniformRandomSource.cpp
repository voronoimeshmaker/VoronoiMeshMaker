// ============================================================================
// File: ut_UniformRandomSource.cpp
// Description: Dart throwing with constant spacing.
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


TEST(UniformRandomSource, SpacingMarginDeterminism) {
    vmm::Random r1(5);
    vmm::Random r2(5);
    const auto a = vmm::UniformRandomSource(0.05).generate(unit_square(), r1);
    const auto b = vmm::UniformRandomSource(0.05).generate(unit_square(), r2);
    ASSERT_TRUE(a && b);
    EXPECT_EQ(*a, *b);
    EXPECT_GT(a->size(), 150u);
    EXPECT_GE(min_distance(*a), 0.6 * 0.05);
    for (const auto& p : *a) EXPECT_GE(unit_square().distance_to_boundary(p), 0.25 * 0.05);
}

TEST(UniformRandomSource, OptionsAndErrors) {
    vmm::Random r(1);
    auto s = vmm::UniformRandomSource(0.1);
    s.min_distance_fraction(0.3).boundary_margin_fraction(0).failure_limit(50);
    const auto a = s.generate(unit_square(), r);
    ASSERT_TRUE(a);
    EXPECT_FALSE(a->empty());
    EXPECT_GE(min_distance(*a), 0.03);
    EXPECT_EQ(vmm::UniformRandomSource(0).generate(unit_square(), r).error().code(), vmm::ErrorCode::InvalidSpacing);
    // A zero minimum distance would never terminate: rejected.
    auto unbounded = vmm::UniformRandomSource(0.1);
    unbounded.min_distance_fraction(0);
    EXPECT_EQ(unbounded.generate(unit_square(), r).error().code(), vmm::ErrorCode::InvalidSpacing);
}

}  // namespace
