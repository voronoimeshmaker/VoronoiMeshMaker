// ============================================================================
// File: ut_SiteSourceRegistry2D.cpp
// Description: SiteSourceRegistry2D: built-in sources by name on a unit
//              square, parameter errors, user sources.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/registries.hpp>
#include <vmm/vmm.hpp>

namespace {

using vmm::ShapeParameters;
using vmm::SiteSourceRegistry2D;

/// Cells of a unit square meshed with the source `name`.
vmm::Result<std::size_t> cells(const SiteSourceRegistry2D& r, const std::string& name, const ShapeParameters& p) {
    vmm::MeshRequest2D request;
    const auto medium = *request.declaration.media().add("m");
    const auto region = *request.declaration.add_region("square", medium, vmm::Rectangle({0, 0}, {1, 1}));
    auto source = r.make(name, region, p);
    if (!source) return std::unexpected(source.error());
    request.sources.push_back(std::move(*source));
    request.sites.seed = 5;
    auto result = vmm::generate_mesh_2d(request);
    if (!result) return std::unexpected(result.error());
    return result->mesh.cell_count();
}

ShapeParameters numbers(std::map<std::string, std::vector<vmm::Real>> n) { return {std::move(n), {}}; }

TEST(SiteSourceRegistry2D, BuiltinSources) {
    const auto r = SiteSourceRegistry2D::with_builtin_sources();
    for (const char* name : {"uniform", "count", "grid", "hexagonal", "explicit"}) EXPECT_TRUE(r.contains(name));
    EXPECT_GT(*cells(r, "uniform", numbers({{"spacing", {0.2}}})), 10u);
    EXPECT_GT(*cells(r, "uniform", numbers({{"spacing", {0.2}}, {"min_distance", {0.5}}, {"margin", {0.3}}})), 10u);
    EXPECT_EQ(*cells(r, "count", numbers({{"count", {17}}})), 17u);
    EXPECT_EQ(*cells(r, "count", numbers({{"count", {9}}, {"margin", {0.1}}})), 9u);
    EXPECT_EQ(*cells(r, "grid", numbers({{"spacing", {0.25}}, {"origin", {0.125, 0.125}}, {"margin", {0.1}}})), 16u);
    EXPECT_GT(*cells(r, "grid", numbers({{"spacing", {0.25}}})), 0u);
    EXPECT_GT(*cells(r, "hexagonal", numbers({{"spacing", {0.2}}})), 10u);
    EXPECT_EQ(*cells(r, "explicit", numbers({{"xy", {0.25, 0.25, 0.75, 0.25, 0.25, 0.75, 0.75, 0.75}}})), 4u);
}

TEST(SiteSourceRegistry2D, ParameterErrors) {
    const auto r = SiteSourceRegistry2D::with_builtin_sources();
    const auto code = [&](const std::string& name, const ShapeParameters& p) {
        const auto c = cells(r, name, p);
        return c ? vmm::ErrorCode::InternalError : c.error().code();
    };
    const auto bad = vmm::ErrorCode::InvalidShapeParameter;
    EXPECT_EQ(code("uniform", {}), bad);
    EXPECT_EQ(code("uniform", numbers({{"spacing", {1, 2}}})), bad);
    EXPECT_EQ(code("uniform", ShapeParameters{{{"spacing", {0.2}}}, {{"min_distance", "x"}}}), bad);
    EXPECT_EQ(code("uniform", ShapeParameters{{{"spacing", {0.2}}}, {{"margin", "x"}}}), bad);
    EXPECT_EQ(code("count", {}), bad);
    EXPECT_EQ(code("count", numbers({{"count", {2.5}}})), bad);
    EXPECT_EQ(code("count", numbers({{"count", {0}}})), bad);
    EXPECT_EQ(code("count", ShapeParameters{{{"count", {3}}}, {{"margin", "wide"}}}), bad);
    EXPECT_EQ(code("grid", {}), bad);
    EXPECT_EQ(code("grid", numbers({{"spacing", {0.2}}, {"origin", {0}}})), bad);
    EXPECT_EQ(code("grid", ShapeParameters{{{"spacing", {0.2}}}, {{"margin", "x"}}}), bad);
    EXPECT_EQ(code("explicit", {}), bad);
    EXPECT_EQ(code("explicit", numbers({{"xy", {0.5, 0.5, 0.5}}})), bad);
    EXPECT_EQ(code("poisson", {}), vmm::ErrorCode::InvalidArgument);
}

TEST(SiteSourceRegistry2D, UserSources) {
    auto r = SiteSourceRegistry2D::with_builtin_sources();
    const auto centre = [](vmm::RegionId region, const ShapeParameters&) -> vmm::Result<vmm::RegionSites> {
        return vmm::sites_for(region, vmm::ExplicitSites({{0.5, 0.5}}));
    };
    ASSERT_TRUE(r.add("centre", centre));
    EXPECT_EQ(*cells(r, "centre", {}), 1u);
    EXPECT_EQ(r.add("centre", centre).error().code(), vmm::ErrorCode::DuplicateName);
    EXPECT_EQ(r.add("", centre).error().code(), vmm::ErrorCode::InvalidArgument);
    EXPECT_EQ(r.add("none", nullptr).error().code(), vmm::ErrorCode::InvalidArgument);
}

}  // namespace
