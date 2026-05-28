#include <iostream>
#include <string>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/IO/Sites2D/Sites2DExport.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteSet.hpp>
#include <VoronoiMeshMaker/Sites2D/SiteValidation.hpp>

int main() {
    namespace b2d = ::vmm::b2d;
    namespace s2d = ::vmm::s2d;
    namespace io = ::vmm::io;

    const auto boundary = b2d::make_boundary(
        b2d::Rectangle(b2d::Point2{0.0, 0.0}, 6.0, 4.0),
        b2d::PolygonizePolicy{},
        b2d::RegionId{1});

    s2d::SiteSet sites;
    sites.add(s2d::Point2{1.0, 1.0}, s2d::RegionId{1});
    sites.add(s2d::Point2{3.0, 1.0}, s2d::RegionId{1});
    sites.add(s2d::Point2{5.0, 1.0}, s2d::RegionId{1});
    sites.add(s2d::Point2{2.0, 3.0}, s2d::RegionId{1});
    sites.add(s2d::Point2{4.0, 3.0}, s2d::RegionId{1});

    sites.renumber_sequential();

    const s2d::SiteValidationOptions validation_options{
        .min_distance_to_boundary = 0.5,
        .min_distance_between_sites = 0.75,
        .require_sequential_ids = true
    };
    s2d::validate_sites_or_throw(sites, boundary, validation_options);

    io::VtkOptions vtk_options{};
    vtk_options.precision = 12;
    vtk_options.cell_data = true;

    const std::string output_folder =
        std::string(VMM_EXAMPLE_SOURCE_DIR) + "/vtk_output";
    const auto output_path = io::write_sites_vtk_legacy(
        sites,
        boundary,
        output_folder,
        "site2d_basic",
        vtk_options);

    std::cout << "Site2D example written to: " << output_path << '\n';
    return 0;
}
