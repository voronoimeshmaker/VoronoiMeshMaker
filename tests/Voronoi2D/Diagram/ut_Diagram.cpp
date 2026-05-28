#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <CGAL/number_utils.h>
#include <gtest/gtest.h>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/ClippedVoronoiDiagram2DTransform.hpp>
#include <VoronoiMeshMaker/ClippedVoronoi2DExport.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp>
#include <VoronoiMeshMaker/VoronoiBandwidth2D.hpp>
#include <VoronoiMeshMaker/VoronoiEdgeLengthDiagnostics2D.hpp>
#include <VoronoiMeshMaker/VoronoiGenerationTimer2D.hpp>
#include <VoronoiMeshMaker/VoronoiVolumeOrdering2D.hpp>

using namespace vmm::b2d;
using namespace vmm::s2d;
using namespace vmm::vd2d;

namespace {

SiteSet make_five_sites_inside_square() {
    SiteSet sites;
    sites.add(Point2{1.0, 1.0}, RegionId{4});
    sites.add(Point2{0.5, 1.0}, RegionId{4});
    sites.add(Point2{1.5, 1.0}, RegionId{4});
    sites.add(Point2{1.0, 0.5}, RegionId{4});
    sites.add(Point2{1.0, 1.5}, RegionId{4});
    return sites;
}

bool contains_site_id(const std::vector<SiteId>& ids, SiteId id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

double test_point_squared_distance(Point2 a, Point2 b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

bool point_on_boundary(Point2 p, double width, double height, double tol) {
    return std::abs(p.x) <= tol ||
           std::abs(p.x - width) <= tol ||
           std::abs(p.y) <= tol ||
           std::abs(p.y - height) <= tol;
}

bool polygon_has_boundary_edge(const std::vector<Point2>& polygon,
                               double width,
                               double height,
                               double tol) {
    if (polygon.size() < 2U) {
        return false;
    }
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const auto& a = polygon[i];
        const auto& b = polygon[(i + 1U) % polygon.size()];
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        if ((dx * dx + dy * dy) <= tol * tol) {
            continue;
        }
        if (!point_on_boundary(a, width, height, tol) ||
            !point_on_boundary(b, width, height, tol)) {
            continue;
        }
        if ((std::abs(a.x) <= tol && std::abs(b.x) <= tol) ||
            (std::abs(a.x - width) <= tol && std::abs(b.x - width) <= tol) ||
            (std::abs(a.y) <= tol && std::abs(b.y) <= tol) ||
            (std::abs(a.y - height) <= tol && std::abs(b.y - height) <= tol)) {
            return true;
        }
    }
    return false;
}

bool has_lattice_distance(double dist2, double h, double tol) {
    const std::array<double, 4> expected{
        h * h,
        3.0 * h * h,
        4.0 * h * h,
        7.0 * h * h
    };
    return std::any_of(
        expected.begin(),
        expected.end(),
        [dist2, tol](double value) {
            return std::abs(dist2 - value) <= tol;
        });
}

std::string polygon_to_string(const std::vector<Point2>& polygon) {
    std::ostringstream out;
    out.precision(17);
    for (const auto& p : polygon) {
        out << " (" << p.x << "," << p.y << ")";
    }
    return out.str();
}

} // namespace

TEST(ClippedVoronoiDiagram2D, BuildsAllVolumesAndClassifiesBoundaryCells) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));
    const auto sites = make_five_sites_inside_square();

    auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    ASSERT_EQ(diagram.cell_count(), sites.size());
    EXPECT_EQ(diagram.volume_count(), sites.size());
    EXPECT_EQ(diagram.boundary_volume_count(), 4U);
    EXPECT_EQ(diagram.internal_volume_count(), 1U);
    EXPECT_NEAR(diagram.total_volume_area(), 4.0, 1.0e-12);

    const auto& central = diagram.cell(SiteId{0});
    EXPECT_EQ(central.volume_id, 0U);
    EXPECT_FALSE(central.is_boundary_cell);
    EXPECT_NEAR(central.area(), 0.25, 1.0e-12);

    auto mutable_all = diagram.all_volumes();
    static_assert(std::is_same_v<decltype(*mutable_all.begin()), VoronoiCell2D&>);
    const auto& const_diagram = diagram;
    auto const_all = const_diagram.all_volumes();
    static_assert(std::is_same_v<decltype(*const_all.begin()), const VoronoiCell2D&>);

    std::size_t iterated_all = 0;
    for (const auto& volume : diagram.all_volumes()) {
        EXPECT_EQ(volume.volume_id, iterated_all);
        ++iterated_all;
    }
    EXPECT_EQ(iterated_all, diagram.volume_count());

    std::size_t iterated_all_ptrs = 0;
    for (auto* volume : diagram.all_volume_pointers()) {
        ASSERT_NE(volume, nullptr);
        EXPECT_EQ(volume->volume_id, iterated_all_ptrs);
        volume->volume_id = volume->volume_id;
        ++iterated_all_ptrs;
    }
    EXPECT_EQ(iterated_all_ptrs, diagram.volume_count());

    std::size_t iterated_const_all_ptrs = 0;
    for (const auto* volume : const_diagram.all_volume_pointers()) {
        ASSERT_NE(volume, nullptr);
        EXPECT_EQ(volume->volume_id, iterated_const_all_ptrs);
        ++iterated_const_all_ptrs;
    }
    EXPECT_EQ(iterated_const_all_ptrs, diagram.volume_count());

    std::size_t iterated_internal = 0;
    for (const auto& volume : diagram.internal_volumes()) {
        EXPECT_FALSE(volume.is_boundary_cell);
        ++iterated_internal;
    }
    EXPECT_EQ(iterated_internal, 1U);

    std::size_t iterated_internal_ptrs = 0;
    for (const auto* volume : diagram.internal_volume_pointers()) {
        ASSERT_NE(volume, nullptr);
        EXPECT_FALSE(volume->is_boundary_cell);
        ++iterated_internal_ptrs;
    }
    EXPECT_EQ(iterated_internal_ptrs, 1U);

    std::size_t iterated_boundary = 0;
    for (const auto& volume : diagram.boundary_volumes()) {
        EXPECT_TRUE(volume.is_boundary_cell);
        ++iterated_boundary;
    }
    EXPECT_EQ(iterated_boundary, 4U);

    std::size_t iterated_boundary_ptrs = 0;
    for (const auto* volume : diagram.boundary_volume_pointers()) {
        ASSERT_NE(volume, nullptr);
        EXPECT_TRUE(volume->is_boundary_cell);
        ++iterated_boundary_ptrs;
    }
    EXPECT_EQ(iterated_boundary_ptrs, 4U);
}

