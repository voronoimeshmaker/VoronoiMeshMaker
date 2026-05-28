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
        b2d::Ring2D(b2d::Point2{0.0, 0.0}, 1.0, 4.0, 128),
        b2d::PolygonizePolicy{},
        b2d::RegionId{1});

    const s2d::SiteValidationOptions validation{
        .min_distance_to_boundary = 0.20,
        .min_distance_between_sites = 0.30,
        .require_sequential_ids = true
    };

    auto sites = s2d::make_sites(
        boundary,
        s2d::CartesianGrid2D{0.50}.with_epsilon(0.25),
        validation,
        s2d::RegionId{1});

    s2d::append_sites(
        sites,
        boundary,
        s2d::UniformRandom2D{60, 20260515U},
        validation,
        s2d::RegionId{2});

    io::VtkOptions vtk_options{};
    vtk_options.precision = 12;
    vtk_options.cell_data = true;

    const std::string output_folder =
        std::string(VMM_EXAMPLE_SOURCE_DIR) + "/vtk_output";
    const auto output_path = io::write_sites_vtk_legacy(
        sites,
        boundary,
        output_folder,
        "site_factory_cartesian_plus_random_ring",
        vtk_options);

    std::cout << "Generated " << sites.size()
              << " Site2D points from Cartesian + random layers\n";
    std::cout << "SiteFactory example written to: " << output_path << '\n';
    return 0;
}
