#pragma once
// Legacy POLYDATA contains site VERTICES, boundary/Delaunay LINES, then cell
// POLYGONS. Cell arrays pad the first two groups; Voronoi values form the tail.
// Vertices are duplicated per polygon. For a connected volume mesh, prefer
// write_clipped_voronoi_vtu in VTK_XML_ClippedVoronoi2D.hpp.
//==============================================================================
// Name        : VTK_Legacy_ClippedVoronoi2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO / Voronoi2D / Writers
// Description : Writer VTK Legacy for clipped 2D Voronoi diagrams.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file VTK_Legacy_ClippedVoronoi2D.hpp
 * @brief Writes Boundary2D, Site2D, Delaunay and Voronoi cells to POLYDATA.
 */

#include <cstddef>
#include <iomanip>
#include <span>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include <CGAL/number_utils.h>

#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/IO/Boundary2D/Topology/PolyLinesView.hpp>
#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

struct VtkVolumeScalarField {
    std::string_view name{};
    std::span<const double> values_by_volume_id{};
    double non_volume_value{0.0};
};

struct VtkVector3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

struct VtkVolumeVectorField {
    std::string_view name{};
    std::span<const VtkVector3D> values_by_volume_id{};
    VtkVector3D non_volume_value{};
};

/**
 * @brief Functor writer: VTK Legacy POLYDATA for a complete 2D diagram.
 */
struct VTK_Legacy_ClippedVoronoi2DWriter {
    template <class Sink>
    void operator()(const ::vmm::vd2d::ClippedVoronoiDiagram2D& diagram,
                    const VtkOptions& opt,
                    Sink& sink) const
    {
        (*this)(diagram,
                std::span<const VtkVolumeScalarField>{},
                std::span<const VtkVolumeVectorField>{},
                opt,
                sink);
    }

