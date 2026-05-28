#include <iostream>
#include <string>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/IO/Voronoi2D/Delaunay2DExport.hpp>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp>

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
        .min_distance_to_boundary = 0.35,
        .min_distance_between_sites = 0.25,
        .require_sequential_ids = true
    };

    const auto sites = s2d::make_sites(
        boundary,
        s2d::HexagonalGrid2D{7},
        validation,
        s2d::RegionId{1});

    const auto delaunay = vd2d::DelaunayBuilder2D::build(sites);

    io::VtkOptions vtk_options{};
    vtk_options.precision = 12;
    vtk_options.cell_data = true;

    const std::string output_folder =
        std::string(VMM_EXAMPLE_SOURCE_DIR) + "/vtk_output";
    const auto output_path = io::write_delaunay_vtk_legacy(
        sites,
        boundary,
        delaunay,
        output_folder,
        "delaunay2d_hexagonal_sites_rectangle",
        vtk_options);

    std::cout << "Delaunay vertices: " << delaunay.number_of_vertices() << '\n';
    std::cout << "Delaunay example written to: " << output_path << '\n';
    return 0;
}
