#include <iostream>
#include <string>
#include <string_view>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ellipse.hpp>
#include <VoronoiMeshMaker/Boundary2D/Transforms/Boundary2DTransform.hpp>
#include <VoronoiMeshMaker/ClippedVoronoi2DExport.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/Site2DTransform.hpp>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp>

namespace {

struct MeshCaseSummary {
    std::string_view name{};
    std::size_t site_count{};
    std::size_t internal_count{};
    std::size_t boundary_count{};
    double total_area{};
    std::filesystem::path path{};
};

template <class Pattern>
MeshCaseSummary write_case(std::string_view name,
                           const Pattern& pattern,
                           std::string_view filename)
{
    namespace b2d = ::vmm::b2d;
    namespace s2d = ::vmm::s2d;
    namespace vd2d = ::vmm::vd2d;
    namespace io = ::vmm::io;

    const b2d::Point2 center{0.0, 0.0};
    const double rotation_angle = ::vmm::constants::kPi / 4.0;

    auto boundary = b2d::make_boundary(
        b2d::Ellipse(center, 2.0, 1.0, 192),
        b2d::PolygonizePolicy{},
        b2d::RegionId{1});

    const s2d::SiteValidationOptions validation{
        .min_distance_to_boundary = 0.035,
        .min_distance_between_sites = 0.04,
        .require_sequential_ids = true
    };

    auto sites = s2d::make_sites(
        boundary,
        pattern,
        validation,
        s2d::RegionId{1});

    const auto rotation = b2d::rotation_transform(rotation_angle, center);
    b2d::transform_in_place(boundary, rotation);
    s2d::transform_in_place(sites, rotation);

    const auto diagram = vd2d::ClippedVoronoiBuilder2D::build(sites, boundary);

    io::VtkOptions vtk_options{};
    vtk_options.precision = 12;
    vtk_options.cell_data = true;

    const std::string output_folder =
        std::string(VMM_EXAMPLE_SOURCE_DIR) + "/vtk_output";
    const auto output_path = io::write_clipped_voronoi_vtk_legacy(
        diagram,
        output_folder,
        filename,
        vtk_options);

    return MeshCaseSummary{
        name,
        diagram.sites.size(),
        diagram.internal_volume_count(),
        diagram.boundary_volume_count(),
        static_cast<double>(diagram.total_volume_area()),
        output_path
    };
}

void print_summary(const MeshCaseSummary& summary) {
    std::cout << summary.name << '\n'
              << "  sites: " << summary.site_count << '\n'
              << "  internal volumes: " << summary.internal_count << '\n'
              << "  boundary volumes: " << summary.boundary_count << '\n'
              << "  total area: " << summary.total_area << '\n'
              << "  vtk: " << summary.path << '\n';
}

} // namespace

int main() {
    namespace s2d = ::vmm::s2d;

    const auto cartesian = write_case(
        "Figure 4 - Cartesian mesh",
        s2d::CartesianGrid2D{0.16, 0.16}.centered_in_box(),
        "figure4_cartesian");

    const auto hexagonal = write_case(
        "Figure 4 - Hexagonal mesh",
        s2d::HexagonalGrid2D{25},
        "figure4_hexagonal");

    const auto random = write_case(
        "Figure 4 - Random mesh",
        s2d::UniformRandom2D{185, 20260517U},
        "figure4_random");

    print_summary(cartesian);
    print_summary(hexagonal);
    print_summary(random);

    return 0;
}
