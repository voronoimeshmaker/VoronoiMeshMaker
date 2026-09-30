// ============================================================================
// File: ut_InterfacePairs3D.cpp
// Description: InterfacePairs3D: mirrored pairs across a flat and a curved
//              interface, spacing, sites inside their regions, the exclusion
//              of the region-source sites, invalid parameters.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/cgal.hpp>
#include <vmm/domain/shapes3d.hpp>
#include <vmm/sites/sources3d.hpp>

namespace {

using vmm::RegionId;
using vmm::Vec3;

vmm::Partition3D halves() {
    vmm::Declaration3D d;
    const auto m = *d.media().add("m");
    EXPECT_TRUE(d.add_region("low", m, vmm::Cuboid({0, 0, 0}, {1, 1, 1})));
    EXPECT_TRUE(d.add_region("high", m, vmm::Cuboid({0, 0, 0.5}, {1, 1, 1})));
    return *vmm::cgal_backend_3d().build_partition(d);
}

TEST(InterfacePairs3D, MirroredAcrossAFlatInterface) {
    const auto p = halves();
    const vmm::InterfacePairs3D pairs(0.1);
    EXPECT_DOUBLE_EQ(pairs.spacing(), 0.1);
    const auto made = pairs.generate(p);
    ASSERT_TRUE(made) << made.error().message();
    EXPECT_GT(made->size(), 50u);
    for (const auto& pr : *made) {
        // Each site lies on the side of its own region (low below z = 0.5, high above).
        EXPECT_NE(pr.inside_region, pr.outside_region);
        for (const auto& [x, r] : {std::pair(pr.inside, pr.inside_region), std::pair(pr.outside, pr.outside_region)}) {
            EXPECT_EQ(x[2] < 0.5, r == RegionId::from_index(0));
        }
        EXPECT_NEAR(pr.inside[2] + pr.outside[2], 1.0, 1e-15);  // mirrored across z = 0.5
        EXPECT_DOUBLE_EQ(pr.inside[0], pr.outside[0]);
        EXPECT_DOUBLE_EQ(pr.inside[1], pr.outside[1]);
        const double offset = std::abs(0.5 - pr.inside[2]);
        EXPECT_GE(offset, 0.8 * 0.025 - 1e-15);
        EXPECT_LE(offset, 1.2 * 0.025 + 1e-15);
    }
    for (std::size_t i = 0; i < made->size(); ++i) {
        for (std::size_t j = 0; j < i; ++j) {
            const Vec3 d = (*made)[i].inside - (*made)[j].inside;
            EXPECT_GE(std::hypot(d[0], d[1]), 0.05 - 1e-12);
        }
    }
}

TEST(InterfacePairs3D, SitesGeneratedWithPairs) {
    const auto p = halves();
    std::vector<vmm::RegionSites3D> src{vmm::sites_for_3d(RegionId::from_index(0), vmm::UniformRandomSource3D(0.1)),
                                        vmm::sites_for_3d(RegionId::from_index(1), vmm::UniformRandomSource3D(0.1))};
    vmm::SiteGenerationOptions3D options;
    options.interface_pairs = vmm::InterfacePairs3D(0.1);
    const auto sites = vmm::generate_sites_3d(p, src, options);
    ASSERT_TRUE(sites) << sites.error().message();
    const auto pairs = *options.interface_pairs->generate(p);
    // The pairs come first; no region-source site is closer than 0.75 h to them.
    for (std::size_t k = 0; k < 2 * pairs.size(); ++k) {
        const Vec3& expected = k % 2 == 0 ? pairs[k / 2].inside : pairs[k / 2].outside;
        EXPECT_EQ(sites->positions()[k], expected);
    }
    for (std::size_t k = 2 * pairs.size(); k < sites->size(); ++k) {
        for (const auto& pr : pairs) {
            EXPECT_GE(vmm::norm(sites->positions()[k] - pr.inside), 0.075);
            EXPECT_GE(vmm::norm(sites->positions()[k] - pr.outside), 0.075);
        }
    }
}

TEST(InterfacePairs3D, CurvedInterfaceAndErrors) {
    vmm::Declaration3D d;
    const auto m = *d.media().add("m");
    ASSERT_TRUE(d.add_region("cube", m, vmm::Cuboid({0, 0, 0}, {1, 1, 1})));
    ASSERT_TRUE(d.add_region("ball", m, vmm::Sphere({0.5, 0.5, 0.5}, 0.3)));
    const auto p = *vmm::cgal_backend_3d().build_partition(d);
    const auto made = vmm::InterfacePairs3D(0.08).generate(p);
    ASSERT_TRUE(made);
    EXPECT_GT(made->size(), 20u);
    for (const auto& pr : *made) {
        EXPECT_EQ(pr.inside_region, RegionId::from_index(1));  // the ball is inside its interface
        EXPECT_LT(vmm::norm(pr.inside - Vec3{0.5, 0.5, 0.5}), 0.3);
        EXPECT_GT(vmm::norm(pr.outside - Vec3{0.5, 0.5, 0.5}), vmm::norm(pr.inside - Vec3{0.5, 0.5, 0.5}));
    }
    EXPECT_EQ(vmm::InterfacePairs3D(0).generate(p).error().code(), vmm::ErrorCode::InvalidSpacing);
    EXPECT_EQ(vmm::InterfacePairs3D(0.1, 0.6).generate(p).error().code(), vmm::ErrorCode::InvalidSpacing);
    vmm::SiteGenerationOptions3D bad;
    bad.interface_pairs = vmm::InterfacePairs3D(-1);
    EXPECT_EQ(vmm::generate_sites_3d(p, {}, bad).error().code(), vmm::ErrorCode::InvalidSpacing);
}

}  // namespace
