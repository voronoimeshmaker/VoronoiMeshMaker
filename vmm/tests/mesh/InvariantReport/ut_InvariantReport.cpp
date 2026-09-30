// ============================================================================
// File: ut_InvariantReport.cpp
// Description: check_invariants() passes on a correct mesh and detects each
//              kind of defect (wrong area, open cell, flipped face, missing
//              interface).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "simple_meshes.hpp"
#include <vmm/mesh/invariants.hpp>

namespace {

vmm::InvariantReference two_squares_reference() {
    vmm::InvariantReference ref;
    ref.length_scale = std::sqrt(5.0);
    ref.total_measure = 2;
    ref.region_measure = {2};
    ref.boundary_measure = 6;
    return ref;
}

TEST(InvariantReport, CorrectMeshPasses) {
    const auto m = *vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    const auto ref = two_squares_reference();
    const auto r = vmm::check_invariants(m, ref);
    EXPECT_TRUE(r.passed(ref)) << r.first_problem;
    EXPECT_EQ(r.max_closure, 0.0);
    EXPECT_TRUE(r.adjacency_symmetric);
}

TEST(InvariantReport, WrongReferenceIsDetected) {
    const auto m = *vmm::Mesh2D::from_data(vmm::test::two_squares_data());
    auto ref = two_squares_reference();
    ref.total_measure = 2.1;
    ref.region_measure = {2.1};
    ref.boundary_measure = 7;
    const auto r = vmm::check_invariants(m, ref);
    EXPECT_FALSE(r.passed(ref));
    EXPECT_GT(r.total_relative_error, 0.04);
    EXPECT_GT(r.boundary_relative_error, 0.1);
}

TEST(InvariantReport, OpenCellAndFlippedFace) {
    auto d = vmm::test::two_squares_data();
    std::swap(d.face_vertices.values[0], d.face_vertices.values[1]);  // internal face flipped
    const auto m = *vmm::Mesh2D::from_data(d);
    const auto ref = two_squares_reference();
    const auto r = vmm::check_invariants(m, ref);
    EXPECT_FALSE(r.passed(ref));
    EXPECT_GT(r.bad_faces, 0u);
    EXPECT_GT(r.max_closure, 1.0);
    EXPECT_FALSE(r.first_problem.empty());
}

TEST(InvariantReport, InterfacesAndCellMeasures) {
    auto d = vmm::test::two_squares_data();
    d.regions.push_back({"s", vmm::MediumId{0}});
    d.cell_region[1] = vmm::RegionId{1};
    const auto m = *vmm::Mesh2D::from_data(d);
    auto ref = two_squares_reference();
    ref.region_measure = {1, 1};
    ref.interface_measure[{0, 1}] = 1;
    const double cells[] = {1, 1};
    ref.cell_measure = cells;
    auto r = vmm::check_invariants(m, ref);
    EXPECT_TRUE(r.passed(ref)) << r.first_problem;
    EXPECT_EQ(r.interface_faces, 1u);
    // An interface the partition does not have is non-conforming.
    ref.interface_measure.clear();
    ref.interface_measure[{0, 2}] = 1;
    r = vmm::check_invariants(m, ref);
    EXPECT_EQ(r.nonconforming_faces, 1u);
    EXPECT_FALSE(r.passed(ref));
}

/// Two unit cubes along x sharing the face x = 1 (cells 0 and 1), patch "wall".
vmm::MeshData<3> two_cubes_data() {
    vmm::MeshData<3> d;
    for (int k = 0; k < 12; ++k) {
        d.points.push_back({static_cast<double>(k % 3), static_cast<double>((k / 3) % 2), static_cast<double>(k / 6)});
    }
    const auto id = [](int x, int y, int z) { return vmm::VertexId{static_cast<std::uint32_t>(x + 3 * y + 6 * z)}; };
    const auto face = [&](std::vector<vmm::VertexId> v, std::uint32_t owner) {
        d.face_vertices.push_row(v);
        d.owner.push_back(vmm::CellId{owner});
    };
    face({id(1, 0, 0), id(1, 1, 0), id(1, 1, 1), id(1, 0, 1)}, 0);
    d.neighbour.push_back(vmm::CellId{1});
    face({id(0, 0, 0), id(0, 0, 1), id(0, 1, 1), id(0, 1, 0)}, 0);
    face({id(2, 0, 0), id(2, 1, 0), id(2, 1, 1), id(2, 0, 1)}, 1);
    for (std::uint32_t c = 0; c < 2; ++c) {
        const int x0 = static_cast<int>(c);
        const int x1 = x0 + 1;
        face({id(x0, 0, 0), id(x1, 0, 0), id(x1, 0, 1), id(x0, 0, 1)}, c);
        face({id(x0, 1, 0), id(x0, 1, 1), id(x1, 1, 1), id(x1, 1, 0)}, c);
        face({id(x0, 0, 0), id(x0, 1, 0), id(x1, 1, 0), id(x1, 0, 0)}, c);
        face({id(x0, 0, 1), id(x1, 0, 1), id(x1, 1, 1), id(x0, 1, 1)}, c);
    }
    d.patches = {{"wall", 1, 10}};
    d.sites = {{0.5, 0.5, 0.5}, {1.5, 0.5, 0.5}};
    d.cell_region = {vmm::RegionId{0}, vmm::RegionId{0}};
    d.cell_input_site = {vmm::SiteId{0}, vmm::SiteId{1}};
    d.regions = {{"r", vmm::MediumId{0}}};
    d.media = {"m"};
    return d;
}

vmm::InvariantReference two_cubes_reference() {
    vmm::InvariantReference ref;
    ref.length_scale = std::sqrt(6.0);
    ref.total_measure = 2;
    ref.region_measure = {2};
    ref.boundary_measure = 10;
    return ref;
}

TEST(InvariantReport, ThreeDimensionalMeshes) {
    const auto ok = *vmm::Mesh3D::from_data(two_cubes_data());
    auto ref = two_cubes_reference();
    auto r = vmm::check_invariants(ok, ref);
    EXPECT_TRUE(r.passed(ref)) << r.first_problem;
    ref.total_measure = 2.1;
    ref.boundary_measure = 11;
    EXPECT_FALSE(vmm::check_invariants(ok, ref).passed(ref));
    auto flipped = two_cubes_data();
    std::swap(flipped.face_vertices.values[1], flipped.face_vertices.values[3]);  // internal face reversed
    ref = two_cubes_reference();
    r = vmm::check_invariants(*vmm::Mesh3D::from_data(flipped), ref);
    EXPECT_GT(r.bad_faces, 0u);
    EXPECT_FALSE(r.passed(ref));
    auto two = two_cubes_data();
    two.regions.push_back({"s", vmm::MediumId{0}});
    two.cell_region[1] = vmm::RegionId{1};
    const auto m = *vmm::Mesh3D::from_data(two);
    ref.region_measure = {1, 1};
    ref.interface_measure[{0, 1}] = 1;
    r = vmm::check_invariants(m, ref);
    EXPECT_TRUE(r.passed(ref)) << r.first_problem;
    ref.interface_measure.clear();
    ref.interface_measure[{0, 2}] = 1;
    r = vmm::check_invariants(m, ref);
    EXPECT_EQ(r.nonconforming_faces, 1u);
}

TEST(InvariantReport, EachConditionFailsOnItsOwn) {
    const vmm::InvariantReference ref;
    vmm::InvariantReport ok;
    ok.closure_tolerance = 1;
    ok.adjacency_symmetric = true;
    ASSERT_TRUE(ok.passed(ref));
    const std::vector<void (*)(vmm::InvariantReport&)> breakers{
        [](auto& r) { r.total_relative_error = 1; },      [](auto& r) { r.max_region_relative_error = 1; },
        [](auto& r) { r.max_cell_measure_error = 1; },    [](auto& r) { r.boundary_relative_error = 1; },
        [](auto& r) { r.max_interface_relative_error = 1; }, [](auto& r) { r.max_closure = 2; },
        [](auto& r) { r.bad_faces = 1; },                 [](auto& r) { r.nonpositive_cells = 1; },
        [](auto& r) { r.nonconforming_faces = 1; },       [](auto& r) { r.adjacency_symmetric = false; }};
    for (std::size_t k = 0; k < breakers.size(); ++k) {
        auto r = ok;
        breakers[k](r);
        EXPECT_FALSE(r.passed(ref)) << k;
    }
}

TEST(InvariantReport, NonOrthogonalityIsReportedNotChecked) {
    const vmm::InvariantReference ref;
    vmm::InvariantReport r;
    r.closure_tolerance = 1;
    r.adjacency_symmetric = true;
    r.max_nonortho_internal = 1e-6;  // rounding of the stored vertices (DEC-032)
    EXPECT_TRUE(r.passed(ref));
}

}  // namespace