    template <class Sink>
    void operator()(const ::vmm::vd2d::ClippedVoronoiDiagram2D& diagram,
                    std::span<const VtkVolumeScalarField> scalar_fields,
                    std::span<const VtkVolumeVectorField> vector_fields,
                    const VtkOptions& opt,
                    Sink& sink) const
    {
        using ::vmm::error::CoreErr;

        if (opt.encoding != Encoding::ASCII) {
            VMM_THROW(CoreErr::NotImplemented,
                      {{"where", "VTK_Legacy_ClippedVoronoi2D(non-ASCII)"}});
        }
        if (!diagram.boundary.invariant_ok()) {
            VMM_THROW(CoreErr::AssertFailed, {{"where", "b2d.invariant_ok"}});
        }
        if (!diagram.sites.ids_are_sequential()) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"where", "VTK_Legacy_ClippedVoronoi2D"},
                       {"reason", "site_ids_must_be_sequential"}});
        }
        validate_volume_fields(diagram, scalar_fields, vector_fields);

        const auto& boundary = diagram.boundary;
        const auto& sites = diagram.sites;
        const PolyLinesView boundary_view{boundary};
        const std::size_t boundary_point_count =
            opt.write_boundary ? boundary.vertex_count() : 0U;
        const std::size_t site_point_count =
            opt.write_sites ? sites.size() : 0U;
        const bool write_delaunay_edges =
            opt.write_delaunay_edges && !sites.empty();
        const bool write_site_points =
            opt.write_sites || write_delaunay_edges;
        const std::size_t site_geometry_point_count =
            write_site_points ? sites.size() : 0U;

        std::vector<std::pair<std::size_t, std::size_t>> delaunay_edges;
        if (write_delaunay_edges) {
            delaunay_edges.reserve(static_cast<std::size_t>(
                diagram.delaunay.number_of_faces()) * 3U);

            for (auto edge = diagram.delaunay.finite_edges_begin();
                 edge != diagram.delaunay.finite_edges_end();
                 ++edge) {
                const auto face = edge->first;
                const int i = edge->second;
                const auto a = face->vertex((i + 1) % 3);
                const auto b = face->vertex((i + 2) % 3);
                if (diagram.delaunay.is_infinite(a) ||
                    diagram.delaunay.is_infinite(b)) {
                    continue;
                }
                delaunay_edges.emplace_back(
                    static_cast<std::size_t>(a->info().value),
                    static_cast<std::size_t>(b->info().value));
            }
        }

        std::size_t cell_polygon_point_count = 0;
        std::size_t polygon_connectivity_size = 0;
        std::size_t polygon_count = 0;
        if (opt.write_voronoi_cells) {
            for (const auto& cell : diagram.cells) {
                if (cell.polygon.size() < 3U) continue;
                cell_polygon_point_count += cell.polygon.size();
                polygon_connectivity_size += cell.polygon.size() + 1U;
                ++polygon_count;
            }
        }

        const std::size_t total_point_count =
            boundary_point_count + site_geometry_point_count + cell_polygon_point_count;
        const std::size_t line_count =
            (opt.write_boundary ? boundary_view.num_lines() : 0U) +
            delaunay_edges.size();
        const std::size_t line_connectivity_size =
            (opt.write_boundary ? boundary_view.total_connectivity_size() : 0U) +
            (delaunay_edges.size() * 3U);
        const std::size_t vtk_cell_count =
            site_point_count + line_count + polygon_count;

        sink.write("# vtk DataFile Version 4.2\n");
        sink.write("VMM Boundary2D + Site2D + Delaunay2D + Voronoi2D\n");
        sink.write("ASCII\n");
        sink.write("DATASET POLYDATA\n");

        std::stringstream ss;
        ss << std::fixed << std::setprecision(opt.precision);

        ss << "POINTS " << total_point_count << " double\n";
        if (opt.write_boundary) {
            for (const auto& point : boundary.points) {
                ss << static_cast<double>(point.x) << " "
                   << static_cast<double>(point.y) << " 0.0\n";
            }
        }
        if (write_site_points) {
            for (const auto& site : sites) {
                ss << static_cast<double>(site.point.x) << " "
                   << static_cast<double>(site.point.y) << " 0.0\n";
            }
        }
        if (opt.write_voronoi_cells) {
            for (const auto& cell : diagram.cells) {
                if (cell.polygon.size() < 3U) continue;
                for (const auto& point : cell.polygon) {
                    ss << static_cast<double>(point.x) << " "
                       << static_cast<double>(point.y) << " 0.0\n";
                }
            }
        }
        sink.write(ss.str());
        ss.str("");
        ss.clear();

        if (site_point_count > 0U) {
            ss << "VERTICES " << site_point_count << " "
               << (site_point_count * 2U) << "\n";
            for (std::size_t i = 0; i < site_point_count; ++i) {
                ss << "1 " << (boundary_point_count + i) << "\n";
            }
            sink.write(ss.str());
            ss.str("");
            ss.clear();
        }

        if (line_count > 0U) {
            ss << "LINES " << line_count << " " << line_connectivity_size << "\n";
            if (opt.write_boundary) {
                for (std::size_t r = 0; r < boundary_view.num_lines(); ++r) {
                    const auto ring_view = boundary.ring(static_cast<::vmm::b2d::Index>(r));
                    const auto n = ring_view.size();
                    const auto base_index = boundary.ring_off[r];

                    ss << (n + 1U);
                    for (std::size_t i = 0; i < n; ++i) ss << " " << (base_index + i);
                    ss << " " << base_index << "\n";
                }
            }
            for (const auto& [a, b] : delaunay_edges) {
                ss << "2 " << (boundary_point_count + a)
                   << " " << (boundary_point_count + b) << "\n";
            }
            sink.write(ss.str());
            ss.str("");
            ss.clear();
        }

        if (polygon_count > 0U) {
            ss << "POLYGONS " << polygon_count << " "
               << polygon_connectivity_size << "\n";
            std::size_t cell_point_base =
                boundary_point_count + site_geometry_point_count;
            for (const auto& cell : diagram.cells) {
                if (cell.polygon.size() < 3U) continue;
                ss << cell.polygon.size();
                for (std::size_t i = 0; i < cell.polygon.size(); ++i) {
                    ss << " " << (cell_point_base + i);
                }
                ss << "\n";
                cell_point_base += cell.polygon.size();
            }
            sink.write(ss.str());
            ss.str("");
            ss.clear();
        }

        if (opt.cell_data && vtk_cell_count > 0U) {
            ss << "CELL_DATA " << vtk_cell_count << "\n";

            ss << "SCALARS entity_kind int 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < site_point_count; ++i) ss << "1\n";
            if (opt.write_boundary) {
                for (std::size_t i = 0; i < boundary_view.num_lines(); ++i) ss << "0\n";
            }
            for (std::size_t i = 0; i < delaunay_edges.size(); ++i) ss << "2\n";
            for (std::size_t i = 0; i < polygon_count; ++i) ss << "3\n";

            ss << "SCALARS site_id int 1\nLOOKUP_TABLE default\n";
            if (opt.write_sites) {
                for (const auto& site : sites) ss << site.id.value << "\n";
            }
            for (std::size_t i = 0; i < line_count; ++i) ss << "-1\n";
            if (opt.write_voronoi_cells) {
                for (const auto& cell : diagram.cells) {
                    if (cell.polygon.size() < 3U) continue;
                    ss << cell.site_id.value << "\n";
                }
            }

            ss << "SCALARS volume_id int 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < site_point_count + line_count; ++i) {
                ss << "-1\n";
            }
            if (opt.write_voronoi_cells) {
                for (const auto& cell : diagram.cells) {
                    if (cell.polygon.size() < 3U) continue;
                    ss << cell.volume_id << "\n";
                }
            }

            ss << "SCALARS region_id int 1\nLOOKUP_TABLE default\n";
            if (opt.write_sites) {
                for (const auto& site : sites) ss << site.region.value << "\n";
            }
            if (opt.write_boundary) {
                for (const auto& region : boundary.regions) {
                    ss << static_cast<int>(region.value) << "\n";
                }
            }
            for (std::size_t i = 0; i < delaunay_edges.size(); ++i) ss << "-1\n";
            if (opt.write_voronoi_cells) {
                for (const auto& cell : diagram.cells) {
                    if (cell.polygon.size() < 3U) continue;
                    const auto raw = static_cast<std::size_t>(cell.site_id.value);
                    ss << diagram.sites[raw].region.value << "\n";
                }
            }

            ss << "SCALARS is_boundary_volume int 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < site_point_count + line_count; ++i) {
                ss << "0\n";
            }
            if (opt.write_voronoi_cells) {
                for (const auto& cell : diagram.cells) {
                    if (cell.polygon.size() < 3U) continue;
                    ss << (cell.is_boundary_cell ? 1 : 0) << "\n";
                }
            }

            ss << "SCALARS volume_area double 1\nLOOKUP_TABLE default\n";
            for (std::size_t i = 0; i < site_point_count + line_count; ++i) {
                ss << "0\n";
            }
            if (opt.write_voronoi_cells) {
                for (const auto& cell : diagram.cells) {
                    if (cell.polygon.size() < 3U) continue;
                    ss << static_cast<double>(cell.area()) << "\n";
                }
            }

            for (const auto& field : scalar_fields) {
                ss << "SCALARS " << field.name << " double 1\n"
                   << "LOOKUP_TABLE default\n";
                for (std::size_t i = 0; i < site_point_count + line_count; ++i) {
                    ss << field.non_volume_value << "\n";
                }
                if (opt.write_voronoi_cells) {
                    for (const auto& cell : diagram.cells) {
                        if (cell.polygon.size() < 3U) continue;
                        ss << field.values_by_volume_id[cell.volume_id] << "\n";
                    }
                }
            }

            for (const auto& field : vector_fields) {
                ss << "VECTORS " << field.name << " double\n";
                for (std::size_t i = 0; i < site_point_count + line_count; ++i) {
                    ss << field.non_volume_value.x << " "
                       << field.non_volume_value.y << " "
                       << field.non_volume_value.z << "\n";
                }
                if (opt.write_voronoi_cells) {
                    for (const auto& cell : diagram.cells) {
                        if (cell.polygon.size() < 3U) continue;
                        const auto& value = field.values_by_volume_id[cell.volume_id];
                        ss << value.x << " " << value.y << " " << value.z << "\n";
                    }
                }
            }
        }

        sink.write(ss.str());
        ss.str("");
        ss.clear();

        ss << "POINT_DATA " << total_point_count << "\n";

        ss << "SCALARS point_entity_kind int 1\nLOOKUP_TABLE default\n";
        for (std::size_t i = 0; i < boundary_point_count; ++i) ss << "0\n";
        for (std::size_t i = 0; i < site_geometry_point_count; ++i) ss << "1\n";
        for (std::size_t i = 0; i < cell_polygon_point_count; ++i) ss << "3\n";

        ss << "SCALARS point_site_id int 1\nLOOKUP_TABLE default\n";
        for (std::size_t i = 0; i < boundary_point_count; ++i) ss << "-1\n";
        for (std::size_t i = 0; i < site_geometry_point_count; ++i) ss << i << "\n";
        if (opt.write_voronoi_cells) {
            for (const auto& cell : diagram.cells) {
                if (cell.polygon.size() < 3U) continue;
                for (std::size_t i = 0; i < cell.polygon.size(); ++i) {
                    ss << cell.site_id.value << "\n";
                }
            }
        }

        sink.write(ss.str());
        sink.flush();
    }

