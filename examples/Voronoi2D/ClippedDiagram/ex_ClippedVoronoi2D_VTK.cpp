#include <iostream>
#include <string>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/ClippedVoronoi2DExport.hpp>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp>

int main() {
    namespace b2d = ::vmm::b2d;
    namespace s2d = ::vmm::s2d;
    namespace vd2d = ::vmm::vd2d;
    namespace io = ::vmm::io;

    const auto boundary = b2d::make_boundary(
        b2d::Rectangle(b2d::Point2{0.0, 0.0}, 4.0, 3.0),
        b2d::PolygonizePolicy{},
        b2d::RegionId{1});

    const s2d::SiteValidationOptions validation{
        .min_distance_to_boundary = 0.25,
        .min_distance_between_sites = 0.2,
        .require_sequential_ids = true
    };

    const auto sites = s2d::make_sites(
        boundary,
        s2d::HexagonalGrid2D{8},
        validation,
        s2d::RegionId{1});

    const auto diagram = vd2d::ClippedVoronoiBuilder2D::build(
        sites,
        boundary);

    io::VtkOptions vtk_options{};
    vtk_options.precision = 12;
    vtk_options.cell_data = true;

    const std::string output_folder =
        std::string(VMM_EXAMPLE_SOURCE_DIR) + "/vtk_output";
    const auto output_path = io::write_clipped_voronoi_vtk_legacy(
        diagram,
        output_folder,
        "clipped_voronoi2d_hexagonal_sites_rectangle",
        vtk_options);

    std::cout << "Sites: " << diagram.sites.size() << '\n';
    std::cout << "Voronoi volumes: " << diagram.volume_count() << '\n';
    std::cout << "Internal volumes: " << diagram.internal_volume_count() << '\n';
    std::cout << "Boundary volumes: " << diagram.boundary_volume_count() << '\n';
    std::cout << "Total area: " << diagram.total_volume_area() << '\n';
    std::cout << "Voronoi example written to: " << output_path << '\n';
    return 0;
}