TEST(ClippedVoronoiDiagram2D, InternalAndBoundaryVolumeSetsAreDisjoint) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));
    const auto sites = make_five_sites_inside_square();
    const auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    std::vector<unsigned char> internal_volume_seen(
        diagram.volume_count(),
        0U);
    for (const auto& volume : diagram.internal_volumes()) {
        ASSERT_LT(volume.volume_id, internal_volume_seen.size());
        internal_volume_seen[volume.volume_id] = 1U;
    }

    for (const auto& volume : diagram.boundary_volumes()) {
        ASSERT_LT(volume.volume_id, internal_volume_seen.size());
        EXPECT_EQ(internal_volume_seen[volume.volume_id], 0U)
            << "volume_id=" << volume.volume_id
            << " belongs to both internal and boundary volume sets";
    }
}

TEST(ClippedVoronoiDiagram2D, HexagonalMeshHasConsistentGeometryAndNeighbors) {
    constexpr double width = 10.0;
    constexpr double height = 4.0;
    constexpr std::size_t count_on_longest_side = 10U;
    constexpr double h = width / static_cast<double>(count_on_longest_side);
    constexpr double sqrt3 = 1.73205080756887729352744634150587236;
    constexpr double row_spacing = sqrt3 * 0.5 * h;
    constexpr double expected_internal_area = sqrt3 * 0.5 * h * h;
    constexpr double expected_internal_perimeter = 2.0 * sqrt3 * h;

    const auto boundary = make_boundary(
        Rectangle(Point2{0.0, 0.0}, width, height));
    const auto sites = make_sites(boundary, HexagonalGrid2D{count_on_longest_side});

    ASSERT_EQ(sites.size(), 38U);
    EXPECT_TRUE(sites.ids_are_sequential());
    EXPECT_TRUE(validate_sites(sites, boundary));

    double min_y = sites[0].point.y;
    double max_y = sites[0].point.y;
    for (const auto& site : sites) {
        EXPECT_GE(site.point.x, 0.0);
        EXPECT_LE(site.point.x, width);
        EXPECT_GT(site.point.y, 0.0);
        EXPECT_LT(site.point.y, height);
        min_y = std::min(min_y, site.point.y);
        max_y = std::max(max_y, site.point.y);
    }
    EXPECT_NEAR(min_y, (height - 3.0 * row_spacing) * 0.5, 1.0e-12);
    EXPECT_NEAR(min_y, height - max_y, 1.0e-12);

    const auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);
    ASSERT_EQ(diagram.volume_count(), sites.size());
    EXPECT_EQ(diagram.internal_volume_count() + diagram.boundary_volume_count(),
              diagram.volume_count());
    EXPECT_NEAR(diagram.total_volume_area(), width * height, 1.0e-10);

    std::vector<unsigned char> seen_volume(diagram.volume_count(), 0U);
    std::size_t checked_regular_internal = 0U;
    for (const auto& cell : diagram.all_volumes()) {
        ASSERT_LT(cell.volume_id, seen_volume.size());
        EXPECT_EQ(seen_volume[cell.volume_id], 0U);
        seen_volume[cell.volume_id] = 1U;

        ASSERT_GE(cell.site_id.value, 0);
        const auto site_index = static_cast<std::size_t>(cell.site_id.value);
        ASSERT_LT(site_index, sites.size());
        EXPECT_EQ(sites[site_index].id, cell.site_id);
        EXPECT_GT(cell.area(), 0.0);
        EXPECT_TRUE(std::isfinite(cell.area()));
        EXPECT_TRUE(std::isfinite(cell.perimeter()));
        EXPECT_GE(cell.polygon.size(), 3U);

        EXPECT_EQ(
            cell.is_boundary_cell,
            polygon_has_boundary_edge(cell.polygon, width, height, 1.0e-10))
            << "site_id=" << cell.site_id.value
            << " polygon=" << polygon_to_string(cell.polygon);

        std::vector<SiteId> unique_neighbors = cell.neighbor_ids;
        std::sort(
            unique_neighbors.begin(),
            unique_neighbors.end(),
            [](SiteId a, SiteId b) { return a.value < b.value; });
        EXPECT_EQ(
            std::unique(unique_neighbors.begin(), unique_neighbors.end()),
            unique_neighbors.end())
            << "duplicated neighbor in site_id=" << cell.site_id.value;

        for (const auto neighbor_id : cell.neighbor_ids) {
            ASSERT_GE(neighbor_id.value, 0);
            const auto neighbor_index =
                static_cast<std::size_t>(neighbor_id.value);
            ASSERT_LT(neighbor_index, sites.size());
            EXPECT_NE(neighbor_id, cell.site_id);

            const auto& neighbor_cell = diagram.cell(neighbor_id);
            EXPECT_TRUE(contains_site_id(neighbor_cell.neighbor_ids, cell.site_id))
                << "non reciprocal neighbor relation: "
                << cell.site_id.value << " -> " << neighbor_id.value;

            const double dist2 = test_point_squared_distance(
                sites[site_index].point,
                sites[neighbor_index].point);
            EXPECT_TRUE(has_lattice_distance(dist2, h, 1.0e-10))
                << "unexpected neighbor distance: site_id="
                << cell.site_id.value
                << " neighbor_id=" << neighbor_id.value
                << " dist2=" << dist2;
        }

        if (cell.is_boundary_cell) {
            EXPECT_TRUE(cell.boundary_projection_valid)
                << "boundary site_id=" << cell.site_id.value;
            const auto& site = sites[site_index].point;
            EXPECT_NEAR(
                test_point_squared_distance(
                    site,
                    cell.boundary_projection_point),
                cell.boundary_projection_distance *
                    cell.boundary_projection_distance,
                1.0e-10)
                << "boundary site_id=" << cell.site_id.value;
            EXPECT_NEAR(
                cell.boundary_projection_distance,
                distance_to_boundary(site, boundary),
                1.0e-12)
                << "boundary site_id=" << cell.site_id.value;
        } else {
            EXPECT_FALSE(cell.boundary_projection_valid);
            bool has_regular_hex_stencil = cell.neighbor_ids.size() == 6U;
            for (const auto neighbor_id : cell.neighbor_ids) {
                const auto neighbor_index =
                    static_cast<std::size_t>(neighbor_id.value);
                const double dist2 = test_point_squared_distance(
                    sites[site_index].point,
                    sites[neighbor_index].point);
                has_regular_hex_stencil =
                    has_regular_hex_stencil && std::abs(dist2 - h * h) <= 1.0e-10;
            }
            if (has_regular_hex_stencil) {
                ++checked_regular_internal;
                EXPECT_NEAR(cell.area(), expected_internal_area, 1.0e-10)
                    << "internal site_id=" << cell.site_id.value;
                EXPECT_NEAR(cell.perimeter(), expected_internal_perimeter, 1.0e-10)
                    << "internal site_id=" << cell.site_id.value;

                const auto centroid = cell.centroid();
                EXPECT_NEAR(centroid.x, sites[site_index].point.x, 1.0e-10);
                EXPECT_NEAR(centroid.y, sites[site_index].point.y, 1.0e-10);
            }
        }
    }

    EXPECT_GT(checked_regular_internal, 0U);
    EXPECT_TRUE(std::all_of(
        seen_volume.begin(),
        seen_volume.end(),
        [](unsigned char value) { return value == 1U; }));
}