private:
    static void validate_field_name(std::string_view name) {
        using ::vmm::error::CoreErr;

        if (name.empty()) {
            VMM_THROW(CoreErr::InvalidArgument,
                      {{"where", "VTK_Legacy_ClippedVoronoi2D"},
                       {"reason", "empty_volume_field_name"}});
        }
        for (const char ch : name) {
            const bool valid =
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= 'a' && ch <= 'z') ||
                (ch >= '0' && ch <= '9') ||
                ch == '_';
            if (!valid) {
                VMM_THROW(CoreErr::InvalidArgument,
                          {{"where", "VTK_Legacy_ClippedVoronoi2D"},
                           {"reason", "invalid_volume_field_name"}});
            }
        }
    }

    static void validate_volume_fields(
        const ::vmm::vd2d::ClippedVoronoiDiagram2D& diagram,
        std::span<const VtkVolumeScalarField> scalar_fields,
        std::span<const VtkVolumeVectorField> vector_fields) {
        using ::vmm::error::CoreErr;

        for (const auto& field : scalar_fields) {
            validate_field_name(field.name);
            if (field.values_by_volume_id.size() < diagram.volume_count()) {
                VMM_THROW(CoreErr::InvalidArgument,
                          {{"where", "VTK_Legacy_ClippedVoronoi2D"},
                           {"reason", "scalar_field_too_small"}});
            }
        }
        for (const auto& field : vector_fields) {
            validate_field_name(field.name);
            if (field.values_by_volume_id.size() < diagram.volume_count()) {
                VMM_THROW(CoreErr::InvalidArgument,
                          {{"where", "VTK_Legacy_ClippedVoronoi2D"},
                           {"reason", "vector_field_too_small"}});
            }
        }
    }
};

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
