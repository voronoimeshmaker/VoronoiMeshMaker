#include <filesystem>
#include <iostream>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/RoundedRect.hpp>
#include <VoronoiMeshMaker/Boundary2D/Transforms/Boundary2DTransform.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/IO/Boundary2D/Boundary2DExport.hpp>

namespace b2d = vmm::b2d;
namespace io = vmm::io;

void write_boundary(const b2d::Boundary2DData& boundary,
                    const std::filesystem::path& output_dir,
                    std::string_view name)
{
    io::VtkOptions options{};
    options.precision = 12;
    options.cell_data = true;

    const auto path = io::write_boundary_vtk_legacy(
        boundary,
        output_dir.string(),
        name,
        options);

    std::cout << path.string() << '\n';
}

int main() {
#ifdef VMM_EXAMPLE_SOURCE_DIR
    const std::filesystem::path base_dir = VMM_EXAMPLE_SOURCE_DIR;
#else
    const std::filesystem::path base_dir = std::filesystem::current_path();
#endif

    const auto output_dir = base_dir / "vtk_output";
    std::filesystem::create_directories(output_dir);

    auto ring = b2d::make_boundary(
        b2d::Ring2D(b2d::Point2{0.0, 0.0}, 0.45, 1.4, 72),
        b2d::PolygonizePolicy{},
        b2d::RegionId{10});
    write_boundary(ring, output_dir, "ring2d_original");

    b2d::rotate_in_place(ring, vmm::constants::kPi / 5.0, b2d::Point2{0.25, -0.15});
    b2d::translate_in_place(ring, 2.0, 0.75);
    write_boundary(ring, output_dir, "ring2d_rotated_translated");

    auto rounded = b2d::make_boundary(
        b2d::RoundedRect(b2d::Point2{-1.0, -0.6}, 2.0, 1.2, 0.25, 16),
        b2d::PolygonizePolicy{},
        b2d::RegionId{20});
    write_boundary(rounded, output_dir, "rounded_rect_original");

    b2d::rotate_in_place(rounded, vmm::constants::kPi / 4.0, b2d::Point2{0.35, 0.20});
    b2d::translate_in_place(rounded, -2.0, -0.4);
    write_boundary(rounded, output_dir, "rounded_rect_rotated_translated");

    return 0;
}
