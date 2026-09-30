// ============================================================================
// File: ut_CartesianGridSource.cpp
// Description: Square lattice.
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


TEST(CartesianGridSource, Lattice) {
    vmm::Random r(0);
    const auto a = vmm::CartesianGridSource(0.1, Vec2{0.05, 0.05}).generate(unit_square(), r);
    ASSERT_TRUE(a);
    EXPECT_EQ(a->size(), 100u);
    EXPECT_NEAR(min_distance(*a), 0.1, 1e-12);
    EXPECT_EQ(vmm::CartesianGridSource(-1).generate(unit_square(), r).error().code(), vmm::ErrorCode::InvalidSpacing);
}

}  // namespace
