// SPDX-License-Identifier: GPL-3.0-or-later
/** @file ut_Connectivity.cpp
 * @brief Physical-face connectivity and boundary-coordinate regression tests.
 * @ingroup voronoi2d_tests
 */

//==============================================================================
// C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

//==============================================================================
// GTest
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
// VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/ErrorHandling/ErrorManager.h>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Ordering/VoronoiVolumeOrdering2D.hpp>

namespace {
using namespace vmm::vd2d;
using namespace vmm::s2d;
using vmm::error::VMMException;

auto square() {
    return vmm::b2d::make_boundary(vmm::b2d::Rectangle(Point2{0, 0}, 1, 1));
}

auto grid() {
    const auto boundary = square();
    ClippedVoronoiBuildOptions2D options;
    options.allow_parallel_cell_build = false;
    return ClippedVoronoiBuilder2D::build(
        make_sites(boundary, CartesianGridCount2D{4, 4}), boundary, options);
}

using EdgeSignature = std::array<Real, 8>;
auto signature(const ClippedVoronoiDiagram2D& diagram) {
    std::vector<EdgeSignature> values;
    for (const auto& site : diagram.sites) {
        for (const auto& edge : diagram.cell(site.id).edges) {
            values.push_back({edge.a.x, edge.a.y, edge.b.x, edge.b.y,
                edge.length, static_cast<Real>(edge.neighbour_site_id.value),
                edge.boundary_condition_geometry().point().x,
                edge.boundary_condition_geometry().point().y});
        }
    }
    return values;
}

struct LogScope {
    vmm::error::ErrorConfig original{*vmm::error::Config::get()};
    LogScope() {
        static_cast<void>(vmm::error::ErrorManager::flush());
        auto config = original;
        config.thread_buffer_cap = 10000;
        config.min_severity = vmm::error::Severity::Warning;
        vmm::error::Config::set(config);
    }
    ~LogScope() {
        static_cast<void>(vmm::error::ErrorManager::flush());
        vmm::error::Config::set(original);
    }
};
}

TEST(FaceLengthLimits, PreservesPositiveFacesAndSupportsExplicitQualityThreshold) {
    EXPECT_EQ(VoronoiFaceLengthLimits2D{}.minimum_length(16, 4), Real{0});
    const VoronoiFaceLengthLimits2D limits{Real{0.1}, Real{0.2}};
    EXPECT_EQ(limits.minimum_length(16, 4), Real{0.4});
    EXPECT_THROW(static_cast<void>(limits.minimum_length(0, 4)), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(limits.minimum_length(1, 0)), std::invalid_argument);
    const VoronoiFaceLengthLimits2D invalid{Real{-1}, Real{0}};
    EXPECT_THROW(static_cast<void>(invalid.minimum_length(1, 1)), std::invalid_argument);
    const VoronoiFaceLengthLimits2D nan{0, std::numeric_limits<Real>::quiet_NaN()};
    EXPECT_THROW(static_cast<void>(nan.minimum_length(1, 1)), std::invalid_argument);
}

TEST(FaceConnectivity, DoesNotUseDelaunayCandidatesOrPointContacts) {
    auto diagram = grid();
    const auto before = signature(diagram);
    for (auto& cell : diagram.cells) {
        cell.neighbor_ids.clear();
        for (auto& edge : cell.edges) edge.neighbour_site_id = SiteId{999};
    }
    VoronoiFaceConnectivity2D::rebuild(diagram.cells, diagram.sites, 0);
    EXPECT_EQ(signature(diagram), before);
    std::size_t directed_faces = 0;
    for (const auto& cell : diagram.cells) {
        EXPECT_TRUE(cell.delaunay_neighbours().empty());
        for (const auto id : cell.face_neighbours()) {
            auto neighbours = diagram.cell(id).face_neighbours();
            EXPECT_EQ(std::ranges::count(neighbours, cell.site_id), 1);
            ++directed_faces;
        }
    }
    EXPECT_EQ(directed_faces, 48U);
}

