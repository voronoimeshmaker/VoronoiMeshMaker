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