TEST(ClippedVoronoiDiagram2D, StoresBoundaryNormalProjectionForBoundaryVolumes) {
    const auto boundary = make_boundary(
        Rectangle(Point2{0.0, 0.0}, 4.0, 2.0));
    const auto sites = make_sites(boundary, CartesianGridCount2D{4, 2});
    const auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    ASSERT_GT(diagram.boundary_volume_count(), 0U);
    for (const auto& cell : diagram.boundary_volumes()) {
        const auto site_index = static_cast<std::size_t>(cell.site_id.value);
        const auto& site = sites[site_index].point;
        const auto projection = project_to_boundary(site, boundary);

        EXPECT_TRUE(cell.boundary_projection_valid);
        EXPECT_NEAR(cell.boundary_projection_point.x,
                    projection.point.x,
                    1.0e-12);
        EXPECT_NEAR(cell.boundary_projection_point.y,
                    projection.point.y,
                    1.0e-12);
        EXPECT_NEAR(cell.boundary_projection_distance,
                    projection.distance,
                    1.0e-12);
        EXPECT_NEAR(
            test_point_squared_distance(site, cell.boundary_projection_point),
            cell.boundary_projection_distance *
                cell.boundary_projection_distance,
            1.0e-12);
    }
}

TEST(ClippedVoronoiDiagram2DTransform, RotatesClippedDiagramInPlace) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));
    const auto sites = make_five_sites_inside_square();
    auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    const double area_before = diagram.total_volume_area();
    const auto polygon_point_before = diagram.cell(SiteId{1}).polygon.front();

    rotate_in_place(
        diagram,
        ::vmm::constants::kPi / 2.0,
        Point2{1.0, 1.0});

    EXPECT_NEAR(diagram.total_volume_area(), area_before, 1.0e-12);
    EXPECT_EQ(diagram.volume_count(), 5U);
    EXPECT_EQ(diagram.internal_volume_count(), 1U);
    EXPECT_EQ(diagram.boundary_volume_count(), 4U);

    EXPECT_NEAR(diagram.sites[0].point.x, 1.0, 1.0e-12);
    EXPECT_NEAR(diagram.sites[0].point.y, 1.0, 1.0e-12);
    EXPECT_NEAR(diagram.sites[1].point.x, 1.0, 1.0e-12);
    EXPECT_NEAR(diagram.sites[1].point.y, 0.5, 1.0e-12);

    const auto polygon_point_after = diagram.cell(SiteId{1}).polygon.front();
    EXPECT_NEAR(polygon_point_after.x,
                2.0 - polygon_point_before.y,
                1.0e-12);
    EXPECT_NEAR(polygon_point_after.y,
                polygon_point_before.x,
                1.0e-12);

    bool found_rotated_site_in_delaunay = false;
    for (auto vertex = diagram.delaunay.finite_vertices_begin();
         vertex != diagram.delaunay.finite_vertices_end();
         ++vertex) {
        if (vertex->info().value == 1) {
            found_rotated_site_in_delaunay = true;
            EXPECT_NEAR(CGAL::to_double(vertex->point().x()), 1.0, 1.0e-12);
            EXPECT_NEAR(CGAL::to_double(vertex->point().y()), 0.5, 1.0e-12);
        }
    }
    EXPECT_TRUE(found_rotated_site_in_delaunay);
}

