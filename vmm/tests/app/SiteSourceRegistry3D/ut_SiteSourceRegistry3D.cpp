// ============================================================================
// File: ut_SiteSourceRegistry3D.cpp
// Description: SiteSourceRegistry3D: built-in sources by name on a unit
//              cube, parameter errors, user sources.
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
using vmm::SiteSourceRegistry3D;

/// Cells of a unit cube meshed with the source `name`.
vmm::Result<std::size_t> cells(const SiteSourceRegistry3D& r, const std::string& name, const ShapeParameters& p) {
    vmm::MeshRequest3D request;
    const auto medium = *request.declaration.media().add("m");
    const auto region = *request.declaration.add_region("cube", medium, vmm::Cuboid({0, 0, 0}, {1, 1, 1}));
    auto source = r.make(name, region, p);
    if (!source) return std::unexpected(source.error());
    request.sources.push_back(std::move(*source));
    request.sites.seed = 5;
    auto result = vmm::generate_mesh_3d(request);
    if (!result) return std::unexpected(result.error());
    return result->mesh.cell_count();
}

ShapeParameters numbers(std::map<std::string, std::vector<vmm::Real>> n) { return {std::move(n), {}}; }

TEST(SiteSourceRegistry3D, BuiltinSources) {
    const auto r = SiteSourceRegistry3D::with_builtin_sources();
    for (const char* name : {"uniform", "count", "grid", "explicit"}) EXPECT_TRUE(r.contains(name));
    EXPECT_GT(*cells(r, "uniform", numbers({{"spacing", {0.3}}})), 10u);
    EXPECT_GT(*cells(r, "uniform", numbers({{"spacing", {0.3}}, {"min_distance", {0.5}}, {"margin", {0.3}}})), 10u);
    EXPECT_EQ(*cells(r, "count", numbers({{"count", {20}}, {"margin", {0.05}}})), 20u);
    EXPECT_EQ(*cells(r, "grid", numbers({{"spacing", {0.5}}, {"origin", {0.25, 0.25, 0.25}}, {"margin", {0.1}}})),
              8u);
    EXPECT_EQ(*cells(r, "explicit", numbers({{"xyz", {0.25, 0.5, 0.5, 0.75, 0.5, 0.5}}})), 2u);
}

TEST(SiteSourceRegistry3D, ParameterErrors) {
    const auto r = SiteSourceRegistry3D::with_builtin_sources();
    const auto code = [&](const std::string& name, const ShapeParameters& p) {
        const auto c = cells(r, name, p);
        return c ? vmm::ErrorCode::InternalError : c.error().code();
    };
    const auto bad = vmm::ErrorCode::InvalidShapeParameter;
    EXPECT_EQ(code("uniform", {}), bad);
    EXPECT_EQ(code("count", {}), bad);
    EXPECT_EQ(code("count", ShapeParameters{{{"count", {3}}}, {{"margin", "x"}}}), bad);
    EXPECT_EQ(code("grid", {}), bad);
    EXPECT_EQ(code("grid", numbers({{"spacing", {0.5}}, {"origin", {0, 0}}})), bad);
    EXPECT_EQ(code("grid", ShapeParameters{{{"spacing", {0.5}}}, {{"margin", "x"}}}), bad);
    EXPECT_EQ(code("explicit", numbers({{"xyz", {0.5, 0.5}}})), bad);
    EXPECT_EQ(code("explicit", {}), bad);
    EXPECT_EQ(code("hexagonal", {}), vmm::ErrorCode::InvalidArgument);
}

TEST(SiteSourceRegistry3D, UserSources) {
    auto r = SiteSourceRegistry3D::with_builtin_sources();
    const auto centre = [](vmm::RegionId region, const ShapeParameters&) -> vmm::Result<vmm::RegionSites3D> {
        return vmm::sites_for_3d(region, vmm::ExplicitSites3D({{0.5, 0.5, 0.5}}));
    };
    ASSERT_TRUE(r.add("centre", centre));
    EXPECT_EQ(*cells(r, "centre", {}), 1u);
    EXPECT_EQ(r.add("centre", centre).error().code(), vmm::ErrorCode::DuplicateName);
    EXPECT_EQ(r.add("", centre).error().code(), vmm::ErrorCode::InvalidArgument);
}

}  // namespace
