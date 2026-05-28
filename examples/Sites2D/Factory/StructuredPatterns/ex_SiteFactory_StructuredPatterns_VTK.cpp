#include <iostream>
#include <string>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/IO/Sites2D/Sites2DExport.hpp>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>

int main() {
    namespace b2d = ::vmm::b2d;
    namespace s2d = ::vmm::s2d;
    namespace io = ::vmm::io;

    const auto boundary = b2d::make_boundary(
        b2d::Ring2D(b2d::Point2{0.0, 0.0}, 1.0, 4.0, 160),
        b2d::PolygonizePolicy{},
        b2d::RegionId{1});

    const s2d::SiteValidationOptions validation{
        .min_distance_to_boundary = 0.18,
        .min_distance_between_sites = 0.15,
        .require_sequential_ids = true
    };

    const auto triangular_ii = s2d::make_sites(
        boundary,
        s2d::TriangularIIGrid2D{0.55}.with_epsilon(0.25),
        validation,
        s2d::RegionId{2});

    const auto triangular_iv = s2d::make_sites(
        boundary,
        s2d::TriangularIVGrid2D{0.62}.with_epsilon(-0.20),
        validation,
        s2d::RegionId{3});

    const auto hexagonal = s2d::make_sites(
        boundary,
        s2d::HexagonalGrid2D{8},
        validation,
        s2d::RegionId{4});

    io::VtkOptions vtk_options{};
    vtk_options.precision = 12;
    vtk_options.cell_data = true;

    const std::string output_folder =
        std::string(VMM_EXAMPLE_SOURCE_DIR) + "/vtk_output";

    const auto tri2_path = io::write_sites_vtk_legacy(
        triangular_ii,
        boundary,
        output_folder,
        "site_factory_triangular_ii_ring",
        vtk_options);

    const auto tri4_path = io::write_sites_vtk_legacy(
        triangular_iv,
        boundary,
        output_folder,
        "site_factory_triangular_iv_ring",
        vtk_options);

    const auto hex_path = io::write_sites_vtk_legacy(
        hexagonal,
        boundary,
        output_folder,
        "site_factory_hexagonal_ring",
        vtk_options);

    std::cout << "Triangular II sites: " << triangular_ii.size()
              << "\n  " << tri2_path << '\n';
    std::cout << "Triangular IV sites: " << triangular_iv.size()
              << "\n  " << tri4_path << '\n';
    std::cout << "Hexagonal sites: " << hexagonal.size()
              << "\n  " << hex_path << '\n';
    return 0;
}