TEST(ClippedVoronoiDiagram2D, ComputesBandwidthFromVolumeAdjacency) {
    ClippedVoronoiDiagram2D diagram;
    diagram.sites.add(Point2{0.0, 0.0}, RegionId{1});
    diagram.sites.add(Point2{1.0, 0.0}, RegionId{1});
    diagram.sites.add(Point2{2.0, 0.0}, RegionId{1});
    diagram.sites.add(Point2{3.0, 0.0}, RegionId{1});

    diagram.cells = {
        VoronoiCell2D{SiteId{0}, {SiteId{2}}, {}, false},
        VoronoiCell2D{SiteId{1}, {SiteId{3}}, {}, false},
        VoronoiCell2D{SiteId{2}, {SiteId{0}}, {}, false},
        VoronoiCell2D{SiteId{3}, {SiteId{1}}, {}, false}
    };
    diagram.rebuild_indices();

    const auto bandwidth = compute_voronoi_bandwidth(diagram);

    EXPECT_EQ(bandwidth.max_volume_id_distance, 2U);
    EXPECT_EQ(bandwidth.matrix_bandwidth, 3U);
    EXPECT_EQ(bandwidth.adjacency_count, 2U);
}

TEST(ClippedVoronoiDiagram2D, BandwidthFollowsCurrentVolumeNumbering) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));
    const auto sites = make_five_sites_inside_square();
    auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    const auto original_bandwidth = compute_voronoi_bandwidth(diagram);
    ASSERT_GT(original_bandwidth.matrix_bandwidth, 0U);

    renumber_volumes(diagram, HilbertVolumeOrdering2D{8});
    const auto hilbert_bandwidth = compute_voronoi_bandwidth(diagram);

    EXPECT_GT(hilbert_bandwidth.matrix_bandwidth, 0U);
    EXPECT_EQ(hilbert_bandwidth.adjacency_count,
              original_bandwidth.adjacency_count);
}

