// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <limits>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/backend/cgal.hpp>
#include <vmm/cvt.hpp>
#include <vmm/domain/shapes.hpp>
#include <vmm/domain/shapes3d.hpp>

namespace {
template<class Partition, class Sites>
void invalid_controls(const Partition& partition, const Sites& sites) {
    for (double value : {0., -1., 1.1, std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
        vmm::CvtOptions options;
        options.relaxation = value;
        auto result = vmm::optimize_cvt(partition, sites, options);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().code(), vmm::ErrorCode::InvalidArgument);
    }
    for (double value : {0., -1., std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
        vmm::CvtOptions options;
        options.relative_tolerance = value;
        EXPECT_FALSE(vmm::optimize_cvt(partition, sites, options));
    }
    vmm::CvtOptions options;
    options.max_backtracks = 0;
    EXPECT_FALSE(vmm::optimize_cvt(partition, sites, options));
    EXPECT_FALSE(vmm::optimize_cvt(Partition{}, sites));
    EXPECT_FALSE(vmm::optimize_cvt(partition, Sites{}));
}

template<class Partition, class Sites>
void unresolved_step_stalls(const Partition& partition, const Sites& sites) {
    vmm::CvtOptions options;
    options.relaxation = std::numeric_limits<double>::epsilon();
    options.max_backtracks = 2;
    auto result = vmm::optimize_cvt(partition, sites, options); ASSERT_TRUE(result);
    EXPECT_TRUE(result->report.stalled);
    EXPECT_FALSE(result->report.converged);
    EXPECT_EQ(result->report.rejected_steps, options.max_backtracks);
    EXPECT_EQ(result->report.energy.size(), 1u);
    EXPECT_TRUE(result->report.relative_displacement.empty());
    EXPECT_GT(result->report.relative_residual, options.relative_tolerance);
    EXPECT_EQ(result->report.last_rejection, "no numerically resolved decrease in CVT energy");
}

TEST(CvtApi, PlanarControlsAndResidual) {
    vmm::Declaration2D declaration;
    auto medium = declaration.media().add("geometry"); ASSERT_TRUE(medium);
    ASSERT_TRUE(declaration.add_region("square", *medium, vmm::Rectangle({0, 0}, {1, 1})));
    auto partition = vmm::cgal_backend_2d().build_partition(declaration); ASSERT_TRUE(partition);
    vmm::SiteSet sites;
    sites.add({0.2, 0.3}, vmm::RegionId{0});
    invalid_controls(*partition, sites);
    unresolved_step_stalls(*partition, sites);
    vmm::CvtOptions options;
    options.max_iterations = 0;
    auto initial = vmm::optimize_cvt(*partition, sites, options); ASSERT_TRUE(initial);
    EXPECT_FALSE(initial->report.converged);
    EXPECT_FALSE(initial->report.stalled);
    EXPECT_TRUE(initial->report.relative_displacement.empty());
    EXPECT_NEAR(initial->report.relative_residual, std::sqrt(0.13 / 2), 1e-12);
    EXPECT_DOUBLE_EQ(vmm::cvt_energy(initial->mesh), initial->report.energy.front());
    options.max_iterations = 1;
    auto result = vmm::optimize_cvt(*partition, sites, options); ASSERT_TRUE(result);
    EXPECT_TRUE(result->report.converged);
    EXPECT_LE(result->report.relative_residual, options.relative_tolerance);
    sites.set_weights({1});
    EXPECT_FALSE(vmm::optimize_cvt(*partition, sites));
}

TEST(CvtApi, SpatialControlsAndResidual) {
    vmm::Declaration3D declaration;
    auto medium = declaration.media().add("geometry"); ASSERT_TRUE(medium);
    ASSERT_TRUE(declaration.add_region("cube", *medium, vmm::Cuboid({0, 0, 0}, {1, 1, 1})));
    auto partition = vmm::cgal_backend_3d().build_partition(declaration); ASSERT_TRUE(partition);
    vmm::SiteSet3D sites;
    sites.add({0.2, 0.3, 0.4}, vmm::RegionId{0});
    invalid_controls(*partition, sites);
    unresolved_step_stalls(*partition, sites);
    vmm::CvtOptions options;
    options.max_iterations = 0;
    auto initial = vmm::optimize_cvt(*partition, sites, options); ASSERT_TRUE(initial);
    EXPECT_FALSE(initial->report.converged);
    EXPECT_NEAR(initial->report.relative_residual, std::sqrt(0.14 / 3), 1e-12);
    EXPECT_DOUBLE_EQ(vmm::cvt_energy(initial->mesh), initial->report.energy.front());
    options.max_iterations = 1;
    options.relaxation = 0.5;
    auto limited = vmm::optimize_cvt(*partition, sites, options); ASSERT_TRUE(limited);
    EXPECT_FALSE(limited->report.converged);
    EXPECT_FALSE(limited->report.stalled);
    EXPECT_NEAR(limited->report.relative_residual, 0.5 * initial->report.relative_residual, 1e-12);
    options.relaxation = 1;
    auto result = vmm::optimize_cvt(*partition, sites, options); ASSERT_TRUE(result);
    EXPECT_TRUE(result->report.converged);
    EXPECT_LE(result->report.relative_residual, options.relative_tolerance);
}
} // namespace