TEST(FaceConnectivity, FailingLengthGateLeavesAllEdgesUnchanged) {
    auto diagram = grid();
    const auto before = signature(diagram);
    EXPECT_THROW(VoronoiFaceConnectivity2D::rebuild(
        diagram.cells, diagram.sites, Real{0.3}), std::runtime_error);
    EXPECT_EQ(signature(diagram), before);
}

TEST(FaceConnectivity, RejectsMissingReciprocalFaceTransactionally) {
    auto diagram = grid();
    auto& cell = diagram.cells.front();
    const auto face = std::ranges::find_if(cell.edges, [](const auto& edge) {
        return edge.is_boundary_edge;
    });
    ASSERT_NE(face, cell.edges.end());
    face->is_boundary_edge = false;
    const auto before = signature(diagram);
    EXPECT_THROW(VoronoiFaceConnectivity2D::rebuild(
        diagram.cells, diagram.sites, 0), std::runtime_error);
    EXPECT_EQ(signature(diagram), before);
}

TEST(FaceConnectivity, RejectsInvalidIdentityAndInconsistentEndpoints) {
    auto diagram = grid();
    diagram.cells.front().edges.front().a.x += Real{0.01};
    EXPECT_THROW(VoronoiFaceConnectivity2D::rebuild(
        diagram.cells, diagram.sites, 0), std::invalid_argument);
    diagram = grid();
    diagram.cells.back().site_id = diagram.cells.front().site_id;
    EXPECT_THROW(VoronoiFaceConnectivity2D::rebuild(
        diagram.cells, diagram.sites, 0), std::invalid_argument);
}

TEST(FaceConnectivity, WorksAfterPhysicalVolumePermutation) {
    auto diagram = grid();
    const auto before = signature(diagram);
    const auto permutation = renumber_volumes(diagram, HilbertVolumeOrdering2D{});
    ASSERT_EQ(permutation.new_to_old.size(), diagram.cells.size());
    VoronoiFaceConnectivity2D::rebuild(diagram.cells, diagram.sites, 0);
    EXPECT_EQ(signature(diagram), before);
    static_cast<void>(renumber_volumes(diagram, [&](const auto&) {
        return permutation.old_to_new;
    }));
    EXPECT_EQ(signature(diagram), before);
}

TEST(FaceConnectivity, RejectsNonManifoldSharedSegments) {
    SiteSet sites;
    sites.add({Real{0.25}, Real{0.5}});
    sites.add({Real{0.75}, Real{0.5}});
    sites.add({Real{0.75}, Real{0.6}});
    std::vector<VoronoiCell2D> cells(3);
    cells[0].polygon = {{0, 0}, {Real{0.5}, 0}, {Real{0.5}, 1}, {0, 1}};
    cells[1].polygon = {{Real{0.5}, 0}, {1, 0}, {1, 1}, {Real{0.5}, 1}};
    cells[2].polygon = cells[1].polygon;
    for (std::size_t c = 0; c < cells.size(); ++c) {
        auto& cell = cells[c];
        cell.site_id = sites[c].id;
        for (std::size_t e = 0; e < cell.polygon.size(); ++e) {
            const auto a = cell.polygon[e];
            const auto b = cell.polygon[(e + 1U) % cell.polygon.size()];
            const bool internal = (c == 0 && e == 1) || (c != 0 && e == 3);
            cell.edges.emplace_back(a, b, Point2{}, Real{0}, !internal);
        }
    }
    EXPECT_THROW(VoronoiFaceConnectivity2D::rebuild(cells, sites, 0), std::runtime_error);
    for (const auto& cell : cells) {
        for (const auto& edge : cell.edges) EXPECT_EQ(edge.length, Real{0});
    }
}