TEST(ClippedVoronoiDiagram2D, ReportsShortEdgesFromCellEdgeMetadata) {
    ClippedVoronoiDiagram2D diagram;
    diagram.sites.add(Point2{0.0, 0.0}, RegionId{1});
    diagram.sites.add(Point2{1.0, 0.0}, RegionId{1});

    VoronoiCell2D first;
    first.site_id = SiteId{0};
    first.edges.push_back(VoronoiCellEdge2D{
        Point2{0.0, 0.0},
        Point2{0.05, 0.0},
        Point2{0.025, 0.0},
        0.05,
        true
    });
    first.edges.push_back(VoronoiCellEdge2D{
        Point2{0.0, 0.0},
        Point2{1.0, 0.0},
        Point2{0.5, 0.0},
        1.0,
        false
    });

    VoronoiCell2D second;
    second.site_id = SiteId{1};
    second.edges.push_back(VoronoiCellEdge2D{
        Point2{0.0, 0.0},
        Point2{0.2, 0.0},
        Point2{0.1, 0.0},
        0.2,
        false
    });

    diagram.cells = {first, second};
    diagram.rebuild_indices();

    const auto report = diagnose_voronoi_edge_lengths(diagram, 0.1);

    EXPECT_TRUE(report.has_short_edges());
    EXPECT_EQ(report.edge_count, 3U);
    EXPECT_EQ(report.short_edge_count, 1U);
    ASSERT_EQ(report.short_edges.size(), 1U);
    EXPECT_EQ(report.short_edges.front().volume_id, 0U);
    EXPECT_EQ(report.short_edges.front().site_id, SiteId{0});
    EXPECT_EQ(report.short_edges.front().edge_index, 0U);
    EXPECT_TRUE(report.short_edges.front().is_boundary_edge);
    EXPECT_NEAR(report.short_edges.front().length, 0.05, 1.0e-14);
    EXPECT_NEAR(report.minimum_edge_length, 0.05, 1.0e-14);
}

TEST(ClippedVoronoiDiagram2D, ReportsShortEdgesFromPolygonFallback) {
    ClippedVoronoiDiagram2D diagram;
    diagram.sites.add(Point2{0.0, 0.0}, RegionId{1});

    VoronoiCell2D cell;
    cell.site_id = SiteId{0};
    cell.polygon = {
        Point2{0.0, 0.0},
        Point2{0.05, 0.0},
        Point2{0.05, 1.0},
        Point2{0.0, 1.0}
    };

    diagram.cells = {cell};
    diagram.rebuild_indices();

    const auto report = diagnose_voronoi_edge_lengths(diagram, 0.1);

    EXPECT_TRUE(report.has_short_edges());
    EXPECT_EQ(report.edge_count, 4U);
    EXPECT_EQ(report.short_edge_count, 2U);
    EXPECT_NEAR(report.minimum_edge_length, 0.05, 1.0e-14);
}

