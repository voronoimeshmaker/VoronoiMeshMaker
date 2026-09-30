// ============================================================================
// File: ut_SiteSet3D.cpp
// Description: SiteSet3D, generate_sites_3d and validate_sites_3d.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <limits>
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

using vmm::ErrorCode;
using vmm::RegionId;
using vmm::SiteSet3D;
using vmm::Vec3;

vmm::Partition3D unit_cube() {
    vmm::Declaration3D d;
    const auto m = *d.media().add("m");
    EXPECT_TRUE(d.add_region("cube", m, vmm::Cuboid({0, 0, 0}, {1, 1, 1})));
    return *vmm::cgal_backend_3d().build_partition(d);
}

TEST(SiteSet3D, AddAppendCount) {
    SiteSet3D s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.add(Vec3{0.5, 0.5, 0.5}, RegionId::from_index(0)).value, 0u);
    const std::vector<Vec3> more{{0.1, 0.1, 0.1}, {0.9, 0.9, 0.9}};
    s.append(more, RegionId::from_index(1));
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.count(RegionId::from_index(1)), 2u);
    EXPECT_EQ(s.positions()[2], (Vec3{0.9, 0.9, 0.9}));
    EXPECT_EQ(s.regions()[0], RegionId::from_index(0));
}

TEST(SiteSet3D, GenerateIsDeterministic) {
    const auto p = unit_cube();
    const std::vector<vmm::RegionSites3D> src{vmm::sites_for_3d(RegionId::from_index(0), vmm::UniformRandomSource3D(0.2))};
    const auto a = vmm::generate_sites_3d(p, src, {5});
    const auto b = vmm::generate_sites_3d(p, src, {5});
    const auto c = vmm::generate_sites_3d(p, src, {6});
    ASSERT_TRUE(a) << a.error().message();
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    EXPECT_GT(a->size(), 50u);
    EXPECT_TRUE(std::ranges::equal(a->positions(), b->positions()));
    EXPECT_FALSE(std::ranges::equal(a->positions(), c->positions()));
}

TEST(SiteSet3D, GenerateErrors) {
    const auto p = unit_cube();
    const std::vector<vmm::RegionSites3D> unknown{vmm::sites_for_3d(RegionId::from_index(3), vmm::UniformRandomSource3D(0.2))};
    EXPECT_EQ(vmm::generate_sites_3d(p, unknown).error().code(), ErrorCode::InvalidArgument);
    const std::vector<vmm::RegionSites3D> bad{vmm::sites_for_3d(RegionId::from_index(0), vmm::UniformRandomSource3D(-1))};
    EXPECT_EQ(vmm::generate_sites_3d(p, bad).error().code(), ErrorCode::InvalidSpacing);
    EXPECT_EQ(vmm::generate_sites_3d(p, {}).error().code(), ErrorCode::RegionWithoutSites);
}

TEST(SiteSet3D, ValidateSites) {
    const auto p = unit_cube();
    const RegionId r = RegionId::from_index(0);
    SiteSet3D ok;
    ok.add(Vec3{0.5, 0.5, 0.5}, r);
    EXPECT_TRUE(vmm::validate_sites_3d(p, ok));
    SiteSet3D outside = ok;
    outside.add(Vec3{2, 0.5, 0.5}, r);
    EXPECT_EQ(vmm::validate_sites_3d(p, outside).error().code(), ErrorCode::SiteOutsideRegion);
    SiteSet3D on_surface = ok;
    on_surface.add(Vec3{0, 0.5, 0.5}, r);
    EXPECT_EQ(vmm::validate_sites_3d(p, on_surface).error().code(), ErrorCode::SiteOutsideRegion);
    SiteSet3D nan = ok;
    nan.add(Vec3{std::numeric_limits<double>::quiet_NaN(), 0.5, 0.5}, r);
    EXPECT_EQ(vmm::validate_sites_3d(p, nan).error().code(), ErrorCode::SiteOutsideRegion);
    SiteSet3D region = ok;
    region.add(Vec3{0.2, 0.2, 0.2}, RegionId::from_index(4));
    EXPECT_EQ(vmm::validate_sites_3d(p, region).error().code(), ErrorCode::SiteOutsideRegion);
    SiteSet3D twice = ok;
    twice.add(Vec3{0.5, 0.5, 0.5}, r);
    EXPECT_EQ(vmm::validate_sites_3d(p, twice).error().code(), ErrorCode::DuplicateSite);
    EXPECT_EQ(vmm::validate_sites_3d(p, SiteSet3D{}).error().code(), ErrorCode::RegionWithoutSites);
    EXPECT_EQ(vmm::validate_sites_3d(vmm::Partition3D{}, ok).error().code(), ErrorCode::InvalidLengthScale);
}

}  // namespace