TEST(FaceConnectivity, RejectsZeroLengthPhysicalFaces) {
    auto diagram = grid();
    auto& cell = diagram.cells.front();
    cell.polygon.insert(cell.polygon.begin(), cell.polygon.front());
    const auto point = cell.polygon.front();
    cell.edges.insert(cell.edges.begin(), VoronoiCellEdge2D{point, point, point, 0, true});
    EXPECT_THROW(VoronoiFaceConnectivity2D::rebuild(diagram.cells, diagram.sites, 0),
                 std::runtime_error);
}

TEST(BoundaryCoordinates, ExposesConstantReferenceWithoutClamping) {
    const auto geometry = BoundaryConditionPoint2D::from_support_line(
        {Real{0.5}, Real{0.5}}, {1, 0}, {2, 0}, {0, 0}, {2, 0});
    static_assert(std::is_same_v<decltype(geometry.point()), const Point2&>);
    ASSERT_TRUE(geometry.valid());
    EXPECT_FALSE(geometry.point_inside_local_face());
    EXPECT_EQ(geometry.point().x, Real{0.5});
    EXPECT_EQ(geometry.point().y, Real{0});
    EXPECT_EQ(geometry.face_parameter(), Real{-0.5});
    EXPECT_EQ(geometry.perpendicular_distance(), Real{0.5});
    EXPECT_EQ(geometry.outward_normal().x, Real{0});
    EXPECT_EQ(geometry.outward_normal().y, Real{-1});
    EXPECT_TRUE(geometry.normal_matches_face());
    EXPECT_EQ(geometry.support_line_parameter(), Real{0.25});
}

TEST(BoundaryCoordinates, RejectsInvalidSupportAndMismatchedFace) {
    EXPECT_FALSE(BoundaryConditionPoint2D::from_support_line(
        {0, 1}, {0, 0}, {1, 0}, {0, 0}, {0, 0}).valid());
    EXPECT_FALSE(BoundaryConditionPoint2D::from_support_line(
        {0, 1}, {0, 1}, {1, 1}, {0, 0}, {1, 0}).normal_matches_face());
}

TEST(BoundaryCoordinates, TraversesReferencesAndConstantPointersAfterRenumbering) {
    auto diagram = grid();
    const auto verify = [&] {
        auto points = diagram.boundary_condition_points();
        auto pointers = diagram.boundary_condition_point_pointers();
        static_assert(std::is_same_v<std::ranges::range_reference_t<decltype(points)>,
                                     const Point2&>);
        static_assert(std::is_same_v<std::ranges::range_value_t<decltype(pointers)>,
                                     const Point2*>);
        auto current = pointers.begin();
        std::size_t count = 0;
        for (const auto& point : points) {
            ASSERT_NE(current, pointers.end());
            EXPECT_EQ(*current, &point);
            ++current;
            ++count;
        }
        EXPECT_EQ(current, pointers.end());
        EXPECT_EQ(count, 16U);
    };
    verify();
    static_cast<void>(renumber_volumes(diagram, HilbertVolumeOrdering2D{}));
    verify();
    const ClippedVoronoiDiagram2D empty;
    EXPECT_TRUE(empty.boundary_condition_points().empty());
}

TEST(BoundaryCoordinates, WarnsForExteriorProjectionsWithoutMovingGeometry) {
    const LogScope log;
    const auto boundary = square();
    const auto sites = make_sites(boundary, UniformRandom2D{200, 8675309U});
    const auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);
    std::size_t outside = 0;
    for (const auto& cell : diagram.cells) {
        for (std::size_t i = 0; i < cell.edges.size(); ++i) {
            const auto& edge = cell.edges[i];
            EXPECT_EQ(edge.a.x, cell.polygon[i].x);
            EXPECT_EQ(edge.a.y, cell.polygon[i].y);
            if (!edge.is_boundary_edge) continue;
            const auto& geometry = edge.boundary_condition_geometry();
            EXPECT_TRUE(geometry.valid());
            if (!geometry.point_inside_local_face()) ++outside;
        }
    }
    const auto warnings = vmm::error::ErrorManager::flush();
    EXPECT_GT(outside, 0U);
    ASSERT_EQ(warnings.size(), outside);
    for (const auto& warning : warnings) {
        EXPECT_EQ(warning.severity, vmm::error::Severity::Warning);
        EXPECT_NE(warning.message.find("volume="), std::string::npos);
        EXPECT_NE(warning.message.find("site="), std::string::npos);
        EXPECT_NE(warning.message.find("face="), std::string::npos);
    }
}