TEST(ClippedVoronoiDiagram2D, MeasuresGenerationPipelineTimingsInLibrary) {
    const auto result = VoronoiGenerationTimer2D::measure(
        [] {
            return make_boundary(Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));
        },
        [](const Boundary2DData& boundary) {
            const SiteValidationOptions validation{
                .min_distance_to_boundary = 0.25,
                .min_distance_between_sites = 0.2,
                .require_sequential_ids = true
            };
            return make_sites(
                boundary,
                CartesianGrid2D{0.5, 0.5},
                validation,
                RegionId{1});
        },
        [](const Boundary2DData& boundary, const SiteSet& sites) {
            return ClippedVoronoiBuilder2D::build(sites, boundary);
        });

    EXPECT_GT(result.boundary.vertex_count(), 0U);
    EXPECT_FALSE(result.sites.empty());
    EXPECT_FALSE(result.diagram.empty());
    EXPECT_GE(result.timings.boundary2d_ms, 0.0);
    EXPECT_GE(result.timings.sites2d_ms, 0.0);
    EXPECT_GE(result.timings.voronoi2d_ms, 0.0);
}

TEST(ClippedVoronoiDiagram2D, CanRenumberVolumesWithHilbertOrdering) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));
    const auto sites = make_five_sites_inside_square();
    auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    EXPECT_FALSE(diagram.volumes_are_renumbered());
    EXPECT_EQ(diagram.volume_numbering_method(),
              VolumeNumberingMethod2D::Original);

    const auto renumbering = renumber_volumes(
        diagram,
        HilbertVolumeOrdering2D{8});

    EXPECT_TRUE(diagram.volumes_are_renumbered());
    EXPECT_EQ(diagram.volume_numbering_method(),
              VolumeNumberingMethod2D::Hilbert);

    ASSERT_EQ(diagram.volume_count(), sites.size());
    ASSERT_EQ(renumbering.old_to_new.size(), sites.size());
    ASSERT_EQ(renumbering.new_to_old.size(), sites.size());

    for (std::size_t i = 0; i < diagram.volume_count(); ++i) {
        EXPECT_EQ(diagram.cell(i).volume_id, i);
    }

    std::size_t iterated_all = 0;
    for (const auto& volume : diagram.all_volumes()) {
        EXPECT_EQ(volume.volume_id, iterated_all);
        ++iterated_all;
    }
    EXPECT_EQ(iterated_all, diagram.volume_count());

    EXPECT_EQ(diagram.cell(SiteId{0}).site_id, SiteId{0});
    EXPECT_FALSE(diagram.cell(SiteId{0}).is_boundary_cell);
    EXPECT_EQ(diagram.internal_volume_count(), 1U);
    EXPECT_EQ(diagram.boundary_volume_count(), 4U);
}

TEST(ClippedVoronoiDiagram2D, CanRenumberVolumesWithCustomOrdering) {
    struct ReverseOrdering {
        std::vector<std::size_t> operator()(const ClippedVoronoiDiagram2D& diagram) const {
            std::vector<std::size_t> order(diagram.volume_count());
            std::iota(order.begin(), order.end(), std::size_t{0});
            std::reverse(order.begin(), order.end());
            return order;
        }
    };

    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));
    const auto sites = make_five_sites_inside_square();
    auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    const auto renumbering = renumber_volumes(diagram, ReverseOrdering{});

    EXPECT_TRUE(diagram.volumes_are_renumbered());
    EXPECT_EQ(diagram.volume_numbering_method(),
              VolumeNumberingMethod2D::Custom);

    ASSERT_EQ(renumbering.new_to_old.front(), sites.size() - 1U);
    EXPECT_EQ(diagram.cell(0).site_id, SiteId{4});
    EXPECT_EQ(diagram.cell(SiteId{0}).site_id, SiteId{0});
    EXPECT_EQ(diagram.cell(SiteId{0}).volume_id, sites.size() - 1U);
}