TEST(BoundaryCoordinates, RegularGridDoesNotEmitExteriorWarnings) {
    const LogScope log;
    static_cast<void>(grid());
    EXPECT_TRUE(vmm::error::ErrorManager::flush().empty());
}

TEST(DiagramValidation, RejectsNonFiniteSitesBeforeCGAL) {
    const auto boundary = square();
    SiteSet sites;
    sites.add({Real{0.2}, Real{0.2}});
    sites.add({Real{0.8}, Real{0.2}});
    sites.add({std::numeric_limits<Real>::quiet_NaN(), Real{0.8}});
    EXPECT_THROW(static_cast<void>(ClippedVoronoiBuilder2D::build(sites, boundary)),
                 std::invalid_argument);
}

TEST(DiagramValidation, RejectsZeroBoundaryNormalDistance) {
    SiteSet sites;
    sites.add({0, Real{0.25}});
    sites.add({Real{0.6}, Real{0.3}});
    sites.add({Real{0.4}, Real{0.8}});
    EXPECT_THROW(static_cast<void>(ClippedVoronoiBuilder2D::build(sites, square())),
                 std::runtime_error);
}

TEST(DiagramValidation, RejectsHolesEvenWhenLegacyFlagIsFalse) {
    const auto boundary = vmm::b2d::make_boundary(
        vmm::b2d::Ring2D(Point2{0, 0}, Real{0.25}, Real{2}, 32));
    const auto sites = make_sites(square(), CartesianGridCount2D{3, 3});
    ClippedVoronoiBuildOptions2D options;
    options.cells.reject_boundaries_with_holes = false;
    EXPECT_THROW(static_cast<void>(ClippedVoronoiBuilder2D::build(sites, boundary, options)),
                 VMMException);
}

TEST(DiagramValidation, RejectsUnsupportedAndMalformedRings) {
    auto boundary = square();
    boundary.points[1] = {Real{0.25}, Real{0.5}};
    EXPECT_THROW(VoronoiCellBuilder2D::validate_domain(boundary), VMMException);
    boundary = square();
    std::reverse(boundary.points.begin(), boundary.points.end());
    EXPECT_THROW(VoronoiCellBuilder2D::validate_domain(boundary), VMMException);
    boundary = square();
    boundary.points[0].x = std::numeric_limits<Real>::infinity();
    EXPECT_THROW(VoronoiCellBuilder2D::validate_domain(boundary), VMMException);
    EXPECT_THROW(VoronoiCellBuilder2D::validate_domain({}), VMMException);
}

TEST(DiagramValidation, PartitionsNonRectangularConvexDomain) {
    vmm::b2d::Boundary2DData boundary;
    const std::array<Point2, 3> vertices{{{0, 0}, {1, 0}, {0, 1}}};
    boundary.append_ring(vertices, vmm::b2d::LoopKind::Outer, vmm::b2d::RegionId{0});
    SiteSet sites;
    sites.add({Real{0.1}, Real{0.1}});
    sites.add({Real{0.7}, Real{0.1}});
    sites.add({Real{0.1}, Real{0.7}});
    const auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);
    EXPECT_NEAR(diagram.total_volume_area(), Real{0.5}, Real{1e-14});
    EXPECT_EQ(diagram.cells.size(), 3U);
    for (const auto& cell : diagram.cells) {
        EXPECT_GT(cell.signed_area(), Real{0});
        for (const auto& edge : cell.edges) {
            if (edge.is_boundary_edge) EXPECT_TRUE(edge.boundary_condition_geometry().valid());
        }
    }
}