TEST(ClippedVoronoiDiagram2D, BuildsFromFactorySites) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 3.0, 2.0));
    const SiteValidationOptions validation{
        .min_distance_to_boundary = 0.25,
        .min_distance_between_sites = 0.2,
        .require_sequential_ids = true
    };
    const auto sites = make_sites(
        boundary,
        CartesianGrid2D{0.5, 0.5},
        validation,
        RegionId{8});

    const auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    ASSERT_FALSE(diagram.empty());
    EXPECT_EQ(diagram.cell_count(), sites.size());
    EXPECT_GT(diagram.internal_volume_count(), 0U);
    EXPECT_GT(diagram.boundary_volume_count(), 0U);
    EXPECT_NEAR(diagram.total_volume_area(), 6.0, 1.0e-8);
}

TEST(ClippedVoronoiDiagram2D, BuildsLargeRandomDiagramBeyondTenThousandSites) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 200.0, 200.0));
    const SiteValidationOptions validation{
        .min_distance_to_boundary = 0.05,
        .min_distance_between_sites = 0.02,
        .require_sequential_ids = true
    };

    const auto sites = make_sites(
        boundary,
        UniformRandom2D{10001, 314159U},
        validation);

    ClippedVoronoiBuildOptions2D options{};
    options.min_boundary_edge_length = 1.0e-10;
    options.boundary_short_edge_policy =
        BoundaryShortEdgePolicy2D::CollapseToBoundaryProjection;

    const auto diagram = ClippedVoronoiBuilder2D::build(
        sites,
        boundary,
        options);

    ASSERT_EQ(diagram.volume_count(), sites.size());
    EXPECT_EQ(diagram.internal_volume_count() + diagram.boundary_volume_count(),
              diagram.volume_count());
    EXPECT_NEAR(diagram.total_volume_area(), 40000.0, 1.0e-5);
}

TEST(ClippedVoronoi2DIO, WritesBoundarySitesDelaunayAndVoronoiCells) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));
    const auto sites = make_five_sites_inside_square();
    const auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    vmm::io::VtkOptions options{};
    options.precision = 12;
    options.cell_data = true;

    const auto path = vmm::io::write_clipped_voronoi_vtk_legacy(
        diagram,
        ".",
        "ut_clipped_voronoi2d",
        options);

    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

TEST(ClippedVoronoi2DIO, WritesUserVolumeFields) {
    const auto boundary = make_boundary(Rectangle(Point2{0.0, 0.0}, 2.0, 2.0));
    const auto sites = make_five_sites_inside_square();
    const auto diagram = ClippedVoronoiBuilder2D::build(sites, boundary);

    std::vector<double> psi(diagram.volume_count(), 0.0);
    std::vector<vmm::io::VtkVector3D> grad_psi(diagram.volume_count());
    for (const auto& cell : diagram.all_volumes()) {
        psi[cell.volume_id] = 10.0 + static_cast<double>(cell.volume_id);
        grad_psi[cell.volume_id] = vmm::io::VtkVector3D{
            static_cast<double>(cell.volume_id),
            -static_cast<double>(cell.volume_id),
            0.0};
    }

    const std::array scalar_fields{
        vmm::io::VtkVolumeScalarField{"psi_exact", psi}};
    const std::array vector_fields{
        vmm::io::VtkVolumeVectorField{"grad_psi", grad_psi}};

    vmm::io::VtkOptions options{};
    options.precision = 12;
    options.write_boundary = false;
    options.write_sites = false;
    options.write_delaunay_edges = false;

    const auto path = vmm::io::write_clipped_voronoi_vtk_legacy(
        diagram,
        std::span<const vmm::io::VtkVolumeScalarField>{scalar_fields},
        std::span<const vmm::io::VtkVolumeVectorField>{vector_fields},
        ".",
        "ut_clipped_voronoi2d_fields",
        options);

    std::ifstream input(path);
    ASSERT_TRUE(input);
    const std::string content{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{}};

    EXPECT_NE(content.find("CELL_DATA 5"), std::string::npos);
    EXPECT_NE(content.find("SCALARS psi_exact double 1"), std::string::npos);
    EXPECT_NE(content.find("VECTORS grad_psi double"), std::string::npos);
    EXPECT_NE(content.find("10.000000000000"), std::string::npos);
    EXPECT_NE(content.find("4.000000000000 -4.000000000000 0.000000000000"),
              std::string::npos);

    std::filesystem::remove(path);
}
